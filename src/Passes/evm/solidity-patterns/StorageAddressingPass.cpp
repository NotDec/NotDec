#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/Statistic.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumStorageAddressing,
          "Number of Solidity storage addressing candidates found");

PreservedAnalyses StorageAddressingPass::run(Function &F,
                                             FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_sha3") ||
        Call->arg_size() != 3) {
      continue;
    }

    StringRef Kind = "candidate";
    if (isConstantIntValue(Call->getArgOperand(2), 64)) {
      Kind = "mapping_slot_candidate";
    } else if (isConstantIntValue(Call->getArgOperand(2), 32)) {
      Kind = "dynamic_array_data_slot_candidate";
    }
    addStringMetadata(Ctx, I, KIND_SOLIDITY_STORAGE_ADDRESSING, Kind);
    ++NumStorageAddressing;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
