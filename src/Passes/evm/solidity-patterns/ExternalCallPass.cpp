#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/Statistic.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumExternalCalls, "Number of Solidity external calls found");

PreservedAnalyses ExternalCallPass::run(Function &F,
                                        FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    StringRef Kind = classifyExternalCall(getCalleeName(Call));
    if (Kind.empty()) {
      continue;
    }
    addStringMetadata(Ctx, I, KIND_SOLIDITY_EXTERNAL_CALL, Kind);
    ++NumExternalCalls;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
