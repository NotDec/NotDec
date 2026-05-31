#ifndef _NOTDEC_PASSES_EVM_SOLIDITY_PATTERNS_H_
#define _NOTDEC_PASSES_EVM_SOLIDITY_PATTERNS_H_

#include <llvm/IR/PassManager.h>

namespace notdec::passes::evm {

extern const char *KIND_SOLIDITY_NONPAYABLE;
extern const char *KIND_SOLIDITY_PAYABILITY_GUARD;
extern const char *KIND_SOLIDITY_ABI_DECODE;
extern const char *KIND_SOLIDITY_ABI_RETURN;
extern const char *KIND_SOLIDITY_REVERT;
extern const char *KIND_SOLIDITY_CHECKED_BOUNDS;
extern const char *KIND_SOLIDITY_CLEANUP;
extern const char *KIND_SOLIDITY_STORAGE_ADDRESSING;
extern const char *KIND_SOLIDITY_PACKED_STORAGE_FIELD;
extern const char *KIND_SOLIDITY_STORAGE_BYTES_STRING;
extern const char *KIND_SOLIDITY_MEMORY_OBJECT;
extern const char *KIND_SOLIDITY_EVENT;
extern const char *KIND_SOLIDITY_EXTERNAL_CALL;

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

// Marks calldata reads that are likely part of ABI argument decoding.  This is
// still a conservative annotation pass: it records static word loads and
// dynamic calldata copies, but does not infer final parameter types.
struct AbiDecodePass : llvm::PassInfoMixin<AbiDecodePass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks low-risk ABI return sites.  The current pass intentionally does not
// rewrite memory writes because return buffers often share code with dynamic
// object construction.
struct AbiReturnPass : llvm::PassInfoMixin<AbiReturnPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks empty reverts, Panic(uint256) reverts, and returndata bubbling.
struct SolidityRevertPass : llvm::PassInfoMixin<SolidityRevertPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks compiler-inserted guard candidates that branch to empty revert or Panic.
// It deliberately leaves the exact source category to later passes.
struct CheckedBoundsPass : llvm::PassInfoMixin<CheckedBoundsPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks common value cleanup idioms such as address masks, low-bit masks and
// signextend.  These are type hints only, not final recovered types.
struct ValueCleanupTypeHintPass
    : llvm::PassInfoMixin<ValueCleanupTypeHintPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks sha3-based storage addressing candidates.  Exact packed field recovery
// still needs stronger dataflow, so this pass only labels the address roots.
struct StorageAddressingPass : llvm::PassInfoMixin<StorageAddressingPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks packed storage load/store idioms built from sload/sstore plus
// shift/mask/or chains.
struct PackedStorageFieldPass
    : llvm::PassInfoMixin<PackedStorageFieldPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks short/long storage bytes-string encoding candidates.  These are still
// low-level candidates, not recovered Solidity string operations.
struct StorageBytesStringPass
    : llvm::PassInfoMixin<StorageBytesStringPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks free-memory-pointer loads/stores used by Solidity memory allocation.
struct MemoryObjectPass : llvm::PassInfoMixin<MemoryObjectPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks EVM log instructions as event emission candidates.
struct EventLogPass : llvm::PassInfoMixin<EventLogPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

// Marks low-level call instructions.  Proxy/library recognition is deliberately
// outside this pass; the metadata only records the EVM call kind.
struct ExternalCallPass : llvm::PassInfoMixin<ExternalCallPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

} // namespace notdec::passes::evm

#endif
