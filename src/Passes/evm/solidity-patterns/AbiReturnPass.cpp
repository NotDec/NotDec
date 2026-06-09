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

// Evidence found through a Solidity ABI encoder helper call instead of the
// final evm_return base value itself.
struct AbiReturnHelperPayload {
  bool HasOffset0StoreEvidence = false;
  bool HasDynamicLengthStoreEvidence = false;
  bool HasFixedTupleStoreEvidence = false;
};

bool hasStoreEvidenceAt(ArrayRef<mlsub::EVMStoreEvidence> Stores, Value *Base,
                        int64_t Offset) {
  return !getHTypeStoreValuesAtOffset(Stores, Base, Offset).empty();
}

// Some Solidity ABI helpers write the dynamic head, then delegate the tail
// buffer at base+32 to a second helper.  Accept only the one-hop shape where
// the outer helper returns the delegated helper's end pointer.
bool hasDelegatedStoreEvidenceAt(ArrayRef<mlsub::EVMStoreEvidence> Stores,
                                 Function &Helper, Argument *Base,
                                 int64_t Offset) {
  auto HelperReturnsFrom = [&](CallBase &Call) {
    for (BasicBlock &BB : Helper) {
      auto *Ret = dyn_cast<ReturnInst>(BB.getTerminator());
      if (Ret != nullptr && dependsOnValue(Ret->getReturnValue(), &Call)) {
        return true;
      }
    }
    return false;
  };

  for (Instruction &Inst : instructions(Helper)) {
    auto *Call = dyn_cast<CallBase>(&Inst);
    Function *Callee = Call == nullptr ? nullptr : Call->getCalledFunction();
    if (Callee == nullptr || Callee->isDeclaration() ||
        !HelperReturnsFrom(*Call)) {
      continue;
    }

    for (unsigned I = 0, E = Call->arg_size(); I != E; ++I) {
      std::optional<uint64_t> ArgOffset =
          getOffsetFromBase(Call->getArgOperand(I), Base);
      if (!ArgOffset.has_value() ||
          *ArgOffset != static_cast<uint64_t>(Offset) ||
          I >= Callee->arg_size()) {
        continue;
      }
      if (hasStoreEvidenceAt(Stores, Callee->getArg(I), 0)) {
        return true;
      }
    }
  }

  return false;
}

bool helperReturnsBasePlusSize(Function &Helper, Argument *Base,
                               uint64_t Size) {
  bool SawReturn = false;
  for (BasicBlock &BB : Helper) {
    auto *Ret = dyn_cast<ReturnInst>(BB.getTerminator());
    if (Ret == nullptr) {
      continue;
    }
    SawReturn = true;
    std::optional<uint64_t> Offset =
        getOffsetFromBase(Ret->getReturnValue(), Base);
    if (!Offset.has_value() || *Offset != Size) {
      return false;
    }
  }
  return SawReturn;
}

bool hasNestedHelperStoreEvidenceAt(llvm2c::HTypeResult &HTypes,
                                    ArrayRef<mlsub::EVMStoreEvidence> Stores,
                                    Function &Helper, Argument *Base,
                                    int64_t Offset) {
  for (Instruction &Inst : instructions(Helper)) {
    auto *Call = dyn_cast<CallBase>(&Inst);
    Function *Callee = Call == nullptr ? nullptr : Call->getCalledFunction();
    if (Callee == nullptr || Callee->isDeclaration()) {
      continue;
    }

    for (unsigned I = 0, E = Call->arg_size(); I != E; ++I) {
      std::optional<uint64_t> ArgOffset =
          getOffsetFromBase(Call->getArgOperand(I), Base);
      if (!ArgOffset.has_value() ||
          *ArgOffset > static_cast<uint64_t>(Offset) ||
          I >= Callee->arg_size()) {
        continue;
      }

      int64_t NestedOffset = Offset - static_cast<int64_t>(*ArgOffset);
      Argument *CalleeArg = Callee->getArg(I);
      HTypeBufferView CalleeView = getHTypeValueBufferView(HTypes, CalleeArg);
      if (hasHTypeBufferFieldAt(CalleeView, NestedOffset) &&
          hasStoreEvidenceAt(Stores, CalleeArg, NestedOffset)) {
        return true;
      }
    }
  }

  return false;
}

bool hasFixedTupleHelperPayloadHType(llvm2c::HTypeResult &HTypes,
                                     ArrayRef<mlsub::EVMStoreEvidence> Stores,
                                     Function &Helper, Argument *Base,
                                     const HTypeBufferView &View) {
  if (View.Record == nullptr) {
    return false;
  }

  SimpleRange Range = View.Record->getRange();
  if (Range.Start != 0 || Range.Size <= 0 || Range.Size % 32 != 0 ||
      Range.Size > 4096) {
    return false;
  }
  if (!helperReturnsBasePlusSize(Helper, Base,
                                 static_cast<uint64_t>(Range.Size))) {
    return false;
  }

  for (const ast::FieldDecl &Field : View.Record->getFields()) {
    if (Field.isPadding) {
      continue;
    }
    if (Field.R.Start < 0 || Field.R.Size <= 0 || Field.R.Size % 32 != 0) {
      return false;
    }
    for (int64_t Offset = Field.R.Start, End = Field.R.Start + Field.R.Size;
         Offset < End; Offset += 32) {
      if (!hasStoreEvidenceAt(Stores, Base, Offset) &&
          !hasNestedHelperStoreEvidenceAt(HTypes, Stores, Helper, Base,
                                          Offset)) {
        return false;
      }
    }
  }

  return true;
}

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

std::optional<AbiReturnHelperPayload>
getDynamicReturnHelperPayloadHType(llvm2c::HTypeResult &HTypes,
                                   ArrayRef<mlsub::EVMStoreEvidence> Stores,
                                   CallBase &Return) {
  if (!isFreeMemoryPointerLoad(Return.getArgOperand(1))) {
    return std::nullopt;
  }

  auto *Size = dyn_cast<BinaryOperator>(Return.getArgOperand(2));
  if (Size == nullptr || Size->getOpcode() != Instruction::Sub ||
      !isSameOrReloadedFreeMemoryBase(Size->getOperand(1),
                                      Return.getArgOperand(1))) {
    return std::nullopt;
  }

  auto *HelperCall = dyn_cast<CallBase>(Size->getOperand(0));
  Function *Helper =
      HelperCall == nullptr ? nullptr : HelperCall->getCalledFunction();
  if (Helper == nullptr || Helper->isDeclaration()) {
    return std::nullopt;
  }

  for (unsigned I = 0, E = HelperCall->arg_size(); I != E; ++I) {
    if (!isSameOrReloadedFreeMemoryBase(HelperCall->getArgOperand(I),
                                        Return.getArgOperand(1))) {
      continue;
    }
    if (I >= Helper->arg_size()) {
      continue;
    }

    HTypeBufferView View = getHTypeBufferView(
        HTypes, HelperCall->getArgOperand(I), *HelperCall, I);
    Argument *HelperArg = Helper->getArg(I);
    if (View.Record == nullptr && !View.HasTransparentOffset0Field) {
      // Some public wrappers keep the free-memory load as top, while the
      // encoder helper formal still has the recovered ABI buffer record.
      // Trust that formal only for this free-memory-base helper shape.
      View = getHTypeValueBufferView(HTypes, HelperArg);
    }
    if (View.Record == nullptr && !View.HasTransparentOffset0Field) {
      continue;
    }

    bool HasHead = hasHTypeBufferFieldAt(View, 0) &&
                   hasStoreEvidenceAt(Stores, HelperArg, 0);
    bool HasLength =
        hasHTypeBufferFieldAt(View, 32) &&
        (hasStoreEvidenceAt(Stores, HelperArg, 32) ||
         hasDelegatedStoreEvidenceAt(Stores, *Helper, HelperArg, 32));
    if (HasHead && HasLength) {
      return AbiReturnHelperPayload{HasHead, HasLength};
    }
    if (hasFixedTupleHelperPayloadHType(HTypes, Stores, *Helper, HelperArg,
                                        View)) {
      return AbiReturnHelperPayload{false, false, true};
    }
  }

  return std::nullopt;
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

  std::optional<AbiReturnHelperPayload> HelperPayload =
      getDynamicReturnHelperPayloadHType(HTypes, Stores, Return);
  if (HelperPayload.has_value() && HelperPayload->HasOffset0StoreEvidence &&
      HelperPayload->HasDynamicLengthStoreEvidence) {
    return StringRef("dynamic_candidate");
  }
  if (HelperPayload.has_value() && HelperPayload->HasFixedTupleStoreEvidence) {
    return StringRef("tuple_candidate");
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
