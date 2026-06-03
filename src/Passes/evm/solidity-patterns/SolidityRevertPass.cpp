#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumReverts, "Number of Solidity revert sites found");
STATISTIC(NumRevertMemoryConsumers,
          "Number of Solidity revert memory consumers found");
STATISTIC(NumRevertDataWordWrites,
          "Number of Solidity revert data word writes found");
STATISTIC(NumRevertDataCopyWrites,
          "Number of Solidity revert data copy writes found");

namespace {

// Revert payload markers are semantic annotations. Keep them disabled until the
// type recovery path has a stable post-inference place to regenerate them.
constexpr bool kEmitRevertDataMarkers = false;

uint64_t getRevertKindCode(StringRef Kind) {
  if (Kind == "empty") {
    return 1;
  }
  if (Kind == "panic") {
    return 2;
  }
  if (Kind == "returndata_bubble") {
    return 3;
  }
  if (Kind == "error_string") {
    return 4;
  }
  if (Kind == "custom_error_candidate") {
    return 5;
  }
  if (Kind == "encoded_candidate") {
    return 6;
  }
  return 0;
}

// Consumer markers encode buffer role rather than memory layout. Keep this
// disabled until type recovery has a stable role carrier.
// void insertRevertMemoryConsumerMarker(LLVMContext &Ctx, CallBase &Revert,
//                                       Value *Base, Value *Size,
//                                       StringRef Kind) {
//   Module *M = Revert.getModule();
//   Type *I256 = Type::getIntNTy(Ctx, 256);
//   FunctionCallee Marker = M->getOrInsertFunction(
//       "notdec_solidity_revert_memory_consumer",
//       FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256}, false));
//
//   IRBuilder<> Builder(&Revert);
//   Builder.CreateCall(Marker,
//                      {Base, Size,
//                       ConstantInt::get(I256, getRevertKindCode(Kind))});
// }

void collectRevertDataWordWriteMarkers(BasicBlock &BB, CallBase &Revert,
                                       Value *RevertBase,
                                       SmallVectorImpl<CallBase *> &Writes) {
  SmallVector<CallBase *, 8> Candidates;

  for (Instruction &I : BB) {
    if (&I == &Revert) {
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
        Call->arg_size() != 3) {
      continue;
    }

    if (isSameOrReloadedFreeMemoryBase(Call->getArgOperand(0), RevertBase)) {
      Candidates.push_back(Call);
    }
  }

  Writes.append(Candidates.begin(), Candidates.end());
}

void collectRevertDataCopyWriteMarkers(BasicBlock &BB, CallBase &Revert,
                                       Value *RevertBase,
                                       SmallVectorImpl<CallBase *> &Writes) {
  SmallVector<CallBase *, 4> Candidates;

  for (Instruction &I : BB) {
    if (&I == &Revert) {
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
        Call->arg_size() != 5) {
      continue;
    }

    if (isSameOrReloadedFreeMemoryBase(Call->getArgOperand(0), RevertBase)) {
      Candidates.push_back(Call);
    }
  }

  Writes.append(Candidates.begin(), Candidates.end());
}

void insertRevertDataWordWriteMarker(LLVMContext &Ctx, CallBase &Revert,
                                     CallBase &WordWrite, StringRef Kind) {
  Module *M = Revert.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_revert_data_word_write",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256, I256},
                        false));

  IRBuilder<> Builder(&Revert);
  Builder.CreateCall(Marker,
                     {WordWrite.getArgOperand(0), WordWrite.getArgOperand(1),
                      WordWrite.getArgOperand(2),
                      ConstantInt::get(I256, getRevertKindCode(Kind))});
}

void insertRevertDataCopyWriteMarker(LLVMContext &Ctx, CallBase &Revert,
                                     CallBase &CopyWrite, StringRef Kind) {
  Module *M = Revert.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_revert_data_copy_write",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&Revert);
  Builder.CreateCall(Marker,
                     {CopyWrite.getArgOperand(0), CopyWrite.getArgOperand(1),
                      CopyWrite.getArgOperand(2), CopyWrite.getArgOperand(3),
                      CopyWrite.getArgOperand(4),
                      ConstantInt::get(I256, getRevertKindCode(Kind))});
}

} // namespace

PreservedAnalyses SolidityRevertPass::run(Function &F,
                                          FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      auto *Call = dyn_cast<CallBase>(&I);
      if (Call == nullptr || !isCallTo(Call, "evm_revert") ||
          Call->arg_size() != 3) {
        continue;
      }

      std::optional<SolidityRevertMatch> Match =
          matchSolidityRevert(BB, *Call);
      if (!Match.has_value()) {
        continue;
      }

      addRevertMatchMetadata(Ctx, *Match);
      if (!isConstantIntValue(Call->getArgOperand(2), 0)) {
        Value *RevertBase = Call->getArgOperand(1);
        // Value *RevertSize = Call->getArgOperand(2);
        // insertRevertMemoryConsumerMarker(Ctx, *Call, RevertBase, RevertSize,
        //                                  Match->Kind);
        // ++NumRevertMemoryConsumers;
        SmallVector<CallBase *, 8> WordWrites;
        collectRevertDataWordWriteMarkers(BB, *Call, RevertBase, WordWrites);
        if (kEmitRevertDataMarkers) {
          for (CallBase *WordWrite : WordWrites) {
            insertRevertDataWordWriteMarker(Ctx, *Call, *WordWrite,
                                            Match->Kind);
            ++NumRevertDataWordWrites;
          }
        }
        SmallVector<CallBase *, 4> CopyWrites;
        collectRevertDataCopyWriteMarkers(BB, *Call, RevertBase, CopyWrites);
        if (kEmitRevertDataMarkers) {
          for (CallBase *CopyWrite : CopyWrites) {
            insertRevertDataCopyWriteMarker(Ctx, *Call, *CopyWrite,
                                            Match->Kind);
            ++NumRevertDataCopyWrites;
          }
        }
      }
      insertRevertMemoryWriteMatchMarker(Ctx, *Match);
      if (Match->Kind == "panic") {
        insertPanicRewriteMarker(Ctx, *Match);
      } else if (Match->Kind == "returndata_bubble") {
        insertReturndataBubbleRewriteMarker(Ctx, *Match);
      } else if (Match->Kind == "error_string") {
        insertSelectorRewriteMarker(Ctx, *Match,
                                    "notdec_solidity_rewrite_revert_error_string",
                                    Match->ErrorStringLength);
      } else if (Match->Kind == "custom_error_candidate") {
        insertSelectorRewriteMarker(Ctx, *Match,
                                    "notdec_solidity_rewrite_revert_custom_error",
                                    Match->CustomErrorArgCount);
      }
      ++NumReverts;
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
