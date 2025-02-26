#ifndef _NOTDEC_PASSES_MEMOP_MATCHER_H_
#define _NOTDEC_PASSES_MEMOP_MATCHER_H_

#include <llvm/IR/Instruction.h>
#include <llvm/IR/PassManager.h>

namespace notdec {

using namespace llvm;

struct MemsetMatcher : PassInfoMixin<MemsetMatcher> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &);
};

} // namespace notdec

#endif
