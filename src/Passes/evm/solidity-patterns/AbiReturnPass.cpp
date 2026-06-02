#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumAbiReturns, "Number of Solidity ABI return sites found");
STATISTIC(NumAbiReturnDataWordWrites,
          "Number of Solidity ABI return data word writes found");
STATISTIC(NumAbiReturnDataCopyWrites,
          "Number of Solidity ABI return data copy writes found");
STATISTIC(NumAbiReturnDataAllocations,
          "Number of Solidity ABI return data allocations found");
STATISTIC(NumAbiReturnDynamicArraySources,
          "Number of Solidity ABI return dynamic array sources found");
STATISTIC(NumAbiReturnDynamicArrayCopyLoops,
          "Number of Solidity ABI return dynamic array copy loops found");
STATISTIC(NumAbiReturnDynamicArrayMCopies,
          "Number of Solidity ABI return dynamic array mcopies found");
STATISTIC(NumAbiReturnDynamicArrayHelperCopies,
          "Number of Solidity ABI return dynamic array helper copies found");
STATISTIC(NumAbiReturnDynamicArrayStorageSources,
          "Number of Solidity ABI return dynamic array storage sources found");
STATISTIC(NumAbiReturnDynamicArrayMemorySources,
          "Number of Solidity ABI return dynamic array memory sources found");
STATISTIC(NumAbiReturnDynamicArrayLiteralSources,
          "Number of Solidity ABI return dynamic array literal sources found");

namespace {

StringRef classifyAbiReturnSize(Value *Size) {
  if (isConstantIntValue(Size, 32)) {
    return "static_1_word";
  }
  if (isReturndataSize(Size)) {
    return "returndata_forward";
  }
  return "candidate";
}

uint64_t getAbiReturnKindCode(StringRef Kind) {
  if (Kind == "static_1_word") {
    return 1;
  }
  if (Kind == "returndata_forward") {
    return 2;
  }
  return 0;
}

CallBase *findReturnConsumerMarker(BasicBlock &BB, CallBase &Return) {
  for (Instruction &I : BB) {
    if (&I == &Return) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr ||
        !isCallTo(Call, "notdec_solidity_memory_consumer") ||
        Call->arg_size() != 3 ||
        !isConstantIntValue(Call->getArgOperand(2), 1)) {
      continue;
    }
    if (Call->getArgOperand(0) == Return.getArgOperand(1) &&
        Call->getArgOperand(1) == Return.getArgOperand(2)) {
      return Call;
    }
  }
  return nullptr;
}

bool isAbiHeadOffset(Value *V) {
  std::optional<uint64_t> Offset = getUInt64Constant(V);
  return Offset.has_value() && (*Offset % 32) == 0;
}

bool isSameAbiReturnDataBase(Value *LHS, Value *RHS) {
  if (LHS == RHS) {
    return true;
  }
  std::optional<uint64_t> LConst = getUInt64Constant(LHS);
  std::optional<uint64_t> RConst = getUInt64Constant(RHS);
  return LConst.has_value() && RConst.has_value() && *LConst == *RConst;
}

void collectAbiReturnDataWordWriteMarkers(
    BasicBlock &BB, CallBase &Return, Value *ReturnBase,
    SmallVectorImpl<CallBase *> &Writes) {
  SmallVector<CallBase *, 8> Candidates;

  for (Instruction &I : BB) {
    if (&I == &Return) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isFreeMemoryPointerStore(Call)) {
      Candidates.clear();
      continue;
    }

    if (!isCallTo(Call, "notdec_solidity_memory_write") ||
        Call->arg_size() != 3 || !isAbiHeadOffset(Call->getArgOperand(1))) {
      continue;
    }

    Value *WriteBase = Call->getArgOperand(0);
    if (isSameOrReloadedFreeMemoryBase(WriteBase, ReturnBase)) {
      Candidates.push_back(Call);
    }
  }

  Writes.append(Candidates.begin(), Candidates.end());
}

void collectAbiReturnDataCopyWriteMarkers(
    BasicBlock &BB, CallBase &Return, Value *ReturnBase,
    SmallVectorImpl<CallBase *> &Writes) {
  SmallVector<CallBase *, 8> Candidates;

  for (Instruction &I : BB) {
    if (&I == &Return) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isFreeMemoryPointerStore(Call)) {
      Candidates.clear();
      continue;
    }

    if (!isCallTo(Call, "notdec_solidity_memory_copy_write") ||
        Call->arg_size() != 5 || !isAbiHeadOffset(Call->getArgOperand(1))) {
      continue;
    }

    Value *CopyBase = Call->getArgOperand(0);
    if (isSameOrReloadedFreeMemoryBase(CopyBase, ReturnBase)) {
      Candidates.push_back(Call);
    }
  }

  Writes.append(Candidates.begin(), Candidates.end());
}

CallBase *findAbiReturnDataAllocationMarker(BasicBlock &BB, CallBase &Return,
                                            ArrayRef<CallBase *> WordWrites,
                                            ArrayRef<CallBase *> CopyWrites) {
  auto MatchesReturnDataBase = [&](Value *AllocationBase) {
    for (CallBase *Write : WordWrites) {
      if (Write->arg_size() == 3 &&
          isSameAbiReturnDataBase(AllocationBase, Write->getArgOperand(0))) {
        return true;
      }
    }
    for (CallBase *Copy : CopyWrites) {
      if (Copy->arg_size() == 5 &&
          isSameAbiReturnDataBase(AllocationBase, Copy->getArgOperand(0))) {
        return true;
      }
    }
    return false;
  };

  CallBase *Candidate = nullptr;
  for (Instruction &I : BB) {
    if (&I == &Return) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (!isCallTo(Call, "notdec_solidity_memory_allocation") ||
        Call->arg_size() != 2) {
      continue;
    }
    if (MatchesReturnDataBase(Call->getArgOperand(0))) {
      Candidate = Call;
    }
  }

  return Candidate;
}

struct AbiReturnDynamicArraySource {
  // Dynamic bytes/string ABI returns write offset 32 and then the source array
  // length; keep that source explicit for later copy-loop rewrites.
  Value *ReturnBase = nullptr;
  Value *SourceArray = nullptr;
  Value *Length = nullptr;
};

struct AbiReturnDynamicArrayCopyLoop {
  Value *ReturnBase = nullptr;
  Value *SourceArray = nullptr;
  Value *Length = nullptr;
  CallBase *Load = nullptr;
  CallBase *Store = nullptr;
};

struct AbiReturnDynamicArrayMCopy {
  Value *ReturnBase = nullptr;
  Value *SourceArray = nullptr;
  Value *Length = nullptr;
  CallBase *CopyWrite = nullptr;
};

struct AbiReturnDynamicArrayHelperCopy {
  // Some Solidity builds keep copy_memory_to_memory_with_cleanup outlined; this
  // keeps the call site tied to the same ABI return source/copy relation.
  Value *ReturnBase = nullptr;
  Value *SourceArray = nullptr;
  Value *Length = nullptr;
  CallBase *CopyCall = nullptr;
};

struct AbiReturnDynamicArrayStorageSource {
  // Public string/bytes getters often call a helper that decodes storage bytes
  // into memory and returns the source array later copied into ABI return data.
  Value *ReturnBase = nullptr;
  Value *SourceArray = nullptr;
  Value *Length = nullptr;
  CallBase *SourceCall = nullptr;
};

struct AbiReturnDynamicArrayMemorySource {
  // Literal and in-memory string/bytes builders materialize a Solidity memory
  // array as [length][data...] before the ABI wrapper copies it to return data.
  Value *ReturnBase = nullptr;
  Value *SourceArray = nullptr;
  Value *Length = nullptr;
  CallBase *LengthWrite = nullptr;
  CallBase *DataWrite = nullptr;
};

Value *getMemoryLoadPointer(Value *V) {
  auto *Call = dyn_cast_or_null<CallBase>(V);
  if (Call == nullptr || !isCallTo(Call, "evm_mload") ||
      Call->arg_size() != 2) {
    return nullptr;
  }
  return Call->getArgOperand(1);
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

std::optional<AbiReturnDynamicArraySource>
findAbiReturnDynamicArraySource(Function &F, CallBase &Return,
                                Value *ReturnBase, DominatorTree &DT) {
  CallBase *HeadOffsetWrite = nullptr;
  CallBase *LengthWrite = nullptr;

  for (Instruction &I : instructions(F)) {
    if (&I == &Return || !DT.dominates(&I, &Return)) {
      continue;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isFreeMemoryPointerStore(Call)) {
      HeadOffsetWrite = nullptr;
      LengthWrite = nullptr;
      continue;
    }

    if (!isCallTo(Call, "notdec_solidity_memory_write") ||
        Call->arg_size() != 3 ||
        !isSameOrReloadedFreeMemoryBase(Call->getArgOperand(0), ReturnBase)) {
      continue;
    }

    if (isConstantIntValue(Call->getArgOperand(1), 0) &&
        isConstantIntValue(Call->getArgOperand(2), 32)) {
      HeadOffsetWrite = Call;
      continue;
    }

    if (!isConstantIntValue(Call->getArgOperand(1), 32)) {
      continue;
    }
    Value *SourceArray = getMemoryLoadPointer(Call->getArgOperand(2));
    if (SourceArray == nullptr || !valueAvailableAt(SourceArray, Return, DT)) {
      continue;
    }
    LengthWrite = Call;
  }

  if (HeadOffsetWrite == nullptr || LengthWrite == nullptr) {
    return std::nullopt;
  }
  Value *Length = LengthWrite->getArgOperand(2);
  Value *SourceArray = getMemoryLoadPointer(Length);
  if (SourceArray == nullptr) {
    return std::nullopt;
  }
  return AbiReturnDynamicArraySource{ReturnBase, SourceArray, Length};
}

Value *stripAddConstant(Value *V, uint64_t Constant) {
  auto *Add = dyn_cast_or_null<BinaryOperator>(V);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return nullptr;
  }
  for (unsigned I = 0; I < 2; ++I) {
    if (isConstantIntValue(Add->getOperand(I), Constant)) {
      return Add->getOperand(1 - I);
    }
  }
  return nullptr;
}

Value *matchBasePlusIndex(Value *Ptr, Value *Base) {
  auto *Add = dyn_cast_or_null<BinaryOperator>(Ptr);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return nullptr;
  }
  for (unsigned I = 0; I < 2; ++I) {
    if (Add->getOperand(I) == Base ||
        isSameOrReloadedFreeMemoryBase(Add->getOperand(I), Base)) {
      return Add->getOperand(1 - I);
    }
  }
  return nullptr;
}

Value *matchBasePlusConstantPlusIndex(Value *Ptr, Value *Base,
                                      uint64_t Constant) {
  if (Value *BasePlusIndex = stripAddConstant(Ptr, Constant)) {
    if (Value *Index = matchBasePlusIndex(BasePlusIndex, Base)) {
      return Index;
    }
  }

  auto *Add = dyn_cast_or_null<BinaryOperator>(Ptr);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return nullptr;
  }
  for (unsigned I = 0; I < 2; ++I) {
    Value *BaseWithConstant = Add->getOperand(I);
    Value *Index = Add->getOperand(1 - I);
    Value *MaybeBase = stripAddConstant(BaseWithConstant, Constant);
    if (MaybeBase != nullptr &&
        isSameOrReloadedFreeMemoryBase(MaybeBase, Base)) {
      return Index;
    }
    if (MaybeBase != nullptr && MaybeBase == Base) {
      return Index;
    }
  }
  return nullptr;
}

std::optional<AbiReturnDynamicArrayCopyLoop>
findAbiReturnDynamicArrayCopyLoop(Function &F,
                                  const AbiReturnDynamicArraySource &Source,
                                  DominatorTree &DT) {
  for (Instruction &I : instructions(F)) {
    auto *Store = dyn_cast<CallBase>(&I);
    if (Store == nullptr || !isCallTo(Store, "evm_mstore") ||
        Store->arg_size() != 3) {
      continue;
    }

    auto *Load = dyn_cast<CallBase>(Store->getArgOperand(2));
    if (Load == nullptr || !isCallTo(Load, "evm_mload") ||
        Load->arg_size() != 2 || !valueAvailableAt(Load, *Store, DT)) {
      continue;
    }

    Value *DstIndex = matchBasePlusConstantPlusIndex(
        Store->getArgOperand(1), Source.ReturnBase, 64);
    Value *SrcIndex = matchBasePlusConstantPlusIndex(
        Load->getArgOperand(1), Source.SourceArray, 32);
    if (DstIndex == nullptr || SrcIndex == nullptr || DstIndex != SrcIndex) {
      continue;
    }

    return AbiReturnDynamicArrayCopyLoop{Source.ReturnBase, Source.SourceArray,
                                         Source.Length, Load, Store};
  }
  return std::nullopt;
}

bool isSourceArrayDataStart(Value *V, Value *SourceArray) {
  auto *Add = dyn_cast_or_null<BinaryOperator>(V);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return false;
  }
  for (unsigned I = 0; I < 2; ++I) {
    if (Add->getOperand(I) == SourceArray &&
        isConstantIntValue(Add->getOperand(1 - I), 32)) {
      return true;
    }
  }
  return false;
}

Value *getArg(Function &F, unsigned Index) {
  if (Index >= F.arg_size()) {
    return nullptr;
  }
  return F.getArg(Index);
}

bool isAddOf(Value *V, Value *LHS, Value *RHS) {
  auto *Add = dyn_cast_or_null<BinaryOperator>(V);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return false;
  }
  return (Add->getOperand(0) == LHS && Add->getOperand(1) == RHS) ||
         (Add->getOperand(0) == RHS && Add->getOperand(1) == LHS);
}

bool isMemoryToMemoryCopyHelper(Function *Callee) {
  if (Callee == nullptr || Callee->arg_size() < 7) {
    return false;
  }
  Value *Src = getArg(*Callee, 4);
  Value *Dst = getArg(*Callee, 5);
  Value *Length = getArg(*Callee, 6);

  bool HasCopy = false;
  bool HasCleanup = false;
  for (Instruction &I : instructions(Callee)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_mstore") ||
        Call->arg_size() != 3) {
      continue;
    }

    Value *StorePtr = Call->getArgOperand(1);
    if (isConstantIntValue(Call->getArgOperand(2), 0) &&
        isAddOf(StorePtr, Dst, Length)) {
      HasCleanup = true;
      continue;
    }

    auto *Load = dyn_cast<CallBase>(Call->getArgOperand(2));
    if (Load == nullptr || !isCallTo(Load, "evm_mload") ||
        Load->arg_size() != 2) {
      continue;
    }
    Value *DstIndex = matchBasePlusIndex(StorePtr, Dst);
    Value *SrcIndex = matchBasePlusIndex(Load->getArgOperand(1), Src);
    if (DstIndex != nullptr && DstIndex == SrcIndex) {
      HasCopy = true;
    }
  }

  return HasCopy && HasCleanup;
}

std::optional<AbiReturnDynamicArrayMCopy>
findAbiReturnDynamicArrayMCopy(Function &F, CallBase &Return,
                               const AbiReturnDynamicArraySource &Source,
                               DominatorTree &DT) {
  for (Instruction &I : instructions(F)) {
    auto *Copy = dyn_cast<CallBase>(&I);
    if (Copy == nullptr ||
        !isCallTo(Copy, "notdec_solidity_memory_copy_write") ||
        Copy->arg_size() != 5 || !DT.dominates(Copy, &Return)) {
      continue;
    }
    if (!isSameOrReloadedFreeMemoryBase(Copy->getArgOperand(0),
                                        Source.ReturnBase) ||
        !isConstantIntValue(Copy->getArgOperand(1), 64) ||
        !isSourceArrayDataStart(Copy->getArgOperand(2), Source.SourceArray) ||
        Copy->getArgOperand(3) != Source.Length ||
        !isConstantIntValue(Copy->getArgOperand(4), 6)) {
      continue;
    }
    return AbiReturnDynamicArrayMCopy{Source.ReturnBase, Source.SourceArray,
                                      Source.Length, Copy};
  }
  return std::nullopt;
}

std::optional<AbiReturnDynamicArrayHelperCopy>
findAbiReturnDynamicArrayHelperCopy(Function &F, CallBase &Return,
                                    const AbiReturnDynamicArraySource &Source,
                                    DominatorTree &DT) {
  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !DT.dominates(Call, &Return) ||
        Call->arg_size() < 7 ||
        !isMemoryToMemoryCopyHelper(Call->getCalledFunction())) {
      continue;
    }
    Value *DstBase = stripAddConstant(Call->getArgOperand(5), 64);
    if (!isSourceArrayDataStart(Call->getArgOperand(4), Source.SourceArray) ||
        DstBase == nullptr ||
        !isSameOrReloadedFreeMemoryBase(DstBase, Source.ReturnBase) ||
        Call->getArgOperand(6) != Source.Length) {
      continue;
    }
    return AbiReturnDynamicArrayHelperCopy{Source.ReturnBase,
                                           Source.SourceArray, Source.Length,
                                           Call};
  }
  return std::nullopt;
}

CallBase *getSourceArrayProducerCall(Value *SourceArray) {
  if (auto *Call = dyn_cast_or_null<CallBase>(SourceArray)) {
    return Call;
  }
  auto *Extract = dyn_cast_or_null<ExtractValueInst>(SourceArray);
  if (Extract == nullptr || Extract->getNumIndices() != 1 ||
      *Extract->idx_begin() != 0) {
    return nullptr;
  }
  return dyn_cast<CallBase>(Extract->getAggregateOperand());
}

bool functionHasStorageBytesSource(Function *Callee) {
  if (Callee == nullptr) {
    return false;
  }
  bool HasStorageSlotLoad = false;
  bool HasStorageDataHash = false;
  bool HasStorageDataLoad = false;
  for (Instruction &I : instructions(Callee)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    if (isCallTo(Call, "evm_sload") && Call->arg_size() == 1) {
      if (getUInt64Constant(Call->getArgOperand(0)).has_value()) {
        HasStorageSlotLoad = true;
      } else {
        HasStorageDataLoad = true;
      }
      continue;
    }
    if (isCallTo(Call, "evm_sha3") && Call->arg_size() == 3 &&
        isConstantIntValue(Call->getArgOperand(1), 0) &&
        isConstantIntValue(Call->getArgOperand(2), 32)) {
      HasStorageDataHash = true;
    }
  }
  return HasStorageSlotLoad && HasStorageDataHash && HasStorageDataLoad;
}

std::optional<AbiReturnDynamicArrayStorageSource>
findAbiReturnDynamicArrayStorageSource(
    Function &F, CallBase &Return, const AbiReturnDynamicArraySource &Source,
    DominatorTree &DT) {
  CallBase *SourceCall = getSourceArrayProducerCall(Source.SourceArray);
  if (SourceCall == nullptr || !DT.dominates(SourceCall, &Return) ||
      SourceCall->getParent()->getParent() != &F) {
    return std::nullopt;
  }
  Function *Callee = SourceCall->getCalledFunction();
  if (!functionHasStorageBytesSource(Callee)) {
    return std::nullopt;
  }
  return AbiReturnDynamicArrayStorageSource{
      Source.ReturnBase, Source.SourceArray, Source.Length, SourceCall};
}

std::optional<AbiReturnDynamicArrayMemorySource>
findAbiReturnDynamicArrayMemorySource(
    Function &F, CallBase &Return, const AbiReturnDynamicArraySource &Source,
    DominatorTree &DT) {
  CallBase *LengthWrite = nullptr;
  CallBase *DataWrite = nullptr;

  for (Instruction &I : instructions(F)) {
    if (&I == &Return || !DT.dominates(&I, &Return)) {
      continue;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "notdec_solidity_memory_write") ||
        Call->arg_size() != 3 ||
        !isSameAbiReturnDataBase(Call->getArgOperand(0), Source.SourceArray)) {
      continue;
    }

    std::optional<uint64_t> Offset = getUInt64Constant(Call->getArgOperand(1));
    if (!Offset.has_value()) {
      continue;
    }
    if (*Offset == 0) {
      LengthWrite = Call;
      continue;
    }
    if (*Offset >= 32) {
      DataWrite = Call;
    }
  }

  if (LengthWrite == nullptr || DataWrite == nullptr) {
    return std::nullopt;
  }
  return AbiReturnDynamicArrayMemorySource{Source.ReturnBase,
                                           Source.SourceArray, Source.Length,
                                           LengthWrite, DataWrite};
}

bool isLiteralDataWord(Value *V) {
  if (isa_and_nonnull<ConstantInt>(V)) {
    return true;
  }
  auto *Call = dyn_cast_or_null<CallBase>(V);
  if (Call == nullptr || !isCallTo(Call, "evm_shl") || Call->arg_size() != 2) {
    return false;
  }
  return isa<ConstantInt>(Call->getArgOperand(0)) &&
         isa<ConstantInt>(Call->getArgOperand(1));
}

bool isAbiReturnDynamicArrayLiteralSource(
    const AbiReturnDynamicArrayMemorySource &MemorySource) {
  return isa<ConstantInt>(MemorySource.LengthWrite->getArgOperand(2)) &&
         isLiteralDataWord(MemorySource.DataWrite->getArgOperand(2));
}

void insertAbiReturnMemoryConsumerMarker(LLVMContext &Ctx, CallBase &Return,
                                         CallBase &Consumer, StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_memory_consumer",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256}, false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(Marker,
                     {Consumer.getArgOperand(0), Consumer.getArgOperand(1),
                      ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

void insertAbiReturnDynamicArraySourceMarker(
    LLVMContext &Ctx, CallBase &Return,
    const AbiReturnDynamicArraySource &Source, CallBase &Consumer,
    StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_dynamic_array_source",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(Marker,
                     {Source.ReturnBase, Source.SourceArray, Source.Length,
                      Consumer.getArgOperand(1),
                      ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

void insertAbiReturnDynamicArrayCopyLoopMarker(
    LLVMContext &Ctx, CallBase &Return,
    const AbiReturnDynamicArrayCopyLoop &CopyLoop, CallBase &Consumer,
    StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_dynamic_array_copy_loop",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(Marker,
                     {CopyLoop.ReturnBase, CopyLoop.SourceArray,
                      CopyLoop.Length, Consumer.getArgOperand(1),
                      ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

void insertAbiReturnDynamicArrayMCopyMarker(
    LLVMContext &Ctx, CallBase &Return,
    const AbiReturnDynamicArrayMCopy &MCopy, CallBase &Consumer,
    StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_dynamic_array_mcopy",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(Marker,
                     {MCopy.ReturnBase, MCopy.SourceArray, MCopy.Length,
                      Consumer.getArgOperand(1),
                      ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

void insertAbiReturnDynamicArrayHelperCopyMarker(
    LLVMContext &Ctx, CallBase &Return,
    const AbiReturnDynamicArrayHelperCopy &HelperCopy, CallBase &Consumer,
    StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_dynamic_array_helper_copy",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(
      Marker, {HelperCopy.ReturnBase, HelperCopy.SourceArray, HelperCopy.Length,
               Consumer.getArgOperand(1),
               ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

void insertAbiReturnDynamicArrayStorageSourceMarker(
    LLVMContext &Ctx, CallBase &Return,
    const AbiReturnDynamicArrayStorageSource &StorageSource, CallBase &Consumer,
    StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_dynamic_array_storage_source",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(
      Marker,
      {StorageSource.ReturnBase, StorageSource.SourceArray, StorageSource.Length,
       Consumer.getArgOperand(1),
       ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

void insertAbiReturnDynamicArrayMemorySourceMarker(
    LLVMContext &Ctx, CallBase &Return,
    const AbiReturnDynamicArrayMemorySource &MemorySource, CallBase &Consumer,
    StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_dynamic_array_memory_source",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(
      Marker,
      {MemorySource.ReturnBase, MemorySource.SourceArray, MemorySource.Length,
       Consumer.getArgOperand(1),
       ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

void insertAbiReturnDynamicArrayLiteralSourceMarker(
    LLVMContext &Ctx, CallBase &Return,
    const AbiReturnDynamicArrayMemorySource &MemorySource, CallBase &Consumer,
    StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_dynamic_array_literal_source",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(
      Marker,
      {MemorySource.ReturnBase, MemorySource.SourceArray, MemorySource.Length,
       Consumer.getArgOperand(1),
       ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

void insertAbiReturnDataAllocationMarker(LLVMContext &Ctx, CallBase &Return,
                                         CallBase &Allocation,
                                         CallBase &Consumer, StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_data_allocation",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256, I256},
                        false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(Marker,
                     {Allocation.getArgOperand(0), Allocation.getArgOperand(1),
                      Consumer.getArgOperand(1),
                      ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

void insertAbiReturnDataWordWriteMarker(LLVMContext &Ctx, CallBase &Return,
                                        CallBase &WordWrite, StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_data_word_write",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256, I256},
                        false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(Marker,
                     {WordWrite.getArgOperand(0), WordWrite.getArgOperand(1),
                      WordWrite.getArgOperand(2),
                      ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

void insertAbiReturnDataCopyWriteMarker(LLVMContext &Ctx, CallBase &Return,
                                        CallBase &CopyWrite, StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_data_copy_write",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(Marker,
                     {CopyWrite.getArgOperand(0), CopyWrite.getArgOperand(1),
                      CopyWrite.getArgOperand(2), CopyWrite.getArgOperand(3),
                      CopyWrite.getArgOperand(4),
                      ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

} // namespace

PreservedAnalyses AbiReturnPass::run(Function &F, FunctionAnalysisManager &FAM) {
  LLVMContext &Ctx = F.getContext();
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_return") ||
        Call->arg_size() != 3) {
      continue;
    }

    StringRef Kind = classifyAbiReturnSize(Call->getArgOperand(2));
    if (CallBase *Consumer = findReturnConsumerMarker(*Call->getParent(),
                                                      *Call)) {
      Kind = classifyAbiReturnSize(Consumer->getArgOperand(1));
      insertAbiReturnMemoryConsumerMarker(Ctx, *Call, *Consumer, Kind);
      SmallVector<CallBase *, 8> WordWrites;
      collectAbiReturnDataWordWriteMarkers(*Call->getParent(), *Call,
                                           Consumer->getArgOperand(0),
                                           WordWrites);
      for (CallBase *WordWrite : WordWrites) {
        insertAbiReturnDataWordWriteMarker(Ctx, *Call, *WordWrite, Kind);
        ++NumAbiReturnDataWordWrites;
      }
      SmallVector<CallBase *, 8> CopyWrites;
      collectAbiReturnDataCopyWriteMarkers(*Call->getParent(), *Call,
                                           Consumer->getArgOperand(0),
                                           CopyWrites);
      for (CallBase *CopyWrite : CopyWrites) {
        insertAbiReturnDataCopyWriteMarker(Ctx, *Call, *CopyWrite, Kind);
        ++NumAbiReturnDataCopyWrites;
      }
      if (CallBase *Allocation = findAbiReturnDataAllocationMarker(
              *Call->getParent(), *Call, WordWrites, CopyWrites)) {
        insertAbiReturnDataAllocationMarker(Ctx, *Call, *Allocation, *Consumer,
                                            Kind);
        ++NumAbiReturnDataAllocations;
      }
      std::optional<AbiReturnDynamicArraySource> DynamicSource =
          findAbiReturnDynamicArraySource(F, *Call, Consumer->getArgOperand(0),
                                          DT);
      if (DynamicSource.has_value()) {
        insertAbiReturnDynamicArraySourceMarker(Ctx, *Call, *DynamicSource,
                                                *Consumer, Kind);
        ++NumAbiReturnDynamicArraySources;
        std::optional<AbiReturnDynamicArrayCopyLoop> CopyLoop =
            findAbiReturnDynamicArrayCopyLoop(F, *DynamicSource, DT);
        if (CopyLoop.has_value()) {
          insertAbiReturnDynamicArrayCopyLoopMarker(Ctx, *Call, *CopyLoop,
                                                    *Consumer, Kind);
          ++NumAbiReturnDynamicArrayCopyLoops;
        }
        std::optional<AbiReturnDynamicArrayMCopy> MCopy =
            findAbiReturnDynamicArrayMCopy(F, *Call, *DynamicSource, DT);
        if (MCopy.has_value()) {
          insertAbiReturnDynamicArrayMCopyMarker(Ctx, *Call, *MCopy, *Consumer,
                                                 Kind);
          ++NumAbiReturnDynamicArrayMCopies;
        }
        std::optional<AbiReturnDynamicArrayHelperCopy> HelperCopy =
            findAbiReturnDynamicArrayHelperCopy(F, *Call, *DynamicSource, DT);
        if (HelperCopy.has_value()) {
          insertAbiReturnDynamicArrayHelperCopyMarker(Ctx, *Call, *HelperCopy,
                                                      *Consumer, Kind);
          ++NumAbiReturnDynamicArrayHelperCopies;
        }
        std::optional<AbiReturnDynamicArrayStorageSource> StorageSource =
            findAbiReturnDynamicArrayStorageSource(F, *Call, *DynamicSource,
                                                   DT);
        if (StorageSource.has_value()) {
          insertAbiReturnDynamicArrayStorageSourceMarker(
              Ctx, *Call, *StorageSource, *Consumer, Kind);
          ++NumAbiReturnDynamicArrayStorageSources;
        }
        std::optional<AbiReturnDynamicArrayMemorySource> MemorySource =
            findAbiReturnDynamicArrayMemorySource(F, *Call, *DynamicSource, DT);
        if (MemorySource.has_value()) {
          insertAbiReturnDynamicArrayMemorySourceMarker(
              Ctx, *Call, *MemorySource, *Consumer, Kind);
          ++NumAbiReturnDynamicArrayMemorySources;
          if (isAbiReturnDynamicArrayLiteralSource(*MemorySource)) {
            insertAbiReturnDynamicArrayLiteralSourceMarker(
                Ctx, *Call, *MemorySource, *Consumer, Kind);
            ++NumAbiReturnDynamicArrayLiteralSources;
          }
        }
      }
    }
    addStringMetadata(Ctx, I, KIND_SOLIDITY_ABI_RETURN, Kind);
    addStringMetadata(Ctx, F, KIND_SOLIDITY_ABI_RETURN, "true");
    ++NumAbiReturns;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
