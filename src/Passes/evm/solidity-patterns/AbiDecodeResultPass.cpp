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

// Recognize the EvmCalldataAccessPass address shape
// inttoptr(add(ptrtoint(calldata), const)) and return the constant byte offset.
std::optional<uint64_t> calldataAddressOffset(const Value *V,
                                              const Value *Calldata,
                                              unsigned Depth = 0) {
  if (V == nullptr || Depth > 8) {
    return std::nullopt;
  }
  const auto *ITP = dyn_cast<IntToPtrInst>(V);
  if (ITP == nullptr) {
    return std::nullopt;
  }
  const auto *Add = dyn_cast<BinaryOperator>(ITP->getOperand(0));
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return std::nullopt;
  }
  const Value *Base = Add->getOperand(0);
  const auto *Offset = dyn_cast<ConstantInt>(Add->getOperand(1));
  if (Offset == nullptr) {
    Offset = dyn_cast<ConstantInt>(Add->getOperand(0));
    Base = Add->getOperand(1);
  }
  const auto *PTI = dyn_cast<PtrToIntInst>(Base);
  if (Offset == nullptr || PTI == nullptr ||
      !isCurrentCalldataPointer(PTI->getOperand(0), Calldata)) {
    return std::nullopt;
  }
  if (Offset->getValue().getActiveBits() > 64) {
    return std::nullopt;
  }
  return Offset->getZExtValue();
}

std::optional<uint64_t> calldataLoadOffset(const Value *V,
                                           const Value *Calldata) {
  const auto *Load = dyn_cast_or_null<LoadInst>(V);
  if (Load == nullptr) {
    return std::nullopt;
  }
  return calldataAddressOffset(Load->getPointerOperand(), Calldata);
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

std::optional<unsigned> abiIndexFromByteOffset(uint64_t ByteOffset) {
  if (ByteOffset < 4 || (ByteOffset - 4) % 32 != 0) {
    return std::nullopt;
  }
  uint64_t Index = (ByteOffset - 4) / 32;
  if (Index > 4096) {
    return std::nullopt;
  }
  return static_cast<unsigned>(Index);
}

// Backend-interface annotation only: the instruction itself stays as-is, so
// this deliberately does not go through addStringMetadata() and its markers.
bool annotateAbiIndex(Instruction &I, uint64_t ByteOffset) {
  std::optional<unsigned> Index = abiIndexFromByteOffset(ByteOffset);
  if (!Index.has_value()) {
    return false;
  }
  LLVMContext &Ctx = I.getContext();
  I.setMetadata(KIND_SOLIDITY_CALLDATA_ARG_INDEX,
                MDNode::get(Ctx, MDString::get(Ctx, std::to_string(*Index))));
  ++NumAbiDecoderResultsAnnotated;
  return true;
}

} // namespace

// Outlined ABI decoder helpers return calldata words that the caller then
// consumes as ABI arguments.  Trace each returned element back to its constant
// calldata offset and record the ABI argument index on the caller's
// extractvalue, so the Solidity backend does not need a local call/local
// materialization surface for the decoder half of the private-helper problem.
PreservedAnalyses AbiDecodeResultPass::run(Module &M, ModuleAnalysisManager &) {
  std::map<const Function *, std::map<unsigned, uint64_t>> HelperOffsets;
  for (Function &F : M) {
    if (F.isDeclaration() || detail::isPublicEntryFunction(F)) {
      continue;
    }
    const Value *Calldata = getCalldataArg(F);
    if (Calldata == nullptr) {
      continue;
    }
    std::optional<std::map<unsigned, const Value *>> Elements =
        returnedElements(F);
    if (!Elements.has_value()) {
      continue;
    }
    std::map<unsigned, uint64_t> Offsets;
    bool Traced = true;
    for (const auto &[ElementIndex, ElementValue] : *Elements) {
      std::optional<uint64_t> Offset =
          calldataLoadOffset(ElementValue, Calldata);
      if (!Offset.has_value()) {
        Traced = false;
        break;
      }
      Offsets[ElementIndex] = *Offset;
    }
    if (Traced && !Offsets.empty()) {
      ++NumAbiDecoderHelpersMatched;
      HelperOffsets.emplace(&F, std::move(Offsets));
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
      auto It = HelperOffsets.find(CB->getCalledFunction());
      if (It == HelperOffsets.end()) {
        continue;
      }
      if (CB->getType()->isIntegerTy()) {
        auto Element = It->second.find(0);
        if (Element != It->second.end()) {
          Changed |= annotateAbiIndex(*CB, Element->second);
        }
      }
      for (User *U : CB->users()) {
        auto *EV = dyn_cast<ExtractValueInst>(U);
        if (EV == nullptr || EV->getNumIndices() != 1) {
          continue;
        }
        auto Element = It->second.find(EV->getIndices()[0]);
        if (Element == It->second.end()) {
          continue;
        }
        Changed |= annotateAbiIndex(*EV, Element->second);
      }
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
