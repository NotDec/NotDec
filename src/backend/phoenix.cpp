// Phoenix: Native x86 Decompilation Using Semantics-Preserving Structural
// Analysis and Iterative Control-Flow Structuring

#include <cassert>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceLocation.h>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <set>
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
    // Dom.dump();

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

bool Phoenix::IsCyclic(CFGBlock *Block) {
  for (auto pred : Block->preds()) {
    // 1 self loop
    if (pred.getBlock() == Block) {
      return true;
    }
    // 2 back edge: edge target dominates source
    if (Dom.properlyDominates(Block, pred.getBlock())) {
      return true;
    }
  }
  return false;
}

void backwardVisit(std::set<CFGBlock *> &visited, CFGBlock *Entry,
                   CFGBlock *Current) {
  assert(visited.insert(Current).second);
  if (Current == Entry) {
    return;
  }
  for (auto pred : Current->preds()) {
    auto P = pred.getBlock();
    if (visited.find(P) != visited.end()) {
      visited.insert(P);
      backwardVisit(visited, Entry, P);
    }
  }
}

/// Visits given node and recursively visits all its GRAY successors.
/// All the visited nodes are painted Black.
void forwardVisit(std::set<CFGBlock *> &visited, std::set<CFGBlock *> gray,
                  CFGBlock *Current) {
  assert(gray.count(Current) == 1);
  assert(visited.insert(Current).second);
  for (auto pred : Current->succs()) {
    auto P = pred.getBlock();
    if (visited.find(P) != visited.end() && gray.count(P) == 1) {
      forwardVisit(visited, gray, P);
    }
  }
}

std::unique_ptr<std::vector<CFGBlock *>> getLoopNodes(CFGBlock *Block,
                                                      CFGDomTree &Dom) {
  std::set<CFGBlock *> Gray;
  std::set<CFGBlock *> Black;
  std::unique_ptr<std::vector<CFGBlock *>> ret =
      std::make_unique<std::vector<CFGBlock *>>();
  for (auto pred : Block->preds()) {
    auto P = pred.getBlock();
    if (Dom.properlyDominates(Block, P)) {
      // backward visit from back edge node
      // because Block dom P, back visit from P will find Block eventually.
      backwardVisit(Gray, Block, P);
    } else if (P == Block) {
      // self loop
      ret->push_back(P);
    }
  }

  if (Gray.find(Block) != Gray.end()) {
    forwardVisit(Black, Gray, Block);
  }
  ret->insert(ret->end(), Black.begin(), Black.end());
  return ret;
}

bool Phoenix::ReduceCyclic(CFGBlock *Block) {
  bool changed = false;
  clang::ASTContext &Ctx = FCtx.getASTContext();
  std::unique_ptr<std::vector<CFGBlock *>> loopNodes = getLoopNodes(Block, Dom);
  while (true) {
    // here only reduce sequence, because ReduceAcyclic is called when succ size
    // is 1.
    if (Block->succ_size() != 1 || !ReduceAcyclic(Block)) {
      break;
    }
    changed = true;
  }
  // handle any self loop edges, and convert to while loop
  for (auto succ : Block->succs()) {
    auto S = succ.getBlock();
    if (S == Block) {
      clang::Stmt *whil;
      if (Block->succ_size() == 1) {
        // infinite loop
        // create while(true){stmts}
        auto body = makeCompoundStmt(Block, true);
        whil = createWhileTrue(body);
      } else {
        assert(Block->succ_size() == 2);
        // Do while loop
        auto body = makeCompoundStmt(Block, true);
        auto cond = takeBinaryCond(*Block);
        // invert if loop is the false branch
        if (S != *Block->succ_begin()) {
          cond = invertCond(cond);
        }
        // create do while stmt
        whil = new (Ctx)
            clang::DoStmt(body, cond, clang::SourceLocation(),
                          clang::SourceLocation(), clang::SourceLocation());
      }
      // organize edges: now it has only one successor.
      Block->appendStmt(whil);
      removeEdge(Block, Block);
      return true;
    }
  }
  // Should be condition. Switches should not match a cyclic pattern
  if (Block->succ_size() > 2) {
    return changed;
  }
  for (auto succ : Block->succs()) {
    auto S = succ.getBlock();
    // While loop: Block -> S -> Block, and S has no other succ or pred
    if (linearSuccessor(S) == Block && singlePredecessor(S) == Block) {
      auto cond = takeBinaryCond(*Block);
      // invert cond if the enter the
      if (S != *Block->succ_begin()) {
        cond = invertCond(cond);
      }
      clang::Stmt *whil;
      if (Block->size() == 0) {
        // S is the only body block
        auto body = makeCompoundStmt(S);
        whil = clang::WhileStmt::Create(
            Ctx, nullptr, cond, body, clang::SourceLocation(),
            clang::SourceLocation(), clang::SourceLocation());
      } else {
        // Wrap in a big while true, and use break to leave
        // while (true) {Block; if(cond) break; S;}
        std::vector<clang::Stmt *> stmts;
        // add all stmts in Block and clear.
        for (auto elem = Block->begin(); elem != Block->end(); ++elem) {
          if (auto stmt = elem->getAs<CFGStmt>()) {
            stmts.push_back(const_cast<clang::Stmt *>(stmt->getStmt()));
          }
        }
        Block->clear();
        // add a if-break stmt.
        auto if_break = clang::IfStmt::Create(
            Ctx, clang::SourceLocation(), clang::IfStatementKind::Ordinary,
            nullptr, nullptr, cond, clang::SourceLocation(),
            clang::SourceLocation(),
            new (Ctx) clang::BreakStmt(clang::SourceLocation()));
        stmts.push_back(if_break);
        // add stmts in the S block
        for (auto elem = S->begin(); elem != S->end(); ++elem) {
          if (auto stmt = elem->getAs<CFGStmt>()) {
            stmts.push_back(const_cast<clang::Stmt *>(stmt->getStmt()));
          }
        }
        // create the while true
        auto body = clang::CompoundStmt::Create(FCtx.getASTContext(), stmts,
                                                clang::SourceLocation(),
                                                clang::SourceLocation());
        whil = createWhileTrue(body);
      }
      // now the while is the only stmt in Block
      Block->appendStmt(whil);
      removeEdge(Block, S);
      removeEdge(S, Block);
      CFG.remove(S);
      return true;
    } // end Block -> S -> Block
  }
  // It's a cyclic region, but we are unable to collapse it.
  // Schedule it for refinement after the whole graph has been
  // traversed.
  // enqueueUnresolvedLoop(Block, loopNodes);
  // Do not refine cycle if there are unresolved switches
  if (unresolvedSwitches.size() == 0) {
    unresolvedCycles.emplace_back(Block, std::move(*loopNodes));
  }
  return changed;
}
bool Phoenix::ProcessUnresolvedRegions() {
  if (unresolvedCycles.size() > 0) {
    auto cycle = *unresolvedCycles.erase(unresolvedCycles.begin());
    if (refineLoop(cycle.first, cycle.second)) {
      return true;
    }
  }
  if (unresolvedSwitches.size() > 0) {
    auto switchHead = *unresolvedSwitches.erase(unresolvedSwitches.begin());
    refineIncSwitch(switchHead);
    return true;
  }
  auto postView = PostOrderCFGView::create(&CFG);
  for (const CFGBlock *CBlock : *postView) {
    CFGBlock *Block = const_cast<CFGBlock *>(CBlock);
    if (virtualizeReturn(Block)) {
      return true;
    }
  }
  for (const CFGBlock *CBlock : *postView) {
    CFGBlock *Block = const_cast<CFGBlock *>(CBlock);
    if (CoalesceTailRegion(Block, CFG.nodes())) {
      return true;
    }
  }
  for (const CFGBlock *CBlock : *postView) {
    CFGBlock *Block = const_cast<CFGBlock *>(CBlock);
    if (LastResort(Block)) {
      return true;
    }
  }
  return false;
}

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
    // TODO support switch
    assert(false && "Not implemented");
    break;
  }
  // unreachable
  assert(false && "unreachable");
}

} // namespace notdec::backend
