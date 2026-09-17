#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <cstdint>
#include <map>
#include <optional>
#include <string>

#define DEBUG_TYPE "evm-abi-decode-result"

using namespace llvm;

STATISTIC(NumAbiDecoderHelpersMatched,
          "Number of private ABI decoder helpers with traced calldata words");
STATISTIC(NumAbiDecoderResultsAnnotated,
          "Number of decoder results annotated with an ABI argument index");

namespace notdec::passes::evm {
namespace {

// One decoded word is either at an absolute calldata byte offset or relative to
// the helper's base-offset formal (its first extra argument).  The relative form
// is what nested decoder helpers look like before inlining.
struct TracedOffset {
  bool Relative = false;
  int64_t Value = 0;
};

using ElementOffsets = std::map<unsigned, TracedOffset>;
using HelperOffsets = std::map<const Function *, ElementOffsets>;

Value *getCalldataArg(Function &F) {
  if (F.arg_size() < 2) {
    return nullptr;
  }
  auto It = F.arg_begin();
  ++It;
  Argument &Arg = *It;
  if (!Arg.getType()->isPointerTy() || Arg.getName() != "calldata") {
    return nullptr;
  }
  return &Arg;
}

// Outline helpers receive (mem, calldata, returndata, env, base, size, target).
// The base formal is the calldata byte offset the helper decodes from.
const Argument *getBaseFormal(const Function &F) {
  if (F.arg_size() < 5) {
    return nullptr;
  }
  return F.getArg(4);
}

bool isCurrentCalldataPointer(const Value *V, const Value *Calldata) {
  V = V == nullptr ? nullptr : V->stripPointerCasts();
  if (V == Calldata) {
    return true;
  }
  const auto *Call = dyn_cast_or_null<CallBase>(V);
  return Call != nullptr &&
         detail::isCallTo(Call, "notdec_evm_calldata_min_size") &&
         Call->arg_size() >= 1 &&
         isCurrentCalldataPointer(Call->getArgOperand(0), Calldata);
}

std::optional<int64_t> constantOffsetValue(const ConstantInt &C) {
  const APInt &Value = C.getValue();
  if (Value.getActiveBits() > 62) {
    return std::nullopt;
  }
  return static_cast<int64_t>(Value.getZExtValue());
}

// Trace the integer operand of the rewritten calldata address:
// inttoptr(add(ptrtoint(calldata), Offset)).  Offset is either a constant, the
// helper's base formal, or base + constant.
std::optional<TracedOffset> traceOffsetOperand(const Value *Offset,
                                               const Argument *BaseFormal,
                                               unsigned Depth = 0) {
  if (Offset == nullptr || Depth > 8) {
    return std::nullopt;
  }
  if (const auto *C = dyn_cast<ConstantInt>(Offset)) {
    std::optional<int64_t> Value = constantOffsetValue(*C);
    if (!Value.has_value()) {
      return std::nullopt;
    }
    return TracedOffset{false, *Value};
  }
  if (Offset == BaseFormal) {
    return TracedOffset{true, 0};
  }
  const auto *Add = dyn_cast<BinaryOperator>(Offset);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return std::nullopt;
  }
  for (unsigned ConstantIndex = 0; ConstantIndex < 2; ++ConstantIndex) {
    const auto *C = dyn_cast<ConstantInt>(Add->getOperand(ConstantIndex));
    if (C == nullptr) {
      continue;
    }
    std::optional<int64_t> Value = constantOffsetValue(*C);
    std::optional<TracedOffset> Base = traceOffsetOperand(
        Add->getOperand(1 - ConstantIndex), BaseFormal, Depth + 1);
    if (!Value.has_value() || !Base.has_value() || !Base->Relative) {
      continue;
    }
    return TracedOffset{true, Base->Value + *Value};
  }
  return std::nullopt;
}

std::optional<TracedOffset> traceCalldataLoad(const Value *V,
                                              const Value *Calldata,
                                              const Argument *BaseFormal) {
  const auto *Load = dyn_cast_or_null<LoadInst>(V);
  if (Load == nullptr) {
    return std::nullopt;
  }
  const auto *ITP = dyn_cast<IntToPtrInst>(Load->getPointerOperand());
  if (ITP == nullptr) {
    return std::nullopt;
  }
  const auto *Add = dyn_cast<BinaryOperator>(ITP->getOperand(0));
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return std::nullopt;
  }
  for (unsigned CalldataIndex = 0; CalldataIndex < 2; ++CalldataIndex) {
    const auto *PTI = dyn_cast<PtrToIntInst>(Add->getOperand(CalldataIndex));
    if (PTI == nullptr ||
        !isCurrentCalldataPointer(PTI->getOperand(0), Calldata)) {
      continue;
    }
    return traceOffsetOperand(Add->getOperand(1 - CalldataIndex), BaseFormal);
  }
  return std::nullopt;
}

// Decompose the function's single return value into element -> value.  A scalar
// return is element 0.  Struct returns must be built by a plain insertvalue
// chain over poison/undef; anything else is left alone.
std::optional<std::map<unsigned, const Value *>>
returnedElements(const Function &F) {
  const ReturnInst *Ret = nullptr;
  for (const BasicBlock &BB : F) {
    const auto *R = dyn_cast<ReturnInst>(BB.getTerminator());
    if (R == nullptr) {
      continue;
    }
    if (Ret != nullptr) {
      return std::nullopt;
    }
    Ret = R;
  }
  if (Ret == nullptr || Ret->getReturnValue() == nullptr) {
    return std::nullopt;
  }

  const Value *RetVal = Ret->getReturnValue();
  if (RetVal->getType()->isIntegerTy()) {
    return std::map<unsigned, const Value *>{{0, RetVal}};
  }
  const auto *ST = dyn_cast<StructType>(RetVal->getType());
  if (ST == nullptr) {
    return std::nullopt;
  }

  std::map<unsigned, const Value *> Elements;
  const Value *Current = RetVal;
  unsigned Guard = 0;
  while (const auto *IV = dyn_cast<InsertValueInst>(Current)) {
    if (++Guard > 64 || IV->getNumIndices() != 1) {
      return std::nullopt;
    }
    if (Elements.count(IV->getIndices()[0]) != 0) {
      return std::nullopt;
    }
    Elements[IV->getIndices()[0]] = IV->getInsertedValueOperand();
    Current = IV->getAggregateOperand();
  }
  if (!isa<PoisonValue>(Current) && !isa<UndefValue>(Current)) {
    return std::nullopt;
  }
  for (unsigned I = 0; I < ST->getNumElements(); ++I) {
    if (Elements.count(I) == 0) {
      return std::nullopt;
    }
  }
  return Elements;
}

// Trace one returned element value.  It is either a direct calldata load or the
// result of another decoder helper whose already-traced offsets compose with the
// callee's base argument.
std::optional<TracedOffset> traceElementValue(const Value *V,
                                              const Value *Calldata,
                                              const Argument *BaseFormal,
                                              const HelperOffsets &Known,
                                              unsigned Depth = 0) {
  if (V == nullptr || Depth > 6) {
    return std::nullopt;
  }
  if (std::optional<TracedOffset> Load =
          traceCalldataLoad(V, Calldata, BaseFormal)) {
    return Load;
  }
  const auto *Call = dyn_cast<CallBase>(V);
  if (Call == nullptr) {
    return std::nullopt;
  }
  unsigned ElementIndex = 0;
  if (const auto *EV = dyn_cast<ExtractValueInst>(V)) {
    if (EV->getNumIndices() != 1) {
      return std::nullopt;
    }
    ElementIndex = EV->getIndices()[0];
    Call = dyn_cast<CallBase>(EV->getAggregateOperand());
    if (Call == nullptr) {
      return std::nullopt;
    }
  }
  auto KnownIt = Known.find(Call->getCalledFunction());
  if (KnownIt == Known.end()) {
    return std::nullopt;
  }
  auto Element = KnownIt->second.find(ElementIndex);
  if (Element == KnownIt->second.end()) {
    return std::nullopt;
  }
  if (!Element->second.Relative) {
    return Element->second;
  }
  if (Call->arg_size() < 5) {
    return std::nullopt;
  }
  std::optional<TracedOffset> CallBase =
      traceOffsetOperand(Call->getArgOperand(4), BaseFormal, Depth + 1);
  if (!CallBase.has_value()) {
    return std::nullopt;
  }
  return TracedOffset{CallBase->Relative,
                      CallBase->Value + Element->second.Value};
}

std::optional<unsigned> abiIndexFromByteOffset(int64_t ByteOffset) {
  if (ByteOffset < 4 || (ByteOffset - 4) % 32 != 0) {
    return std::nullopt;
  }
  int64_t Index = (ByteOffset - 4) / 32;
  if (Index > 4096) {
    return std::nullopt;
  }
  return static_cast<unsigned>(Index);
}

// Backend-interface annotation only: the instruction itself stays as-is, so
// this deliberately does not go through addStringMetadata() and its markers.
bool annotateAbiIndex(Instruction &I, unsigned Index) {
  LLVMContext &Ctx = I.getContext();
  I.setMetadata(KIND_SOLIDITY_CALLDATA_ARG_INDEX,
                MDNode::get(Ctx, MDString::get(Ctx, std::to_string(Index))));
  ++NumAbiDecoderResultsAnnotated;
  return true;
}

} // namespace

// Outlined ABI decoder helpers return calldata words that the caller then
// consumes as ABI arguments.  Trace each returned element back to its calldata
// byte offset (absolute, or relative to the helper's base formal for nested
// decoder calls) and record the ABI argument index on the caller's extractvalue
// or scalar call.  This keeps the decoder half of private-helper recovery in
// metadata instead of requiring local call materialization in the backend.
PreservedAnalyses AbiDecodeResultPass::run(Module &M, ModuleAnalysisManager &) {
  HelperOffsets Known;
  for (unsigned Round = 0; Round < 6; ++Round) {
    bool Added = false;
    for (Function &F : M) {
      if (F.isDeclaration() || detail::isPublicEntryFunction(F) ||
          Known.count(&F) != 0) {
        continue;
      }
      Value *Calldata = getCalldataArg(F);
      const Argument *BaseFormal = getBaseFormal(F);
      if (Calldata == nullptr || BaseFormal == nullptr) {
        continue;
      }
      std::optional<std::map<unsigned, const Value *>> Elements =
          returnedElements(F);
      if (!Elements.has_value()) {
        continue;
      }
      ElementOffsets Offsets;
      bool Traced = true;
      for (const auto &[ElementIndex, ElementValue] : *Elements) {
        std::optional<TracedOffset> Offset = traceElementValue(
            ElementValue, Calldata, BaseFormal, Known);
        if (!Offset.has_value()) {
          Traced = false;
          break;
        }
        Offsets[ElementIndex] = *Offset;
      }
      if (Traced && !Offsets.empty()) {
        ++NumAbiDecoderHelpersMatched;
        Known.emplace(&F, std::move(Offsets));
        Added = true;
      }
    }
    if (!Added) {
      break;
    }
  }

  bool Changed = false;
  for (Function &F : M) {
    if (F.isDeclaration() || !detail::isPublicEntryFunction(F)) {
      continue;
    }
    for (Instruction &I : instructions(F)) {
      auto *CB = dyn_cast<CallBase>(&I);
      if (CB == nullptr) {
        continue;
      }
      auto It = Known.find(CB->getCalledFunction());
      if (It == Known.end() || CB->arg_size() < 5) {
        continue;
      }
      const auto *BaseArg = dyn_cast<ConstantInt>(CB->getArgOperand(4));
      if (BaseArg == nullptr) {
        continue;
      }
      std::optional<int64_t> BaseValue = constantOffsetValue(*BaseArg);
      if (!BaseValue.has_value()) {
        continue;
      }
      auto Annotate = [&](Instruction &Target, unsigned ElementIndex) {
        auto Element = It->second.find(ElementIndex);
        if (Element == It->second.end()) {
          return;
        }
        int64_t Absolute = Element->second.Relative
                               ? *BaseValue + Element->second.Value
                               : Element->second.Value;
        std::optional<unsigned> Index = abiIndexFromByteOffset(Absolute);
        if (Index.has_value()) {
          Changed |= annotateAbiIndex(Target, *Index);
        }
      };
      if (CB->getType()->isIntegerTy()) {
        Annotate(*CB, 0);
      }
      for (User *U : CB->users()) {
        auto *EV = dyn_cast<ExtractValueInst>(U);
        if (EV == nullptr || EV->getNumIndices() != 1) {
          continue;
        }
        Annotate(*EV, EV->getIndices()[0]);
      }
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
