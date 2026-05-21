#include "Passes/evm/SolidityPatterns.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

STATISTIC(NumNonpayableGuards, "Number of Solidity nonpayable guards found");

namespace notdec::passes::evm {

const char *KIND_SOLIDITY_NONPAYABLE = "notdec.solidity.nonpayable";
const char *KIND_SOLIDITY_PAYABILITY_GUARD = "notdec.solidity.payability_guard";

namespace {

bool isCallTo(const Value *V, StringRef Name) {
  auto *Call = dyn_cast<CallBase>(V);
  if (Call == nullptr) {
    return false;
  }
  const Function *Callee = Call->getCalledFunction();
  return Callee != nullptr && Callee->getName() == Name;
}

bool isZero(const Value *V) {
  auto *C = dyn_cast<ConstantInt>(V);
  return C != nullptr && C->isZero();
}

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
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_revert") ||
        Call->arg_size() != 3 || !isZero(Call->getArgOperand(1)) ||
        !isZero(Call->getArgOperand(2))) {
      return false;
    }
  }
  return true;
}

void addStringMetadata(LLVMContext &Ctx, Instruction &I, StringRef Kind,
                       StringRef Value) {
  I.setMetadata(Kind, MDNode::get(Ctx, {MDString::get(Ctx, Value)}));
}

void markBlock(LLVMContext &Ctx, BasicBlock &BB, StringRef Kind,
               StringRef Value) {
  for (Instruction &I : BB) {
    addStringMetadata(Ctx, I, Kind, Value);
  }
}

} // namespace

PreservedAnalyses PayabilityGuardPass::run(Function &F,
                                           FunctionAnalysisManager &) {
  if (!F.getName().starts_with("public_") ||
      F.getName().contains("function_selector")) {
    return PreservedAnalyses::all();
  }

  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (BasicBlock &BB : F) {
    auto *Br = dyn_cast<BranchInst>(BB.getTerminator());
    if (Br == nullptr || !Br->isConditional()) {
      continue;
    }

    auto *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
    CallBase *CallValue = getCallValueFromPredicate(Cmp);
    if (CallValue == nullptr) {
      continue;
    }

    BasicBlock *Failure = Cmp->getPredicate() == ICmpInst::ICMP_EQ
                              ? Br->getSuccessor(1)
                              : Br->getSuccessor(0);
    if (!isEmptyRevertBlock(Failure)) {
      continue;
    }

    F.setMetadata(KIND_SOLIDITY_NONPAYABLE,
                  MDNode::get(Ctx, {MDString::get(Ctx, "true")}));
    addStringMetadata(Ctx, *CallValue, KIND_SOLIDITY_PAYABILITY_GUARD,
                      "callvalue");
    addStringMetadata(Ctx, *Cmp, KIND_SOLIDITY_PAYABILITY_GUARD, "condition");
    addStringMetadata(Ctx, *Br, KIND_SOLIDITY_PAYABILITY_GUARD, "branch");
    markBlock(Ctx, *Failure, KIND_SOLIDITY_PAYABILITY_GUARD, "revert");

    ++NumNonpayableGuards;
    Changed = true;
    break;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
