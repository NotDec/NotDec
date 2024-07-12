#include <cassert>
#include <cstring>
#include <map>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>

#include <llvm/ADT/SCCIterator.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

#include "Retypd/Graph.h"
#include "Retypd/Schema.h"

#define DEBUG_TYPE "retypd_graph"

namespace notdec::retypd {

std::string toString(EdgeLabel label) {
  if (std::holds_alternative<One>(label)) {
    return "_1_";
  } else if (std::holds_alternative<ForgetLabel>(label)) {
    return "forget " + toString(std::get<ForgetLabel>(label).label);
  } else if (std::holds_alternative<ForgetBase>(label)) {
    return "forget " + std::get<ForgetBase>(label).base;
  } else if (std::holds_alternative<RecallLabel>(label)) {
    return "recall " + toString(std::get<RecallLabel>(label).label);
  } else if (std::holds_alternative<RecallBase>(label)) {
    return "recall " + std::get<RecallBase>(label).base;
  }
  return "unknown";
}

void ConstraintGraph::build(std::vector<Constraint> &Cons) {
  buildInitialGraph(Cons);
  if (const char *path = std::getenv("DEBUG_TRANS_INIT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) ||
        (std::strstr(path, FuncName.c_str()))) {
      printGraph("trans_init.dot");
    }
  }
  saturate();
  if (const char *path = std::getenv("DEBUG_TRANS_SAT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) ||
        (std::strstr(path, FuncName.c_str()))) {
      printGraph("trans_sat.dot");
    }
  }
  layerSplit();
  buildPathSequence();
  // GetOrInsertNode(NodeKey{DerivedTypeVariable{.name = "#Start"}});
  // GetOrInsertNode(NodeKey{DerivedTypeVariable{.name = "#End"}});
}

void ConstraintGraph::buildPathSequence() {
  assert(PathSeq.size() == 0 && "Path sequence already built!?");
  std::vector<std::vector<CGNode *>> SCCs;
  for (auto I = llvm::scc_begin(this); !I.isAtEnd(); ++I) {
    SCCs.push_back(*I);
  }
}

void ConstraintGraph::layerSplit() {
  // add new layer nodes
  for (auto &Ent : Nodes) {
    NodeKey NewKey = Ent.first;
    NewKey.IsNewLayer = true;
    auto &NewNode = getOrInsertNode(NewKey);
  }
  // add edges
  for (auto &Ent : Nodes) {
    auto &Source = Ent.second;
    for (auto &Edge : Source.outEdges) {
      auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      NodeKey NewSrc = Source.key;
      NewSrc.IsNewLayer = true;
      auto &NewSrcNode = getOrInsertNode(NewSrc);
      NodeKey NewDst = Target.key;
      NewDst.IsNewLayer = true;
      auto &NewDstNode = getOrInsertNode(NewDst);
      if (std::holds_alternative<RecallLabel>(Edge.Label)) {
        continue;
      }
      addEdge(NewSrcNode, NewDstNode, Edge.Label);
      if (std::holds_alternative<ForgetLabel>(Edge.Label)) {
        removeEdge(Source, Target, Edge.Label);
        addEdge(Source, NewDstNode, Edge.Label);
      }
    }
  }
}

/// Algorithm D.2 Saturation algorithm
void ConstraintGraph::saturate() {
  bool Changed = false;
  std::map<CGNode *, std::set<std::pair<FieldLabel, CGNode *>>> ReachingSet;

  // 1. add forget edge to reaching set
  for (auto &Ent : Nodes) {
    auto &Source = Ent.second;
    for (auto &Edge : Source.outEdges) {
      // For each edge, check if is forget edge.
      if (std::holds_alternative<ForgetLabel>(Edge.Label)) {
        auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
        auto Capa = std::get<ForgetLabel>(Edge.Label);
        auto Res = ReachingSet[&Target].insert({
            Capa.label,
            &Source,
        });
        Changed |= Res.second;
      }
    }
  }

  while (Changed) {
    Changed = false;
    // For each edge, if it is One edge, add reaching set.
    for (auto &Ent : Nodes) {
      auto &Source = Ent.second;
      for (auto &Edge : Source.outEdges) {
        if (std::holds_alternative<One>(Edge.Label)) {
          auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
          // For each One edge.
          if (ReachingSet.count(&Source)) {
            for (auto &Reach : ReachingSet[&Source]) {
              auto Res = ReachingSet[&Target].insert(Reach);
              Changed |= Res.second;
            }
          }
        }
      }
    }
    // The standard saturation rule.
    // begin: For each recall edge,
    for (auto &Ent : Nodes) {
      auto &Source = Ent.second;
      for (auto &Edge : Source.outEdges) {
        if (std::holds_alternative<RecallLabel>(Edge.Label)) {
          auto Capa = std::get<RecallLabel>(Edge.Label);
          auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
          // end: for each recall edge.
          if (ReachingSet.count(&Source)) {
            for (auto &Reach : ReachingSet[&Source]) {
              if (Reach.first == Capa.label) {
                LLVM_DEBUG(llvm::dbgs()
                           << "Adding Edge From " << Reach.second->key.str()
                           << " to " << Target.key.str() << " with _1_ \n");
                // We are iterating through Recall edges, and we insert One
                // edge, so it is ok to modify edge during iterating.
                Changed |= addEdge(*Reach.second, Target, One{});
              }
            }
          }
        }
      }
    }

    // Lazily apply saturation rules corresponding to S-POINTER.
    for (auto &Ent : ReachingSet) {
      if (Ent.first->key.SuffixVariance == Contravariant) {
        for (auto &Reach : Ent.second) {
          std::optional<FieldLabel> Label;
          if (std::holds_alternative<StoreLabel>(Reach.first)) {
            LLVM_DEBUG(llvm::dbgs()
                       << "node " << Reach.second->key.str() << " can reach "
                       << Ent.first->key.str() << " with \".store\" \n");
            Label = LoadLabel{};
          } else if (std::holds_alternative<LoadLabel>(Reach.first)) {
            LLVM_DEBUG(llvm::dbgs()
                       << "node " << Reach.second->key.str() << " can reach "
                       << Ent.first->key.str() << " with \".load\" \n");
            Label = StoreLabel{};
          } else {
            continue;
          }
          // clone the key and find the node with opposite variance.
          NodeKey Opposite = Ent.first->key;
          Opposite.SuffixVariance = Covariant;
          auto &OppositeNode = getOrInsertNode(Opposite);
          auto Res =
              ReachingSet[&OppositeNode].insert({Label.value(), Reach.second});
          Changed |= Res.second;
        }
      }
    }
  }
}

/// build the initial graph (Algorithm D.1 Transducer)
void ConstraintGraph::buildInitialGraph(std::vector<Constraint> &Cons) {
  for (Constraint &C : Cons) {
    if (std::holds_alternative<SubTypeConstraint>(C)) {
      auto &SC = std::get<SubTypeConstraint>(C);
      // 1. add two node and 1-labeled edge
      auto &NodeL = getOrInsertNode(SC.sub);
      auto &NodeR = getOrInsertNode(SC.sup);
      // add 1-labeled edge between them
      addEdge(NodeL, NodeR, One{});
      // 2. add each sub var node and edges.
      // 2.1 left
      addRecalls(NodeL);
      // 2.2 right
      addForgets(NodeR);

      // 3-4 the inverse of the above
      // 3. inverse node and 1-labeled edge
      auto &RNodeL = getOrInsertNode(NodeKey(SC.sub, Contravariant));
      auto &RNodeR = getOrInsertNode(NodeKey(SC.sup, Contravariant));
      // add 1-labeled edge between them
      addEdge(RNodeR, RNodeL, One{});
      // 4.1 inverse left
      addRecalls(RNodeL);
      // 4.2 inverse right
      addForgets(RNodeR);
    } else {
      AddConstraints.push_back(C);
    }
  }
}

std::optional<std::pair<FieldLabel, NodeKey>> NodeKey::forgetOnce() const {
  if (Base.Labels.empty()) {
    return std::nullopt;
  }
  NodeKey NewKey(*this);
  FieldLabel Label = NewKey.Base.Labels.back();
  NewKey.Base.Labels.pop_back();
  NewKey.SuffixVariance = combine(this->SuffixVariance, getVariance(Label));
  return std::make_pair(Label, NewKey);
}

void ConstraintGraph::addRecalls(CGNode &N) {
  CGNode *T = &N;
  auto V1 = T->key.forgetOnce();
  while (V1.has_value()) {
    auto [Cap, Next] = V1.value();
    auto &NNext = getOrInsertNode(Next);
    addEdge(NNext, *T, RecallLabel{Cap});
    V1 = Next.forgetOnce();
    T = &NNext;
  }
  // We do not really link the node to #Start
  StartNodes.insert(T);
}

void ConstraintGraph::addForgets(CGNode &N) {
  CGNode *T = &N;
  auto V1 = T->key.forgetOnce();
  while (V1.has_value()) {
    auto [Cap, Next] = V1.value();
    auto &NNext = getOrInsertNode(Next);
    addEdge(*T, NNext, ForgetLabel{Cap});
    V1 = Next.forgetOnce();
    T = &NNext;
  }
  // We do not really link the node to #Start
  EndNodes.insert(T);
}

CGNode &ConstraintGraph::getOrInsertNode(const NodeKey &N) {
  auto [it, inserted] = Nodes.try_emplace(N, N);
  if (inserted) {
    assert(addNode(it->second));
  }

  return it->second;
}

void ConstraintGraph::printGraph(const char *DotFile) {
  std::error_code EC;
  llvm::raw_fd_ostream OutStream(DotFile, EC);
  if (EC) {
    llvm::errs() << "Error: " << EC.message() << "\n";
    return;
  }
  llvm::WriteGraph(OutStream, this, false);
  OutStream.flush();
  OutStream.close();
}

std::string toString(NodeKey K) { return K.str(); }

} // namespace notdec::retypd
