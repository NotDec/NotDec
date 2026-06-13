#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Pass.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Transforms/Utils/Local.h>

#define DEBUG_TYPE "evm-calldata-access"

using namespace llvm;

STATISTIC(NumCalldataLoadsRewritten,
          "Number of evm_calldataload calls rewritten to LLVM loads");
STATISTIC(NumCalldataCopiesRewritten,
          "Number of evm_calldatacopy calls rewritten to LLVM memcpy calls");
STATISTIC(NumCalldataCallsiteConstantOffsets,
          "Number of calldata offsets resolved from unique callsite constants");
STATISTIC(NumCalldataPolymorphicHelpers,
          "Number of private calldata helpers marked as polymorphic");

namespace notdec::passes::evm {
namespace {

bool shouldRewriteFunction(Function &F) {
  return detail::isPublicEntryFunction(F) || detail::isSelectorFunction(F) ||
         F.getName().starts_with("private__");
}

bool shouldMarkPolymorphicFunction(Function &F) {
  return F.getName().starts_with("private__") && !F.isDeclaration();
}

Value *getCalldataArg(Function &F) {
  if (!shouldRewriteFunction(F) || F.arg_size() < 2) {
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

bool usesCurrentCalldata(CallBase &Call, unsigned ArgIndex, Value *Calldata) {
  return Call.arg_size() > ArgIndex && Call.getArgOperand(ArgIndex) == Calldata;
}

std::optional<APInt> resolveOffsetConstant(Value *Offset, unsigned Depth = 0) {
  if (!Offset->getType()->isIntegerTy() || Depth > 4) {
    return std::nullopt;
  }

  if (auto *Constant = dyn_cast<ConstantInt>(Offset)) {
    return Constant->getValue();
  }

  if (std::optional<uint64_t> Constant =
          detail::getUniqueCallsiteArgUInt64Constant(Offset)) {
    return APInt(Offset->getType()->getIntegerBitWidth(), *Constant);
  }

  auto *Add = dyn_cast<BinaryOperator>(Offset);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return std::nullopt;
  }

  // Solidity helpers often compute a field address as "formal offset + 32".
  // Fold that only when the formal offset itself is constant across all direct
  // callsites; conflicting call contexts stay as a dynamic offset.
  std::optional<APInt> LHS =
      resolveOffsetConstant(Add->getOperand(0), Depth + 1);
  std::optional<APInt> RHS =
      resolveOffsetConstant(Add->getOperand(1), Depth + 1);
  if (!LHS.has_value() || !RHS.has_value()) {
    return std::nullopt;
  }
  return *LHS + *RHS;
}

Value *resolveCallsiteConstantOffset(Value *Offset) {
  std::optional<APInt> Constant = resolveOffsetConstant(Offset);
  if (!Constant.has_value() || isa<ConstantInt>(Offset)) {
    return Offset;
  }

  ++NumCalldataCallsiteConstantOffsets;
  return ConstantInt::get(Offset->getType(), *Constant);
}

Value *asPointer(IRBuilder<> &Builder, Value *V, const Twine &Name) {
  if (V->getType()->isPointerTy()) {
    return V;
  }
  return Builder.CreateIntToPtr(V, PointerType::get(Builder.getContext(), 0),
                                Name);
}

Value *calldataBytePtr(IRBuilder<> &Builder, Value *Calldata, Value *Offset,
                       const Twine &Name) {
  Type *OffsetTy = Offset->getType();
  Value *Base = Builder.CreatePtrToInt(Calldata, OffsetTy, Name + ".base");
  Value *Addr = Builder.CreateAdd(Base, Offset, Name + ".addr");
  return Builder.CreateIntToPtr(Addr, PointerType::get(Builder.getContext(), 0),
                                Name);
}

void collectArgumentUses(Value *V, SmallVectorImpl<Argument *> &Args,
                         unsigned Depth = 0) {
  if (V == nullptr || Depth > 8) {
    return;
  }
  if (auto *Arg = dyn_cast<Argument>(V)) {
    if (!Arg->getType()->isPointerTy()) {
      Args.push_back(Arg);
    }
    return;
  }
  auto *Inst = dyn_cast<Instruction>(V);
  if (Inst == nullptr) {
    return;
  }
  for (Use &Operand : Inst->operands()) {
    collectArgumentUses(Operand.get(), Args, Depth + 1);
  }
}

SmallVector<unsigned, 4> getCalldataOffsetArgNos(Function &F,
                                                 Value *Calldata) {
  SmallVector<Argument *, 8> Args;
  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    if (detail::isCallTo(Call, "evm_calldataload") &&
        usesCurrentCalldata(*Call, 0, Calldata) && Call->arg_size() == 2) {
      collectArgumentUses(Call->getArgOperand(1), Args);
    } else if (detail::isCallTo(Call, "evm_calldatacopy") &&
               usesCurrentCalldata(*Call, 1, Calldata) &&
               Call->arg_size() == 5) {
      collectArgumentUses(Call->getArgOperand(3), Args);
    }
  }

  SmallVector<unsigned, 4> ArgNos;
  llvm::sort(Args, [](Argument *LHS, Argument *RHS) {
    return LHS->getArgNo() < RHS->getArgNo();
  });
  for (Argument *Arg : Args) {
    if (ArgNos.empty() || ArgNos.back() != Arg->getArgNo()) {
      ArgNos.push_back(Arg->getArgNo());
    }
  }
  return ArgNos;
}

bool hasPolymorphicCallsiteOffset(Function &F, unsigned ArgNo) {
  std::optional<APInt> FirstConstant;
  bool SawConstant = false;
  bool SawDynamic = false;

  for (Use &U : F.uses()) {
    auto *Call = dyn_cast<CallBase>(U.getUser());
    if (Call == nullptr || Call->getCalledFunction() != &F ||
        ArgNo >= Call->arg_size()) {
      continue;
    }

    auto *Constant = dyn_cast<ConstantInt>(Call->getArgOperand(ArgNo));
    if (Constant == nullptr) {
      SawDynamic = true;
      continue;
    }

    if (FirstConstant.has_value() &&
        Constant->getValue() != *FirstConstant) {
      return true;
    }
    FirstConstant = Constant->getValue();
    SawConstant = true;
  }

  return SawConstant && SawDynamic;
}

bool markPolymorphicHelpers(Module &M) {
  bool Changed = false;

  for (Function &F : M) {
    if (!shouldMarkPolymorphicFunction(F)) {
      continue;
    }
    Value *Calldata = getCalldataArg(F);
    if (Calldata == nullptr) {
      continue;
    }
    SmallVector<unsigned, 4> OffsetArgNos = getCalldataOffsetArgNos(F, Calldata);
    bool Polymorphic = false;
    for (unsigned ArgNo : OffsetArgNos) {
      if (hasPolymorphicCallsiteOffset(F, ArgNo)) {
        Polymorphic = true;
        break;
      }
    }
    if (!Polymorphic ||
        F.getMetadata(detail::KIND_EVM_CALLDATA_POLYMORPHIC_HELPER) !=
            nullptr) {
      continue;
    }

    F.setMetadata(detail::KIND_EVM_CALLDATA_POLYMORPHIC_HELPER,
                  MDNode::get(F.getContext(),
                              {MDString::get(F.getContext(), "true")}));
    ++NumCalldataPolymorphicHelpers;
    Changed = true;
  }

  return Changed;
}

bool rewriteCalldataLoad(CallBase &Call, Value *Calldata) {
  if (!detail::isCallTo(&Call, "evm_calldataload") ||
      !usesCurrentCalldata(Call, 0, Calldata) || Call.arg_size() != 2) {
    return false;
  }

  IRBuilder<> Builder(&Call);
  Value *Offset = resolveCallsiteConstantOffset(Call.getArgOperand(1));
  Value *Addr = calldataBytePtr(Builder, Calldata, Offset, "calldata.ptr");
  auto *Load = Builder.CreateLoad(Builder.getIntNTy(256), Addr,
                                  Call.getName() + ".load");
  Load->setAlignment(Align(1));
  Load->copyMetadata(Call, {LLVMContext::MD_dbg, LLVMContext::MD_annotation});
  Call.replaceAllUsesWith(Load);
  Call.eraseFromParent();
  ++NumCalldataLoadsRewritten;
  return true;
}

bool rewriteCalldataCopy(CallBase &Call, Value *Calldata) {
  if (!detail::isCallTo(&Call, "evm_calldatacopy") ||
      !usesCurrentCalldata(Call, 1, Calldata) || Call.arg_size() != 5) {
    return false;
  }

  IRBuilder<> Builder(&Call);
  Value *Dst = asPointer(Builder, Call.getArgOperand(2), "calldata.copy.dst");
  Value *SrcOffset = resolveCallsiteConstantOffset(Call.getArgOperand(3));
  Value *Src =
      calldataBytePtr(Builder, Calldata, SrcOffset, "calldata.copy.src");
  Value *Len = Call.getArgOperand(4);
  CallInst *Memcpy =
      Builder.CreateMemCpy(Dst, Align(1), Src, Align(1), Len, false);
  Memcpy->copyMetadata(Call,
                       {LLVMContext::MD_dbg, LLVMContext::MD_annotation});
  Call.eraseFromParent();
  ++NumCalldataCopiesRewritten;
  return true;
}

bool rewriteFunction(Function &F) {
  Value *Calldata = getCalldataArg(F);
  if (Calldata == nullptr) {
    return false;
  }

  SmallVector<CallBase *, 16> Calls;
  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    if (detail::isCallTo(Call, "evm_calldataload") ||
        detail::isCallTo(Call, "evm_calldatacopy")) {
      Calls.push_back(Call);
    }
  }

  bool Changed = false;
  for (CallBase *Call : Calls) {
    if (Call->getParent() == nullptr) {
      continue;
    }
    if (rewriteCalldataLoad(*Call, Calldata)) {
      Changed = true;
      continue;
    }
    if (rewriteCalldataCopy(*Call, Calldata)) {
      Changed = true;
    }
  }

  return Changed;
}

} // namespace

PreservedAnalyses EvmCalldataAccessPass::run(Module &M,
                                             ModuleAnalysisManager &) {
  bool Changed = markPolymorphicHelpers(M);
  SmallVector<Function *, 32> Functions;
  for (Function &F : M) {
    if (!F.isDeclaration()) {
      Functions.push_back(&F);
    }
  }
  for (Function *F : Functions) {
    Changed |= rewriteFunction(*F);
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
