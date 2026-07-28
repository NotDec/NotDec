#ifndef _NOTDEC_MLSUB_MERGEPOLICYEVAL_H_
#define _NOTDEC_MLSUB_MERGEPOLICYEVAL_H_

#include "binarysub/binarysub-core.h"
#include "notdec-llvm2c/Interface/ExtValuePtr.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>

#include <cstddef>
#include <memory>
#include <string>

namespace llvm {
class Module;
} // namespace llvm

namespace notdec::mlsub {

struct AllGraphs;

/// DebugInfo-based merge-policy evaluation.  Normal type recovery does not
/// create this object; it is only wired when the explicit eval flag is used.
class MergePolicyEval {
public:
  MergePolicyEval(const llvm::Module &M, llvm::StringRef OutputDir,
                  unsigned PointerSize);
  ~MergePolicyEval();

  void observeValueNode(const ExtValuePtr &Val, binarysub::SimpleType Ty);
  void observeValueMapMerge(binarysub::SimpleType From,
                            binarysub::SimpleType To,
                            llvm::ArrayRef<ExtValuePtr> MovedValues);
  void observeVariableMerged(const binarysub::MergeEvent &Event);
  void observeAddSubtype();

  // Variable-merge evaluation needs the graph state at each individual merge.
  // A speculative call-interface transaction therefore records immediately and
  // truncates back to this checkpoint if the graph transaction rolls back.
  std::size_t badUnionCheckpoint() const;
  void rollbackBadUnions(std::size_t Checkpoint);

  void finish(const AllGraphs &AG);

private:
  struct Impl;
  std::unique_ptr<Impl> P;
};

} // namespace notdec::mlsub

#endif
