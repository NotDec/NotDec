#ifndef _NOTDEC_PASSES_DSROA_H_
#define _NOTDEC_PASSES_DSROA_H_

#include <llvm/ADT/SetVector.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>

namespace notdec {

using namespace llvm;

namespace dsroa {

class Partition;
class AllocaSlices;
class AllocaSliceRewriter;

}; // namespace dsroa

// Decompiler SROA
struct DSROAPass : PassInfoMixin<DSROAPass> {
  LLVMContext *C = nullptr;
  DominatorTree *DT = nullptr;
  AssumptionCache *AC = nullptr;

  /// Worklist of alloca instructions to simplify.
  ///
  /// Each alloca in the function is added to this. Each new alloca formed gets
  /// added to it as well to recursively simplify unless that alloca can be
  /// directly promoted. Finally, each time we rewrite a use of an alloca other
  /// the one being actively rewritten, we add it back onto the list if not
  /// already present to ensure it is re-visited.
  SetVector<AllocaInst *, SmallVector<AllocaInst *, 16>> Worklist;

  /// A collection of instructions to delete.
  /// We try to batch deletions to simplify code and make things a bit more
  /// efficient. We also make sure there is no dangling pointers.
  SmallVector<WeakVH, 8> DeadInsts;

  /// Post-promotion worklist.
  ///
  /// Sometimes we discover an alloca which has a high probability of becoming
  /// viable for SROA after a round of promotion takes place. In those cases,
  /// the alloca is enqueued here for re-processing.
  ///
  /// Note that we have to be very careful to clear allocas out of this list in
  /// the event they are deleted.
  SetVector<AllocaInst *, SmallVector<AllocaInst *, 16>> PostPromotionWorklist;

  /// A collection of alloca instructions we can directly promote.
  std::vector<AllocaInst *> PromotableAllocas;

  /// A worklist of PHIs to speculate prior to promoting allocas.
  ///
  /// All of these PHIs have been checked for the safety of speculation and by
  /// being speculated will allow promoting allocas currently in the promotable
  /// queue.
  SetVector<PHINode *, SmallVector<PHINode *, 2>> SpeculatablePHIs;

  /// A worklist of select instructions to speculate prior to promoting
  /// allocas.
  ///
  /// All of these select instructions have been checked for the safety of
  /// speculation and by being speculated will allow promoting allocas
  /// currently in the promotable queue.
  SetVector<SelectInst *, SmallVector<SelectInst *, 2>> SpeculatableSelects;

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

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

private:
  friend class dsroa::AllocaSliceRewriter;

  bool presplitLoadsAndStores(AllocaInst &AI, dsroa::AllocaSlices &AS);
  AllocaInst *rewritePartition(AllocaInst &AI, dsroa::AllocaSlices &AS,
                               dsroa::Partition &P);
  bool splitAlloca(AllocaInst &AI, dsroa::AllocaSlices &AS);
  bool runOnAlloca(AllocaInst &AI);
  void clobberUse(Use &U);
  bool deleteDeadInstructions(SmallPtrSetImpl<AllocaInst *> &DeletedAllocas);
  bool promoteAllocas(Function &F);
};

} // namespace notdec

#endif
