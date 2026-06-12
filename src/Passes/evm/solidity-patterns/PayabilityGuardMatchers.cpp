#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <optional>

using namespace llvm;

namespace notdec::passes::evm::detail {

CallBase *getCallValueFromPredicate(ICmpInst *Cmp) {
  if (Cmp == nullptr) {
    return nullptr;
  }
  if (Cmp->getPredicate() != ICmpInst::ICMP_EQ &&
      Cmp->getPredicate() != ICmpInst::ICMP_NE) {
    return nullptr;
  }

  for (unsigned I = 0; I < 2; ++I) {
    Value *MaybeCallValue = Cmp->getOperand(I);
    Value *MaybeZero = Cmp->getOperand(1 - I);
    if (isCallTo(MaybeCallValue, "evm_callvalue") && isZero(MaybeZero)) {
      return cast<CallBase>(MaybeCallValue);
    }
  }
  return nullptr;
}

bool isEmptyRevertBlock(BasicBlock *BB) {
  if (BB == nullptr || !isa<UnreachableInst>(BB->getTerminator())) {
    return false;
  }

  for (Instruction &I : *BB) {
    if (isa<UnreachableInst>(&I)) {
      continue;
    }
    if (isRewriteMarkerCall(I)) {
      continue;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_revert") ||
        Call->arg_size() != 3 || !isZero(Call->getArgOperand(1)) ||
        !isZero(Call->getArgOperand(2))) {
      return false;
    }
  }
  return true;
}

std::optional<PayabilityGuardMatch> matchPayabilityGuard(BasicBlock &BB) {
  auto *Br = dyn_cast<BranchInst>(BB.getTerminator());
  if (Br == nullptr || !Br->isConditional()) {
    return std::nullopt;
  }

  auto *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
  CallBase *CallValue = getCallValueFromPredicate(Cmp);
  if (CallValue == nullptr) {
    return std::nullopt;
  }

  BasicBlock *Success = nullptr;
  BasicBlock *Failure = nullptr;
  if (Cmp->getPredicate() == ICmpInst::ICMP_EQ) {
    Success = Br->getSuccessor(0);
    Failure = Br->getSuccessor(1);
  } else {
    Success = Br->getSuccessor(1);
    Failure = Br->getSuccessor(0);
  }
  if (!isEmptyRevertBlock(Failure)) {
    return std::nullopt;
  }

  return PayabilityGuardMatch{&BB, Success, Failure, CallValue, Cmp, Br};
}

void insertPayabilityCfgRewriteMarker(LLVMContext &Ctx,
                                      const PayabilityGuardMatch &Match) {
  Module *M = Match.GuardBlock->getModule();
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_cfg_rewrite_payability_guard",
      FunctionType::get(Type::getVoidTy(Ctx), {Type::getIntNTy(Ctx, 256)},
                        false));

  IRBuilder<> Builder(Match.Branch);
  Value *Args[] = {ConstantInt::get(Type::getIntNTy(Ctx, 256),
                                    getRewriteKindCode("payability_guard"))};
  Builder.CreateCall(Marker, Args);
}

} // namespace notdec::passes::evm::detail
