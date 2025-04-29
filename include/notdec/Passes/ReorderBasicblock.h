#ifndef _NOTDEC_PASSES_REORDER_BB_H_
#define _NOTDEC_PASSES_REORDER_BB_H_

#include <llvm/IR/Dominators.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

using namespace llvm;


namespace notdec::passes {
struct ReorderBlocksPass : public PassInfoMixin<ReorderBlocksPass> {
  // 收集支配顺序的基本块列表（入口块优先）
  SmallVector<BasicBlock *, 32> getDomOrder(Function &F);

  void traverseDomTree(DomTreeNode *node, SmallVectorImpl<BasicBlock *> &blocks,
                       SmallPtrSetImpl<BasicBlock *> &visited);

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};
} // namespace notdec::frontend::passes

#endif
