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
  bool HasDynamicHead = false;
  bool HasDynamicLength = false;
};

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

std::optional<AbiReturnPayloadHType>
getAbiReturnPayloadHType(llvm2c::HTypeResult &HTypes, CallBase &Return) {
  ast::RecordDecl *Record =
      getRecordPointeeHType(HTypes, Return.getArgOperand(1), Return, 1);
  if (Record == nullptr) {
    return std::nullopt;
  }

  AbiReturnPayloadHType Payload;
  Payload.Record = Record;
  Payload.FieldCount = 0;
  for (const ast::FieldDecl &Field : Record->getFields()) {
    if (!Field.isPadding) {
      ++Payload.FieldCount;
    }
  }
  Payload.HasOffset0 = hasHTypeFieldAt(*Record, 0);
  Payload.HasDynamicHead = hasHTypeFieldAt(*Record, 0);
  Payload.HasDynamicLength = hasHTypeFieldAt(*Record, 32);
  return Payload;
}

std::optional<StringRef> classifyAbiReturnFromHType(llvm2c::HTypeResult &HTypes,
                                                    CallBase &Return) {
  if (isConstantIntValue(Return.getArgOperand(2), 0)) {
    return StringRef("empty");
  }
  if (isReturndataSize(Return.getArgOperand(2))) {
    return StringRef("returndata_forward");
  }

  std::optional<AbiReturnPayloadHType> Payload =
      getAbiReturnPayloadHType(HTypes, Return);
  if (!Payload.has_value()) {
    ++NumAbiReturnMissingPayloadHTypes;
    return std::nullopt;
  }

  if (isConstantIntValue(Return.getArgOperand(2), 32) && Payload->HasOffset0) {
    return StringRef("static_1_word");
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
          classifyAbiReturnFromHType(*HighTypes, *Call);
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
