#include "notdec/Passes/ReorderBasicblock.h"

#include "llvm/Pass.h"
#include "llvm/IR/CFG.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace notdec::passes {
SmallVector<BasicBlock *, 32> ReorderBlocksPass::getDomOrder(Function &F) {
  DominatorTree DT;
  DT.recalculate(F);

  SmallVector<BasicBlock *, 32> blocks;
  SmallPtrSet<BasicBlock *, 32> visited;

  // 保留入口块作为第一个元素
  BasicBlock *entry = &F.getEntryBlock();
  blocks.push_back(entry);
  visited.insert(entry);

  // 前序遍历支配树（从entry开始）
  for (DomTreeNode *child : DT.getRootNode()->children()) {
    traverseDomTree(child, blocks, visited);
  }

  // 添加不可达块
  for (BasicBlock &BB : F) {
    if (!visited.contains(&BB)) {
      blocks.push_back(&BB);
    }
  }
  return blocks;
}

void ReorderBlocksPass::traverseDomTree(DomTreeNode *node, SmallVectorImpl<BasicBlock *> &blocks,
                     SmallPtrSetImpl<BasicBlock *> &visited) {
  BasicBlock *bb = node->getBlock();
  if (!visited.insert(bb).second)
    return;

  blocks.push_back(bb);
  for (DomTreeNode *child : node->children()) {
    traverseDomTree(child, blocks, visited);
  }
}

PreservedAnalyses ReorderBlocksPass::run(Function &F, FunctionAnalysisManager &AM) {
  // 获取新的块顺序（入口块保持第一个）
  SmallVector<BasicBlock *, 32> newOrder = getDomOrder(F);

  // 重新排列基本块顺序
  BasicBlock *prev = nullptr;
  for (BasicBlock *bb : newOrder) {
    if (prev) {
      bb->moveAfter(prev);
    } else {
      // 确保entry保持在第一个位置
      assert(bb == &F.getEntryBlock() && "First block not entry");
    }
    prev = bb;
  }

  return PreservedAnalyses::none();
}
}
