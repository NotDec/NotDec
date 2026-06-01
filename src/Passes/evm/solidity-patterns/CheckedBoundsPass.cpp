#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/Statistic.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumCheckedBounds,
          "Number of Solidity checked operation/bounds candidates found");

PreservedAnalyses CheckedBoundsPass::run(Function &F,
                                         FunctionAnalysisManager &FAM) {
  LLVMContext &Ctx = F.getContext();
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  bool Changed = false;
  SmallVector<CheckedBoundsMatch, 8> Matches;

  for (BasicBlock &BB : F) {
    std::optional<CheckedBoundsMatch> Match = matchCheckedBoundsGuard(BB);
    if (!Match.has_value()) {
      continue;
    }
    Matches.push_back(*Match);
  }

  for (CheckedBoundsMatch &Match : Matches) {
    if (!checkedBoundsOperandsDominateBranch(Match, DT)) {
      downgradeCheckedBoundsRewrite(Match, "operand_not_available_at_guard");
    }
    addCheckedBoundsMetadata(Ctx, Match);
    rewriteCheckedBoundsGuard(Match);
    ++NumCheckedBounds;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
