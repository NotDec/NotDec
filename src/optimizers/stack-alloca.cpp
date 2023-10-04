#include "optimizers/stack-alloca.h"
#include "optimizers/stack-pointer-finder.h"

namespace notdec::optimizers {
PreservedAnalyses LinearAllocationRecovery::run(Module &M,
                                                ModuleAnalysisManager &MAM) {
  errs() << "(llvm-tutor) Hello Module: " << M.getName() << "\n";
  auto sp = MAM.getResult<StackPointerFinderAnalysis>(M);
  if (sp.result == nullptr) {
    std::cerr << "ERROR: Stack Pointer not found!!";
    std::cerr << "LinearAllocationRecovery cannot proceed!\n";
    return PreservedAnalyses::all();
  }

  return PreservedAnalyses::none();
}
} // namespace notdec::optimizers
