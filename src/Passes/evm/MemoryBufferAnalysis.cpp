#include "Passes/evm/MemoryBufferAnalysis.h"
#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

using namespace llvm;

#define DEBUG_TYPE "evm-memory-buffer"

STATISTIC(NumMemoryAllocations,
          "Number of Solidity memory allocations rewritten");
STATISTIC(NumMemoryWrites, "Number of Solidity memory writes rewritten");
STATISTIC(NumMemoryConsumers,
          "Number of Solidity memory consumers rewritten");

namespace notdec::passes::evm {
namespace {

bool isSameValue(Value *LHS, Value *RHS) {
  return LHS == RHS;
}

std::optional<uint64_t> getUInt64Constant(Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr || C->getValue().getActiveBits() > 64) {
    return std::nullopt;
  }
  return C->getZExtValue();
}

bool isFreeMemoryPointerLoad(Value *V) {
  auto *Call = dyn_cast_or_null<CallBase>(V);
  return Call != nullptr && detail::isCallTo(Call, "evm_mload") &&
         Call->arg_size() == 2 &&
         detail::isConstantIntValue(Call->getArgOperand(1), 64);
}

bool isZero(Value *V) { return detail::isConstantIntValue(V, 0); }

bool isEvmLogCall(CallBase *Call) {
  StringRef Name = detail::getCalleeName(Call);
  if (!Name.starts_with("evm_log") || Name.size() != 8) {
    return false;
  }
  char TopicCount = Name.back();
  return TopicCount >= '0' && TopicCount <= '4';
}

struct ExternalCallMemoryArgs {
  Value *InputBase = nullptr;
  Value *InputSize = nullptr;
  Value *OutputBase = nullptr;
  Value *OutputSize = nullptr;
};

std::optional<ExternalCallMemoryArgs> getExternalCallMemoryArgs(CallBase *Call) {
  StringRef Name = detail::getCalleeName(Call);
  if ((Name == "evm_call" || Name == "evm_callcode") &&
      Call->arg_size() == 10) {
    return ExternalCallMemoryArgs{Call->getArgOperand(6),
                                  Call->getArgOperand(7),
                                  Call->getArgOperand(8),
                                  Call->getArgOperand(9)};
  }
  if ((Name == "evm_delegatecall" || Name == "evm_staticcall") &&
      Call->arg_size() == 9) {
    return ExternalCallMemoryArgs{Call->getArgOperand(5),
                                  Call->getArgOperand(6),
                                  Call->getArgOperand(7),
                                  Call->getArgOperand(8)};
  }
  return std::nullopt;
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
  FunctionType *FTy = FunctionType::get(Type::getVoidTy(M.getContext()), Args,
                                        false);
  FunctionCallee Callee = M.getOrInsertFunction(Name, FTy);
  Out = cast<Function>(Callee.getCallee());
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
  Builder.CreateCall(Marker,
                     {asI256(Builder, Alloc.Base), asI256(Builder, Alloc.Size)});
}

void insertWriteMarker(LLVMContext &Ctx, const MemoryWrite &Write) {
  if (Write.StoreOrCopy == nullptr || Write.Base == nullptr ||
      !Write.Offset.has_value() || Write.ValueOrSize == nullptr) {
    return;
  }
  Module *M = Write.StoreOrCopy->getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  IRBuilder<> Builder(Write.StoreOrCopy);
  Function *Marker = nullptr;
  if (Write.Kind == MemoryWriteKind::MStore) {
    getOrDeclareMarker(*M, "notdec_solidity_memory_write",
                       {I256, I256, I256}, Marker);
    Builder.CreateCall(Marker, {asI256(Builder, Write.Base),
                                ConstantInt::get(I256, *Write.Offset),
                                asI256(Builder, Write.ValueOrSize)});
    return;
  }

  if (Write.SourceOffset == nullptr) {
    return;
  }
  getOrDeclareMarker(*M, "notdec_solidity_memory_copy_write",
                     {I256, I256, I256, I256, I256}, Marker);
  Builder.CreateCall(
      Marker, {asI256(Builder, Write.Base),
               ConstantInt::get(I256, *Write.Offset),
               asI256(Builder, Write.SourceOffset),
               asI256(Builder, Write.ValueOrSize),
               ConstantInt::get(I256, static_cast<uint64_t>(Write.Kind))});
}

void insertConsumerMarker(LLVMContext &Ctx, const MemoryConsumer &Consumer) {
  if (Consumer.Call == nullptr || Consumer.Base == nullptr ||
      Consumer.Size == nullptr) {
    return;
  }
  Module *M = Consumer.Call->getModule();
  Function *Marker = nullptr;
  Type *I256 = Type::getIntNTy(Ctx, 256);
  getOrDeclareMarker(*M, "notdec_solidity_memory_consumer",
                     {I256, I256, I256}, Marker);
  IRBuilder<> Builder(Consumer.Call);
  Builder.CreateCall(
      Marker, {asI256(Builder, Consumer.Base), asI256(Builder, Consumer.Size),
               ConstantInt::get(I256, static_cast<uint64_t>(Consumer.Kind))});
}

} // namespace

MemoryBufferFacts analyzeMemoryBuffers(Function &F) {
  MemoryBufferFacts Facts;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !detail::isCallTo(Call, "evm_mstore") ||
        Call->arg_size() != 3) {
      continue;
    }
    if (!detail::isConstantIntValue(Call->getArgOperand(1), 64)) {
      continue;
    }
    Value *NewPtr = Call->getArgOperand(2);
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
      Facts.Allocations.push_back(
          MemoryAllocation{Op, Size, cast<Instruction>(Op), &I, true});
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
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (detail::isCallTo(Call, "evm_mstore") && Call->arg_size() == 3) {
      Value *Ptr = Call->getArgOperand(1);
      for (Value *Base : Bases) {
        std::optional<uint64_t> Offset = getOffsetFromBase(Ptr, Base);
        if (!Offset.has_value()) {
          continue;
        }
        Facts.Writes.push_back(MemoryWrite{Call, Base, Offset,
                                           Call->getArgOperand(2),
                                           nullptr,
                                           MemoryWriteKind::MStore});
        break;
      }
      continue;
    }

    if ((detail::isCallTo(Call, "evm_calldatacopy") ||
         detail::isCallTo(Call, "evm_returndatacopy")) &&
        Call->arg_size() == 5) {
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
        std::optional<uint64_t> Offset = getOffsetFromBase(Dst, Base);
        if (!Offset.has_value()) {
          continue;
        }
        MemoryWriteKind Kind = detail::isCallTo(Call, "evm_calldatacopy")
                                   ? MemoryWriteKind::CalldataCopy
                                   : MemoryWriteKind::ReturndataCopy;
        Facts.Writes.push_back(MemoryWrite{Call, Base, Offset,
                                           Call->getArgOperand(4),
                                           Call->getArgOperand(3), Kind});
        MatchedBase = true;
        break;
      }
      if (!MatchedBase) {
        MemoryWriteKind Kind = detail::isCallTo(Call, "evm_calldatacopy")
                                   ? MemoryWriteKind::CalldataCopy
                                   : MemoryWriteKind::ReturndataCopy;
        Facts.Writes.push_back(MemoryWrite{Call, Dst, 0, Call->getArgOperand(4),
                                           Call->getArgOperand(3), Kind});
      }
      continue;
    }

    if (detail::isCallTo(Call, "evm_return") && Call->arg_size() == 3) {
      if (isFreeMemoryPointerLoad(Call->getArgOperand(1))) {
        Facts.Consumers.push_back(MemoryConsumer{
            Call, MemoryConsumerKind::Return, Call->getArgOperand(1),
            Call->getArgOperand(2)});
      } else if (isZero(Call->getArgOperand(1)) &&
                 !isZero(Call->getArgOperand(2))) {
        Facts.Consumers.push_back(MemoryConsumer{
            Call, MemoryConsumerKind::Return,
            ConstantInt::get(Call->getArgOperand(1)->getType(), 0),
            Call->getArgOperand(2)});
      }
      continue;
    }

    if (detail::isCallTo(Call, "evm_revert") && Call->arg_size() == 3) {
      if (isFreeMemoryPointerLoad(Call->getArgOperand(1))) {
        Facts.Consumers.push_back(MemoryConsumer{
            Call, MemoryConsumerKind::Revert, Call->getArgOperand(1),
            Call->getArgOperand(2)});
      } else if (isZero(Call->getArgOperand(1)) &&
                 !isZero(Call->getArgOperand(2))) {
        Facts.Consumers.push_back(MemoryConsumer{
            Call, MemoryConsumerKind::Revert,
            ConstantInt::get(Call->getArgOperand(1)->getType(), 0),
            Call->getArgOperand(2)});
      }
      continue;
    }

    if (isEvmLogCall(Call) && Call->arg_size() >= 3) {
      if (isFreeMemoryPointerLoad(Call->getArgOperand(1))) {
        Facts.Consumers.push_back(MemoryConsumer{
            Call, MemoryConsumerKind::EventLog, Call->getArgOperand(1),
            Call->getArgOperand(2)});
      }
      continue;
    }

    std::optional<ExternalCallMemoryArgs> ExternalArgs =
        getExternalCallMemoryArgs(Call);
    if (ExternalArgs.has_value()) {
      if (isFreeMemoryPointerLoad(ExternalArgs->InputBase)) {
        Facts.Consumers.push_back(MemoryConsumer{
            Call, MemoryConsumerKind::ExternalCallInput,
            ExternalArgs->InputBase, ExternalArgs->InputSize});
      }
      if (isFreeMemoryPointerLoad(ExternalArgs->OutputBase)) {
        Facts.Consumers.push_back(MemoryConsumer{
            Call, MemoryConsumerKind::ExternalCallOutput,
            ExternalArgs->OutputBase, ExternalArgs->OutputSize});
      }
      continue;
    }
  }

  return Facts;
}

PreservedAnalyses MemoryBufferRewritePass::run(Function &F,
                                               FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  MemoryBufferFacts Facts = analyzeMemoryBuffers(F);
  bool Changed = false;

  for (const MemoryAllocation &Alloc : Facts.Allocations) {
    insertAllocationMarker(Ctx, Alloc);
    ++NumMemoryAllocations;
    Changed = true;
  }

  for (const MemoryWrite &Write : Facts.Writes) {
    insertWriteMarker(Ctx, Write);
    ++NumMemoryWrites;
    Changed = true;
  }

  for (const MemoryConsumer &Consumer : Facts.Consumers) {
    insertConsumerMarker(Ctx, Consumer);
    ++NumMemoryConsumers;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
