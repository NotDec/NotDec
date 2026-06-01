PreservedAnalyses PayabilityGuardPass::run(Function &F,
                                           FunctionAnalysisManager &) {
  if (!isPublicEntryFunction(F)) {
    return PreservedAnalyses::all();
  }

  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (BasicBlock &BB : F) {
    std::optional<PayabilityGuardMatch> Match = matchPayabilityGuard(BB);
    if (!Match) {
      continue;
    }

    F.setMetadata(KIND_SOLIDITY_NONPAYABLE,
                  MDNode::get(Ctx, {MDString::get(Ctx, "true")}));
    addStringMetadata(Ctx, *Match->CallValue, KIND_SOLIDITY_PAYABILITY_GUARD,
                      "callvalue");
    addStringMetadata(Ctx, *Match->Condition, KIND_SOLIDITY_PAYABILITY_GUARD,
                      "condition");
    addStringMetadata(Ctx, *Match->Branch, KIND_SOLIDITY_PAYABILITY_GUARD,
                      "branch");
    markBlock(Ctx, *Match->FailureBlock, KIND_SOLIDITY_PAYABILITY_GUARD,
              "revert");

    // Keep the guard block as the predecessor, but consume the compiler
    // nonpayable branch so later passes do not see the outer reject wrapper.
    insertPayabilityCfgRewriteMarker(Ctx, *Match);
    IRBuilder<> Builder(Match->Branch);
    BranchInst *NewBranch = Builder.CreateBr(Match->SuccessBlock);
    NewBranch->copyMetadata(*Match->Branch);
    Match->Branch->eraseFromParent();

    ++NumNonpayableGuards;
    ++NumPayabilityCfgRewrites;
    Changed = true;
    break;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
