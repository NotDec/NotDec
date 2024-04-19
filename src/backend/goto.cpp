#include "backend/goto.h"
#include "backend/CFG.h"
#include "backend/utils.h"
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Analysis/CFG.h>
#include <clang/Basic/Specifiers.h>
#include <cstdlib>
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
  CFG &CFG = ctx.getCFG();
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
      auto label = getBlockLabel(Succ.getBlock());
      auto Goto = createGotoStmt(label);
      Current->appendStmt(Goto);
    } else if (succ_size == 2) {
      auto tmp = Current->succ_begin();
      auto b1 = (*tmp).getBlock();
      ++tmp;
      auto b2 = (*tmp).getBlock();
      bool invert = false;

      // Make b1 the fallthrough block
      auto nextBlock = &(*next(it));
      // if false block == fall through, then can eliminate the goto
      if (nextBlock == ctx.getBlock(*b1)) {
        std::swap(b1, b2);
        invert = true;
      }
      clang::Expr *cond = llvm::cast<clang::Expr>(Current->getTerminatorStmt());
      Current->setTerminator(nullptr);
      if (invert) {
        cond = createUnaryOperator(ASTCtx, cond, clang::UO_LNot, ASTCtx.BoolTy,
                                   clang::VK_PRValue);
      }
      auto GotoB1 = createGotoStmt(getBlockLabel(b1));
      auto IfGotoB1 = clang::IfStmt::Create(
          ASTCtx, clang::SourceLocation(), clang::IfStatementKind::Ordinary,
          nullptr, nullptr, cond, clang::SourceLocation(),
          clang::SourceLocation(), GotoB1, clang::SourceLocation());
      Current->appendStmt(IfGotoB1);
      auto GotoB2 = createGotoStmt(getBlockLabel(b2));
      Current->appendStmt(GotoB2);
    } else if (succ_size > 2) {
      // TODO handle switch.
      std::abort();
    }
  }
  // merge all blocks into one. Remove all other blocks and edges.
  CFGBlock &Entry = CFG.getEntry();
  // because we only need one block, instead of maintaining all edges, we remove
  // all succs and preds of entry block, and ignore other blocks' edges.
  Entry.succ_clear();
  Entry.pred_clear();
  for (auto it = bbs.begin(); it != bbs.end(); ++it) {
    // push all statements into entry block
    auto Current = ctx.getBlock(*it);
    if (&*Current == &Entry) {
      continue;
    }
    // add if there is a label statement
    if (Current->getLabel() != nullptr) {
      Entry.appendStmt(Current->getLabel());
    }

    for (auto elem = Current->begin(); elem != Current->end(); ++elem) {
      Entry.appendElement(*elem);
    }
    if (Current->getTerminatorStmt() != nullptr) {
      Entry.appendStmt(Current->getTerminatorStmt());
    }
    // remove the block from the cfg
    CFG.erase(Current);
  }
  simplifyBlock(Entry);
  // The exit block is set to the first stub block created. We currently do not
  // care about it.
}

clang::Stmt *getStmt(CFGElement e) {
  if (auto stmt = e.getAs<CFGStmt>()) {
    return const_cast<clang::Stmt *>(stmt->getStmt());
  }
  return nullptr;
}

// remote all NullStmt and merge GotoStmt with LabelStmt
void Goto::simplifyBlock(CFGBlock &Block) {
  // remove two adjacent goto + label
  for (auto it = Block.begin(); it != Block.end();) {
    if (auto stmt = getStmt(*it)) {
      if (auto gotoStmt = llvm::dyn_cast<clang::GotoStmt>(stmt)) {
        auto next = std::next(it);
        if (auto label =
                llvm::dyn_cast_or_null<clang::LabelStmt>(getStmt(*next))) {
          if (label->getDecl() == gotoStmt->getLabel()) {
            llvm::errs() << "Removing redundant goto: ", stmt->dump();
            it = Block.erase(it);
            continue;
          }
        }
      }
    }
    // increment the iterator if not continue
    ++it;
  }

  // replace adjacent label with the last label
  for (auto it = Block.begin(); it != Block.end();) {
    if (auto stmt = getStmt(*it)) {
      // for each label stmt
      if ((llvm::isa<clang::LabelStmt>(stmt)) && std::next(it) != Block.end()) {
        clang::LabelStmt *label = llvm::cast<clang::LabelStmt>(stmt);
        if (auto nextLabel =
                llvm::dyn_cast<clang::LabelStmt>(getStmt(*std::next(it)))) {
          llvm::errs() << "Merge adjacent label: " << label->getName()
                       << " and " << nextLabel->getName() << "\n";

          assert(llvm::isa<clang::NullStmt>(label->getSubStmt()));
          assert(llvm::isa<clang::NullStmt>(nextLabel->getSubStmt()));
          replaceAllUsesWith(label->getDecl(), nextLabel->getDecl());
          it = Block.erase(it);
          continue;
        }
      }
    }
    // increment the iterator if not continue
    ++it;
  }

  // fold label with NullStmt and remove NullStmt
  for (auto it = Block.begin(); it != Block.end();) {
    if (auto stmt = getStmt(*it)) {
      if (auto label = llvm::dyn_cast<clang::LabelStmt>(stmt)) {
        if (llvm::isa<clang::NullStmt>(label->getSubStmt()) &&
            std::next(it) != Block.end()) {
          if (auto next = getStmt(*std::next(it))) {
            llvm::errs() << "Folding LabelStmt " << label->getName() << "\n";
            label->setSubStmt(next);
            Block.erase(std::next(it));
          }
        }
      }

      // remove NullStmt
      if (auto term = llvm::dyn_cast<clang::NullStmt>(stmt)) {
        llvm::errs() << "Removing NullStmt\n";
        it = Block.erase(it);
        continue;
      }
    }
    // increment the iterator if not continue
    ++it;
  }
}

} // namespace notdec::backend
