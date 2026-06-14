#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#define DEBUG_TYPE "evm-abi-decoder-helper-rename"

using namespace llvm;

STATISTIC(NumAbiDecoderHelpersRenamed,
          "Number of private ABI decoder helpers renamed");

namespace notdec::passes::evm {
namespace {

// Coarse Solidity ABI helper categories.  These names intentionally describe
// the helper role, not the final Solidity parameter type, because this pass
// only renames functions and does not recover ABI type summaries.
enum class HelperKind {
  None,
  Word,
  DynamicHead,
  ArrayHead,
  ArrayIndexAccess,
  DynamicTail,
};

bool isSmallPrivateHelper(Function &F) {
  if (!F.getName().starts_with("private__") || F.isDeclaration()) {
    return false;
  }
  unsigned Blocks = 0;
  unsigned Insts = 0;
  for (BasicBlock &BB : F) {
    ++Blocks;
    if (Blocks > 8) {
      return false;
    }
    for (Instruction &Inst : BB) {
      (void)Inst;
      ++Insts;
      if (Insts > 90) {
        return false;
      }
    }
  }
  return true;
}

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

bool isCurrentCalldataPointer(Value *V, Value *Calldata) {
  V = V == nullptr ? nullptr : V->stripPointerCasts();
  if (V == Calldata) {
    return true;
  }
  auto *Call = dyn_cast_or_null<CallBase>(V);
  return Call != nullptr && detail::isCallTo(Call, "notdec_evm_calldata_min_size") &&
         Call->arg_size() >= 1 &&
         isCurrentCalldataPointer(Call->getArgOperand(0), Calldata);
}

bool isCalldataAddress(Value *V, Value *Calldata, unsigned Depth = 0) {
  if (V == nullptr || Depth > 8) {
    return false;
  }
  V = V->stripPointerCasts();
  if (isCurrentCalldataPointer(V, Calldata)) {
    return true;
  }
  if (auto *PTI = dyn_cast<PtrToIntInst>(V)) {
    return isCurrentCalldataPointer(PTI->getOperand(0), Calldata);
  }
  if (auto *ITP = dyn_cast<IntToPtrInst>(V)) {
    return isCalldataAddress(ITP->getOperand(0), Calldata, Depth + 1);
  }
  auto *Op = dyn_cast<BinaryOperator>(V);
  if (Op == nullptr || Op->getOpcode() != Instruction::Add) {
    return false;
  }
  return isCalldataAddress(Op->getOperand(0), Calldata, Depth + 1) ||
         isCalldataAddress(Op->getOperand(1), Calldata, Depth + 1);
}

bool isCalldataLoad(Value *V, Value *Calldata) {
  auto *Load = dyn_cast_or_null<LoadInst>(V);
  return Load != nullptr && Load->getType()->isIntegerTy(256) &&
         isCalldataAddress(Load->getPointerOperand(), Calldata);
}

bool valueDependsOn(Value *V, Value *Needle, unsigned Depth = 0) {
  if (V == nullptr || Needle == nullptr || Depth > 8) {
    return false;
  }
  if (V == Needle) {
    return true;
  }
  auto *Inst = dyn_cast<Instruction>(V);
  if (Inst == nullptr) {
    return false;
  }
  for (Value *Op : Inst->operands()) {
    if (valueDependsOn(Op, Needle, Depth + 1)) {
      return true;
    }
  }
  return false;
}

bool isConstantValue(Value *V, uint64_t N) {
  return detail::isConstantIntValue(V, N);
}

bool hasAddConstant(Value *V, uint64_t N) {
  auto *Op = dyn_cast_or_null<BinaryOperator>(V);
  if (Op == nullptr || Op->getOpcode() != Instruction::Add) {
    return false;
  }
  return isConstantValue(Op->getOperand(0), N) ||
         isConstantValue(Op->getOperand(1), N);
}

bool isNonPointerArgument(Value *V, Function &F) {
  auto *Arg = dyn_cast_or_null<Argument>(V);
  return Arg != nullptr && Arg->getParent() == &F && !Arg->getType()->isPointerTy();
}

bool isIndexScale(Value *V, Function &F) {
  auto *Op = dyn_cast_or_null<BinaryOperator>(V);
  if (Op == nullptr) {
    return false;
  }
  if (Op->getOpcode() == Instruction::Shl) {
    auto *Shift = dyn_cast<ConstantInt>(Op->getOperand(1));
    return Shift != nullptr && !Shift->isZero() &&
           isNonPointerArgument(Op->getOperand(0), F);
  }
  if (Op->getOpcode() != Instruction::Mul) {
    return false;
  }
  for (unsigned I = 0; I < 2; ++I) {
    auto *Scale = dyn_cast<ConstantInt>(Op->getOperand(1 - I));
    if (Scale != nullptr && !Scale->isZero() &&
        isNonPointerArgument(Op->getOperand(I), F)) {
      return true;
    }
  }
  return false;
}

bool valueContainsIndexScale(Value *V, Function &F, unsigned Depth = 0) {
  if (V == nullptr || Depth > 8) {
    return false;
  }
  if (isIndexScale(V, F)) {
    return true;
  }
  auto *Inst = dyn_cast<Instruction>(V);
  if (Inst == nullptr) {
    return false;
  }
  for (Value *Op : Inst->operands()) {
    if (valueContainsIndexScale(Op, F, Depth + 1)) {
      return true;
    }
  }
  return false;
}

bool hasCalldataSize(Value *V, Value *Calldata) {
  auto *Call = dyn_cast_or_null<CallBase>(V);
  return Call != nullptr && detail::isCallTo(Call, "evm_calldatasize") &&
         Call->arg_size() >= 1 && Call->getArgOperand(0) == Calldata;
}

bool valueMentionsCalldataSize(Value *V, Value *Calldata, unsigned Depth = 0) {
  if (V == nullptr || Depth > 8) {
    return false;
  }
  if (hasCalldataSize(V, Calldata)) {
    return true;
  }
  auto *Inst = dyn_cast<Instruction>(V);
  if (Inst == nullptr) {
    return false;
  }
  for (Value *Op : Inst->operands()) {
    if (valueMentionsCalldataSize(Op, Calldata, Depth + 1)) {
      return true;
    }
  }
  return false;
}

bool returnDependsOn(Function &F, Value *V) {
  for (BasicBlock &BB : F) {
    if (auto *Ret = dyn_cast<ReturnInst>(BB.getTerminator())) {
      if (valueDependsOn(Ret->getReturnValue(), V)) {
        return true;
      }
    }
  }
  return false;
}

// Small set of local facts used to keep matching conservative.  We collect only
// body-local evidence that survives calldata-load rewriting, then choose the
// most specific helper name from those facts.
struct HelperFacts {
  SmallVector<LoadInst *, 4> CalldataLoads;
  bool HasCallToPrivateHelper = false;
  bool HasCalldataCopy = false;
  bool HasArrayIndexShape = false;
  bool HasDynamicTailShape = false;
  bool HasArrayHeadShape = false;
};

HelperFacts collectFacts(Function &F, Value *Calldata) {
  HelperFacts Facts;
  for (Instruction &I : instructions(F)) {
    if (auto *Call = dyn_cast<CallBase>(&I)) {
      if (detail::isCallTo(Call, "evm_calldatacopy") ||
          detail::isCallTo(Call, "llvm.memcpy.p0.p0.i256")) {
        Facts.HasCalldataCopy = true;
      }
      if (detail::isPrivateHelperCall(Call)) {
        Facts.HasCallToPrivateHelper = true;
      }
    }
    if (auto *Load = dyn_cast<LoadInst>(&I)) {
      if (isCalldataLoad(Load, Calldata)) {
        Facts.CalldataLoads.push_back(Load);
      }
    }
  }

  for (Instruction &I : instructions(F)) {
    auto *Cmp = dyn_cast<ICmpInst>(&I);
    if (Cmp == nullptr) {
      continue;
    }
    Value *LHS = Cmp->getOperand(0);
    Value *RHS = Cmp->getOperand(1);
    bool MentionsSize = valueMentionsCalldataSize(LHS, Calldata) ||
                        valueMentionsCalldataSize(RHS, Calldata);
    for (LoadInst *Load : Facts.CalldataLoads) {
      bool CmpUsesLength = valueDependsOn(LHS, Load) || valueDependsOn(RHS, Load);
      if (!CmpUsesLength) {
        continue;
      }
      if (MentionsSize) {
        Facts.HasArrayHeadShape = true;
      }
    }
  }

  bool HasIndexBounds = false;
  bool HasIndexAddress = false;
  for (Instruction &I : instructions(F)) {
    auto *Op = dyn_cast<BinaryOperator>(&I);
    if (Op == nullptr) {
      continue;
    }
    if (hasAddConstant(Op, 32)) {
      for (LoadInst *Load : Facts.CalldataLoads) {
        if (valueDependsOn(Op, Load)) {
          Facts.HasDynamicTailShape = true;
        }
      }
    }
  }

  for (Instruction &I : instructions(F)) {
    auto *Cmp = dyn_cast<ICmpInst>(&I);
    if (Cmp != nullptr && Cmp->isUnsigned()) {
      HasIndexBounds = true;
    }
  }
  for (BasicBlock &BB : F) {
    auto *Ret = dyn_cast<ReturnInst>(BB.getTerminator());
    if (Ret != nullptr && valueContainsIndexScale(Ret->getReturnValue(), F)) {
      HasIndexAddress = true;
    }
  }
  Facts.HasArrayIndexShape = HasIndexBounds && HasIndexAddress;
  return Facts;
}

HelperKind classifyHelper(Function &F, const HelperFacts &Facts) {
  if (Facts.HasCallToPrivateHelper || Facts.HasCalldataCopy) {
    return HelperKind::None;
  }
  if (Facts.HasArrayHeadShape) {
    return HelperKind::ArrayHead;
  }
  if (Facts.HasArrayIndexShape && Facts.CalldataLoads.empty()) {
    return HelperKind::ArrayIndexAccess;
  }
  if (Facts.HasDynamicTailShape) {
    return HelperKind::DynamicTail;
  }
  if (Facts.CalldataLoads.size() == 1 && F.getReturnType()->isIntegerTy(256) &&
      returnDependsOn(F, Facts.CalldataLoads.front())) {
    return HelperKind::Word;
  }
  if (Facts.CalldataLoads.size() == 1) {
    return HelperKind::DynamicHead;
  }
  return HelperKind::None;
}

StringRef helperKindName(HelperKind Kind) {
  switch (Kind) {
  case HelperKind::Word:
    return "abi_decode_word_from_calldata";
  case HelperKind::DynamicHead:
    return "abi_decode_calldata_dynamic_head";
  case HelperKind::ArrayHead:
    return "abi_decode_calldata_array_head";
  case HelperKind::ArrayIndexAccess:
    return "calldata_array_index_access";
  case HelperKind::DynamicTail:
    return "calldata_access_dynamic_tail";
  case HelperKind::None:
    return "";
  }
  return "";
}

std::string firstAddressSuffix(StringRef Name) {
  if (!Name.consume_front("private__")) {
    return "unknown";
  }
  return Name.split('_').first.str();
}

void markOriginalPrivateHelper(Function &F) {
  if (F.getMetadata(detail::KIND_EVM_ORIGINAL_PRIVATE_HELPER) != nullptr) {
    return;
  }
  LLVMContext &Ctx = F.getContext();
  F.setMetadata(detail::KIND_EVM_ORIGINAL_PRIVATE_HELPER,
                MDNode::get(Ctx, {MDString::get(Ctx, F.getName())}));
}

bool renameHelper(Function &F, HelperKind Kind) {
  StringRef Base = helperKindName(Kind);
  if (Base.empty()) {
    return false;
  }
  SmallString<96> NewName(Base);
  NewName += "__";
  NewName += firstAddressSuffix(F.getName());
  markOriginalPrivateHelper(F);
  F.setName(NewName);
  ++NumAbiDecoderHelpersRenamed;
  return true;
}

} // namespace

PreservedAnalyses AbiDecoderHelperRenamePass::run(Module &M,
                                                  ModuleAnalysisManager &) {
  bool Changed = false;
  SmallVector<Function *, 16> Functions;
  for (Function &F : M) {
    Functions.push_back(&F);
  }

  for (Function *F : Functions) {
    if (!isSmallPrivateHelper(*F)) {
      continue;
    }
    Value *Calldata = getCalldataArg(*F);
    if (Calldata == nullptr) {
      continue;
    }
    HelperFacts Facts = collectFacts(*F, Calldata);
    HelperKind Kind = classifyHelper(*F, Facts);
    Changed |= renameHelper(*F, Kind);
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
