PreservedAnalyses PackedStorageFieldPass::run(Function &F,
                                              FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call != nullptr && isCallTo(Call, "evm_sstore") &&
        Call->arg_size() == 2) {
      Value *Stored = Call->getArgOperand(1);
      if (dependsOnCallTo(Stored, "evm_sload") &&
          expressionHasPackedStorageOp(Stored)) {
        addStringMetadata(Ctx, I, KIND_SOLIDITY_PACKED_STORAGE_FIELD,
                          "packed_store_candidate");
        ++NumPackedStorageFields;
        Changed = true;
      }
      continue;
    }

    if (Call != nullptr && isCallTo(Call, "evm_sload")) {
      for (User *U : Call->users()) {
        if (expressionHasPackedStorageOp(U)) {
          addStringMetadata(Ctx, I, KIND_SOLIDITY_PACKED_STORAGE_FIELD,
                            "packed_load_candidate");
          ++NumPackedStorageFields;
          Changed = true;
          break;
        }
      }
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
