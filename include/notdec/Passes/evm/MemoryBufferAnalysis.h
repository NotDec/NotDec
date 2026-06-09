#ifndef _NOTDEC_PASSES_EVM_MEMORY_BUFFER_ANALYSIS_H_
#define _NOTDEC_PASSES_EVM_MEMORY_BUFFER_ANALYSIS_H_

#include <cstdint>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>

namespace llvm {
class CallBase;
class DominatorTree;
class Function;
class Instruction;
} // namespace llvm

namespace notdec::passes::evm {

// A Solidity allocation is modeled as the relation between the free-memory
// pointer read and the later write-back to slot 0x40. Some buffers are used
// without updating slot 0x40 and become calloc_unbounded().
struct MemoryAllocation {
  llvm::Value *Base = nullptr;
  llvm::Value *Size = nullptr;
  llvm::Instruction *AllocatePoint = nullptr;
  llvm::Instruction *FinalizePoint = nullptr;
  llvm::SmallVector<llvm::Instruction *, 4> Reloads;
  bool Finalized = false;
};

// Byte writes to memory bytes/string arrays use byte offsets after the 32-byte
// length word. This fact keeps the array base and byte index explicit instead
// of exposing only the final address passed to mstore8.
struct MemoryArrayByteWrite {
  llvm::Instruction *Store = nullptr;
  llvm::Value *ArrayBase = nullptr;
  llvm::Value *Index = nullptr;
  llvm::Value *Value = nullptr;
};

// The analysis result is deliberately simple: later passes can use the same
// facts to do real rewrites instead of rediscovering mstore sequences.
struct MemoryBufferFacts {
  llvm::SmallVector<MemoryAllocation, 8> Allocations;
  // Bases with at least one write are enough to decide whether a free-memory
  // pointer load represents an allocation candidate. Payload details now belong
  // to type recovery, not marker materialization.
  llvm::SmallVector<llvm::Value *, 16> WrittenBases;
  llvm::SmallVector<MemoryArrayByteWrite, 8> ArrayByteWrites;
};

MemoryBufferFacts analyzeMemoryBuffers(llvm::Function &F,
                                       llvm::DominatorTree &DT);

struct MemoryBufferRewritePass : llvm::PassInfoMixin<MemoryBufferRewritePass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

} // namespace notdec::passes::evm

#endif
