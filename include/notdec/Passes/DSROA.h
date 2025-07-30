#ifndef _NOTDEC_PASSES_DSROA_H_
#define _NOTDEC_PASSES_DSROA_H_

#include <llvm/ADT/SetVector.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>

namespace notdec {

namespace dsroa {

class Partition;
class AllocaSlices;
class AllocaSliceRewriter;

}; // namespace dsroa

// Decompiler SROA
struct DSROAPass : llvm::PassInfoMixin<DSROAPass> {
  llvm::LLVMContext *C = nullptr;
  llvm::DominatorTree *DT = nullptr;
  llvm::AssumptionCache *AC = nullptr;

  /// Worklist of alloca instructions to simplify.
  ///
  /// Each alloca in the function is added to this. Each new alloca formed gets
  /// added to it as well to recursively simplify unless that alloca can be
  /// directly promoted. Finally, each time we rewrite a use of an alloca other
  /// the one being actively rewritten, we add it back onto the list if not
  /// already present to ensure it is re-visited.
  llvm::SetVector<llvm::AllocaInst *, llvm::SmallVector<llvm::AllocaInst *, 16>> Worklist;

  /// A collection of instructions to delete.
  /// We try to batch deletions to simplify code and make things a bit more
  /// efficient. We also make sure there is no dangling pointers.
  llvm::SmallVector<llvm::WeakVH, 8> DeadInsts;

  /// Post-promotion worklist.
  ///
  /// Sometimes we discover an alloca which has a high probability of becoming
  /// viable for SROA after a round of promotion takes place. In those cases,
  /// the alloca is enqueued here for re-processing.
  ///
  /// Note that we have to be very careful to clear allocas out of this list in
  /// the event they are deleted.
  llvm::SetVector<llvm::AllocaInst *, llvm::SmallVector<llvm::AllocaInst *, 16>>
      PostPromotionWorklist;

  /// A collection of alloca instructions we can directly promote.
  std::vector<llvm::AllocaInst *> PromotableAllocas;

  /// A worklist of PHIs to speculate prior to promoting allocas.
  ///
  /// All of these PHIs have been checked for the safety of speculation and by
  /// being speculated will allow promoting allocas currently in the promotable
  /// queue.
  llvm::SetVector<llvm::PHINode *, llvm::SmallVector<llvm::PHINode *, 2>>
      SpeculatablePHIs;

  /// A worklist of select instructions to speculate prior to promoting
  /// allocas.
  ///
  /// All of these select instructions have been checked for the safety of
  /// speculation and by being speculated will allow promoting allocas
  /// currently in the promotable queue.
  llvm::SetVector<llvm::SelectInst *, llvm::SmallVector<llvm::SelectInst *, 2>>
      SpeculatableSelects;

  long NumPromoted = 0;
  long NumDeleted = 0;
  long NumAllocasAnalyzed = 0;
  long NumAllocaPartitions = 0;
  long NumNewAllocas = 0;
  long NumAllocaPartitionUses = 0;
  unsigned int MaxPartitionsPerAlloca = 0;
  unsigned MaxUsesPerAllocaPartition = 0;

public:
  DSROAPass() = default;

  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

private:
  friend class dsroa::AllocaSliceRewriter;

  bool presplitLoadsAndStores(llvm::AllocaInst &AI, dsroa::AllocaSlices &AS);
  llvm::AllocaInst *rewritePartition(llvm::AllocaInst &AI,
                                     dsroa::AllocaSlices &AS,
                                     dsroa::Partition &P);
  bool splitAlloca(llvm::AllocaInst &AI, dsroa::AllocaSlices &AS);
  bool runOnAlloca(llvm::AllocaInst &AI);
  void clobberUse(llvm::Use &U);
  bool deleteDeadInstructions(
      llvm::SmallPtrSetImpl<llvm::AllocaInst *> &DeletedAllocas);
  bool promoteAllocas(llvm::Function &F);
};

} // namespace notdec

#endif
