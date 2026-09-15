#ifndef _NOTDEC_PASSES_FREE_PHI_SPLIT_PASS_H_
#define _NOTDEC_PASSES_FREE_PHI_SPLIT_PASS_H_

#include <llvm/IR/PassManager.h>

namespace notdec {

// Split a `free(phi)` sink into edge-local free calls.
//
// `free` takes `void *`, so optimized IR may legally merge unrelated pointer
// values into one PHI only to pass it to `free`.  Keeping that PHI before
// MLsub makes type recovery see false value flow between unrelated pointee
// types.
struct FreePhiSplitPass : llvm::PassInfoMixin<FreePhiSplitPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
  static bool isRequired() { return true; }
};

} // namespace notdec

#endif
