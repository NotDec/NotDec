#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <optional>
#include <utility>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumEvents, "Number of Solidity event candidates found");
STATISTIC(NumEventMemoryConsumers,
          "Number of Solidity event memory consumers found");
STATISTIC(NumEventDataAllocations,
          "Number of Solidity event data allocations found");
STATISTIC(NumEventDataWordWrites,
          "Number of Solidity event data word writes found");
STATISTIC(NumEventDataCopyWrites,
          "Number of Solidity event data copy writes found");

namespace {

bool isEventAbiHeadOffset(Value *V) {
  std::optional<uint64_t> Offset = getUInt64Constant(V);
  return Offset.has_value() && (*Offset % 32) == 0;
}

bool isSameEventDataBase(Value *LHS, Value *RHS) {
  if (LHS == RHS) {
    return true;
  }
  std::optional<uint64_t> LConst = getUInt64Constant(LHS);
  std::optional<uint64_t> RConst = getUInt64Constant(RHS);
  return LConst.has_value() && RConst.has_value() && *LConst == *RConst;
}

Value *getSizedAllocationSize(Value *Base) {
  auto *Call = dyn_cast_or_null<CallBase>(Base);
  if (Call != nullptr && isCallTo(Call, "notdec_evm_alloc") &&
      Call->arg_size() == 1) {
    return Call->getArgOperand(0);
  }

  auto *PtrToInt = dyn_cast_or_null<PtrToIntInst>(Base);
  Call = PtrToInt == nullptr ? nullptr
                             : dyn_cast_or_null<CallBase>(PtrToInt->getOperand(0));
  if (Call != nullptr && isCallTo(Call, "calloc") && Call->arg_size() == 2) {
    return Call->getArgOperand(1);
  }
  return nullptr;
}

void collectEventDataWordWriteMarkers(BasicBlock &BB, CallBase &Log,
                                      Value *DataBase,
                                      SmallVectorImpl<CallBase *> &Writes) {
  SmallVector<CallBase *, 8> Candidates;

  for (Instruction &I : BB) {
    if (&I == &Log) {
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
        Call->arg_size() != 3 ||
        !isEventAbiHeadOffset(Call->getArgOperand(1))) {
      continue;
    }

    Value *WriteBase = Call->getArgOperand(0);
    if (isSameOrReloadedFreeMemoryBase(WriteBase, DataBase)) {
      Candidates.push_back(Call);
    }
  }

  Writes.append(Candidates.begin(), Candidates.end());
}

void collectEventDataCopyWriteMarkers(BasicBlock &BB, CallBase &Log,
                                      Value *DataBase,
                                      SmallVectorImpl<CallBase *> &Writes) {
  SmallVector<CallBase *, 8> Candidates;

  for (Instruction &I : BB) {
    if (&I == &Log) {
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
        Call->arg_size() != 5 ||
        !isEventAbiHeadOffset(Call->getArgOperand(1))) {
      continue;
    }

    Value *CopyBase = Call->getArgOperand(0);
    if (isSameOrReloadedFreeMemoryBase(CopyBase, DataBase)) {
      Candidates.push_back(Call);
    }
  }

  Writes.append(Candidates.begin(), Candidates.end());
}

std::optional<std::pair<Value *, Value *>>
findEventDataAllocation(BasicBlock &BB, CallBase &Log,
                        ArrayRef<CallBase *> WordWrites,
                        ArrayRef<CallBase *> CopyWrites) {
  auto MatchesEventDataBase = [&](Value *AllocationBase) {
    for (CallBase *Write : WordWrites) {
      if (Write->arg_size() == 3 &&
          isSameEventDataBase(AllocationBase, Write->getArgOperand(0))) {
        return true;
      }
    }
    for (CallBase *Copy : CopyWrites) {
      if (Copy->arg_size() == 5 &&
          isSameEventDataBase(AllocationBase, Copy->getArgOperand(0))) {
        return true;
      }
    }
    return false;
  };

  std::optional<std::pair<Value *, Value *>> Candidate;
  for (Instruction &I : BB) {
    if (&I == &Log) {
      break;
    }
    if (auto *Call = dyn_cast<CallBase>(&I)) {
      if (isCallTo(Call, "notdec_evm_finalize_alloc") &&
          Call->arg_size() == 2 &&
          MatchesEventDataBase(Call->getArgOperand(0))) {
        Candidate =
            std::make_pair(Call->getArgOperand(0), Call->getArgOperand(1));
        continue;
      }
    }
    if (Value *Size = getSizedAllocationSize(&I)) {
      if (MatchesEventDataBase(&I)) {
        Candidate = std::make_pair(&I, Size);
      }
    }
  }

  return Candidate;
}

// Consumer markers encode buffer role rather than memory layout. Keep this
// disabled until type recovery has a stable role carrier.
// void insertEventMemoryConsumerMarker(LLVMContext &Ctx, CallBase &Log,
//                                      Value *Base, Value *Size,
//                                      uint64_t TopicCount) {
//   Module *M = Log.getModule();
//   Type *I256 = Type::getIntNTy(Ctx, 256);
//   FunctionCallee Marker = M->getOrInsertFunction(
//       "notdec_solidity_event_memory_consumer",
//       FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256}, false));
//
//   IRBuilder<> Builder(&Log);
//   Builder.CreateCall(Marker, {Base, Size, ConstantInt::get(I256, TopicCount)});
// }

void insertEventDataAllocationMarker(LLVMContext &Ctx, CallBase &Log,
                                     Value *AllocationBase,
                                     Value *AllocationSize,
                                     Value *ConsumerSize,
                                     uint64_t TopicCount) {
  Module *M = Log.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_event_data_allocation",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256, I256},
                        false));

  IRBuilder<> Builder(&Log);
  Builder.CreateCall(Marker, {AllocationBase, AllocationSize, ConsumerSize,
                              ConstantInt::get(I256, TopicCount)});
}

void insertEventDataWordWriteMarker(LLVMContext &Ctx, CallBase &Log,
                                    CallBase &WordWrite,
                                    uint64_t TopicCount) {
  Module *M = Log.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_event_data_word_write",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256, I256},
                        false));

  IRBuilder<> Builder(&Log);
  Builder.CreateCall(Marker,
                     {WordWrite.getArgOperand(0), WordWrite.getArgOperand(1),
                      WordWrite.getArgOperand(2),
                      ConstantInt::get(I256, TopicCount)});
}

void insertEventDataCopyWriteMarker(LLVMContext &Ctx, CallBase &Log,
                                    CallBase &CopyWrite,
                                    uint64_t TopicCount) {
  Module *M = Log.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_event_data_copy_write",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&Log);
  Builder.CreateCall(Marker,
                     {CopyWrite.getArgOperand(0), CopyWrite.getArgOperand(1),
                      CopyWrite.getArgOperand(2), CopyWrite.getArgOperand(3),
                      CopyWrite.getArgOperand(4),
                      ConstantInt::get(I256, TopicCount)});
}

} // namespace

PreservedAnalyses EventLogPass::run(Function &F, FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    StringRef Name = getCalleeName(Call);
    if (!Name.starts_with("evm_log") || Name.size() != 8) {
      continue;
    }
    if (Name.back() < '0' || Name.back() > '4') {
      continue;
    }
    if (Call->arg_size() < 3) {
      continue;
    }
    uint64_t TopicCount = static_cast<uint64_t>(Name.back() - '0');
    {
      Value *DataBase = Call->getArgOperand(1);
      Value *DataSize = Call->getArgOperand(2);
      // insertEventMemoryConsumerMarker(Ctx, *Call, DataBase, DataSize,
      //                                 TopicCount);
      // ++NumEventMemoryConsumers;
      SmallVector<CallBase *, 8> WordWrites;
      collectEventDataWordWriteMarkers(*Call->getParent(), *Call, DataBase,
                                       WordWrites);
      for (CallBase *WordWrite : WordWrites) {
        insertEventDataWordWriteMarker(Ctx, *Call, *WordWrite, TopicCount);
        ++NumEventDataWordWrites;
      }
      SmallVector<CallBase *, 8> CopyWrites;
      collectEventDataCopyWriteMarkers(*Call->getParent(), *Call, DataBase,
                                       CopyWrites);
      for (CallBase *CopyWrite : CopyWrites) {
        insertEventDataCopyWriteMarker(Ctx, *Call, *CopyWrite, TopicCount);
        ++NumEventDataCopyWrites;
      }
      if (std::optional<std::pair<Value *, Value *>> Allocation =
              findEventDataAllocation(*Call->getParent(), *Call, WordWrites,
                                      CopyWrites)) {
        insertEventDataAllocationMarker(Ctx, *Call, Allocation->first,
                                        Allocation->second, DataSize,
                                        TopicCount);
        ++NumEventDataAllocations;
      }
    }
    addStringMetadata(Ctx, I, KIND_SOLIDITY_EVENT,
                      ("topic_count_" + Twine(Name.back())).str());
    ++NumEvents;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
