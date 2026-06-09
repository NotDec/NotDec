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
  SmallVector<Value *, 2> SelectorStores;
  SmallVector<Value *, 2> PanicCodeStores;
  SmallVector<Value *, 2> ErrorLengthStores;
};

std::optional<RevertPayloadHType>
getCanonicalPanicPayloadFromEvidence(ArrayRef<mlsub::EVMStoreEvidence> Stores,
                                     CallBase &Revert) {
  if (!isConstantIntValue(Revert.getArgOperand(1), 0) ||
      !isConstantIntValue(Revert.getArgOperand(2), 36)) {
    return std::nullopt;
  }

  RevertPayloadHType Payload;
  Payload.HasSelector = true;
  Payload.HasPanicCode = true;
  Payload.SelectorStores = getHTypeStoreValuesAtOffsetBefore(
      Stores, Revert.getArgOperand(1), 0, Revert);
  Payload.PanicCodeStores = getHTypeStoreValuesAtOffsetBefore(
      Stores, Revert.getArgOperand(1), 4, Revert);
  if (Payload.SelectorStores.empty() || Payload.PanicCodeStores.empty()) {
    return std::nullopt;
  }
  return Payload;
}

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

std::optional<RevertPayloadHType>
getRevertPayloadHType(llvm2c::HTypeResult &HTypes,
                      ArrayRef<mlsub::EVMStoreEvidence> Stores,
                      CallBase &Revert) {
  if (std::optional<RevertPayloadHType> PanicPayload =
          getCanonicalPanicPayloadFromEvidence(Stores, Revert)) {
    return PanicPayload;
  }

  ast::RecordDecl *Record =
      getRecordPointeeHType(HTypes, Revert.getArgOperand(1), Revert, 1);
  if (Record == nullptr) {
    return std::nullopt;
  }

  RevertPayloadHType Payload;
  Payload.Record = Record;
  Payload.HasSelector = hasHTypeFieldAt(*Record, 0);
  Payload.HasPanicCode = hasHTypeFieldAt(*Record, 4);
  Payload.HasErrorHead = hasHTypeFieldAt(*Record, 4);
  Payload.HasErrorLength = hasHTypeFieldAt(*Record, 36);
  Payload.HasErrorData = hasHTypeFieldAt(*Record, 68);
  Payload.SelectorStores =
      getHTypeFieldStoreValues(Stores, *Record, Revert.getArgOperand(1), 0);
  Payload.PanicCodeStores =
      getHTypeFieldStoreValues(Stores, *Record, Revert.getArgOperand(1), 4);
  Payload.ErrorLengthStores =
      getHTypeFieldStoreValues(Stores, *Record, Revert.getArgOperand(1), 36);
  return Payload;
}

std::optional<SolidityRevertMatch>
classifyRevertFromHType(llvm2c::HTypeResult &HTypes,
                        ArrayRef<mlsub::EVMStoreEvidence> Stores,
                        CallBase &Revert) {
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
      getRevertPayloadHType(HTypes, Stores, Revert);
  if (!Payload.has_value()) {
    ++NumRevertMissingPayloadHTypes;
    return std::nullopt;
  }

  SolidityRevertMatch Match;
  Match.Revert = &Revert;

  std::optional<uint64_t> Selector;
  for (Value *StoredValue : Payload->SelectorStores) {
    std::optional<uint64_t> Candidate = getSelectorWord(StoredValue);
    if (!Candidate.has_value()) {
      continue;
    }
    if (Selector.has_value() && *Selector != *Candidate) {
      LLVM_DEBUG(dbgs() << "evm revert: conflicting selector store evidence: "
                        << Twine::utohexstr(*Selector) << " vs "
                        << Twine::utohexstr(*Candidate) << "\n");
      return std::nullopt;
    }
    Selector = Candidate;
  }
  Match.Selector = Selector;

  if (Selector.has_value() && *Selector == 0x4e487b71 &&
      isConstantIntValue(Revert.getArgOperand(2), 36) &&
      Payload->HasPanicCode) {
    Match.Kind = "panic";
    bool Conflict = false;
    Match.PanicCode =
        getUniqueUInt64FieldValue(Payload->PanicCodeStores, Conflict);
    if (Conflict) {
      return std::nullopt;
    }
    return Match;
  }

  if (Selector.has_value() && *Selector == 0x08c379a0 &&
      Payload->HasErrorHead &&
      Payload->HasErrorLength) {
    Match.Kind = "error_string";
    bool Conflict = false;
    Match.ErrorStringLength =
        getUniqueUInt64FieldValue(Payload->ErrorLengthStores, Conflict);
    if (Conflict) {
      return std::nullopt;
    }
    return Match;
  }

  if (Selector.has_value()) {
    Match.Kind = "custom_error_candidate";
    if (std::optional<uint64_t> RevertLength =
            getUInt64Constant(Revert.getArgOperand(2));
        RevertLength.has_value() && *RevertLength >= 4 &&
        (*RevertLength - 4) % 32 == 0) {
      Match.CustomErrorArgCount = (*RevertLength - 4) / 32;
    }
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
  ArrayRef<mlsub::EVMStoreEvidence> StoreEvidence = TR.getEVMStoreEvidence();

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
            classifyRevertFromHType(*HighTypes, StoreEvidence, *Call);
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
