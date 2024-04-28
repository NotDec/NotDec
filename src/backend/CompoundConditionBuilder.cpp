#include "backend/CompoundConditionBuilder.h"
#include "backend/CFG.h"
#include "backend/PostOrderCFGView.h"
#include "backend/utils.h"
#include <clang/AST/OperationKinds.h>
#include <clang/Basic/Specifiers.h>

namespace notdec::backend {

void CompoundConditionBuilder::execute() {
  bool changed = false;
  do {
    changed = false;
    auto postView = PostOrderCFGView::create(&CFG);

    for (auto block : *postView) {
      if (block->succ_size() == 2) {
        changed |= maybeCoalesce(const_cast<CFGBlock *>(block));
      }
    }
  } while (changed);
}

/// returns if the block has only condition expr and only one pred
bool isSubordinate(CFGBlock *block) {
  return block->pred_size() == 1 && block->size() == 0 &&
         block->succ_size() == 2;
}

void CompoundConditionBuilder::rebuildGraph(CFGBlock *head, CFGBlock *redundant,
                                            CFGBlock *replacement,
                                            CFGBlock *common) {
  removeEdge(redundant, replacement);
  removeEdge(redundant, common);
  head->replaceSucc(redundant, replacement);
  replacement->addPred(head);
  CFG.remove(redundant);
}

bool CompoundConditionBuilder::maybeCoalesce(CFGBlock *block) {
  bool changed = false;
  auto ss = block->getTwoSuccs();
  auto BThen = ss.first;
  auto BElse = ss.second;
  if (isSubordinate(BThen)) {
    auto ss2 = BThen->getTwoSuccs();
    if (ss2.second == BElse) {
      // fold to X && Y
      changed = true;
      auto condX = takeBinaryCond(*block);
      auto condY = takeBinaryCond(*BThen);
      auto result = createBinaryOperator(Ctx, condX, condY, clang::BO_LAnd,
                                         condX->getType(), clang::VK_PRValue);
      block->setTerminator(result);
      rebuildGraph(block, BThen, ss2.first, ss2.second);
    } else if (ss2.first == BElse) {
      // fold to X && !Y
      changed = true;
      auto condX = takeBinaryCond(*block);
      auto condY = takeBinaryCond(*BThen);
      condY = invertCond(condY);
      auto result = createBinaryOperator(Ctx, condX, condY, clang::BO_LAnd,
                                         condX->getType(), clang::VK_PRValue);
      block->setTerminator(result);
      rebuildGraph(block, BThen, ss2.second, ss2.first);
    }
  } else if (isSubordinate(BElse)) {
    auto ss2 = BElse->getTwoSuccs();
    if (ss2.first == BThen) {
      // fold to X || Y
      changed = true;
      auto condX = takeBinaryCond(*block);
      auto condY = takeBinaryCond(*BElse);
      auto result = createBinaryOperator(Ctx, condX, condY, clang::BO_LOr,
                                         condX->getType(), clang::VK_PRValue);
      block->setTerminator(result);
      rebuildGraph(block, BElse, ss2.second, ss2.first);
    } else if (ss2.second == BThen) {
      // fold to X || !Y
      changed = true;
      auto condX = takeBinaryCond(*block);
      auto condY = takeBinaryCond(*BElse);
      condY = invertCond(condY);
      auto result = createBinaryOperator(Ctx, condX, condY, clang::BO_LOr,
                                         condX->getType(), clang::VK_PRValue);
      block->setTerminator(result);
      rebuildGraph(block, BElse, ss2.first, ss2.second);
    }
  }
  return changed;
}

} // namespace notdec::backend
