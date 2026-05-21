#ifndef _NOTDEC_PASSES_EVM_SOLIDITY_PATTERNS_H_
#define _NOTDEC_PASSES_EVM_SOLIDITY_PATTERNS_H_

#include <llvm/IR/PassManager.h>

namespace notdec::passes::evm {

extern const char *KIND_SOLIDITY_NONPAYABLE;
extern const char *KIND_SOLIDITY_PAYABILITY_GUARD;

// Matches Solidity's canonical nonpayable guard after the generic LLVM
// optimizer has simplified the original stack-lifted condition.
struct PayabilityGuardPass : llvm::PassInfoMixin<PayabilityGuardPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

} // namespace notdec::passes::evm

#endif
