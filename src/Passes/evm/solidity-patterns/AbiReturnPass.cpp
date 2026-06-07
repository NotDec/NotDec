#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <notdec-llvm2c/Interface.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumAbiReturns, "Number of Solidity ABI return sites found");
STATISTIC(NumAbiReturnNonPointerBaseHTypes,
          "Number of ABI return bases without pointer HType");
STATISTIC(NumAbiReturnNonRecordBaseHTypes,
          "Number of ABI return bases without record pointee HType");
STATISTIC(NumAbiReturnMissingPayloadHTypes,
          "Number of ABI return payloads not recoverable from HType");

namespace {

StringRef classifyAbiReturnSize(Value *Size) {
  if (isConstantIntValue(Size, 32)) {
    return "static_1_word";
  }
  if (isReturndataSize(Size)) {
    return "returndata_forward";
  }
  return "candidate";
}

ast::RecordDecl *getRecordPointeeHType(llvm2c::HTypeResult &HTypes,
                                       Value *Base, CallBase &Use,
                                       unsigned ArgIndex) {
  HTypeBufferView View = getHTypeBufferView(HTypes, Base, Use, ArgIndex);
  if (View.Record != nullptr) {
    return View.Record;
  }

  if (View.Gap == HTypeBufferGap::NoPointerType) {
    LLVM_DEBUG(dbgs() << "evm abi return: base has no pointer HType: "
                      << *Base << "\n");
    ++NumAbiReturnNonPointerBaseHTypes;
    return nullptr;
  }

  LLVM_DEBUG(dbgs() << "evm abi return: base HType is not record pointer: "
                    << View.BaseType->getAsString() << " for " << *Base
                    << "\n");
  ++NumAbiReturnNonRecordBaseHTypes;
  return nullptr;
}

} // namespace

PreservedAnalyses AbiReturnPass::run(Module &M, ModuleAnalysisManager &MAM) {
  std::unique_ptr<mlsub::MLsubRecovery::Result> &HighTypes =
      TR.getResult(M, MAM);
  if (HighTypes == nullptr) {
    return PreservedAnalyses::all();
  }

  bool Changed = false;

  for (Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }

    LLVMContext &Ctx = F.getContext();
    for (Instruction &I : instructions(F)) {
      auto *Call = dyn_cast<CallBase>(&I);
      if (Call == nullptr || !isCallTo(Call, "evm_return") ||
          Call->arg_size() != 3) {
        continue;
      }

      StringRef Kind = classifyAbiReturnSize(Call->getArgOperand(2));
      Value *ReturnBase = Call->getArgOperand(1);
      Value *ReturnSize = Call->getArgOperand(2);
      if (!isConstantIntValue(ReturnSize, 0)) {
        if (getRecordPointeeHType(*HighTypes, ReturnBase, *Call, 1) ==
            nullptr) {
          ++NumAbiReturnMissingPayloadHTypes;
          continue;
        }
      }
      addStringMetadata(Ctx, I, KIND_SOLIDITY_ABI_RETURN, Kind);
      addStringMetadata(Ctx, F, KIND_SOLIDITY_ABI_RETURN, "true");
      ++NumAbiReturns;
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
