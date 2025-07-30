#ifndef _NOTDEC_PASSES_REORDER_BB_H_
#define _NOTDEC_PASSES_REORDER_BB_H_

#include <llvm/IR/Dominators.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

namespace notdec::passes {
struct ReorderBlocksPass : public llvm::PassInfoMixin<ReorderBlocksPass> {
  // 收集支配顺序的基本块列表（入口块优先）
  llvm::SmallVector<llvm::BasicBlock *, 32> getDomOrder(llvm::Function &F);

  void traverseDomTree(llvm::DomTreeNode *node,
                       llvm::SmallVectorImpl<llvm::BasicBlock *> &blocks,
                       llvm::SmallPtrSetImpl<llvm::BasicBlock *> &visited);

  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};
} // namespace notdec::passes

#endif
