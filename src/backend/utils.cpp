
#include "backend/utils.h"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <iostream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Scalar/Reg2Mem.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/Local.h>
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
  MPM.addPass(createModuleToFunctionPassAdaptor(DemotePhiPass()));
  MPM.addPass(createModuleToFunctionPassAdaptor(AdjustCFGPass()));
  MPM.run(M, MAM);
}

// ===============
// Pass
// ===============

llvm::PreservedAnalyses DemotePhiPass::run(llvm::Function &F,
                                           llvm::FunctionAnalysisManager &) {
  using namespace llvm;
  // Insert all new allocas into entry block.
  BasicBlock *BBEntry = &F.getEntryBlock();
  assert(pred_empty(BBEntry) &&
         "Entry block to function must not have predecessors!");

  // Find first non-alloca instruction and create insertion point. This is
  // safe if block is well-formed: it always have terminator, otherwise
  // we'll get and assertion.
  BasicBlock::iterator I = BBEntry->begin();
  while (isa<AllocaInst>(I))
    ++I;

  CastInst *AllocaInsertionPoint = new BitCastInst(
      Constant::getNullValue(Type::getInt32Ty(F.getContext())),
      Type::getInt32Ty(F.getContext()), "reg2mem alloca point", &*I);
  std::list<Instruction *> WorkList;
  // Find all phi's
  for (BasicBlock &BB : F)
    for (auto &Phi : BB.phis())
      WorkList.push_front(&Phi);

  // Demote phi nodes
  for (Instruction *I : WorkList)
    DemotePHIToStack(cast<PHINode>(I), AllocaInsertionPoint);
  return llvm::PreservedAnalyses::none();
}

// https://github.com/llvm/llvm-project/blob/7cf1fef45f13991e2d3b97e0612cfb88bf906a50/llvm/examples/IRTransforms/SimplifyCFG.cpp#L63
static bool removeDeadBlocks_v1(llvm::Function &F) {
  using namespace llvm;
  bool Changed = false;

  // Remove trivially dead blocks.
  for (BasicBlock &BB : make_early_inc_range(F)) {
    // Skip blocks we know to not be trivially dead. We know a block is
    // guaranteed to be dead, iff it is neither the entry block nor
    // has any predecessors.
    if (&F.getEntryBlock() == &BB || !pred_empty(&BB))
      continue;

    // Notify successors of BB that BB is going to be removed. This removes
    // incoming values from BB from PHIs in the successors. Note that this will
    // not actually remove BB from the predecessor lists of its successors.
    for (BasicBlock *Succ : successors(&BB))
      Succ->removePredecessor(&BB);
    // TODO: Find a better place to put such small variations.
    // Alternatively, we can update the PHI nodes manually:
    // for (PHINode &PN : make_early_inc_range(Succ->phis()))
    //  PN.removeIncomingValue(&BB);

    // Replace all instructions in BB with an undef constant. The block is
    // unreachable, so the results of the instructions should never get used.
    while (!BB.empty()) {
      Instruction &I = BB.back();
      I.replaceAllUsesWith(UndefValue::get(I.getType()));
      I.eraseFromParent();
    }

    // Finally remove the basic block.
    BB.eraseFromParent();
    Changed = true;
  }

  return Changed;
}

static bool mergeIntoSinglePredecessor_v1(llvm::Function &F) {
  using namespace llvm;
  bool Changed = false;

  // Merge blocks with single predecessors.
  for (BasicBlock &BB : make_early_inc_range(F)) {
    BasicBlock *Pred = BB.getSinglePredecessor();
    // Make sure  BB has a single predecessor Pred and BB is the single
    // successor of Pred.
    if (!Pred || Pred->getSingleSuccessor() != &BB)
      continue;

    // Do not try to merge self loops. That can happen in dead blocks.
    if (Pred == &BB)
      continue;

    // Need to replace it before nuking the branch.
    BB.replaceAllUsesWith(Pred);
    // PHI nodes in BB can only have a single incoming value. Remove them.
    for (PHINode &PN : make_early_inc_range(BB.phis())) {
      PN.replaceAllUsesWith(PN.getIncomingValue(0));
      PN.eraseFromParent();
    }
    // Move all instructions from BB to Pred.
    for (Instruction &I : make_early_inc_range(BB))
      I.moveBefore(Pred->getTerminator());

    // Remove the Pred's terminator (which jumped to BB). BB's terminator
    // will become Pred's terminator.
    Pred->getTerminator()->eraseFromParent();
    BB.eraseFromParent();

    Changed = true;
  }

  return Changed;
}

static bool eliminateEmptyBr(llvm::Function &F) {
  using namespace llvm;
  bool Changed = false;
  // Remove blocks with single successors.
  for (BasicBlock &BB : make_early_inc_range(F)) {
    BasicBlock *Succ = BB.getSingleSuccessor();
    // Make sure BB has a single successor Succ and BB is empty
    if (!Succ || BB.size() > 1)
      continue;
    assert(isa<BranchInst>(BB.front()));
    assert(cast<BranchInst>(BB.front()).isUnconditional());

    for (BasicBlock *Pred : make_early_inc_range(predecessors(&BB))) {
      Pred->getTerminator()->replaceSuccessorWith(&BB, Succ);
    }

    BB.eraseFromParent();
    Changed = true;
  }

  return Changed;
}

llvm::PreservedAnalyses AdjustCFGPass::run(llvm::Function &F,
                                           llvm::FunctionAnalysisManager &) {
  bool Changed = false;
  Changed |= removeDeadBlocks_v1(F);
  Changed |= mergeIntoSinglePredecessor_v1(F);
  Changed |= eliminateEmptyBr(F);
  return llvm::PreservedAnalyses::none();
}

static llvm::Value *matchReturn(llvm::BasicBlock &BB) {
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
      if (pred->getSingleSuccessor() == B) {
        auto br = pred->getTerminator();
        builder.SetInsertPoint(br);
        if (auto *p = llvm::dyn_cast<llvm::PHINode>(r)) {
          auto rv = p->getIncomingValueForBlock(pred);
          builder.CreateRet(rv);
        } else {
          builder.CreateRet(r);
        }
        B->removePredecessor(pred, true);
        br->eraseFromParent();
      } else {
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
