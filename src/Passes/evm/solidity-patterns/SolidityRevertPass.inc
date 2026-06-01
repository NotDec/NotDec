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
