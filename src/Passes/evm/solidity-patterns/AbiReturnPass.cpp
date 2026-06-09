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

// Local view of one return buffer.  It only records fields already present in
// the HType record; missing record fields stay as type-recovery gaps.
struct AbiReturnPayloadHType {
  ast::RecordDecl *Record = nullptr;
  unsigned FieldCount = 0;
  bool HasOffset0 = false;
  bool HasTransparentOffset0 = false;
  bool HasOffset0StoreEvidence = false;
  bool HasDynamicHead = false;
  bool HasDynamicLength = false;
};

std::optional<HTypeBufferView>
getAbiReturnBufferHType(llvm2c::HTypeResult &HTypes, Value *Base, CallBase &Use,
                        unsigned ArgIndex) {
  HTypeBufferView View = getHTypeBufferView(HTypes, Base, Use, ArgIndex);
  if (View.Record != nullptr) {
    return View;
  }
  if (View.HasTransparentOffset0Field) {
    return View;
  }
  if (isa<Constant>(Base)) {
    return View;
  }
  if (View.Gap == HTypeBufferGap::NoPointerType) {
    LLVM_DEBUG(dbgs() << "evm abi return: base has no pointer HType: " << *Base
                      << "\n");
    ++NumAbiReturnNonPointerBaseHTypes;
    return std::nullopt;
  }

  return View;
}

std::optional<AbiReturnPayloadHType>
getAbiReturnPayloadHType(llvm2c::HTypeResult &HTypes,
                         ArrayRef<mlsub::EVMStoreEvidence> Stores,
                         CallBase &Return) {
  std::optional<HTypeBufferView> View =
      getAbiReturnBufferHType(HTypes, Return.getArgOperand(1), Return, 1);
  if (!View.has_value()) {
    return std::nullopt;
  }

  AbiReturnPayloadHType Payload;
  Payload.Record = View->Record;
  Payload.HasTransparentOffset0 = View->HasTransparentOffset0Field;
  if (Payload.Record != nullptr) {
    Payload.FieldCount = 0;
    for (const ast::FieldDecl &Field : Payload.Record->getFields()) {
      if (!Field.isPadding) {
        ++Payload.FieldCount;
      }
    }
  } else if (!Payload.HasTransparentOffset0) {
    SmallVector<Value *, 2> Offset0Stores = getHTypeStoreValuesAtOffsetBefore(
        Stores, Return.getArgOperand(1), 0, Return);
    Payload.HasOffset0StoreEvidence = !Offset0Stores.empty();
    if (!Payload.HasOffset0StoreEvidence) {
      return std::nullopt;
    }
  }
  Payload.HasOffset0 = hasHTypeBufferFieldAt(*View, 0);
  Payload.HasDynamicHead = hasHTypeBufferFieldAt(*View, 0);
  Payload.HasDynamicLength = hasHTypeBufferFieldAt(*View, 32);
  return Payload;
}

std::optional<StringRef>
classifyAbiReturnFromHType(llvm2c::HTypeResult &HTypes,
                           ArrayRef<mlsub::EVMStoreEvidence> Stores,
                           CallBase &Return) {
  if (isConstantIntValue(Return.getArgOperand(2), 0)) {
    return StringRef("empty");
  }
  if (isReturndataSize(Return.getArgOperand(2))) {
    return StringRef("returndata_forward");
  }

  std::optional<AbiReturnPayloadHType> Payload =
      getAbiReturnPayloadHType(HTypes, Stores, Return);
  if (!Payload.has_value()) {
    ++NumAbiReturnMissingPayloadHTypes;
    return std::nullopt;
  }

  bool IsSingleWordReturn = isConstantIntValue(Return.getArgOperand(2), 32);
  if (IsSingleWordReturn &&
      (Payload->HasOffset0 || Payload->HasTransparentOffset0 ||
       Payload->HasOffset0StoreEvidence)) {
    return StringRef("static_1_word");
  }
  if (Payload->HasOffset0StoreEvidence && !Payload->HasOffset0 &&
      !Payload->HasTransparentOffset0) {
    return std::nullopt;
  }
  if (Payload->HasDynamicHead && Payload->HasDynamicLength) {
    return StringRef("dynamic_candidate");
  }
  if (Payload->FieldCount > 1) {
    return StringRef("tuple_candidate");
  }
  return StringRef("candidate");
}

} // namespace

PreservedAnalyses AbiReturnPass::run(Module &M, ModuleAnalysisManager &MAM) {
  std::unique_ptr<mlsub::MLsubRecovery::Result> &HighTypes =
      TR.getResult(M, MAM);
  if (HighTypes == nullptr) {
    return PreservedAnalyses::all();
  }
  ArrayRef<mlsub::EVMStoreEvidence> StoreEvidence = TR.getEVMStoreEvidence();
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

      std::optional<StringRef> Kind =
          classifyAbiReturnFromHType(*HighTypes, StoreEvidence, *Call);
      if (!Kind.has_value()) {
        continue;
      }
      addStringMetadata(Ctx, I, KIND_SOLIDITY_ABI_RETURN, *Kind);
      addStringMetadata(Ctx, F, KIND_SOLIDITY_ABI_RETURN, "true");
      ++NumAbiReturns;
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
