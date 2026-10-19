#pragma once

#include <set>
#include <string>

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>

namespace llvm {
class Module;
} // namespace llvm

namespace notdec::mlsub {

class MLsubRecovery;

// Recognition and marking of polymorphic functions.
//
// The pass runs after lifting and before MLsubRecoveryMain.  The mark has to
// exist before SCC level assignment and constraint generation, because a
// polymorphic SCC becomes a summary boundary: its level is raised by one and it
// is kept out of same-level merging, so its call sites are instantiated
// independently instead of sharing one function type.
//
// Sources: the builtin buffer/allocator name list, the structural malloc
// wrapper detector, the allocator/raw-buffer boundary analysis, and (consulted
// again during constraint generation) the NOTDEC_POLY_FUNCS policy file and the
// summary override document.
struct PolymorphicMarkingPass : llvm::PassInfoMixin<PolymorphicMarkingPass> {
  explicit PolymorphicMarkingPass(MLsubRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return true; }

private:
  MLsubRecovery &TR;
};

// The report path: NOTDEC_LOG_DIR + poly-marking.json (fixed name).
//
// The path is empty when NOTDEC_LOG_DIR is unset, so no dump is written.
std::string polymorphicMarkingReportPath();

// NOTDEC_POLY_MARKING_ONLY=1 stops the recovery right after the marking pass,
// so a recognition-only run never builds the call graph or the constraints.
// This is a behaviour switch, not a dump switch: writing the report itself only
// needs NOTDEC_LOG_DIR.
bool polymorphicMarkingRecognitionOnly();

// NOTDEC_POLY_MARK_ALLOCATOR_BOUNDARIES=1 additionally marks the functions the
// allocator/raw-buffer boundary analysis reports with high confidence.
bool polymorphicMarkingMarksAllocatorBoundaries();

// The function names of the NOTDEC_POLY_FUNCS policy file (empty when the file
// is absent or the master switch is off).
std::set<std::string> loadPolyPolicyFunctionNames();

} // namespace notdec::mlsub
