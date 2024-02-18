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
  clang::CFGBlock &entry = CFG.getEntry();
  CFG.blo;
  for (auto it = bbs.begin(); it != bbs.end(); ++it) {
    if ((*it).isEntryBlock()) {
      continue;
    }
  }
}

} // namespace notdec::backend
