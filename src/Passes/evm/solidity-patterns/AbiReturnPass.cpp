#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/IRBuilder.h>
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
