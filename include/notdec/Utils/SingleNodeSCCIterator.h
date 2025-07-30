#include <llvm/ADT/SCCIterator.h>
#include <llvm/Analysis/CallGraph.h>

#include "Utils/AllSCCIterator.h"

namespace notdec {

class SingleNodeSCCIterator {
public:
  using NodeVector = std::vector<llvm::CallGraphNode *>;
  using SCCIteratorType = all_scc_iterator<llvm::CallGraph *>;

  SingleNodeSCCIterator(SCCIteratorType It) : SCCIt(It), CurrentSCCPos(0) {
    advanceToNextNonEmptySCC();
  }

  bool isAtEnd() const {
    return SCCIt.isAtEnd() && CurrentSCCPos >= CurrentSCC.size();
  }

  // 返回封装数组的引用，和all_scc_iterator风格一致
  const NodeVector &operator*() const { return CurrentSingleNodeVec; }

  SingleNodeSCCIterator &operator++() {
    if (isAtEnd())
      return *this;

    ++CurrentSCCPos;
    if (CurrentSCCPos >= CurrentSCC.size()) {
      ++SCCIt;
      advanceToNextNonEmptySCC();
    } else {
      updateCurrentSingleNode();
    }
    return *this;
  }

private:
  SCCIteratorType SCCIt;
  NodeVector CurrentSCC;
  size_t CurrentSCCPos = 0;

  NodeVector CurrentSingleNodeVec; // 用于封装单个节点

  void advanceToNextNonEmptySCC() {
    CurrentSCCPos = 0;
    while (!SCCIt.isAtEnd()) {
      const NodeVector &SCCNodes = *SCCIt;
      if (!SCCNodes.empty()) {
        CurrentSCC = SCCNodes;
        updateCurrentSingleNode();
        return;
      }
      ++SCCIt;
    }
    CurrentSCC.clear();
    CurrentSingleNodeVec.clear();
  }

  void updateCurrentSingleNode() {
    if (CurrentSCCPos < CurrentSCC.size()) {
      CurrentSingleNodeVec = {CurrentSCC[CurrentSCCPos]};
    } else {
      CurrentSingleNodeVec.clear();
    }
  }
};

} // namespace notdec
