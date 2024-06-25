#ifndef _NOTDEC_OPTIMIZERS_STACK_ALLOCA_H_
#define _NOTDEC_OPTIMIZERS_STACK_ALLOCA_H_

#include <llvm/IR/PassManager.h>

namespace notdec {

using namespace llvm;

struct LinearAllocationRecovery : PassInfoMixin<LinearAllocationRecovery> {
  PreservedAnalyses run(Module &F, ModuleAnalysisManager &);
};

} // namespace notdec

#endif
