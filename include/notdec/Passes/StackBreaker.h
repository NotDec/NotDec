#ifndef _NOTDEC_PASSES_STACK_BREAKER_H_
#define _NOTDEC_PASSES_STACK_BREAKER_H_

#include <vector>

#include <llvm/Analysis/PtrUseVisitor.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>

#include "Passes/ConstraintGenerator.h"
#include "notdec-llvm2c/Interface/StructManager.h"

namespace notdec {

using namespace llvm;

struct NewAllocas {
  SimpleRange R;
  AllocaInst *NewAI;
};

struct StackBreaker {
  bool runOnAlloca(AllocaInst &AI, SCCTypeResult &HT, std::vector<std::pair<SimpleRange, std::string>>* Vec);
};

class StackBreakerRewriter : public PtrUseVisitor<StackBreakerRewriter> {
  friend class PtrUseVisitor<StackBreakerRewriter>;
  friend class InstVisitor<StackBreakerRewriter>;
  using Base = PtrUseVisitor<StackBreakerRewriter>;

  AllocaInst *OldAI;
  const std::vector<NewAllocas> &NAs;
  /// Set to de-duplicate dead instructions found in the use walk.
  SmallPtrSet<Instruction *, 4> VisitedDeadInsts;
  SmallVector<WeakVH, 8> DeadInsts;
  void markAsDead(Instruction &I) {
    if (VisitedDeadInsts.insert(&I).second)
      DeadInsts.push_back(&I);
  }

public:
  StackBreakerRewriter(const DataLayout &DL, const std::vector<NewAllocas> &NAs)
      : PtrUseVisitor<StackBreakerRewriter>(DL), NAs(NAs) {}

  const NewAllocas *getNA(OffsetTy Off);
  bool deleteDeadInstructions();
  PtrInfo visitPtr(Instruction &I) {
    assert(isa<AllocaInst>(I));
    OldAI = cast<AllocaInst>(&I);
    auto Ret = Base::visitPtr(I);
    deleteDeadInstructions();
    return Ret;
  }

  void visitPtrToIntInst(PtrToIntInst &I) {
    enqueueUsers(I);
    markAsDead(I);
  }
  void visitIntToPtrInst(IntToPtrInst &I) {
    enqueueUsers(I);
    markAsDead(I);
  }
  void visitBinaryOperator(BinaryOperator &I);

  void visitInstruction(Instruction &I) {
    llvm::errs() << "No rewriting rule for this inst: " << I << "\n";
    assert(false && "No rewriting rule for this inst");
  }
};

} // namespace notdec

#endif
