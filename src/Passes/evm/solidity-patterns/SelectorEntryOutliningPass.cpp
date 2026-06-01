#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/Statistic.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumSelectorOutlinedBodies,
          "Number of Solidity selector inline bodies outlined");
STATISTIC(NumSelectorOutlineSkipped,
          "Number of Solidity selector inline body outline candidates skipped");

PreservedAnalyses
SelectorEntryOutliningPass::run(Function &F, FunctionAnalysisManager &FAM) {
  if (!isSelectorFunction(F)) {
    return PreservedAnalyses::all();
  }

  LLVMContext &Ctx = F.getContext();
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  bool Changed = false;
  while (true) {
    SmallPtrSet<BasicBlock *, 32> Dispatcher;
    SmallVector<BasicBlock *, 8> Candidates;
    SmallPtrSet<BasicBlock *, 8> CandidateSet;

    for (BasicBlock &BB : F) {
      if (isDispatcherBlock(BB)) {
        Dispatcher.insert(&BB);
      }
    }
    if (Dispatcher.empty()) {
      break;
    }

    // Keep candidate order stable by walking function blocks.  Some selector
    // bodies become outlineable only after a later shared tail has been split
    // out, so each successful rewrite restarts this search on the new CFG.
    for (BasicBlock &BB : F) {
      if (!Dispatcher.contains(&BB)) {
        continue;
      }
      for (BasicBlock *Succ : successors(&BB)) {
        if (!Dispatcher.contains(Succ) && !isPublicCallStub(*Succ) &&
            !isEmptyRejectBlock(*Succ) && CandidateSet.insert(Succ).second) {
          Candidates.push_back(Succ);
        }
      }
    }

    bool OutlinedThisRound = false;
    SmallVector<std::pair<BasicBlock *, StringRef>, 4> Skipped;
    for (BasicBlock *Entry : Candidates) {
      if (Entry->getParent() != &F) {
        continue;
      }

      SmallVector<BasicBlock *, 16> Blocks;
      SmallPtrSet<BasicBlock *, 16> Region;
      collectReachableBody(Entry, Dispatcher, Blocks, Region);

      if (!regionHasBodySignal(Blocks)) {
        continue;
      }

      StringRef SkipReason = getOutlineSkipReason(Blocks, Region);
      if (!SkipReason.empty()) {
        if (!Blocks.empty() && SkipReason != "already_outlined_successor") {
          Skipped.push_back({Blocks.front(), SkipReason});
        }
        continue;
      }

      SmallVector<Instruction *, 8> Inputs;
      collectRegionInputs(Blocks, Region, Inputs);
      if (!regionInputsAvailableAtEntry(Inputs, Entry, Region, DT)) {
        if (!Blocks.empty()) {
          Skipped.push_back({Blocks.front(), "input_not_available"});
        }
        continue;
      }

      Function *Outlined =
          cloneSelectorRegion(F, Blocks, Region, Entry, Inputs);
      Outlined->setMetadata(
          KIND_SOLIDITY_SELECTOR_OUTLINED_BODY,
          MDNode::get(Ctx, {MDString::get(Ctx, F.getName())}));
      replaceRegionWithCall(F, *Outlined, Blocks, Region, Entry, Inputs);
      ++NumSelectorOutlinedBodies;
      Changed = true;
      OutlinedThisRound = true;
      break;
    }

    if (!OutlinedThisRound) {
      for (auto [BB, SkipReason] : Skipped) {
        if (BB->getParent() == &F) {
          addPlainMetadata(Ctx, *BB->getTerminator(),
                           KIND_SOLIDITY_SELECTOR_OUTLINE_SKIPPED, SkipReason);
          ++NumSelectorOutlineSkipped;
        }
      }
      break;
    }
  }

  if (!Changed) {
    SmallVector<BasicBlock *, 16> Blocks;
    SmallPtrSet<BasicBlock *, 16> Region;
    for (BasicBlock &BB : F) {
      Blocks.push_back(&BB);
      Region.insert(&BB);
    }

    if (hasWholeSelectorOutlineShape(F, Blocks)) {
      SmallVector<Instruction *, 1> Inputs;
      Function *Outlined =
          cloneSelectorRegion(F, Blocks, Region, &F.getEntryBlock(), Inputs);
      Outlined->setMetadata(
          KIND_SOLIDITY_SELECTOR_OUTLINED_BODY,
          MDNode::get(Ctx, {MDString::get(Ctx, F.getName())}));
      replaceWholeFunctionWithCall(F, *Outlined);
      ++NumSelectorOutlinedBodies;
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
