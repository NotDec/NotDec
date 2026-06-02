#ifndef _NOTDEC_PASSES_EVM_MEMORY_BUFFER_ANALYSIS_H_
#define _NOTDEC_PASSES_EVM_MEMORY_BUFFER_ANALYSIS_H_

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <optional>
#include <cstdint>

namespace llvm {
class CallBase;
class DominatorTree;
class Function;
class Instruction;
} // namespace llvm

namespace notdec::passes::evm {

enum class MemoryWriteKind {
  MStore = 1,
  CalldataCopy = 2,
  ReturndataCopy = 3,
  ScratchReturndataCopy = 4,
  CodeCopy = 5,
  MemoryCopy = 6,
  MStore8 = 7,
};

enum class MemoryConsumerKind {
  Return = 1,
  Revert = 2,
  EventLog = 3,
  ExternalCallInput = 4,
  ExternalCallOutput = 5,
};

// A Solidity allocation is modeled as the relation between the free-memory
// pointer read and the later write-back to slot 0x40. Some buffers are
// allocate_unbounded() and have no finalize point yet.
struct MemoryAllocation {
  llvm::Value *Base = nullptr;
  llvm::Value *Size = nullptr;
  llvm::Instruction *AllocatePoint = nullptr;
  llvm::Instruction *FinalizePoint = nullptr;
  bool Finalized = false;
};

// A memory write keeps the original memory instruction plus the buffer-relative offset.
// Copy writes also keep their source offset so later passes can distinguish
// calldata/returndata bytes from literal word stores.
struct MemoryWrite {
  llvm::Instruction *StoreOrCopy = nullptr;
  llvm::Value *Base = nullptr;
  std::optional<uint64_t> Offset;
  llvm::Value *ValueOrSize = nullptr;
  llvm::Value *SourceOffset = nullptr;
  MemoryWriteKind Kind = MemoryWriteKind::MStore;
};

// A memory read keeps the loaded value tied to the same base/offset surface as
// writes. Consumers such as external call output decode can then read this
// marker instead of matching raw memory loads again.
struct MemoryRead {
  llvm::Instruction *Load = nullptr;
  llvm::Value *Base = nullptr;
  std::optional<uint64_t> Offset;
  llvm::Value *Value = nullptr;
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

// A consumer is the operation that gives a memory buffer Solidity meaning.
// A single base may have several roles, especially external calls that reuse
// the input base for output or later returndata.
struct MemoryConsumer {
  llvm::CallBase *Call = nullptr;
  MemoryConsumerKind Kind = MemoryConsumerKind::Return;
  llvm::Value *Base = nullptr;
  llvm::Value *Size = nullptr;
};

// The analysis result is deliberately simple: later passes can use the same
// facts to do real rewrites instead of rediscovering mstore sequences.
struct MemoryBufferFacts {
  llvm::SmallVector<MemoryAllocation, 8> Allocations;
  llvm::SmallVector<MemoryWrite, 16> Writes;
  llvm::SmallVector<MemoryRead, 16> Reads;
  llvm::SmallVector<MemoryArrayByteWrite, 8> ArrayByteWrites;
  llvm::SmallVector<MemoryConsumer, 8> Consumers;
};

MemoryBufferFacts analyzeMemoryBuffers(llvm::Function &F,
                                       llvm::DominatorTree &DT);

struct MemoryBufferRewritePass
    : llvm::PassInfoMixin<MemoryBufferRewritePass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

} // namespace notdec::passes::evm

#endif
