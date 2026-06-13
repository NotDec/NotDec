#ifndef _NOTDEC_PASSES_EVM_SOLIDITY_PATTERNS_H_
#define _NOTDEC_PASSES_EVM_SOLIDITY_PATTERNS_H_

#include <llvm/IR/PassManager.h>

namespace notdec::mlsub {
class MLsubRecovery;
}

namespace notdec::passes::evm {

extern const char *KIND_SOLIDITY_NONPAYABLE;
extern const char *KIND_SOLIDITY_PAYABILITY_GUARD;
extern const char *KIND_SOLIDITY_ABI_RETURN;
extern const char *KIND_SOLIDITY_REVERT;
extern const char *KIND_SOLIDITY_CHECKED_BOUNDS;
extern const char *KIND_SOLIDITY_EVENT;

// Splits selector-entry inline fallback/receive bodies into standalone helper
// functions when the CFG boundary is clear.  The pass keeps dispatcher-only
// blocks in the selector function and skips ambiguous regions instead of
// guessing.
struct SelectorEntryOutliningPass
    : llvm::PassInfoMixin<SelectorEntryOutliningPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Matches Solidity's canonical nonpayable guard after the generic LLVM
// optimizer has simplified the original stack-lifted condition.
struct PayabilityGuardPass : llvm::PassInfoMixin<PayabilityGuardPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Rewrites direct calldata word/range reads into normal LLVM memory accesses
// before type recovery.  The pass keeps calldata as the public entry's
// existing pointer argument instead of introducing a NotDec-specific access
// helper, so MLsub can reason about it like other memory objects.
struct EvmCalldataAccessPass
    : llvm::PassInfoMixin<EvmCalldataAccessPass> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks low-risk ABI return sites.  The current pass intentionally does not
// rewrite memory writes because return buffers often share code with dynamic
// object construction.
struct AbiReturnPass : llvm::PassInfoMixin<AbiReturnPass> {
  mlsub::MLsubRecovery &TR;

  explicit AbiReturnPass(mlsub::MLsubRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks empty reverts, Panic(uint256) reverts, and returndata bubbling.
struct SolidityRevertPass : llvm::PassInfoMixin<SolidityRevertPass> {
  mlsub::MLsubRecovery &TR;

  explicit SolidityRevertPass(mlsub::MLsubRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks compiler-inserted checked arithmetic and bounds guards.  Panic guards
// are classified by Solidity's stable Panic(uint256) code; empty reverts stay
// conservative candidates because several compiler and user paths share that
// shape.
struct CheckedBoundsPass : llvm::PassInfoMixin<CheckedBoundsPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks EVM log instructions as event emission candidates.
struct EventLogPass : llvm::PassInfoMixin<EventLogPass> {
  mlsub::MLsubRecovery &TR;

  explicit EventLogPass(mlsub::MLsubRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);

  static bool isRequired() { return true; }
};

} // namespace notdec::passes::evm

#endif
