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

struct NewAllocas {
  SimpleRange R;
  llvm::AllocaInst *NewAI;
};

struct StackBreaker {
  bool runOnAlloca(llvm::AllocaInst &AI, SCCTypeResult &HT,
                   std::vector<std::pair<SimpleRange, std::string>> *Vec);
};

class StackBreakerRewriter : public llvm::PtrUseVisitor<StackBreakerRewriter> {
  friend class PtrUseVisitor<StackBreakerRewriter>;
  friend class InstVisitor<StackBreakerRewriter>;
  using Base = PtrUseVisitor<StackBreakerRewriter>;

  llvm::AllocaInst *OldAI;
  const std::vector<NewAllocas> &NAs;
  /// Set to de-duplicate dead instructions found in the use walk.
  llvm::SmallPtrSet<llvm::Instruction *, 4> VisitedDeadInsts;
  llvm::SmallVector<llvm::WeakVH, 8> DeadInsts;
  void markAsDead(llvm::Instruction &I) {
    if (VisitedDeadInsts.insert(&I).second)
      DeadInsts.push_back(&I);
  }

public:
  StackBreakerRewriter(const llvm::DataLayout &DL,
                       const std::vector<NewAllocas> &NAs)
      : PtrUseVisitor<StackBreakerRewriter>(DL), NAs(NAs) {}

  const NewAllocas *getNA(OffsetTy Off);
  bool deleteDeadInstructions();
  PtrInfo visitPtr(llvm::Instruction &I) {
    assert(llvm::isa<llvm::AllocaInst>(I));
    OldAI = llvm::cast<llvm::AllocaInst>(&I);
    auto Ret = Base::visitPtr(I);
    deleteDeadInstructions();
    return Ret;
  }

  void visitPtrToIntInst(llvm::PtrToIntInst &I) {
    enqueueUsers(I);
    markAsDead(I);
  }
  void visitIntToPtrInst(llvm::IntToPtrInst &I) {
    enqueueUsers(I);
    markAsDead(I);
  }
  void visitBinaryOperator(llvm::BinaryOperator &I);

  void visitInstruction(llvm::Instruction &I) {
    llvm::errs() << "No rewriting rule for this inst: " << I << "\n";
    assert(false && "No rewriting rule for this inst");
  }
};

} // namespace notdec

#endif
