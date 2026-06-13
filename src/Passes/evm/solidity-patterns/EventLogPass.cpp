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

STATISTIC(NumEvents, "Number of Solidity event candidates found");
STATISTIC(NumEventNonPointerDataHTypes,
          "Number of event data bases without pointer HType");
STATISTIC(NumEventNonRecordDataHTypes,
          "Number of event data bases without record pointee HType");
STATISTIC(NumEventMissingDataHTypes,
          "Number of event data payloads not recoverable from HType");

namespace {

bool hasEventDataHType(llvm2c::HTypeResult &HTypes,
                       ArrayRef<mlsub::EVMStoreEvidence> Stores, Value *Base,
                       CallBase &Use, unsigned ArgIndex) {
  HTypeBufferView View = getHTypeBufferView(HTypes, Base, Use, ArgIndex);
  if (View.Record != nullptr) {
    return true;
  }
  if (View.HasTransparentOffset0Field) {
    return true;
  }
  if (View.Gap == HTypeBufferGap::NonRecordPointerType &&
      isConstantIntValue(Use.getArgOperand(2), 32)) {
    return !getHTypeStoreValuesAtOffsetBefore(Stores, Base, 0, Use).empty();
  }

  if (View.Gap == HTypeBufferGap::NoPointerType) {
    LLVM_DEBUG(dbgs() << "evm event: data base has no pointer HType: " << *Base
                      << "\n");
    ++NumEventNonPointerDataHTypes;
    return false;
  }

  LLVM_DEBUG(dbgs() << "evm event: data base HType is not record pointer: "
                    << View.BaseType->getAsString() << " for " << *Base
                    << "\n");
  ++NumEventNonRecordDataHTypes;
  return false;
}

bool isEVMLogCall(CallBase &Call) {
  StringRef Name = getCalleeName(&Call);
  return Name.starts_with("evm_log") && Name.size() == 8 &&
         Name.back() >= '0' && Name.back() <= '4' && Call.arg_size() >= 3;
}

} // namespace

PreservedAnalyses EventLogPass::run(Module &M, ModuleAnalysisManager &MAM) {
  std::unique_ptr<mlsub::MLsubRecovery::Result> &HighTypes =
      TR.getResult(M, MAM);
  if (HighTypes == nullptr) {
    return PreservedAnalyses::all();
  }
  ArrayRef<mlsub::EVMStoreEvidence> StoreEvidence = TR.getEVMStoreEvidence();

  bool Changed = false;

  for (Function &F : M) {
    if (F.isDeclaration() || isCalldataAccessCloneFunction(F)) {
      continue;
    }

    LLVMContext &Ctx = F.getContext();
    for (Instruction &I : instructions(F)) {
      auto *Call = dyn_cast<CallBase>(&I);
      if (Call == nullptr || !isEVMLogCall(*Call)) {
        continue;
      }

      if (!isConstantIntValue(Call->getArgOperand(2), 0) &&
          !hasEventDataHType(*HighTypes, StoreEvidence, Call->getArgOperand(1),
                             *Call, 1)) {
        ++NumEventMissingDataHTypes;
      }

      StringRef Name = getCalleeName(Call);
      addStringMetadata(Ctx, I, KIND_SOLIDITY_EVENT,
                        ("topic_count_" + Twine(Name.back())).str());
      ++NumEvents;
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
