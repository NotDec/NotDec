#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace llvm {
class Function;
class Module;
class raw_ostream;
} // namespace llvm

namespace notdec::mlsub {

// A slot-level record is used instead of a single function flag because a
// common binary API often mixes a fixed-layout owner object with one generic
// byte buffer.  Keeping those roles separate is required before the recovery
// pipeline can eventually instantiate only selected call slots.
struct RawBufferSlotEvidence {
  unsigned ArgIndex = 0;
  int Score = 0;
  std::string Confidence;

  bool HasIntegerPeer = false;
  bool HasAdjacentIntegerPeer = false;
  bool HasByteAccess = false;
  bool HasDynamicOffsetAccess = false;
  bool HasFixedWideAccess = false;
  unsigned PairedForwardCalls = 0;
  unsigned ValidatedForwardCalls = 0;
  unsigned AdjacentLengthForwardCalls = 0;
  unsigned DistinctActualShapes = 0;
};

// Deallocation is also modeled per slot: an owner object may contain one
// generic payload pointer without making every argument a release boundary.
struct DeallocatorSlotEvidence {
  unsigned ArgIndex = 0;
  int Score = 0;
  std::string Confidence;

  unsigned PointerOnlyVoidForwardCalls = 0;
  bool Compared = false;
  bool MutatesGlobalState = false;
  bool HasMemoryAccess = false;
  bool HasFixedWideAccess = false;
  unsigned DistinctActualOrigins = 0;
};

// This report intentionally contains only facts recoverable from lifted IR.
// FunctionIndex is a stable module-order identifier; Function is retained for
// in-process tests and for writing an audit-only symbol label.  Neither the
// symbol nor LLVM attributes/debug metadata participate in scoring.
struct AllocatorBoundaryEvidence {
  const llvm::Function *Function = nullptr;
  unsigned FunctionIndex = 0;
  unsigned DirectCallSites = 0;
  unsigned DistinctCallers = 0;

  int AllocatorScore = 0;
  std::string AllocatorConfidence;
  bool ReturnsPointer = false;
  bool PureReturnedCallForwarder = false;
  bool ReturnFromCall = false;
  bool ReturnFromArgument = false;
  bool ReturnFromGlobal = false;
  bool ReturnFromStack = false;
  bool ReturnFromIntToPtr = false;
  bool ReturnFromLoad = false;
  bool ReturnedStorageAccessedInBody = false;
  bool IntegerArgumentFlowsToReturnedCall = false;
  bool PointerArgumentFlowsToReturnedCall = false;
  bool CoreAllocatorStateEvidence = false;
  bool ReturnedCalleeHasAllocatorOrigin = false;
  unsigned StructuredResultCallSites = 0;
  unsigned DistinctResultShapes = 0;
  unsigned PropagatedStructuredResultCallSites = 0;
  unsigned PropagatedDistinctResultShapes = 0;

  std::vector<RawBufferSlotEvidence> RawBufferSlots;
  std::vector<DeallocatorSlotEvidence> DeallocatorSlots;
};

// Analyze every directly-called function with a pointer return or pointer
// argument.  The result includes low-confidence rows so an oracle can measure
// false negatives instead of seeing only candidates above an arbitrary cut.
std::vector<AllocatorBoundaryEvidence>
analyzeAllocatorBoundaries(const llvm::Module &M);

// Emit one JSON object per eligible function.  Symbols are clearly marked as
// audit labels; production decisions must use FunctionIndex and the evidence.
void writeAllocatorBoundaryReport(
    const llvm::Module &M,
    const std::vector<AllocatorBoundaryEvidence> &Evidence,
    llvm::raw_ostream &Out);

// ---------------------------------------------------------------------------
// Container boundaries, v2: module-level tables + two rules.
//
// The signals are computed once per module instead of being re-derived inside
// every function:
//
//   * the object fields the module invokes: offsets K != 0 where some function
//     loads a value from a parameter's field at K and calls it indirectly
//     without passing that object ("callback boundary", no-this form).  That
//     module table also tells which offsets carry function pointers, so a
//     function that only stores a callback into an object (the initialiser
//     shape, e.g. hash_initialize) is recognised without calling it;
//   * the lifted function addresses (from the !microsub2.func metadata), which
//     separate a stored function pointer from an ordinary data pointer;
//   * per object (parameter or call result - initialisers allocate their own
//     object) the constant offsets the function touches, its access widths and
//     whether it is a table of pointer-sized fields;
//   * per function, the values it never interprets locally but forwards to
//     calls (element opacity), and constant-stride array walks.
//
// Two rules score a function: the callback boundary (it invokes/stores/forwards
// a function pointer supplied from outside its own code) and element opacity
// (it moves values it never interprets).  A derived third fact propagates
// along the object flow: a function that hands its own object parameter to a
// function with callback evidence operates on the same container.
;

;

} // namespace notdec::mlsub
