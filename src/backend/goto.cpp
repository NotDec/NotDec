#include "backend/goto.h"
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Analysis/CFG.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/Support/Casting.h>
#include <type_traits>

namespace notdec::backend {

template <typename ForwardIt>
ForwardIt
next(ForwardIt it,
     typename std::iterator_traits<ForwardIt>::difference_type n = 1) {
  std::advance(it, n);
  return it;
}

// 1. Look at the CFG edges, instead of the basic block terminator edges.
void Goto::execute() {
  auto &ASTCtx = ctx.getASTContext();
  clang::CFG &CFG = ctx.getCFG();
  // for each block, insert goto statement to represent outgoing edges.
  auto &bbs = ctx.getFunction().getBasicBlockList();
  for (auto it = bbs.begin(); it != bbs.end(); ++it) {
    auto Current = ctx.getBlock(*it);
    auto succ_size = Current->succ_size();
    // for unconditional branch
    if (succ_size == 1) {
      // if is the next block in the list
      // auto nextBlock = &(*next(it));
      // if (nextBlock == br->getSuccessor(0)) {
      // }
      auto Succ = *Current->succ_begin();
      auto label = getBlockLabel(Succ.getReachableBlock());
      auto Goto = new (ASTCtx) clang::GotoStmt(label, clang::SourceLocation(),
                                               clang::SourceLocation());
      Current->appendStmt(Goto, ctx.getCFG().getBumpVectorContext());
    } else if (succ_size == 2) {
      auto tmp = Current->succ_begin();
      auto b1 = (*tmp).getReachableBlock();
      ++tmp;
      auto b2 = (*tmp).getReachableBlock();
      bool invert = false;

      // Make b1 the fallthrough block
      auto nextBlock = &(*next(it));
      if (nextBlock == ctx.getBlock(*b2)) {
        std::swap(b1, b2);
        invert = true;
      }
      clang::Expr *cond = llvm::cast<clang::Expr>(Current->getTerminatorStmt());
      if (invert) {
        cond = clang::UnaryOperator::Create(
            ASTCtx, cond, clang::UO_LNot, ASTCtx.BoolTy, clang::VK_PRValue,
            clang::OK_Ordinary, clang::SourceLocation(), false,
            clang::FPOptionsOverride());
      }
      auto GotoB2 = new (ASTCtx) clang::GotoStmt(
          getBlockLabel(b2), clang::SourceLocation(), clang::SourceLocation());
      auto IfGotoB2 = clang::IfStmt::Create(
          ASTCtx, clang::SourceLocation(), clang::IfStatementKind::Ordinary,
          nullptr, nullptr, cond, clang::SourceLocation(),
          clang::SourceLocation(), GotoB2, clang::SourceLocation());
      Current->appendStmt(IfGotoB2, ctx.getCFG().getBumpVectorContext());
      auto GotoB1 = new (ASTCtx) clang::GotoStmt(
          getBlockLabel(b1), clang::SourceLocation(), clang::SourceLocation());
      Current->appendStmt(GotoB1, ctx.getCFG().getBumpVectorContext());
    } else if (succ_size > 2) {
      // TODO handle switch.
    }
  }
  // merge all blocks into one. Remove all other blocks and edges.
  clang::CFGBlock &Entry = CFG.getEntry();
  // because we only need one block, instead of maintaining all edges, we remove
  // all succs and preds of entry block, and ignore other blocks' edges.
  Entry.succ_clear();
  Entry.pred_clear();
  for (auto it = bbs.begin(); it != bbs.end(); ++it) {
    // push all statements into entry block
    auto Current = ctx.getBlock(*it);
    if (Current == &Entry) {
      continue;
    }
    // add if there is a label statement
    if (Current->getLabel() != nullptr) {
      Entry.appendStmt(Current->getLabel(), CFG.getBumpVectorContext());
    }
    CFG.dump(ASTCtx.getLangOpts(), true);
    // elements should be added in reverse order
    for (auto elem = Current->rbegin(); elem != Current->rend(); ++elem) {
      Entry.appendElement(*elem, CFG.getBumpVectorContext());
    }
    // remove the block from the cfg
    CFG.erase(Current);
  }
  // The exit block is set to the first stub block created. We currently do not
  // care about it.
}

} // namespace notdec::backend
