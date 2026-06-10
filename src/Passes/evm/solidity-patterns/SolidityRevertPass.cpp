#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <limits>
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
  bool HasSelector = false;
  bool HasPanicCode = false;
  bool HasErrorHead = false;
  bool HasErrorLength = false;
  bool HasErrorData = false;
  bool SelectorFromStoreEvidenceOnly = false;
  SmallVector<Value *, 2> SelectorStores;
  SmallVector<Value *, 2> PanicCodeStores;
  SmallVector<Value *, 2> ErrorLengthStores;
};

bool isZeroValue(Value *V) { return isConstantIntValue(V, 0); }

bool isReturndataForwardCopy(CallBase &Call, Value *Dest) {
  return isCallTo(&Call, "evm_returndatacopy") && Call.arg_size() == 5 &&
         Call.getArgOperand(2) == Dest && isZeroValue(Call.getArgOperand(3)) &&
         isReturndataSize(Call.getArgOperand(4));
}

bool blockHasReturndataForwardCopyBefore(BasicBlock &BB, Value *Dest,
                                         Instruction *Before) {
  for (Instruction &I : BB) {
    if (&I == Before) {
      return false;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call != nullptr && isReturndataForwardCopy(*Call, Dest)) {
      return true;
    }
  }
  return false;
}

bool hasSolidityReturndataForwardShape(CallBase &Revert) {
  if (!isReturndataSize(Revert.getArgOperand(2))) {
    return false;
  }

  // Solidity can emit the copy in the block before the success/failure branch,
  // with the revert itself in the failure successor.
  Value *Dest = Revert.getArgOperand(1);
  BasicBlock *BB = Revert.getParent();
  if (blockHasReturndataForwardCopyBefore(*BB, Dest, &Revert)) {
    return true;
  }

  BasicBlock *Pred = BB->getUniquePredecessor();
  return Pred != nullptr &&
         blockHasReturndataForwardCopyBefore(*Pred, Dest, nullptr);
}

std::optional<Value *>
getLastFreeMemoryPointerStoreBefore(Instruction &Before) {
  for (auto It = Before.getIterator(); It != Before.getParent()->begin();) {
    --It;
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&*It);
    if (Store.has_value() && Store->StoreBits == 256 &&
        isConstantIntValue(Store->Address, 64)) {
      return Store->StoredValue;
    }
  }
  return std::nullopt;
}

std::optional<uint64_t> getSiblingAddOffsetFromBase(Value *Ptr, Value *Base) {
  if (std::optional<uint64_t> Offset = getOffsetFromBase(Ptr, Base)) {
    return Offset;
  }

  auto *PtrAdd = dyn_cast_or_null<BinaryOperator>(Ptr);
  auto *BaseAdd = dyn_cast_or_null<BinaryOperator>(Base);
  if (PtrAdd == nullptr || BaseAdd == nullptr ||
      PtrAdd->getOpcode() != Instruction::Add ||
      BaseAdd->getOpcode() != Instruction::Add) {
    return std::nullopt;
  }

  for (unsigned PtrBaseIndex = 0; PtrBaseIndex < 2; ++PtrBaseIndex) {
    Value *SharedBase = PtrAdd->getOperand(PtrBaseIndex);
    std::optional<uint64_t> PtrConst =
        getUInt64Constant(PtrAdd->getOperand(1 - PtrBaseIndex));
    if (!PtrConst.has_value()) {
      continue;
    }

    for (unsigned BaseBaseIndex = 0; BaseBaseIndex < 2; ++BaseBaseIndex) {
      if (BaseAdd->getOperand(BaseBaseIndex) != SharedBase) {
        continue;
      }
      std::optional<uint64_t> BaseConst =
          getUInt64Constant(BaseAdd->getOperand(1 - BaseBaseIndex));
      if (BaseConst.has_value() && *PtrConst >= *BaseConst) {
        return *PtrConst - *BaseConst;
      }
    }
  }

  return std::nullopt;
}

bool hasHelperEncodedRevertPayload(llvm2c::HTypeResult &HTypes,
                                   ArrayRef<mlsub::EVMStoreEvidence> Stores,
                                   CallBase &Revert) {
  // This covers helpers that encode the payload after an already-written
  // selector.  The pass can prove an encoded revert, but not the full string.
  if (!isFreeMemoryPointerLoad(Revert.getArgOperand(1))) {
    return false;
  }

  auto *Size = dyn_cast<BinaryOperator>(Revert.getArgOperand(2));
  if (Size == nullptr || Size->getOpcode() != Instruction::Sub ||
      !isSameOrReloadedFreeMemoryBase(Size->getOperand(1),
                                      Revert.getArgOperand(1))) {
    return false;
  }

  auto *HelperCall = dyn_cast<CallBase>(Size->getOperand(0));
  Function *Helper =
      HelperCall == nullptr ? nullptr : HelperCall->getCalledFunction();
  if (Helper == nullptr || Helper->isDeclaration()) {
    return false;
  }

  std::optional<Value *> RevertBase =
      getLastFreeMemoryPointerStoreBefore(*HelperCall);
  if (!RevertBase.has_value()) {
    return false;
  }

  bool HasSelector = false;
  for (Value *StoredValue :
       getHTypeStoreValuesAtOffsetBefore(Stores, *RevertBase, 0, Revert)) {
    if (getSelectorWord(StoredValue).has_value()) {
      HasSelector = true;
      break;
    }
  }
  if (!HasSelector) {
    return false;
  }

  for (unsigned I = 0, E = HelperCall->arg_size(); I != E; ++I) {
    std::optional<uint64_t> ArgOffset =
        getSiblingAddOffsetFromBase(HelperCall->getArgOperand(I), *RevertBase);
    if (!ArgOffset.has_value() || *ArgOffset != 4 ||
        I >= Helper->arg_size()) {
      continue;
    }

    Argument *HelperArg = Helper->getArg(I);
    HTypeBufferView View = getHTypeValueBufferView(HTypes, HelperArg);
    if (hasHTypeBufferFieldAt(View, 0) && hasHTypeBufferFieldAt(View, 32) &&
        !getHTypeStoreValuesAtOffset(Stores, HelperArg, 0).empty() &&
        !getHTypeStoreValuesAtOffset(Stores, HelperArg, 32).empty()) {
      return true;
    }
  }

  return false;
}

bool hasRawBytesHeaderEvidence(llvm2c::HTypeResult &HTypes,
                               ArrayRef<mlsub::EVMStoreEvidence> Stores,
                               Value *HeaderBase, Instruction &Before) {
  HTypeBufferView View = getHTypeValueBufferView(HTypes, HeaderBase);
  if (hasHTypeBufferFieldAt(View, 0)) {
    return true;
  }
  if (View.Gap == HTypeBufferGap::NonRecordPointerType) {
    return true;
  }
  if (!getHTypeStoreValuesAtOffsetBefore(Stores, HeaderBase, 0, Before)
           .empty()) {
    return true;
  }

  // After control-flow merge, the length load may use a PHI instead of the
  // concrete allocation that received the length store.  Keep this fallback
  // narrow: only trust incoming values that still carry HType/store evidence.
  auto *Phi = dyn_cast<PHINode>(HeaderBase);
  if (Phi == nullptr) {
    return false;
  }
  for (Value *Incoming : Phi->incoming_values()) {
    if (isa<Constant>(Incoming)) {
      continue;
    }
    HTypeBufferView IncomingView = getHTypeValueBufferView(HTypes, Incoming);
    if (hasHTypeBufferFieldAt(IncomingView, 0) &&
        !getHTypeStoreValuesAtOffset(Stores, Incoming, 0).empty()) {
      return true;
    }
  }
  return false;
}

bool hasRawBytesPayloadRevert(llvm2c::HTypeResult &HTypes,
                              ArrayRef<mlsub::EVMStoreEvidence> Stores,
                              CallBase &Revert) {
  std::optional<EvmMemoryLoad> Length =
      matchEvmMemoryLoad(Revert.getArgOperand(2));
  if (!Length.has_value()) {
    return false;
  }
  std::optional<uint64_t> PayloadOffset =
      getOffsetFromBase(Revert.getArgOperand(1), Length->Address);
  return PayloadOffset.has_value() && *PayloadOffset == 32 &&
         hasRawBytesHeaderEvidence(HTypes, Stores, Length->Address, Revert);
}

void insertPanicRewriteMarker(LLVMContext &Ctx,
                              const SolidityRevertMatch &Match) {
  if (Match.PanicCode == std::nullopt || Match.Revert == nullptr) {
    return;
  }

  Module *M = Match.Revert->getModule();
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_rewrite_revert_panic",
      FunctionType::get(Type::getVoidTy(Ctx), {Type::getIntNTy(Ctx, 256)},
                        false));

  IRBuilder<> Builder(Ctx);
  if (Instruction *Next = Match.Revert->getNextNode()) {
    Builder.SetInsertPoint(Next);
  } else {
    Builder.SetInsertPoint(Match.Revert->getParent());
  }

  Value *Args[] = {
      ConstantInt::get(Type::getIntNTy(Ctx, 256), *Match.PanicCode)};
  Builder.CreateCall(Marker, Args);
}

void insertSelectorRewriteMarker(LLVMContext &Ctx,
                                 const SolidityRevertMatch &Match,
                                 StringRef MarkerName,
                                 std::optional<uint64_t> PayloadValue) {
  if (!Match.Selector.has_value() || Match.Revert == nullptr) {
    return;
  }

  Module *M = Match.Revert->getModule();
  FunctionCallee Marker = M->getOrInsertFunction(
      MarkerName,
      FunctionType::get(Type::getVoidTy(Ctx),
                        {Type::getIntNTy(Ctx, 256), Type::getIntNTy(Ctx, 256)},
                        false));

  IRBuilder<> Builder(Ctx);
  if (Instruction *Next = Match.Revert->getNextNode()) {
    Builder.SetInsertPoint(Next);
  } else {
    Builder.SetInsertPoint(Match.Revert->getParent());
  }

  constexpr uint64_t UNKNOWN_PAYLOAD = std::numeric_limits<uint64_t>::max();
  Value *Args[] = {ConstantInt::get(Type::getIntNTy(Ctx, 256), *Match.Selector),
                   ConstantInt::get(Type::getIntNTy(Ctx, 256),
                                    PayloadValue.value_or(UNKNOWN_PAYLOAD))};
  Builder.CreateCall(Marker, Args);
}

void addRevertMatchMetadata(LLVMContext &Ctx,
                            const SolidityRevertMatch &Match) {
  if (Match.Revert == nullptr) {
    return;
  }

  addStringMetadata(Ctx, *Match.Revert, KIND_SOLIDITY_REVERT, Match.Kind);
  if (Match.PanicCode.has_value()) {
    addPlainMetadata(Ctx, *Match.Revert, "notdec.solidity_revert.panic_code",
                     Twine(*Match.PanicCode).str());
  }
  if (Match.Selector.has_value()) {
    addPlainMetadata(Ctx, *Match.Revert, "notdec.solidity_revert.selector",
                     Twine::utohexstr(*Match.Selector).str());
  }
  if (Match.CustomErrorArgCount.has_value()) {
    addPlainMetadata(Ctx, *Match.Revert,
                     "notdec.solidity_revert.custom_error_arg_count",
                     Twine(*Match.CustomErrorArgCount).str());
  }
  if (Match.ErrorStringLength.has_value()) {
    addPlainMetadata(Ctx, *Match.Revert,
                     "notdec.solidity_revert.error_string_length",
                     Twine(*Match.ErrorStringLength).str());
  }
}

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

std::optional<HTypeBufferView>
getRevertBufferHType(llvm2c::HTypeResult &HTypes, Value *Base, CallBase &Use,
                     unsigned ArgIndex) {
  HTypeBufferView View = getHTypeBufferView(HTypes, Base, Use, ArgIndex);
  if (View.Record != nullptr) {
    return View;
  }
  if (View.HasTransparentOffset0Field) {
    return View;
  }
  if (View.Gap == HTypeBufferGap::NonRecordPointerType) {
    return View;
  }

  if (View.Gap == HTypeBufferGap::NoPointerType) {
    LLVM_DEBUG(dbgs() << "evm revert: base has no pointer HType: " << *Base
                      << "\n");
    ++NumRevertNonPointerBaseHTypes;
    return std::nullopt;
  }

  LLVM_DEBUG(dbgs() << "evm revert: base HType is not record pointer: "
                    << View.BaseType->getAsString() << " for " << *Base
                    << "\n");
  ++NumRevertNonRecordBaseHTypes;
  return std::nullopt;
}

std::optional<RevertPayloadHType>
getRevertPayloadHType(llvm2c::HTypeResult &HTypes,
                      ArrayRef<mlsub::EVMStoreEvidence> Stores,
                      CallBase &Revert) {
  if (std::optional<RevertPayloadHType> PanicPayload =
          getCanonicalPanicPayloadFromEvidence(Stores, Revert)) {
    return PanicPayload;
  }

  std::optional<HTypeBufferView> View =
      getRevertBufferHType(HTypes, Revert.getArgOperand(1), Revert, 1);
  if (!View.has_value()) {
    return std::nullopt;
  }

  RevertPayloadHType Payload;
  Payload.HasSelector = hasHTypeBufferFieldAt(*View, 0);
  Payload.HasPanicCode = hasHTypeBufferFieldAt(*View, 4);
  Payload.HasErrorHead = hasHTypeBufferFieldAt(*View, 4);
  Payload.HasErrorLength = hasHTypeBufferFieldAt(*View, 36);
  Payload.HasErrorData = hasHTypeBufferFieldAt(*View, 68);
  Payload.SelectorStores = getHTypeBufferFieldStoreValues(
      Stores, *View, Revert.getArgOperand(1), 0);
  Payload.PanicCodeStores = getHTypeBufferFieldStoreValues(
      Stores, *View, Revert.getArgOperand(1), 4);
  Payload.ErrorLengthStores = getHTypeBufferFieldStoreValues(
      Stores, *View, Revert.getArgOperand(1), 36);
  if (!Payload.HasSelector && !Payload.HasPanicCode &&
      !Payload.HasErrorHead && !Payload.HasErrorLength &&
      !Payload.HasErrorData) {
    Payload.SelectorStores = getHTypeStoreValuesAtOffsetBefore(
        Stores, Revert.getArgOperand(1), 0, Revert);
    if (Payload.SelectorStores.empty()) {
      return std::nullopt;
    }
    Payload.HasSelector = true;
    Payload.SelectorFromStoreEvidenceOnly = true;
    Payload.PanicCodeStores = getHTypeStoreValuesAtOffsetBefore(
        Stores, Revert.getArgOperand(1), 4, Revert);
    Payload.HasPanicCode = !Payload.PanicCodeStores.empty();
  }
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

  if (hasSolidityReturndataForwardShape(Revert)) {
    SolidityRevertMatch Match;
    Match.Revert = &Revert;
    Match.Kind = "returndata_forward";
    return Match;
  }

  if (isConstantIntValue(Revert.getArgOperand(2), 0)) {
    SolidityRevertMatch Match;
    Match.Revert = &Revert;
    Match.Kind = "encoded_candidate";
    return Match;
  }

  if (hasHelperEncodedRevertPayload(HTypes, Stores, Revert)) {
    SolidityRevertMatch Match;
    Match.Revert = &Revert;
    Match.Kind = "encoded_candidate";
    return Match;
  }

  if (hasRawBytesPayloadRevert(HTypes, Stores, Revert)) {
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
  if (Selector.has_value() && *Selector == 0x08c379a0 &&
      Payload->SelectorFromStoreEvidenceOnly) {
    return std::nullopt;
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
