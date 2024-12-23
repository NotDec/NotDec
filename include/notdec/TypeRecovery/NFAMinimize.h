
#ifndef _NOTDEC_RETYPD_DFA_MIN_H_
#define _NOTDEC_RETYPD_DFA_MIN_H_

#include <cassert>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <queue>

#include <llvm/ADT/GraphTraits.h>

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Schema.h"
#include "Utils/ValueNamer.h"

namespace notdec::retypd {

/// Minimize a NFA
/// only single #Start and #End node because of the property of the
/// ConstraintGraph.
/// TODO allow minimize only recall half of the graph. view forget as epsilon to
/// #End.
template <typename GraphTy = ConstraintGraph *, typename NodeTy = CGNode *>
struct NFADeterminizer {
  // using GraphTy = llvm::Inverse<ConstraintGraph *>;
  // using NodeTy = llvm::Inverse<CGNode *>;
  // using GraphTy = ConstraintGraph *;
  // using NodeTy = CGNode *;
  using GT = llvm::GraphTraits<GraphTy>;

  ValueNamer NewVN;
  GraphTy OldG;
  ConstraintGraph *NewG;
  std::map<std::set<NodeTy>, CGNode *> DTrans;
  using EntryTy = typename std::map<std::set<NodeTy>, CGNode *>::iterator;
  NFADeterminizer(const ConstraintGraph *Old, ConstraintGraph *New)
      : OldG(const_cast<ConstraintGraph *>(Old)), NewG(New) {}
  void run() {
    std::queue<EntryTy> Worklist;
    auto OldGStart = GT::getEntryNode(OldG);
    auto OldGEnd = GT::getExitNode(OldG);
    assert(NewG->empty() && "NewG must be empty");
    // 1. Map the start/end node
    std::set<NodeTy> StartSet = countClosure({OldGStart});
    auto StartIt = DTrans.emplace(StartSet, NewG->getStartNode());
    DTrans[{OldGEnd}] = NewG->getEndNode();
    Worklist.push(StartIt.first);
    while (!Worklist.empty()) {
      auto It = Worklist.front();
      auto &Node = *It->second;
      std::set<EdgeLabel> outLabels = allOutLabels(It->first);
      for (auto &L : outLabels) {
        auto S = countClosure(move(It->first, L));
        if (S.count(OldGEnd)) {
          assert(S.size() == 1);
        }
        if (DTrans.count(S) == 0) {
          auto NewNodeEnt = getOrSetNewNode(S);
          Worklist.push(NewNodeEnt);
        }
        auto &ToNode = *DTrans[S];
        NewG->onlyAddEdge(Node, ToNode, L);
      }
      Worklist.pop();
    }
  }

  static std::set<NodeTy> move(const std::set<NodeTy> &N, EdgeLabel L) {
    std::set<NodeTy> ret;
    for (auto Node : N) {
      for (auto Edge : llvm::make_range(GT::child_edge_begin(Node),
                                        GT::child_edge_end(Node))) {
        if (Edge->getLabel() == L) {
          ret.insert(&const_cast<CGNode &>(Edge->getTargetNode()));
        }
      }
    }
    return ret;
  }

  static std::set<NodeTy> countClosure(const std::set<NodeTy> &N) {
    // initialize with N
    std::set<NodeTy> Ret(N);
    std::queue<NodeTy> Worklist;
    for (auto Node : N) {
      Worklist.push(Node);
    }
    while (!Worklist.empty()) {
      auto Node = Worklist.front();
      Worklist.pop();
      for (auto Edge : llvm::make_range(GT::child_edge_begin(Node),
                                        GT::child_edge_end(Node))) {
        if (std::holds_alternative<One>(Edge->getLabel())) {
          auto &Target = const_cast<CGNode &>(Edge->getTargetNode());
          if (Ret.count(&Target) == 0) {
            Ret.insert(&Target);
            Worklist.push(&Target);
          }
        }
      }
    }
    return Ret;
  }

  static void printLowTyDiffSet(const std::set<NodeTy> &N) {
    llvm::errs() << "Different low type in a set of nodes: \n";
    for (auto Node : N) {
      llvm::errs() << "Node: " << GT::toString(Node)
                   << " LowType: " << *GT::getLowTy(Node) << "\n";
    }
    llvm::errs() << "End of different low type set.\n";
  }

  static llvm::Type *ensureSameLowTy(const std::set<NodeTy> &N) {
    // ensure the set of nodes has the same low type.
    assert(N.size() > 0);
    llvm::Type *LowTy = nullptr;
    for (auto N1 : N) {
      if (LowTy == nullptr) {
        LowTy = GT::getLowTy(N1);
      } else {
        if (GT::getLowTy(N1) == nullptr || LowTy != GT::getLowTy(N1)) {
          printLowTyDiffSet(N);
          assert(false && "Different low type in a set of nodes");
        }
      }
    }
    return LowTy;
  }

  EntryTy getOrSetNewNode(const std::set<NodeTy> &N) {
    if (DTrans.count(N)) {
      return DTrans.find(N);
    }
    auto &NewNode = NewG->getOrInsertNode(
        NodeKey{TypeVariable::CreateDtv(NewG->Ctx, NewVN.getNewName("dfa_"))},
        ensureSameLowTy(N));
    auto it = DTrans.emplace(N, &NewNode);
    assert(it.second);
    return it.first;
  }

  static std::set<EdgeLabel> allOutLabels(const std::set<NodeTy> &N) {
    std::set<EdgeLabel> ret;
    for (auto Node : N) {
      for (auto Edge : llvm::make_range(GT::child_edge_begin(Node),
                                        GT::child_edge_end(Node))) {
        if (std::holds_alternative<One>(Edge->getLabel())) {
          // ignore one edge
        } else {
          ret.insert(Edge->getLabel());
        }
      }
    }
    return ret;
  }
};

using NFAInvDeterminizer = NFADeterminizer<llvm::InverseVal<ConstraintGraph *>,
                                           llvm::InverseVal<CGNode *>>;
ConstraintGraph determinize(const ConstraintGraph *G);
ConstraintGraph
determinizeWithMap(const ConstraintGraph *G,
                   std::map<std::set<CGNode *>, CGNode *> &NodeMap);
ConstraintGraph minimize(const ConstraintGraph *G);
ConstraintGraph
minimizeWithMap(const ConstraintGraph *G,
                std::map<std::set<CGNode *>, CGNode *> &NodeMap);

} // namespace notdec::retypd

#endif
