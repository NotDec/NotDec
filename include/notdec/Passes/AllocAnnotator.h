#ifndef _NOTDEC_PASSES_ALLOC_ANNOTATOR_H_
#define _NOTDEC_PASSES_ALLOC_ANNOTATOR_H_

#include <llvm/IR/Instruction.h>
#include <llvm/IR/PassManager.h>

namespace notdec {

extern const char *KIND_ALLOC_SIZE;

struct AllocAnnotator : llvm::PassInfoMixin<AllocAnnotator> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

private:
  // 添加大小元数据的辅助函数
  void addSizeMetadata(llvm::Instruction *I, uint64_t Size);
};

} // namespace notdec

#endif
