#ifndef _NOTDEC_PASSES_MEMOP_MATCHER_H_
#define _NOTDEC_PASSES_MEMOP_MATCHER_H_

#include <llvm/IR/Instruction.h>
#include <llvm/IR/PassManager.h>

namespace notdec {

struct MemsetMatcher : llvm::PassInfoMixin<MemsetMatcher> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);
};

struct MemcpyMatcher : llvm::PassInfoMixin<MemcpyMatcher> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);
};

} // namespace notdec

#endif
