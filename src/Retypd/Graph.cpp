#include <boost/range/join.hpp>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <llvm/ADT/STLExtras.h>
#include <map>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>

#include <llvm/ADT/SCCIterator.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

#include "Retypd/Graph.h"
#include "Retypd/RExp.h"
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
  assert(false && "Unknown FieldLabel!");
}

std::vector<SubTypeConstraint> ConstraintGraph::solve_constraints_between(
    std::set<std::string> &InterestingVars) {

  std::map<std::pair<CGNode *, CGNode *>, rexp::PRExp> P;
  // Imagine a "#Start" node connecting to a subset of start nodes that is in
  // the set of interesting variables.
  // And a "#End" node connecting from a subset of end nodes that is in the set
  // of interesting variables.
  std::vector<std::tuple<CGNode *, CGNode *, rexp::PRExp>> SeqPrefix;
  std::vector<std::tuple<CGNode *, CGNode *, rexp::PRExp>> SeqSuffix;
  // 1.1. Imagine that we prepend the path sequence with a set of path from
  // "#Start" to the start nodes, and run solve over these paths
  for (auto N : StartNodes) {
    // is an interesting var or is a primitive type
    if ((InterestingVars.count(N->key.Base.name) != 0) ||
        N->key.Base.name.at(0) == '#') {
      LLVM_DEBUG(llvm::dbgs()
                 << "Adding a path from #Start to " << N->key.str() << "\n");
      SeqPrefix.emplace_back(Start, N,
                             rexp::create(RecallBase{N->key.Base.name}));
    }
  }

  // 1.2. Imagine that we append with a set of path from the end nodes to
  // "#End", and run solve over these paths
  for (auto N : EndNodes) {
    // is an interesting var or is a primitive type
    if ((InterestingVars.count(N->key.Base.name) != 0) ||
        N->key.Base.name.at(0) == '#') {
      LLVM_DEBUG(llvm::dbgs()
                 << "Adding a path from " << N->key.str() << " to #End\n");
      SeqSuffix.emplace_back(N, End,
                             rexp::create(ForgetBase{N->key.Base.name}));
    }
  }

  auto getPexp = [&](CGNode *From, CGNode *To) -> rexp::PRExp {
    auto Key = std::make_pair(From, To);
    if (P.count(Key)) {
      return P[Key];
    }
    if (From == To) {
      return std::make_shared<rexp::RExp>(rexp::Empty{});
    } else {
      return std::make_shared<rexp::RExp>(rexp::Null{});
    }
  };
  auto assignPExp = [&](CGNode *From, CGNode *To, rexp::PRExp E) {
    if (From == To && rexp::isEmpty(E)) {
      return;
    }
    if (From != To && rexp::isNull(E)) {
      return;
    }
    P[std::make_pair(From, To)] = E;
  };
  auto bitandAssignPExp = [&](CGNode *From, CGNode *To, rexp::PRExp E) {
    auto Old = getPexp(From, To);
    assignPExp(From, To, rexp::simplifyOnce(Old & E));
  };
  auto bitorAssignPExp = [&](CGNode *From, CGNode *To, rexp::PRExp E) {
    auto Old = getPexp(From, To);
    assignPExp(From, To, rexp::simplifyOnce(Old | E));
  };

  // https://stackoverflow.com/questions/14366576/boostrangejoin-for-multiple-ranges
  auto R1 = boost::join(SeqPrefix, PathSeq);
  auto SeqIter = boost::range::join(R1, SeqSuffix);
  // 1.3. solve the pathexpr(start)
  auto Source = Start;
  for (auto Ent : SeqIter) {
    auto [From, To, E] = Ent;
    if (From == To) {
      // P[source, from] = P[source, from] & exp
      bitandAssignPExp(Source, From, E);
    } else {
      // P[source, to] = P[source, to] | (P[source, from] & exp)
      auto srcFrom = getPexp(Source, From);
      bitorAssignPExp(Source, To, rexp::simplifyOnce(srcFrom & E));
    }
  }
  auto finalExp = getPexp(Start, End);
  if ((::llvm::DebugFlag && ::llvm::isCurrentDebugType(DEBUG_TYPE))) {
    // flatten the outer most Or
    if (std::holds_alternative<rexp::Or>(*finalExp)) {
      auto &Inner = std::get<rexp::Or>(*finalExp).E;
      llvm::dbgs() << "Final Expression: \n";
      for (auto Exp : Inner) {
        llvm::dbgs() << "  " << toString(Exp) << "\n";
      }
    } else {
      llvm::dbgs() << "Final Expression: " << toString(finalExp) << "\n";
    }
  }

  // 2. adjust the expr to create type schemes.
  auto ret = expToConstraints(finalExp);
  // 3. delete invalid constraints like: int64.in_0
  // 4. substitute temp vars to prevent name collision.
  return ret;
}

std::vector<SubTypeConstraint>
ConstraintGraph::simplify(std::vector<Constraint> &Cons,
                          std::set<std::string> &InterestingVars) {

  // if (!InterestingVars.has_value()) {
  //   assert(false); // TODO: is this useful?
  //   InterestingVars = std::set<std::string>();
  //   InterestingVars->insert(FuncName);
  // }

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
  printGraph("trans_layerSplit.dot");
  buildPathSequence();
  Start = &getOrInsertNode(NodeKey{DerivedTypeVariable{.name = "#Start"}});
  End = &getOrInsertNode(NodeKey{DerivedTypeVariable{.name = "#End"}});
  return solve_constraints_between(InterestingVars);
}

void ConstraintGraph::buildPathSequence() {
  assert(PathSeq.size() == 0 && "Path sequence already built!?");
  std::vector<std::set<CGNode *>> SCCs;
  for (auto I = llvm::scc_begin(this); !I.isAtEnd(); ++I) {
    SCCs.emplace_back((*I).begin(), (*I).end());
  }
  // View the graph as a directed acyclic graph of SCC
  for (auto &SCC : llvm::reverse(SCCs)) {
    // run eliminate for each SCC
    if (SCC.size() > 1) {
      auto Seqs = rexp::eliminate(*this, SCC);
      PathSeq.insert(PathSeq.end(), Seqs.begin(), Seqs.end());
    }
    // Add all edges out of the current SCC to the path sequence.
    for (auto N : SCC) {
      for (auto &E : N->outEdges) {
        auto &Target = const_cast<CGNode &>(E.getTargetNode());
        // Add the edge to the path sequence.
        if (SCC.count(&Target) == 0) {
          PathSeq.emplace_back(N, &Target, rexp::create(E.Label));
        }
      }
    }
  }
}

void ConstraintGraph::layerSplit() {
  // add edges
  std::vector<std::tuple<CGNode *, CGNode *, CGNode *, FieldLabel>> toChange;
  for (auto &Ent : Nodes) {
    if (Ent.second.key.IsNewLayer) {
      continue;
    }
    auto &Source = Ent.second;
    for (auto &Edge : Source.outEdges) {
      auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      if (Target.key.IsNewLayer) {
        continue;
      }
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
        FieldLabel FL = std::get<ForgetLabel>(Edge.Label).label;
        // Edge reference will be invalidated. Edge changes is deferred.
        toChange.emplace_back(&Source, &Target, &NewDstNode, FL);
      }
    }
  }
  // deferred Edge modification
  for (auto &Ent : toChange) {
    auto [Source, Target, NewDst, Label] = Ent;
    LLVM_DEBUG(llvm::dbgs()
               << "layerSplit: Retarget Edge from " << toString(Source->key)
               << " to " << toString(Target->key) << " with forget "
               << toString(Label) << " to " << toString(NewDst->key) << "\n");
    removeEdge(*Source, *Target, ForgetLabel{Label});
    addEdge(*Source, *NewDst, ForgetLabel{Label});
  }
  // Add new edge in the new layer to end nodes.
  for (CGNode *Ent : EndNodes) {
    if (Ent->key.IsNewLayer) {
      continue;
    }
    NodeKey NewKey = Ent->key;
    NewKey.IsNewLayer = true;
    auto &NewNode = getOrInsertNode(NewKey);
    EndNodes.insert(&NewNode);
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
              if (Reach.first == Capa.label && Reach.second != &Target) {
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

/// \arg isLeaf: whether the current node is a leaf node.
static void
expToConstraintsRecursive(std::vector<std::vector<EdgeLabel>> &result,
                          std::vector<EdgeLabel> &stack, rexp::PRExp E,
                          bool isLeaf) {
  // TODO: 这里可能有指数级的复杂度。考虑And(Or("recall a", "recall b"), "forget
  // #A")，这里用栈模拟是错误的。处理And的时候不能假设Or那边只返回单个表达式序列，即不能用栈存储。
  //
  if (std::holds_alternative<rexp::Or>(*E)) {
    std::size_t stackLevel = stack.size();
    for (auto E1 : std::get<rexp::Or>(*E).E) {
      expToConstraintsRecursive(result, stack, E1, isLeaf);
      stack.resize(stackLevel);
    }
  } else if (std::holds_alternative<rexp::And>(*E)) {
    auto &V = std::get<rexp::And>(*E).E;
    for (std::size_t Ind = 0; Ind < V.size(); Ind++) {
      auto E1 = V[Ind];
      bool isLeaf1 = (Ind == V.size() - 1) ? isLeaf : false;
      expToConstraintsRecursive(result, stack, E1, isLeaf1);
    }
  } else if (std::holds_alternative<rexp::Star>(*E)) {
    auto Inner = std::get<rexp::Star>(*E).E;
    std::string tempName = "__temp";

    stack.push_back(ForgetBase{tempName});
    result.push_back(stack);
    stack.clear();

    // Create recursive constraints.
    expToConstraintsRecursive(result, stack,
                              rexp::create(RecallBase{tempName}) & Inner &
                                  rexp::create(ForgetBase{tempName}),
                              true);

    stack.clear();
    stack.push_back(RecallBase{tempName});

    // 4. act as a normal Node
    if (isLeaf) {
      // TODO: ensure
      assert(false && "pexp_to_constraints: Star on leaf node!?");
      result.push_back(stack);
    }
  } else if (std::holds_alternative<rexp::Node>(*E)) {
    stack.push_back(std::get<rexp::Node>(*E).E);
    if (isLeaf) {
      result.push_back(stack);
    }
  } else if (std::holds_alternative<rexp::Null>(*E) ||
             std::holds_alternative<rexp::Empty>(*E)) {
    assert(false && "pexp_to_constraints: Null or Empty on expression leaf! "
                    "The exp is not fully simplified?");
  }
}

std::string toString(const std::vector<EdgeLabel> &ELs) {
  std::string ret;
  for (auto &EL : ELs) {
    ret += toString(EL) + " ";
  }
  return ret;
}

SubTypeConstraint normalizePath(const std::vector<EdgeLabel> &ELs) {
  // the pexp must be in (recall _)*(forget _)*
  SubTypeConstraint Ret;
  if (ELs.size() < 2) {
    assert(false && "normalize_path: path size < 2!");
  }
  auto &First = ELs[0];
  if (!std::holds_alternative<RecallBase>(First)) {
    assert(false && "normalize_path: first label is not RecallBase!");
  }
  Ret.sub.name = std::get<RecallBase>(First).base;
  auto &Last = ELs[ELs.size() - 1];
  if (!std::holds_alternative<ForgetBase>(Last)) {
    assert(false && "normalize_path: Last label is not ForgetBase!");
  }
  Ret.sup.name = std::get<ForgetBase>(Last).base;

  bool isForget = false;
  for (std::size_t Ind = 1; Ind < ELs.size() - 1; Ind++) {
    auto &EL = ELs[Ind];
    if (std::holds_alternative<RecallLabel>(EL)) {
      if (isForget) {
        assert(false && "normalize_path: Path has Recall after Forget!");
      }
      Ret.sub.Labels.push_back(std::get<RecallLabel>(EL).label);
    } else if (std::holds_alternative<ForgetLabel>(EL)) {
      isForget = true;
      Ret.sup.Labels.push_back(std::get<ForgetLabel>(EL).label);
    } else {
      assert(false && "normalize_path: Base label type in the middle!");
    }
  }
  return Ret;
}

/// the pexp must be in (recall _)*(forget _)*
std::vector<SubTypeConstraint> expToConstraints(rexp::PRExp E) {
  std::vector<SubTypeConstraint> ret;
  if (rexp::isEmpty(E)) {
    return ret;
  } else if (rexp::isNull(E)) {
    assert(false && "pexp_to_constraints: Null path!");
  }
  std::vector<std::vector<EdgeLabel>> result;
  std::vector<EdgeLabel> stack;
  expToConstraintsRecursive(result, stack, E, true);
  for (auto &ELs : result) {
    LLVM_DEBUG(llvm::dbgs() << "Normalize path: " << toString(ELs));
    SubTypeConstraint Cons = normalizePath(ELs);
    if (Cons.sub != Cons.sup) {
      ret.push_back(Cons);
    }
  }
  return ret;
}

} // namespace notdec::retypd
