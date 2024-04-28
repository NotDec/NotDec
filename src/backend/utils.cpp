
#include "backend/utils.h"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <iostream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Scalar/Reg2Mem.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <type_traits>
#include <vector>

#define DEBUG_TYPE "notdec-backend-utils"

namespace notdec::backend {

/// Run the RegToMemPass to demote SSA to memory, i.e., eliminate Phi nodes.
void demoteSSA(llvm::Module &M) {
  using namespace llvm;
  // Create the analysis managers.
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  ModulePassManager MPM;
  PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  SimplifyCFGOptions Opts;
  MPM.addPass(createModuleToFunctionPassAdaptor(SimplifyCFGPass(Opts)));
  MPM.addPass(createModuleToFunctionPassAdaptor(RetDupPass()));
  // MPM.addPass(createModuleToFunctionPassAdaptor(RegToMemPass()));
  MPM.run(M, MAM);

  llvm::errs() << M;
}

// ===============
// Pass
// ===============

llvm::Value *matchReturn(llvm::BasicBlock &BB) {
  auto it = BB.begin();
  if (auto *r = llvm::dyn_cast<llvm::ReturnInst>(&*it)) {
    return r->getReturnValue();
  }
  if (auto *p = llvm::dyn_cast<llvm::PHINode>(&*it)) {
    it++;
    if (auto *r = llvm::dyn_cast<llvm::ReturnInst>(&*it)) {
      if (p->hasOneUse() && r->getReturnValue() == p) {
        return p;
      }
    }
  }
  return nullptr;
}

llvm::PreservedAnalyses RetDupPass::run(llvm::Function &F,
                                        llvm::FunctionAnalysisManager &) {
  using namespace llvm;
  auto &C = F.getContext();
  IRBuilder<> builder(C);
  std::vector<llvm::BasicBlock *> BBS;
  for (auto &B : F) {
    BBS.push_back(&B);
  }
  for (auto B : BBS) {
    // match the block with only phi and return
    auto r = matchReturn(*B);
    if (r == nullptr) {
      continue;
    }
    std::vector<llvm::BasicBlock *> preds(pred_begin(B), pred_end(B));
    for (auto pred : preds) {
      BasicBlock *N = BasicBlock::Create(C, B->getName(), &F, B);
      builder.SetInsertPoint(N);
      if (auto *p = llvm::dyn_cast<llvm::PHINode>(r)) {
        auto rv = p->getIncomingValueForBlock(pred);
        builder.CreateRet(rv);
      } else {
        builder.CreateRet(r);
      }
      // update the pred
      B->removePredecessor(pred, true);
      pred->getTerminator()->replaceSuccessorWith(B, N);
    }
    B->eraseFromParent();
  }
  return PreservedAnalyses::none();
}

// ===============
// Precedence
// ===============

PrecedenceLevel getPrecedence(clang::Expr *E) {
  // Unary
  if (auto U = llvm::dyn_cast<clang::UnaryOperator>(E)) {
    return getOperatorPrecedence(U->getOpcode());
  } else if (llvm::isa<clang::CallExpr>(E)) {
    return SuffixUnary;
  } else if (llvm::isa<clang::ArraySubscriptExpr>(E)) {
    return SuffixUnary;
  } else if (llvm::isa<clang::MemberExpr>(E)) {
    return SuffixUnary;
  } else if (llvm::isa<clang::CompoundLiteralExpr>(E)) {
    return SuffixUnary;
  } else if (llvm::isa<clang::CastExpr>(E)) {
    return PrefixUnary;
  } else if (llvm::isa<clang::UnaryExprOrTypeTraitExpr>(E)) {
    return PrefixUnary;
  }
  // binary operator
  if (auto B = llvm::dyn_cast<clang::BinaryOperator>(E)) {
    return getOperatorPrecedence(B->getOpcode());
  }
  // ternary operator
  if (llvm::isa<clang::ConditionalOperator>(E) ||
      llvm::isa<clang::BinaryConditionalOperator>(E)) {
    return Conditional;
  }
  return Unknown;
}

bool isRightAssociative(PrecedenceLevel PL) {
  return PL == Conditional || PL == Assignment || PL == PrefixUnary;
}

bool isLeftAssociative(PrecedenceLevel PL) { return !isRightAssociative(PL); }

bool isUnary(PrecedenceLevel PL) {
  return PL == PrefixUnary || PL == SuffixUnary;
}

bool needParen(PrecedenceLevel PParent, PrecedenceLevel PChild, bool isLeft) {
  if (PParent == Unknown || PChild == Unknown) {
    // LLVM_DEBUG(llvm::dbgs() << "Unknown PrecedenceLevel found!\n");
    return false;
  }
  // if parent is higher precedence, then need paren
  if (PParent > PChild) {
    return true;
  }
  // if parent is same precedence, then need paren if associative equals
  // current location
  if (!isUnary(PParent) && PParent == PChild) {
    return isLeft ? isRightAssociative(PParent) : isLeftAssociative(PParent);
  }
  return false;
}

} // namespace notdec::backend
