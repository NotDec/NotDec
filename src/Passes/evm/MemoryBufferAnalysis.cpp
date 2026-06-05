#include "Passes/evm/MemoryBufferAnalysis.h"
#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

using namespace llvm;

#define DEBUG_TYPE "evm-memory-buffer"

STATISTIC(NumMemoryAllocations,
          "Number of Solidity memory allocations rewritten");
STATISTIC(NumMemoryFinalizeAllocations,
          "Number of Solidity memory allocation finalizers rewritten");
STATISTIC(NumMemoryWrites, "Number of Solidity memory writes rewritten");
STATISTIC(NumMemoryReads, "Number of Solidity memory reads rewritten");
STATISTIC(NumMemoryArrayByteWrites,
          "Number of Solidity memory array byte writes rewritten");

namespace notdec::passes::evm {
namespace {

bool isSameValue(Value *LHS, Value *RHS) { return LHS == RHS; }

std::optional<uint64_t> getUInt64Constant(Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr || C->getValue().getActiveBits() > 64) {
    return std::nullopt;
  }
  return C->getZExtValue();
}

bool isFreeMemoryPointerLoad(Value *V) {
  std::optional<detail::EvmMemoryLoad> Load = detail::matchEvmMemoryLoad(V);
  return Load.has_value() && detail::isConstantIntValue(Load->Address, 64);
}

bool isZero(Value *V) { return detail::isConstantIntValue(V, 0); }

bool isFreeMemoryPointerClobber(Instruction &I) {
  std::optional<detail::EvmMemoryStore> Store = detail::matchEvmMemoryStore(&I);
  return Store.has_value() && detail::isConstantIntValue(Store->Address, 64);
}

bool hasFreeMemoryPointerClobberBetween(Instruction *From, Instruction *To) {
  if (From == nullptr || To == nullptr || From->getParent() != To->getParent() ||
      !From->comesBefore(To)) {
    return true;
  }
  for (Instruction *I = From->getNextNode(); I != nullptr && I != To;
       I = I->getNextNode()) {
    if (isFreeMemoryPointerClobber(*I)) {
      return true;
    }
  }
  return false;
}

bool hasAllocationForBase(ArrayRef<MemoryAllocation> Allocations, Value *Base) {
  for (const MemoryAllocation &Alloc : Allocations) {
    if (Alloc.Base == Base) {
      return true;
    }
  }
  return false;
}

SmallVector<Instruction *, 4> collectFreeMemoryPointerReloads(
    Instruction *Base, ArrayRef<Value *> Bases) {
  SmallVector<Instruction *, 4> Reloads;
  if (Base == nullptr) {
    return Reloads;
  }
  for (Value *MaybeReload : Bases) {
    auto *Reload = dyn_cast_or_null<Instruction>(MaybeReload);
    if (Reload == nullptr || Reload == Base) {
      continue;
    }
    if (!hasFreeMemoryPointerClobberBetween(Base, Reload)) {
      Reloads.push_back(Reload);
    }
  }
  return Reloads;
}

bool valueAvailableAt(Value *V, Instruction &UsePoint, DominatorTree &DT) {
  auto *Def = dyn_cast_or_null<Instruction>(V);
  if (Def == nullptr) {
    return true;
  }
  if (Def->getParent() == UsePoint.getParent()) {
    return Def->comesBefore(&UsePoint);
  }
  return DT.dominates(Def, &UsePoint);
}

bool isMemoryCopyWriteCall(CallBase *Call) {
  return detail::isCallTo(Call, "evm_calldatacopy") ||
         detail::isCallTo(Call, "evm_returndatacopy") ||
         detail::isCallTo(Call, "evm_codecopy");
}

MemoryWriteKind getMemoryCopyWriteKind(CallBase *Call) {
  if (detail::isCallTo(Call, "evm_calldatacopy")) {
    return MemoryWriteKind::CalldataCopy;
  }
  if (detail::isCallTo(Call, "evm_returndatacopy")) {
    return MemoryWriteKind::ReturndataCopy;
  }
  return MemoryWriteKind::CodeCopy;
}

std::optional<uint64_t> getOffsetFromBase(Value *Ptr, Value *Base) {
  if (isSameValue(Ptr, Base)) {
    return 0;
  }

  auto *Add = dyn_cast_or_null<BinaryOperator>(Ptr);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return std::nullopt;
  }

  for (unsigned I = 0; I < 2; ++I) {
    if (!isSameValue(Add->getOperand(I), Base)) {
      continue;
    }
    return getUInt64Constant(Add->getOperand(1 - I));
  }
  return std::nullopt;
}

Value *getArrayByteIndexFromPtr(Value *Ptr, Value *ArrayBase) {
  auto *PtrAdd = dyn_cast_or_null<BinaryOperator>(Ptr);
  if (PtrAdd == nullptr || PtrAdd->getOpcode() != Instruction::Add) {
    return nullptr;
  }

  for (unsigned I = 0; I < 2; ++I) {
    if (!isSameValue(PtrAdd->getOperand(I), ArrayBase)) {
      continue;
    }

    Value *DataOffset = PtrAdd->getOperand(1 - I);
    auto *OffsetAdd = dyn_cast_or_null<BinaryOperator>(DataOffset);
    if (OffsetAdd != nullptr && OffsetAdd->getOpcode() == Instruction::Add) {
      for (unsigned J = 0; J < 2; ++J) {
        if (detail::isConstantIntValue(OffsetAdd->getOperand(J), 32)) {
          return OffsetAdd->getOperand(1 - J);
        }
      }
    }

    std::optional<uint64_t> ConstantOffset = getUInt64Constant(DataOffset);
    if (ConstantOffset.has_value() && *ConstantOffset >= 32) {
      return ConstantInt::get(DataOffset->getType(), *ConstantOffset - 32);
    }
  }

  return nullptr;
}

Value *getAllocationSize(Value *NewPtr, Value *Base) {
  auto *Add = dyn_cast_or_null<BinaryOperator>(NewPtr);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return nullptr;
  }
  if (isSameValue(Add->getOperand(0), Base)) {
    return Add->getOperand(1);
  }
  if (isSameValue(Add->getOperand(1), Base)) {
    return Add->getOperand(0);
  }
  return nullptr;
}

void getOrDeclareMarker(Module &M, StringRef Name, ArrayRef<Type *> Args,
                        Function *&Out) {
  FunctionType *FTy =
      FunctionType::get(Type::getVoidTy(M.getContext()), Args, false);
  FunctionCallee Callee = M.getOrInsertFunction(Name, FTy);
  Out = cast<Function>(Callee.getCallee());
}

Function *getOrDeclareFunction(Module &M, StringRef Name, Type *ReturnType,
                               ArrayRef<Type *> Args) {
  FunctionType *FTy = FunctionType::get(ReturnType, Args, false);
  FunctionCallee Callee = M.getOrInsertFunction(Name, FTy);
  return cast<Function>(Callee.getCallee());
}

Value *asI256(IRBuilder<> &Builder, Value *V) {
  Type *I256 = Builder.getIntNTy(256);
  if (V->getType() == I256) {
    return V;
  }
  if (V->getType()->isIntegerTy() && V->getType()->getIntegerBitWidth() < 256) {
    return Builder.CreateZExt(V, I256);
  }
  if (V->getType()->isIntegerTy() && V->getType()->getIntegerBitWidth() > 256) {
    return Builder.CreateTrunc(V, I256);
  }
  return ConstantInt::get(I256, 0);
}

void insertAllocationMarker(LLVMContext &Ctx, const MemoryAllocation &Alloc) {
  if (Alloc.FinalizePoint == nullptr || Alloc.Base == nullptr ||
      Alloc.Size == nullptr) {
    return;
  }
  Module *M = Alloc.FinalizePoint->getModule();
  Function *Marker = nullptr;
  Type *I256 = Type::getIntNTy(Ctx, 256);
  getOrDeclareMarker(*M, "notdec_solidity_memory_allocation", {I256, I256},
                     Marker);
  IRBuilder<> Builder(Alloc.FinalizePoint);
  Builder.CreateCall(
      Marker, {asI256(Builder, Alloc.Base), asI256(Builder, Alloc.Size)});
}

bool rewriteAllocation(LLVMContext &Ctx, const MemoryAllocation &Alloc,
                       DominatorTree &DT,
                       SmallPtrSetImpl<Instruction *> &RewrittenBases,
                       SmallVectorImpl<Instruction *> &ToErase) {
  auto *BaseLoad = dyn_cast_or_null<Instruction>(Alloc.Base);
  if (BaseLoad == nullptr || Alloc.AllocatePoint == nullptr) {
    return false;
  }
  if (!RewrittenBases.insert(BaseLoad).second) {
    return false;
  }

  Module *M = BaseLoad->getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  Type *PtrTy = PointerType::get(Ctx, 0);
  bool SizeAvailable = Alloc.Size != nullptr &&
                       valueAvailableAt(Alloc.Size, *Alloc.AllocatePoint, DT);
  SmallVector<Type *, 2> AllocArgs;
  if (SizeAvailable) {
    AllocArgs.append({I256, I256});
  }
  Function *AllocFn = getOrDeclareFunction(
      *M, SizeAvailable ? "calloc" : "calloc_unbounded", PtrTy, AllocArgs);
  AllocFn->addRetAttr(Attribute::NoAlias);

  IRBuilder<> AllocBuilder(Alloc.AllocatePoint);
  CallInst *NewBasePtr =
      SizeAvailable
          ? AllocBuilder.CreateCall(AllocFn, {ConstantInt::get(I256, 1),
                                              asI256(AllocBuilder, Alloc.Size)})
          : AllocBuilder.CreateCall(AllocFn);
  Value *NewBase =
      AllocBuilder.CreatePtrToInt(NewBasePtr, I256, "evm.alloc.addr");

  BaseLoad->replaceAllUsesWith(NewBase);
  if (BaseLoad->use_empty()) {
    ToErase.push_back(BaseLoad);
  }
  for (Instruction *Reload : Alloc.Reloads) {
    if (Reload == nullptr || Reload == BaseLoad) {
      continue;
    }
    Reload->replaceAllUsesWith(NewBase);
    if (Reload->use_empty()) {
      ToErase.push_back(Reload);
    }
  }

  if (!SizeAvailable && Alloc.FinalizePoint != nullptr &&
      Alloc.Size != nullptr) {
    Function *FinalizeFn = getOrDeclareFunction(
        *M, "notdec_evm_finalize_alloc", Type::getVoidTy(Ctx), {I256, I256});
    IRBuilder<> FinalizeBuilder(Alloc.FinalizePoint);
    FinalizeBuilder.CreateCall(FinalizeFn,
                               {asI256(FinalizeBuilder, NewBase),
                                asI256(FinalizeBuilder, Alloc.Size)});
    ++NumMemoryFinalizeAllocations;
  }

  if (Alloc.FinalizePoint != nullptr) {
    ToErase.push_back(Alloc.FinalizePoint);
  }
  return true;
}

// The generic memory read/write/copy marker materialization is paused. These
// facts currently duplicate the allocation-backed memory model and should be
// revisited after type recovery consumes normal memory facts directly.
// void insertWriteMarker(LLVMContext &Ctx, const MemoryWrite &Write) {
//   if (Write.StoreOrCopy == nullptr || Write.Base == nullptr ||
//       !Write.Offset.has_value() || Write.ValueOrSize == nullptr) {
//     return;
//   }
//   Module *M = Write.StoreOrCopy->getModule();
//   Type *I256 = Type::getIntNTy(Ctx, 256);
//   IRBuilder<> Builder(Write.StoreOrCopy);
//   Function *Marker = nullptr;
//   if (Write.Kind == MemoryWriteKind::MStore) {
//     getOrDeclareMarker(*M, "notdec_solidity_memory_write",
//                        {I256, I256, I256}, Marker);
//     Builder.CreateCall(Marker, {asI256(Builder, Write.Base),
//                                 ConstantInt::get(I256, *Write.Offset),
//                                 asI256(Builder, Write.ValueOrSize)});
//     return;
//   }
//
//   if (Write.Kind == MemoryWriteKind::MStore8) {
//     getOrDeclareMarker(*M, "notdec_solidity_memory_byte_write",
//                        {I256, I256, I256}, Marker);
//     Builder.CreateCall(Marker, {asI256(Builder, Write.Base),
//                                 ConstantInt::get(I256, *Write.Offset),
//                                 asI256(Builder, Write.ValueOrSize)});
//     return;
//   }
//
//   if (Write.SourceOffset == nullptr) {
//     return;
//   }
//   getOrDeclareMarker(*M, "notdec_solidity_memory_copy_write",
//                      {I256, I256, I256, I256, I256}, Marker);
//   Builder.CreateCall(
//       Marker, {asI256(Builder, Write.Base),
//                ConstantInt::get(I256, *Write.Offset),
//                asI256(Builder, Write.SourceOffset),
//                asI256(Builder, Write.ValueOrSize),
//                ConstantInt::get(I256, static_cast<uint64_t>(Write.Kind))});
// }
//
// void insertReadMarker(LLVMContext &Ctx, const MemoryRead &Read) {
//   if (Read.Load == nullptr || Read.Base == nullptr ||
//       !Read.Offset.has_value() || Read.Value == nullptr) {
//     return;
//   }
//   Instruction *InsertBefore = Read.Load->getNextNode();
//   if (InsertBefore == nullptr) {
//     return;
//   }
//
//   Module *M = Read.Load->getModule();
//   Type *I256 = Type::getIntNTy(Ctx, 256);
//   Function *Marker = nullptr;
//   getOrDeclareMarker(*M, "notdec_solidity_memory_read", {I256, I256, I256},
//                      Marker);
//
//   IRBuilder<> Builder(InsertBefore);
//   Builder.CreateCall(Marker, {asI256(Builder, Read.Base),
//                               ConstantInt::get(I256, *Read.Offset),
//                               asI256(Builder, Read.Value)});
// }

void insertArrayByteWriteMarker(LLVMContext &Ctx,
                                const MemoryArrayByteWrite &Write) {
  if (Write.Store == nullptr || Write.ArrayBase == nullptr ||
      Write.Index == nullptr || Write.Value == nullptr) {
    return;
  }
  Module *M = Write.Store->getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  Function *Marker = nullptr;
  getOrDeclareMarker(*M, "notdec_solidity_memory_array_byte_write",
                     {I256, I256, I256}, Marker);

  IRBuilder<> Builder(Write.Store);
  Builder.CreateCall(Marker, {asI256(Builder, Write.ArrayBase),
                              asI256(Builder, Write.Index),
                              asI256(Builder, Write.Value)});
}

} // namespace

MemoryBufferFacts analyzeMemoryBuffers(Function &F, DominatorTree &DT) {
  MemoryBufferFacts Facts;

  for (Instruction &I : instructions(F)) {
    std::optional<detail::EvmMemoryStore> Store =
        detail::matchEvmMemoryStore(&I);
    if (!Store.has_value() || Store->StoreBits != 256) {
      continue;
    }
    if (!detail::isConstantIntValue(Store->Address, 64)) {
      continue;
    }
    Value *NewPtr = Store->StoredValue;
    auto *NewPtrUser = dyn_cast<User>(NewPtr);
    if (NewPtrUser == nullptr) {
      continue;
    }
    for (Value *Op : NewPtrUser->operands()) {
      if (!isFreeMemoryPointerLoad(Op)) {
        continue;
      }
      Value *Size = getAllocationSize(NewPtr, Op);
      if (Size == nullptr) {
        continue;
      }
      MemoryAllocation Alloc;
      Alloc.Base = Op;
      Alloc.Size = Size;
      Alloc.AllocatePoint = cast<Instruction>(Op);
      Alloc.FinalizePoint = &I;
      Alloc.Finalized = true;
      Facts.Allocations.push_back(Alloc);
      break;
    }
  }

  SmallVector<Value *, 8> Bases;
  for (Instruction &I : instructions(F)) {
    if (isFreeMemoryPointerLoad(&I)) {
      Bases.push_back(&I);
    }
  }

  for (Instruction &I : instructions(F)) {
    std::optional<detail::EvmMemoryStore> Store =
        detail::matchEvmMemoryStore(&I);
    if (Store.has_value() && Store->StoreBits == 256) {
      Value *Ptr = Store->Address;
      for (Value *Base : Bases) {
        if (!valueAvailableAt(Base, I, DT)) {
          continue;
        }
        std::optional<uint64_t> Offset = getOffsetFromBase(Ptr, Base);
        if (!Offset.has_value()) {
          continue;
        }
        Facts.Writes.push_back(MemoryWrite{&I, Base, Offset, Store->StoredValue,
                                           nullptr, MemoryWriteKind::MStore});
        break;
      }
      continue;
    }

    std::optional<detail::EvmMemoryLoad> Load = detail::matchEvmMemoryLoad(&I);
    if (Load.has_value() && !detail::isConstantIntValue(Load->Address, 64)) {
      Value *Ptr = Load->Address;
      bool MatchedBase = false;
      for (Value *Base : Bases) {
        if (!valueAvailableAt(Base, I, DT)) {
          continue;
        }
        std::optional<uint64_t> Offset = getOffsetFromBase(Ptr, Base);
        if (!Offset.has_value()) {
          continue;
        }
        Facts.Reads.push_back(MemoryRead{&I, Base, Offset, Load->LoadedValue});
        MatchedBase = true;
        break;
      }
      if (!MatchedBase && getUInt64Constant(Ptr).has_value()) {
        Facts.Reads.push_back(MemoryRead{&I, Ptr, 0, Load->LoadedValue});
      }
      continue;
    }

    if (Store.has_value() && Store->StoreBits == 8) {
      Value *Ptr = Store->Address;
      bool MatchedBase = false;
      for (Value *Base : Bases) {
        if (!valueAvailableAt(Base, I, DT)) {
          continue;
        }
        if (Value *Index = getArrayByteIndexFromPtr(Ptr, Base)) {
          Facts.ArrayByteWrites.push_back(
              MemoryArrayByteWrite{&I, Base, Index, Store->StoredValue});
        }
        std::optional<uint64_t> Offset = getOffsetFromBase(Ptr, Base);
        if (!Offset.has_value()) {
          continue;
        }
        Facts.Writes.push_back(MemoryWrite{&I, Base, Offset, Store->StoredValue,
                                           nullptr, MemoryWriteKind::MStore8});
        MatchedBase = true;
        break;
      }
      if (!MatchedBase) {
        Facts.Writes.push_back(MemoryWrite{&I, Ptr, 0, Store->StoredValue,
                                           nullptr, MemoryWriteKind::MStore8});
      }
      continue;
    }

    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isMemoryCopyWriteCall(Call) && Call->arg_size() == 5) {
      Value *Dst = Call->getArgOperand(2);
      if (detail::isCallTo(Call, "evm_returndatacopy") && isZero(Dst) &&
          isZero(Call->getArgOperand(3))) {
        Facts.Writes.push_back(MemoryWrite{
            Call, ConstantInt::get(Call->getArgOperand(2)->getType(), 0), 0,
            Call->getArgOperand(4), Call->getArgOperand(3),
            MemoryWriteKind::ScratchReturndataCopy});
        continue;
      }
      bool MatchedBase = false;
      for (Value *Base : Bases) {
        if (!valueAvailableAt(Base, *Call, DT)) {
          continue;
        }
        std::optional<uint64_t> Offset = getOffsetFromBase(Dst, Base);
        if (!Offset.has_value()) {
          continue;
        }
        MemoryWriteKind Kind = getMemoryCopyWriteKind(Call);
        Facts.Writes.push_back(MemoryWrite{Call, Base, Offset,
                                           Call->getArgOperand(4),
                                           Call->getArgOperand(3), Kind});
        MatchedBase = true;
        break;
      }
      if (!MatchedBase) {
        MemoryWriteKind Kind = getMemoryCopyWriteKind(Call);
        Facts.Writes.push_back(MemoryWrite{Call, Dst, 0, Call->getArgOperand(4),
                                           Call->getArgOperand(3), Kind});
      }
      continue;
    }

    if (detail::isCallTo(Call, "evm_mcopy") && Call->arg_size() == 4) {
      Value *Dst = Call->getArgOperand(1);
      bool MatchedBase = false;
      for (Value *Base : Bases) {
        if (!valueAvailableAt(Base, *Call, DT)) {
          continue;
        }
        std::optional<uint64_t> Offset = getOffsetFromBase(Dst, Base);
        if (!Offset.has_value()) {
          continue;
        }
        Facts.Writes.push_back(
            MemoryWrite{Call, Base, Offset, Call->getArgOperand(3),
                        Call->getArgOperand(2), MemoryWriteKind::MemoryCopy});
        MatchedBase = true;
        break;
      }
      if (!MatchedBase) {
        Facts.Writes.push_back(MemoryWrite{Call, Dst, 0, Call->getArgOperand(3),
                                           Call->getArgOperand(2),
                                           MemoryWriteKind::MemoryCopy});
      }
      continue;
    }
  }

  for (Value *Base : Bases) {
    if (hasAllocationForBase(Facts.Allocations, Base)) {
      continue;
    }
    auto *BaseInst = dyn_cast<Instruction>(Base);
    if (BaseInst == nullptr) {
      continue;
    }

    bool HasWrite = false;
    for (const MemoryWrite &Write : Facts.Writes) {
      if (Write.Base == Base) {
        HasWrite = true;
        break;
      }
    }
    if (!HasWrite) {
      continue;
    }

    MemoryAllocation Alloc;
    Alloc.Base = Base;
    Alloc.AllocatePoint = BaseInst;
    Alloc.Reloads = collectFreeMemoryPointerReloads(BaseInst, Bases);
    Alloc.Finalized = false;
    Facts.Allocations.push_back(Alloc);
  }

  return Facts;
}

PreservedAnalyses MemoryBufferRewritePass::run(Function &F,
                                               FunctionAnalysisManager &FAM) {
  LLVMContext &Ctx = F.getContext();
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  MemoryBufferFacts Facts = analyzeMemoryBuffers(F, DT);
  bool Changed = false;

  for (const MemoryAllocation &Alloc : Facts.Allocations) {
    insertAllocationMarker(Ctx, Alloc);
    ++NumMemoryAllocations;
    Changed = true;
  }

  // The generic memory read/write/copy markers duplicate the allocation-backed
  // memory model and currently add noisy facts before type recovery is stable.
  // Keep collecting them for local analysis, but do not materialize marker
  // calls in IR.
  // for (const MemoryWrite &Write : Facts.Writes) {
  //   insertWriteMarker(Ctx, Write);
  //   ++NumMemoryWrites;
  //   Changed = true;
  // }
  //
  // for (const MemoryRead &Read : Facts.Reads) {
  //   insertReadMarker(Ctx, Read);
  //   ++NumMemoryReads;
  //   Changed = true;
  // }

  for (const MemoryArrayByteWrite &Write : Facts.ArrayByteWrites) {
    insertArrayByteWriteMarker(Ctx, Write);
    ++NumMemoryArrayByteWrites;
    Changed = true;
  }

  SmallVector<Instruction *, 16> ToErase;
  SmallPtrSet<Instruction *, 8> RewrittenBases;
  for (const MemoryAllocation &Alloc : Facts.Allocations) {
    if (rewriteAllocation(Ctx, Alloc, DT, RewrittenBases, ToErase)) {
      Changed = true;
    }
  }

  for (Instruction *I : ToErase) {
    if (I != nullptr && I->use_empty()) {
      I->eraseFromParent();
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
