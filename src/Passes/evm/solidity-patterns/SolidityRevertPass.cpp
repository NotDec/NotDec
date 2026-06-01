#include "Passes/evm/SolidityPatternUtils.h"

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

namespace {

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

CallBase *findRevertConsumerMarker(BasicBlock &BB, CallBase &Revert) {
  for (Instruction &I : BB) {
    if (&I == &Revert) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr ||
        !isCallTo(Call, "notdec_solidity_memory_consumer") ||
        Call->arg_size() != 3 ||
        !isConstantIntValue(Call->getArgOperand(2), 2)) {
      continue;
    }
    if (Call->getArgOperand(0) == Revert.getArgOperand(1) &&
        Call->getArgOperand(1) == Revert.getArgOperand(2)) {
      return Call;
    }
  }
  return nullptr;
}

void insertRevertMemoryConsumerMarker(LLVMContext &Ctx, CallBase &Revert,
                                      CallBase &Consumer, StringRef Kind) {
  Module *M = Revert.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_revert_memory_consumer",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256}, false));

  IRBuilder<> Builder(&Revert);
  Builder.CreateCall(Marker,
                     {Consumer.getArgOperand(0), Consumer.getArgOperand(1),
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
      if (CallBase *Consumer = findRevertConsumerMarker(BB, *Call)) {
        insertRevertMemoryConsumerMarker(Ctx, *Call, *Consumer, Match->Kind);
        ++NumRevertMemoryConsumers;
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
