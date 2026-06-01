#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/Statistic.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumEvents, "Number of Solidity event candidates found");

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
    addStringMetadata(Ctx, I, KIND_SOLIDITY_EVENT,
                      ("topic_count_" + Twine(Name.back())).str());
    ++NumEvents;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
