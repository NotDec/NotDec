// Phoenix: Native x86 Decompilation Using Semantics-Preserving Structural
// Analysis and Iterative Control-Flow Structuring

#include <cassert>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <vector>

#include "backend/CFG.h"
#include "backend/PostOrderCFGView.h"
#include "backend/phoenix.h"

namespace notdec::backend {

void Phoenix::execute() {
  int iterations = 0;
  do {
    if (isCanceled) {
      break;
    }
    ++iterations;
    if (iterations > 1000) {
      llvm::errs() << "Structure analysis stopped making progress, quitting."
                   << " Func: " << FCtx.getFunction().getName();
      break;
    }

    int oldCount = CFG.size();
    // this will maintain a list, so modification during iteration is OK
    auto postView = PostOrderCFGView::create(&CFG);
    Dom.buildDominatorTree(&CFG);
    Dom.dump();

    for (const CFGBlock *CBlock : *postView) {
      CFGBlock *Block = const_cast<CFGBlock *>(CBlock);
      bool Changed = false;
      do {
        Changed |= ReduceAcyclic(Block);
        if (!Changed && IsCyclic(Block)) {
          Changed |= ReduceCyclic(Block);
        }
      } while (Changed);

      if (CFG.size() == oldCount && CFG.size() > 1) {
        // Didn't make any progress this round,
        // try refining unstructured regions
        ProcessUnresolvedRegions();
      }
    }

  } while (CFG.size() > 1);
}

bool Phoenix::IsCyclic(CFGBlock *Block) {}
bool Phoenix::ReduceCyclic(CFGBlock *Block) {}
void Phoenix::ProcessUnresolvedRegions() {}

bool Phoenix::ReduceAcyclic(CFGBlock *Block) {
  switch (Block->succ_size()) {
  case 0:
    return false;
    break;
  case 1:
    // reduce sequence
    {
      CFGBlock *Succ = *Block->succ_begin();
      if (Succ->pred_size() == 1) {
        // merge two blocks
        // 1 add all instructions in succ to current block
        for (auto elem = Succ->begin(); elem != Succ->end(); ++elem) {
          Block->appendElement(*elem);
        }
        Block->setTerminator(Succ->getTerminator());
        // remove this edge
        removeEdge(Block, Succ);
        // add all edges from succ to current block
        replaceSuccessors(Block, Succ);
        // TODO faster remove instead of linear search.
        CFG.remove(Succ);
        return true;
      }
    }
    break;
  case 2:
    // reduce if-else
    {
      auto tmp = Block->succ_begin();
      CFGBlock *th = (*tmp).getBlock();
      ++tmp;
      CFGBlock *el = (*tmp).getBlock();
      assert(th != nullptr && el != nullptr);
      CFGBlock *elS = linearSuccessor(el);
      CFGBlock *thS = linearSuccessor(th);

      // the successor of else is then block.
      if (elS == th) {
        if (!onlyPred(el, Block)) {
          break;
        }
        clang::Expr *cond = takeBinaryCond(*Block);
        // collapse the else block: if(!cond){else} elseSucc/then
        cond = invertCond(cond);
        auto then = makeCompoundStmt(el);
        auto IfStmt = clang::IfStmt::Create(
            FCtx.getASTContext(), clang::SourceLocation(),
            clang::IfStatementKind::Ordinary, nullptr, nullptr, cond,
            clang::SourceLocation(), clang::SourceLocation(), then,
            clang::SourceLocation());
        Block->appendStmt(IfStmt);
        // maintain the edge
        removeEdge(Block, el);
        removeEdge(el, elS);
        CFG.remove(el);
        assert(Block->succ_size() == 1);
        return true;
      } else if (thS == el) {
        if (!onlyPred(th, Block)) {
          break;
        }
        clang::Expr *cond = takeBinaryCond(*Block);
        // collapse the then block: if(cond){then} thenSucc/else;
        auto then = makeCompoundStmt(th);
        auto IfStmt = clang::IfStmt::Create(
            FCtx.getASTContext(), clang::SourceLocation(),
            clang::IfStatementKind::Ordinary, nullptr, nullptr, cond,
            clang::SourceLocation(), clang::SourceLocation(), then,
            clang::SourceLocation());
        Block->appendStmt(IfStmt);
        // maintain the edge
        removeEdge(Block, th);
        removeEdge(th, thS);
        CFG.remove(th);
        assert(Block->succ_size() == 1);
        return true;
      } else if (elS != nullptr && elS == thS) {
        if (!(onlyPred(el, Block) && onlyPred(th, Block))) {
          break;
        }
        clang::Expr *cond = takeBinaryCond(*Block);
        // collapse the then and else block: if(cond){thenBlk}else{elseBlk}
        // thenSucc/elseSucc;
        auto then = makeCompoundStmt(th);
        auto els = makeCompoundStmt(el);
        auto IfStmt = clang::IfStmt::Create(
            FCtx.getASTContext(), clang::SourceLocation(),
            clang::IfStatementKind::Ordinary, nullptr, nullptr, cond,
            clang::SourceLocation(), clang::SourceLocation(), then,
            clang::SourceLocation(), els);
        Block->appendStmt(IfStmt);
        // maintain the edge
        removeEdge(Block, th);
        removeEdge(Block, el);
        removeEdge(th, thS);
        removeEdge(el, elS);
        CFG.remove(th);
        CFG.remove(el);
        Block->addSuccessor(elS);
        assert(Block->succ_size() == 1);
        return true;
      }
    }
    break;
  case 3:
    assert(false && "Not implemented");
    break;
  }
  // unreachable
  assert(false && "unreachable");
}

} // namespace notdec::backend
