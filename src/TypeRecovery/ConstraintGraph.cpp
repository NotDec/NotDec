#include <boost/range/join.hpp>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <llvm/ADT/STLExtras.h>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#include <llvm/ADT/SCCIterator.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"

#define DEBUG_TYPE "retypd_graph"

namespace notdec::retypd {

void ConstraintGraph::solve() { SSG.solve(); }

std::string toString(const EdgeLabel &label) {
  if (std::holds_alternative<One>(label)) {
    return "_1_";
  } else if (std::holds_alternative<ForgetLabel>(label)) {
    return "forget " + toString(std::get<ForgetLabel>(label).label);
  } else if (std::holds_alternative<ForgetBase>(label)) {
    return "forget " + toString(std::get<ForgetBase>(label).base);
  } else if (std::holds_alternative<RecallLabel>(label)) {
    return "recall " + toString(std::get<RecallLabel>(label).label);
  } else if (std::holds_alternative<RecallBase>(label)) {
    return "recall " + toString(std::get<RecallBase>(label).base);
  }
  assert(false && "Unknown FieldLabel!");
}

std::vector<SubTypeConstraint> ConstraintGraph::toConstraints() {
  assert(!isLayerSplit && "printConstraints: graph is already split!?");

  std::vector<SubTypeConstraint> ret;
  for (auto &Ent : Nodes) {
    // TODO deduplicate?
    // if (Ent.second.key.SuffixVariance == Contravariant) {
    //   continue;
    // }
    auto &Source = Ent.second;
    for (auto &Edge : Source.outEdges) {
      auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      if (std::holds_alternative<One>(Edge.Label)) {
        ret.push_back(SubTypeConstraint{.sub = Ent.second.key.Base,
                                        .sup = Target.key.Base});
      }
    }
  }
  return ret;
}

std::vector<SubTypeConstraint> ConstraintGraph::solve_constraints_between() {

  std::map<std::pair<CGNode *, CGNode *>, rexp::PRExp> P;

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

  // 1.3. solve the pathexpr(start)
  auto Source = Start;
  for (auto Ent : PathSeq) {
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
ConstraintGraph::simplify(std::set<std::string> &InterestingVars) {

  // if (!InterestingVars.has_value()) {
  //   assert(false); // TODO: is this useful?
  //   InterestingVars = std::set<std::string>();
  //   InterestingVars->insert(FuncName);
  // }

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
  Start = &getOrInsertNode(NodeKey{TypeVariable::CreatePrimitive("#Start")});
  End = &getOrInsertNode(NodeKey{TypeVariable::CreatePrimitive("#End")});

  // Link nodes to "#Start"
  for (auto N : StartNodes) {
    // is an interesting var or is a primitive type
    if ((N->key.Base.isDtv() &&
         InterestingVars.count(N->key.Base.getBaseName()) != 0) ||
        N->key.Base.isPrimitive()) {
      LLVM_DEBUG(llvm::dbgs()
                 << "Adding an edge from #Start to " << N->key.str() << "\n");
      addEdge(*Start, *N, RecallBase{N->key.Base.toBase()});
    }
  }

  // Link nodes to "#End"
  for (auto N : EndNodes) {
    // is an interesting var or is a primitive type
    if ((N->key.Base.isDtv() &&
         InterestingVars.count(N->key.Base.getBaseName()) != 0) ||
        N->key.Base.isPrimitive()) {
      LLVM_DEBUG(llvm::dbgs()
                 << "Adding an edge from " << N->key.str() << " to #End\n");
      addEdge(*N, *End, ForgetBase{N->key.Base.toBase()});
    }
  }

  buildPathSequence();
  return solve_constraints_between();
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
  assert(!isLayerSplit && "layerSplit: already split!?");
  isLayerSplit = true;
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
            Label = LoadLabel{.Size = std::get<StoreLabel>(Reach.first).Size};
          } else if (std::holds_alternative<LoadLabel>(Reach.first)) {
            LLVM_DEBUG(llvm::dbgs()
                       << "node " << Reach.second->key.str() << " can reach "
                       << Ent.first->key.str() << " with \".load\" \n");
            Label = StoreLabel{.Size = std::get<LoadLabel>(Reach.first).Size};
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

ConstraintGraph
ConstraintGraph::fromConstraints(std::string FuncName,
                                 std::vector<Constraint> &Cons) {
  ConstraintGraph G(nullptr, FuncName);
  for (auto &C : Cons) {
    if (std::holds_alternative<SubTypeConstraint>(C)) {
      auto &SCons = std::get<SubTypeConstraint>(C);
      G.addConstraint(SCons.sub, SCons.sup);
    } else {
      assert(false && "buildInitialGraph: Unimplemented constraint type!");
    }
  }
  return G;
}

/// Interface for initial constraint insertion
/// Also build the initial graph (Algorithm D.1 Transducer)
void ConstraintGraph::addConstraint(const TypeVariable &sub,
                                    const TypeVariable &sup) {
  // 1. add two node and 1-labeled edge
  auto &NodeL = getOrInsertNode(sub);
  auto &NodeR = getOrInsertNode(sup);
  // add 1-labeled edge between them
  addEdge(NodeL, NodeR, One{});
  // 2. add each sub var node and edges.
  // 2.1 left
  addRecalls(NodeL);
  // 2.2 right
  addForgets(NodeR);

  // 3-4 the inverse of the above
  // 3. inverse node and 1-labeled edge
  auto &RNodeL = getOrInsertNode(NodeKey(sub, Contravariant));
  auto &RNodeR = getOrInsertNode(NodeKey(sup, Contravariant));
  // add 1-labeled edge between them
  addEdge(RNodeR, RNodeL, One{});
  // 4.1 inverse left
  addRecalls(RNodeL);
  // 4.2 inverse right
  addForgets(RNodeR);
}

std::optional<std::pair<FieldLabel, NodeKey>> NodeKey::forgetOnce() const {
  if (!Base.hasLabel()) {
    return std::nullopt;
  }
  NodeKey NewKey(*this);
  FieldLabel Label = NewKey.Base.getLabels().back();
  NewKey.Base.getLabels().pop_back();
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

CGNode &ConstraintGraph::getOrInsertNode(const NodeKey &N, unsigned int Size) {
  auto [it, inserted] = Nodes.try_emplace(N, *this, N, Size);
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

std::string toString(const NodeKey &K) { return K.str(); }
std::string toString(const std::vector<EdgeLabel> &ELs) {
  std::string ret;
  for (auto &EL : ELs) {
    ret += toString(EL) + " ";
  }
  return ret;
}

struct ExprToConstraintsContext {
  static const char *tempNamePrefix;
  int TempNameIndex = 0;
  // std::function<std::string()> &tempNameGenerator;
  std::string genTempName() {
    return tempNamePrefix + std::to_string(TempNameIndex++);
  }
  std::vector<std::vector<EdgeLabel>> ConstraintsSequence;

  std::vector<std::vector<EdgeLabel>>
  expToConstraintsSequenceRecursive(rexp::PRExp E);
  static std::vector<SubTypeConstraint>
  normalizePath(const std::vector<EdgeLabel> &ELs);
  static std::vector<SubTypeConstraint> constraintsSequenceToConstraints(
      const std::vector<std::vector<EdgeLabel>> &ConstraintsSequence);
};

const char *ExprToConstraintsContext::tempNamePrefix = "__temp_";

/// \arg isLeaf: whether the current node is a leaf node.
std::vector<std::vector<EdgeLabel>>
ExprToConstraintsContext::expToConstraintsSequenceRecursive(rexp::PRExp E) {
  std::vector<std::vector<EdgeLabel>> Result;
  if (std::holds_alternative<rexp::Or>(*E)) {
    for (auto E1 : std::get<rexp::Or>(*E).E) {
      auto R1 = expToConstraintsSequenceRecursive(E1);
      Result.insert(Result.end(), R1.begin(), R1.end());
    }
  } else if (std::holds_alternative<rexp::And>(*E)) {
    auto &V = std::get<rexp::And>(*E).E;
    assert(V.size() > 0 &&
           "expToConstraintsRecursive: rexp::And with size == 0!");
    std::vector<std::vector<EdgeLabel>> Current =
        expToConstraintsSequenceRecursive(V[0]);
    std::vector<std::vector<EdgeLabel>> Result1;
    for (std::size_t Ind = 1; Ind < V.size(); Ind++) {
      auto &E1 = V[Ind];
      auto R1 = expToConstraintsSequenceRecursive(E1);
      // Cartesian product
      for (auto &C1 : Current) {
        for (auto &C2 : R1) {
          Result1.emplace_back(C1.begin(), C1.end());
          Result1.back().insert(Result1.back().end(), C2.begin(), C2.end());
        }
      }
      // Current = Result1
      std::swap(Current, Result1);
      // Result1 = empty
      Result1.clear();
    }
    return Current;
  } else if (std::holds_alternative<rexp::Star>(*E)) {
    auto Inner = std::get<rexp::Star>(*E).E;
    std::string tempName = genTempName();

    assert((*rexp::firstNode(Inner))->index() ==
               (*rexp::lastNode(Inner))->index() &&
           "Star with mixed recall and forget!");

    // Create recursive constraints.
    auto NewInner =
        rexp::create(RecallBase{TypeVariable::CreateDtv(tempName)}) & Inner &
        rexp::create(ForgetBase{TypeVariable::CreateDtv(tempName)});
    auto Seq = expToConstraintsSequenceRecursive(NewInner);
    ConstraintsSequence.insert(ConstraintsSequence.end(), Seq.begin(),
                               Seq.end());

    // Split the Rule in the middle.
    // Later in normalizePath, we will detect ForgetBase/RecallBase in the
    // middle.
    std::vector<EdgeLabel> V2;
    V2.push_back(ForgetBase{TypeVariable::CreateDtv(tempName)});
    V2.push_back(RecallBase{TypeVariable::CreateDtv(tempName)});
    Result.push_back(V2);
  } else if (std::holds_alternative<rexp::Node>(*E)) {
    std::vector<EdgeLabel> Vec;
    Vec.push_back(std::get<rexp::Node>(*E).E);
    Result.push_back(Vec);
  } else if (std::holds_alternative<rexp::Null>(*E) ||
             std::holds_alternative<rexp::Empty>(*E)) {
    assert(false && "pexp_to_constraints: Null or Empty on expression leaf! "
                    "The exp is not fully simplified?");
  }
  return Result;
}

std::vector<SubTypeConstraint>
ExprToConstraintsContext::constraintsSequenceToConstraints(
    const std::vector<std::vector<EdgeLabel>> &ConstraintsSequence) {
  std::vector<SubTypeConstraint> Ret;
  for (auto &CS : ConstraintsSequence) {
    LLVM_DEBUG(llvm::dbgs() << "Normalized path: " << toString(CS) << "\n");
    std::vector<SubTypeConstraint> Cons = normalizePath(CS);
    for (auto &Con : Cons) {
      if (Con.sub != Con.sup) {
        Ret.push_back(Con);
      }
    }
  }
  return Ret;
}

std::vector<SubTypeConstraint>
ExprToConstraintsContext::normalizePath(const std::vector<EdgeLabel> &ELs) {
  // the pexp must be in (recall _)*(forget _)*
  std::vector<SubTypeConstraint> Ret;
  if (ELs.size() < 2) {
    assert(false && "normalize_path: path size < 2!");
  }
  auto &First = ELs[0];
  if (!std::holds_alternative<RecallBase>(First)) {
    assert(false && "normalize_path: first label is not RecallBase!");
  }
  Ret.emplace_back();
  // Ret.back().sub.name = std::get<RecallBase>(First).base;
  // auto &Last = ELs[ELs.size() - 1];
  // if (!std::holds_alternative<ForgetBase>(Last)) {
  //   assert(false && "normalize_path: Last label is not ForgetBase!");
  // }
  // Ret.sup.name = std::get<ForgetBase>(Last).base;

  bool isForget = false;
  for (std::size_t Ind = 0; Ind < ELs.size(); Ind++) {
    auto &EL = ELs[Ind];
    if (std::holds_alternative<RecallBase>(EL)) {
      auto &Name = std::get<RecallBase>(EL).base;
      if (Ind == 0) {
        // OK
      } else {
        assert(Ret.back().sub.getBaseName().empty());
        assert(!Ret.back().sub.hasLabel());
        assert(Ret.back().sup.getBaseName().empty());
        assert(!Ret.back().sup.hasLabel());
      }
      Ret.back().sub = Name;
    } else if (std::holds_alternative<ForgetBase>(EL)) {
      auto &Name = std::get<ForgetBase>(EL).base;
      if (Ret.back().sup.hasLabel()) {
        Ret.back().sup.setName(Name.getBaseName());
      } else {
        Ret.back().sup = Name;
      }
      if (Ind == ELs.size() - 1) {
        // OK
      } else {
        // Must be a temp name.
        // https://stackoverflow.com/a/40441240/13798540
        assert(Name.getBaseName().rfind(tempNamePrefix, 0) == 0);
        // Reset the state.
        isForget = false;
        Ret.emplace_back();
      }
    } else if (std::holds_alternative<RecallLabel>(EL)) {
      if (isForget) {
        assert(false && "normalize_path: Path has Recall after Forget!");
      }
      Ret.back().sub.getLabels().push_back(std::get<RecallLabel>(EL).label);
    } else if (std::holds_alternative<ForgetLabel>(EL)) {
      isForget = true;
      Ret.back().sup.getLabels().push_back(std::get<ForgetLabel>(EL).label);
    } else {
      assert(false && "normalize_path: Base label type in the middle!");
    }
  }
  return Ret;
}

/// the pexp must be in (recall _)*(forget _)*
std::vector<SubTypeConstraint> expToConstraints(rexp::PRExp E) {
  if (rexp::isEmpty(E)) {
    return {};
  } else if (rexp::isNull(E)) {
    assert(false && "pexp_to_constraints: Null path!");
  }
  ExprToConstraintsContext Ctx;
  auto Seq = Ctx.expToConstraintsSequenceRecursive(E);
  Ctx.ConstraintsSequence.insert(Ctx.ConstraintsSequence.end(), Seq.begin(),
                                 Seq.end());
  return Ctx.constraintsSequenceToConstraints(Ctx.ConstraintsSequence);
}

CGNode::CGNode(ConstraintGraph &Parent, NodeKey key, unsigned int Size)
    : Parent(Parent), key(key), Size(Size), Link(Parent.SSG) {
  // Create the link in the SSG.
  if (key.Base.isPrimitive()) {
    Link.setNode(Link.Parent->createPrimitive(key.Base.getBaseName(), Size));
  } else {
    Link.setNode(Link.Parent->createUnknown(Size));
  }
}

} // namespace notdec::retypd
