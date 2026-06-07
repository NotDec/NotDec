#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <notdec-llvm2c/Interface.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumReverts, "Number of Solidity revert sites found");
STATISTIC(NumRevertNonPointerBaseHTypes,
          "Number of revert bases without pointer HType");
STATISTIC(NumRevertNonRecordBaseHTypes,
          "Number of revert bases without record pointee HType");
STATISTIC(NumRevertMissingPayloadHTypes,
          "Number of revert payloads not recoverable from HType");

namespace {

struct RevertPayloadHType {
  ast::RecordDecl *Record = nullptr;
  bool HasSelector = false;
  bool HasPanicCode = false;
  bool HasErrorHead = false;
  bool HasErrorLength = false;
  bool HasErrorData = false;
};

ast::RecordDecl *getRecordPointeeHType(llvm2c::HTypeResult &HTypes,
                                       Value *Base, CallBase &Use,
                                       unsigned ArgIndex) {
  HTypeBufferView View = getHTypeBufferView(HTypes, Base, Use, ArgIndex);
  if (View.Record != nullptr) {
    return View.Record;
  }

  if (View.Gap == HTypeBufferGap::NoPointerType) {
    LLVM_DEBUG(dbgs() << "evm revert: base has no pointer HType: " << *Base
                      << "\n");
    ++NumRevertNonPointerBaseHTypes;
    return nullptr;
  }

  LLVM_DEBUG(dbgs() << "evm revert: base HType is not record pointer: "
                    << View.BaseType->getAsString() << " for " << *Base
                    << "\n");
  ++NumRevertNonRecordBaseHTypes;
  return nullptr;
}

bool hasFieldAt(ast::RecordDecl &Record, int64_t Offset) {
  return Record.getFieldAt(Offset) != nullptr;
}

std::optional<RevertPayloadHType>
getRevertPayloadHType(llvm2c::HTypeResult &HTypes, CallBase &Revert) {
  ast::RecordDecl *Record =
      getRecordPointeeHType(HTypes, Revert.getArgOperand(1), Revert, 1);
  if (Record == nullptr) {
    return std::nullopt;
  }

  RevertPayloadHType Payload;
  Payload.Record = Record;
  Payload.HasSelector = hasFieldAt(*Record, 0);
  Payload.HasPanicCode = hasFieldAt(*Record, 4);
  Payload.HasErrorHead = hasFieldAt(*Record, 4);
  Payload.HasErrorLength = hasFieldAt(*Record, 36);
  Payload.HasErrorData = hasFieldAt(*Record, 68);
  return Payload;
}

std::optional<SolidityRevertMatch>
classifyRevertFromHType(llvm2c::HTypeResult &HTypes, CallBase &Revert) {
  if (isConstantIntValue(Revert.getArgOperand(1), 0) &&
      isConstantIntValue(Revert.getArgOperand(2), 0)) {
    SolidityRevertMatch Match;
    Match.Revert = &Revert;
    Match.Kind = "empty";
    return Match;
  }

  if (isConstantIntValue(Revert.getArgOperand(2), 0)) {
    SolidityRevertMatch Match;
    Match.Revert = &Revert;
    Match.Kind = "encoded_candidate";
    return Match;
  }

  std::optional<RevertPayloadHType> Payload =
      getRevertPayloadHType(HTypes, Revert);
  if (!Payload.has_value()) {
    ++NumRevertMissingPayloadHTypes;
    return std::nullopt;
  }

  SolidityRevertMatch Match;
  Match.Revert = &Revert;
  if (isConstantIntValue(Revert.getArgOperand(2), 36) &&
      Payload->HasSelector && Payload->HasPanicCode) {
    Match.Kind = "panic";
    return Match;
  }

  if (Payload->HasSelector && Payload->HasErrorHead &&
      Payload->HasErrorLength) {
    Match.Kind = "error_string";
    return Match;
  }

  if (Payload->HasSelector) {
    Match.Kind = "custom_error_candidate";
    return Match;
  }

  Match.Kind = "encoded_candidate";
  return Match;
}

} // namespace

PreservedAnalyses SolidityRevertPass::run(Module &M,
                                          ModuleAnalysisManager &MAM) {
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
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        auto *Call = dyn_cast<CallBase>(&I);
        if (Call == nullptr || !isCallTo(Call, "evm_revert") ||
            Call->arg_size() != 3) {
          continue;
        }

        std::optional<SolidityRevertMatch> Match =
            classifyRevertFromHType(*HighTypes, *Call);
        if (!Match.has_value()) {
          continue;
        }

        addRevertMatchMetadata(Ctx, *Match);
        if (Match->Kind == "panic") {
          insertPanicRewriteMarker(Ctx, *Match);
        } else if (Match->Kind == "returndata_bubble") {
          insertReturndataBubbleRewriteMarker(Ctx, *Match);
        } else if (Match->Kind == "error_string") {
          insertSelectorRewriteMarker(
              Ctx, *Match, "notdec_solidity_rewrite_revert_error_string",
              Match->ErrorStringLength);
        } else if (Match->Kind == "custom_error_candidate") {
          insertSelectorRewriteMarker(
              Ctx, *Match, "notdec_solidity_rewrite_revert_custom_error",
              Match->CustomErrorArgCount);
        }
        ++NumReverts;
        Changed = true;
      }
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
