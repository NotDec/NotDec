
#include "TypeRecovery/NFAMinimize.h"
#include "TypeRecovery/Schema.h"
#include "optimizers/ConstraintGenerator.h"
#include <cassert>
#include <queue>
#include <set>

namespace notdec::retypd {

void DFAMinimizer::run() {
  std::queue<EntryTy> Worklist;
  assert(OldG.Start && OldG.End && "OldG.Start and OldG.End must be set");
  assert(NewG.empty() && "NewG must be empty");
  // 1. Create the start/end node
  NewG.Start =
      &NewG.getOrInsertNode(NodeKey{TypeVariable::CreatePrimitive("#Start")});
  NewG.End =
      &NewG.getOrInsertNode(NodeKey{TypeVariable::CreatePrimitive("#End")});
  DTrans[{OldG.Start}] = NewG.Start;
  DTrans[{OldG.End}] = NewG.End;
  Worklist.push(DTrans.begin());
  while (!Worklist.empty()) {
    auto It = Worklist.front();
    auto &Node = *It->second;
    Worklist.pop();
    std::set<EdgeLabel> outLabels = allOutLabels(It->first);
    for (auto &L : outLabels) {
      auto S = countClosure(move(It->first, L));
      if (S.count(OldG.End)) {
        assert(S.size() == 1);
      }
      if (DTrans.count(S) == 0) {
        auto NewNodeEnt = getOrSetNewNode(S);
        Worklist.push(NewNodeEnt);
      }
      auto &ToNode = *DTrans[S];
      NewG.onlyAddEdge(Node, ToNode, L);
    }
  }
}

std::set<EdgeLabel> DFAMinimizer::allOutLabels(const std::set<CGNode *> &N) {
  std::set<EdgeLabel> ret;
  for (auto &Node : N) {
    for (auto &Edge : Node->outEdges) {
      if (std::holds_alternative<One>(Edge.Label)) {
        // ignore one edge
      } else {
        ret.insert(Edge.Label);
      }
    }
  }
  return ret;
}

std::set<CGNode *> DFAMinimizer::move(const std::set<CGNode *> &N,
                                      EdgeLabel L) {
  std::set<CGNode *> ret;
  for (auto &Node : N) {
    for (auto &Edge : Node->outEdges) {
      if (Edge.Label == L) {
        ret.insert(&const_cast<CGNode &>(Edge.getTargetNode()));
      }
    }
  }
  return ret;
}

std::set<CGNode *> DFAMinimizer::countClosure(const std::set<CGNode *> &N) {
  std::set<CGNode *> Ret(N);
  std::queue<CGNode *> Worklist;
  for (auto &Node : N) {
    Worklist.push(Node);
  }
  while (!Worklist.empty()) {
    auto Node = Worklist.front();
    Worklist.pop();
    for (auto &Edge : Node->outEdges) {
      if (std::holds_alternative<One>(Edge.Label)) {
        auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
        if (Ret.count(&Target) == 0) {
          Ret.insert(&Target);
          Worklist.push(&Target);
        }
      }
    }
  }
  return Ret;
}

DFAMinimizer::EntryTy
DFAMinimizer::getOrSetNewNode(const std::set<CGNode *> &N) {
  if (DTrans.count(N)) {
    return DTrans.find(N);
  }
  auto &NewNode = NewG.getOrInsertNode(
      NodeKey{TypeVariable::CreateDtv(ValueNamer::getName("dfa_"))});
  auto it = DTrans.emplace(N, &NewNode);
  assert(it.second);
  return it.first;
}

} // namespace notdec::retypd
