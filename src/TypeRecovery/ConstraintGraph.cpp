#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/iterator_range.h>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <llvm/ADT/SCCIterator.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/NFAMinimize.h"
#include "TypeRecovery/PointerNumberIdentification.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/Sketch.h"
#include "TypeRecovery/TRContext.h"
#include "optimizers/ConstraintGenerator.h"

#define DEBUG_TYPE "retypd_graph"

namespace notdec::retypd {

CGNode &ConstraintGraph::instantiateSketch(std::shared_ptr<retypd::Sketch> Sk) {
  std::map<retypd::SketchNode *, CGNode *> NodeMap;
  // 1. clone the graph nodes
  for (auto &Node : llvm::make_range(Sk->begin(), Sk->end())) {
    auto &NewNode = getOrInsertNode(NodeKey(
        TypeVariable::CreateDtv(Ctx, ValueNamer::getName("sketch_")), Node.V));
    NodeMap.emplace(&Node, &NewNode);
  }
  // 2. clone all edges as recall edges. Also add reverse forget edges.
  for (auto &Node : llvm::make_range(Sk->begin(), Sk->end())) {
    auto *Source = NodeMap[&Node];
    for (auto &Edge : Node) {
      auto &TargetSk = const_cast<SketchNode &>(Edge.getTargetNode());
      auto *Target = NodeMap.at(&TargetSk);
      addEdge(*Source, *Target, RecallLabel{Edge.getLabel()});
      addEdge(*Target, *Source, ForgetLabel{Edge.getLabel()});
    }
  }
  return *NodeMap.at(Sk->getRoot());
}

void ConstraintGraph::replaceNodeKey(const TypeVariable &Old,
                                     const TypeVariable &New) {
  TypeVariable IC = Old;
  const CGNode &OldNode = Nodes.at(Old);
  replaceNodeKeyImpl(OldNode, New);

  // handle contra-variant nodes
  if (Nodes.count({IC, Contravariant})) {
    const CGNode &OldNode1 = Nodes.at({IC, Contravariant});
    replaceNodeKeyImpl(OldNode1, {New, Contravariant});
  }
}

void ConstraintGraph::replaceNodeKeyImpl(const CGNode &Node,
                                         const NodeKey &Key) {
  // auto &TV1 = const_cast<TypeVariable &>(TV);
  // assert(Node.key.Base.isIntConstant());
  // assert(TV.getBase() == getMemoryNode()->key.Base.getBase());
  // assert(TV1.getLabels().size() == 1);
  // assert(std::holds_alternative<OffsetLabel>(TV1.getLabels().back()));

  auto Ent = Nodes.extract(Node.key);
  const_cast<NodeKey &>(Ent.mapped().key) = Key;

  Ent.key() = Ent.mapped().key;
  Nodes.insert(std::move(Ent));
}

void CGNode::onUpdatePNType() {
  if (this->key.Base.isIntConstant()) {
    if (getPNIVar()->isPointer()) {
      // convert to offset of memory
      auto TV = Parent.getMemoryNode()->key.Base;
      auto Label = OffsetLabel{key.Base.getIntConstant()};
      TV = TV.pushLabel(Label);
      std::string name =
          "intptr_" + key.Base.getIntConstant().str().substr(1) + '_';
      TypeVariable NewTV = TypeVariable::CreateDtv(
          Parent.Ctx, ValueNamer::getName(name.c_str()));
      Parent.replaceNodeKey(this->key.Base, NewTV);
      // add subtype of memory
      Parent.addConstraint(TV, this->key.Base);
    } else if (getPNIVar()->isNonPtr()) {
      // do nothing in case of conflict
      // view as int later lazily.
    } else {
      assert(false && "onUpdatePNType: Unknown PNType");
    }
  }
}

void CGNode::setAsPtrAdd(CGNode *Other, OffsetRange Off) {
  auto TV = Other->key.Base;
  TV = TV.pushLabel(OffsetLabel{Off});
  Parent.addConstraint(TV, this->key.Base);
}

void ConstraintGraph::solve() { saturate(); }

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

void ConstraintGraph::instantiate(
    const std::vector<retypd::SubTypeConstraint> &Sum, size_t ID) {
  assert(ID > 0);
  for (auto &C : Sum) {
    auto Sub = C.sub;
    auto Sup = C.sup;
    if (Sub.hasInstanceId()) {
      Sub.setInstanceId(ID);
    }
    if (Sup.hasInstanceId()) {
      Sup.setInstanceId(ID);
    }
    addConstraint(Sub, Sup);
  }
  // TODO What about PNI
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
  auto Source = getStartNode();
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
  auto finalExp = getPexp(getStartNode(), getEndNode());
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
  auto ret = expToConstraints(Ctx, finalExp);
  // 3. delete invalid constraints like: int64.in_0
  // 4. substitute temp vars to prevent name collision.
  return ret;
}

void ConstraintGraph::linkVars(std::set<std::string> &InterestingVars) {
  // Link nodes to "#Start"
  for (auto N : StartNodes) {
    // is an interesting var or is a primitive type
    if (N->key.Base.isPrimitive() ||
        (N->key.Base.hasBaseName() &&
         InterestingVars.count(N->key.Base.getBaseName()) != 0)) {
      // LLVM_DEBUG(llvm::dbgs()
      //            << "Adding an edge from #Start to " << N->key.str() <<
      //            "\n");
      addEdge(
          *getStartNode(), *N,
          RecallBase{.Base = N->key.Base.toBase(), .V = N->key.SuffixVariance});
    }
  }
  linkEndVars(InterestingVars);
}

void ConstraintGraph::linkEndVars(std::set<std::string> &InterestingVars) {
  // Link nodes to "#End"
  for (auto N : EndNodes) {
    // is an interesting var or is a primitive type
    if (N->key.Base.isPrimitive() ||
        (N->key.Base.hasBaseName() &&
         InterestingVars.count(N->key.Base.getBaseName()) != 0)) {
      // LLVM_DEBUG(llvm::dbgs()
      //            << "Adding an edge from " << N->key.str() << " to #End\n");
      addEdge(
          *N, *getEndNode(),
          ForgetBase{.Base = N->key.Base.toBase(), .V = N->key.SuffixVariance});
    }
  }
}

ConstraintGraph ConstraintGraph::cloneAndSimplify() const {
  auto G = clone();
  return G.simplify();
}

ConstraintGraph ConstraintGraph::simplify() {
  layerSplit();

  auto G2 = minimize(this);

  // G2.printGraph("trans_before_push_split.dot");

  G2.pushSplit();
  if (const char *path = std::getenv("DEBUG_TRANS_PUSH_SPLIT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str()))) {
      G2.printGraph("trans_push_split.dot");
    }
  }

  auto G3 = minimize(&G2);

  G3.contraVariantSplit();
  if (const char *path = std::getenv("DEBUG_TRANS_CV_SPLIT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str()))) {
      G3.printGraph("trans_cv_split.dot");
    }
  }
  auto G4 = minimize(&G3);
  if (const char *path = std::getenv("DEBUG_TRANS_MIN_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str()))) {
      G4.printGraph("trans_min.dot");
    }
  }

  return G4;
}

void ConstraintGraph::contraVariantSplit() {
  markVariance();
  std::vector<CGNode *> toHandle;
  // for all contra-variant nodes
  for (auto &Ent : Nodes) {
    auto &Source = Ent.second;
    if (Source.key.SuffixVariance != Contravariant) {
      continue;
    }
    // Check if has both recall and forget edge
    bool HasRecall = false;
    bool HasForget = false;
    for (auto &Edge : Source.outEdges) {
      if (isRecall(Edge.Label)) {
        HasRecall = true;
      }
      if (isForget(Edge.Label)) {
        HasForget = true;
      }
    }
    for (auto Edge : Source.inEdges) {
      if (isRecall(Edge->Label)) {
        HasRecall = true;
      }
      if (isForget(Edge->Label)) {
        HasForget = true;
      }
    }
    if (HasRecall && HasForget) {
      toHandle.push_back(&Source);
    }
  }
  // For these nodes, split them that recall and forget are separated.
  for (auto N : toHandle) {
    // duplicate the node that isolate the recall edge.
    auto &NewNode = getOrInsertNode(
        NodeKey{TypeVariable::CreateDtv(Ctx, ValueNamer::getName("split_"))});
    // Move all incoming recall edge to the new node.
    for (auto InEdge2 : N->inEdges) {
      if (isRecall(InEdge2->Label)) {
        addEdge(InEdge2->getSourceNode(), NewNode, InEdge2->Label);
        removeEdge(InEdge2->getSourceNode(), *N, InEdge2->Label);
      }
    }
    // Move all outgoing recall edge to the new node.
    for (auto &Edge : N->outEdges) {
      if (isRecall(Edge.Label)) {
        auto &Target2 = const_cast<CGNode &>(Edge.getTargetNode());
        addEdge(NewNode, Target2, Edge.Label);
        removeEdge(*N, Target2, Edge.Label);
      }
    }
  }
}

/// Mark the variance of the node according to the edges.
/// DFS traversal.
void ConstraintGraph::markVariance() {
  std::map<CGNode *, Variance> Visited;
  std::queue<std::pair<CGNode *, Variance>> Worklist;
  Worklist.push(std::make_pair(getStartNode(), Covariant));

  while (!Worklist.empty()) {
    auto N = Worklist.front();
    Worklist.pop();
    if (Visited.count(N.first) != 0) {
      // Check if the variance is consistent.
      if (Visited[N.first] != N.second) {
        std::cerr << "Error: labelVariance: variance is inconsistent: "
                  << toString(N.first->key) << "\n";
        std::abort();
      }
      continue;
    }
    Visited.emplace(N);
    for (auto &Edge : N.first->outEdges) {
      auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      auto &Label = Edge.getLabel();
      if (std::holds_alternative<One>(Label)) {
        Worklist.push(std::make_pair(&Target, N.second));
      } else if (auto *RL = std::get_if<RecallLabel>(&Label)) {
        Worklist.push(
            std::make_pair(&Target, combine(N.second, getVariance(RL->label))));
      } else if (auto *FL = std::get_if<ForgetLabel>(&Label)) {
        Worklist.push(
            std::make_pair(&Target, combine(N.second, getVariance(FL->label))));
      } else if (auto *RB = std::get_if<RecallBase>(&Label)) {
        Worklist.push(std::make_pair(&Target, combine(N.second, RB->V)));
      } else if (auto *FB = std::get_if<ForgetBase>(&Label)) {
        // Worklist.push(std::make_pair(&Target, ));
        assert(&Target == getEndNode() && "ForgetBase should only target End");
        if (N.second != FB->V) {
          std::cerr << "Error: labelVariance: variance is inconsistent: "
                    << toString(N.first->key) << "\n";
          std::abort();
        }
      } else {
        std::cerr << "Error: labelVariance: unknown edge label: "
                  << toString(Label) << "\n";
        std::abort();
      }
    }
  }

  // Label all nodes with the variance.
  for (auto &Ent : Nodes) {
    auto &Node = Ent.second;
    if (&Node == Start || &Node == End) {
      continue;
    }
    if (Visited.count(&Node) == 0) {
      std::cerr << "Error: labelVariance: node is not visited: "
                << toString(Node.key) << "\n";
      std::abort();
    }
    // Node.key.SuffixVariance = Visited[&Node];
    // use map extract
    auto Ent2 = Nodes.extract(Node.key);
    const_cast<NodeKey &>(Ent2.mapped().key).SuffixVariance = Visited[&Node];
    Ent2.key() = Ent2.mapped().key;
    Nodes.insert(std::move(Ent2));
    // std::cerr << "Node: " << toString(Node.key)
    //           << " Variance: " << toString(Visited[&Node]) << "\n";
  }
  // std::cerr << " OK\n";

  return;
}

/// Intersect the language, that disallow recall and forget the same thing.
/// Must not have null/epsilon moves.
void ConstraintGraph::pushSplit() {
  std::vector<std::tuple<CGNode *, CGNode *, EdgeLabel>> toRemove;
  std::set<std::tuple<CGNode *, EdgeLabel, EdgeLabel>> toIsolate;

  for (auto &Ent : Nodes) {
    if (&Ent.second == Start || &Ent.second == End) {
      continue;
    }
    CGNode *Current = &Ent.second;
    for (auto InEdge : Current->inEdges) {
      if (std::holds_alternative<One>(InEdge->getLabel())) {
        std::cerr
            << __FILE__ << ":" << __LINE__ << ": "
            << " Error: pushSplit: epsilon/null move is not supported!! \n";
        std::abort();
      }
      if (isRecall(InEdge->getLabel())) {
        auto &Source = InEdge->getSourceNode();
        auto InComingRecall = InEdge->getLabel();
        // find the forget edge
        for (auto OutEdge : Current->outEdges) {
          if (isForget(OutEdge.Label) &&
              hasSameBaseOrLabel(InEdge->getLabel(), OutEdge.getLabel())) {
            auto OutGoingForget = OutEdge.Label;
            auto &Target = OutEdge.getTargetNode();
            if (false) {
              std::cerr << "Removing a path from " << toString(Source.key)
                        << " " << toString(InComingRecall) << " to "
                        << toString(Current->key) << " "
                        << toString(OutGoingForget) << " to "
                        << toString(Target.key) << "\n";
            }
            if (Current->inEdges.size() == 1) {
              // Remove the forget edge.
              toRemove.emplace_back(Current, &Target, OutGoingForget);
            } else if (Current->outEdges.size() == 1) {
              // Remove the recall edge.
              toRemove.emplace_back(&Source, Current, InComingRecall);
            } else {
              toIsolate.emplace(Current, InComingRecall, OutGoingForget);
            }
          }
        }
      }
    }
  }

  for (auto &Ent : toRemove) {
    auto [Source, Target, Label] = Ent;
    removeEdge(*Source, *Target, Label);
  }
  for (auto &Ent : toIsolate) {
    auto [Current, Recall, Forget] = Ent;
    // duplicate the node that isolate the incoming recall edge.
    auto &NewNode = getOrInsertNode(
        NodeKey{TypeVariable::CreateDtv(Ctx, ValueNamer::getName("split_"))});
    // copy all out edges except the forget edge.
    for (auto &Edge : Current->outEdges) {
      auto &Target2 = const_cast<CGNode &>(Edge.getTargetNode());
      if (Edge.Label != Forget) {
        addEdge(NewNode, Target2, Edge.getLabel());
      }
    }
    // Move all same recall edge to the new node.
    for (auto InEdge2 : Current->inEdges) {
      if (InEdge2->getLabel() == Recall) {
        addEdge(InEdge2->getSourceNode(), NewNode, InEdge2->getLabel());
        removeEdge(InEdge2->getSourceNode(), *Current, InEdge2->getLabel());
      }
    }
  }
}

// Simplify graph and build path expr.
std::vector<SubTypeConstraint>
ConstraintGraph::simplifiedExpr(std::set<std::string> &InterestingVars) const {
  // TODO: eliminate this clone by removing Start and End nodes later.
  auto G = clone();
  G.linkVars(InterestingVars);
  auto G2 = G.simplify();

  G2.buildPathSequence();
  return G2.solve_constraints_between();
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
      LLVM_DEBUG(llvm::dbgs()
                 << "Path Sequence for SCC: " << toString(SCC) << ":\n");
      for (auto &Seq : Seqs) {
        LLVM_DEBUG(llvm::dbgs() << "  From " << toString(std::get<0>(Seq)->key)
                                << " To " << toString(std::get<1>(Seq)->key)
                                << ": " << toString(std::get<2>(Seq)) << "\n");
      }
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

/// Intersect the language with recall*forget*.
/// Support null/epsilon moves.
void ConstraintGraph::layerSplit() {
  // using ND = NFADeterminizer<>;
  assert(!isLayerSplit && "layerSplit: already split!?");
  isLayerSplit = true;

  // 1. eliminate null move/epsilon edges
  // Depricated 20240831 because 1 too much edge is added. 2 we need to link
  // start & end first, or you can not infer subtype relation between
  // top-level vars. for (auto &Ent : Nodes) {
  //   auto &Source = Ent.second;
  //   std::set<CGNode *> T = {&Source};
  //   auto SourceClosure = ND::countClosure(T);
  //   std::set<EdgeLabel> outLabels = ND::allOutLabels(SourceClosure);
  //   for (auto &Edge : outLabels) {
  //     if (std::holds_alternative<One>(Edge)) {
  //       continue;
  //     }
  //     auto T2 = ND::move(SourceClosure, Edge);
  //     auto NewTargets = ND::countClosure(T2);
  //     for (auto Target : NewTargets) {
  //       addEdge(Source, *Target, Edge);
  //     }
  //   }
  // }

  // if (const char *path = std::getenv("DEBUG_TRANS_NULL_ELIM1_GRAPH")) {
  //   if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str())))
  //   {
  //     printGraph("trans_nullelim1.dot");
  //   }
  // }

  // // remove all null edges
  // for (auto &Ent : Nodes) {
  //   auto &Source = Ent.second;
  //   for (auto &Edge : Source.outEdges) {
  //     auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
  //     if (std::holds_alternative<One>(Edge.Label)) {
  //       removeEdge(Source, Target, One{});
  //     }
  //   }
  // }

  // if (const char *path = std::getenv("DEBUG_TRANS_NULL_ELIM_GRAPH")) {
  //   if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str())))
  //   {
  //     printGraph("trans_nullelim.dot");
  //   }
  // }

  // 2. perform layer split
  std::vector<std::tuple<CGNode *, CGNode *, CGNode *, EdgeLabel>> toChange;
  for (auto &Ent : Nodes) {
    // Do not copy edges from start.
    if (Ent.second.key.IsNewLayer || &Ent.second == Start) {
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
      if (isRecall(Edge.Label)) {
        continue;
      }
      // Copy the non-Recall edge to the new layer.
      addEdge(NewSrcNode, NewDstNode, Edge.Label);
      // Update the Forget edge to target the new layer.
      if (isForget(Edge.Label)) {
        // Edge reference will be invalidated. Edge changes is deferred.
        toChange.emplace_back(&Source, &Target, &NewDstNode, Edge.Label);
      }
    }
  }
  // deferred Edge modification
  for (auto &Ent : toChange) {
    auto [Source, Target, NewDst, Label] = Ent;
    // LLVM_DEBUG(llvm::dbgs()
    //            << "layerSplit: Retarget Edge from " << toString(Source->key)
    //            << " to " << toString(Target->key) << " with " <<
    //            toString(Label)
    //            << " to " << toString(NewDst->key) << "\n");
    removeEdge(*Source, *Target, Label);
    addEdge(*Source, *NewDst, Label);
  }

  // Update end nodes to nodes in the new layer.
  for (CGNode *Ent : EndNodes) {
    if (Ent->key.IsNewLayer) {
      continue;
    }
    NodeKey NewKey = Ent->key;
    NewKey.IsNewLayer = true;
    auto &NewNode = getOrInsertNode(NewKey);
    EndNodes.insert(&NewNode);
  }
  // remove old end nodes that is not in the new layer.
  for (auto it = EndNodes.begin(); it != EndNodes.end();) {
    if (!(*it)->key.IsNewLayer) {
      it = EndNodes.erase(it);
    } else {
      it++;
    }
  }
  // Update the end node to the new layer.
  if (End != nullptr) {
    NodeKey NewKey = End->key;
    NewKey.IsNewLayer = true;
    auto &NewNode = getOrInsertNode(NewKey);
    End = &NewNode;
  }

  if (const char *path = std::getenv("DEBUG_TRANS_LAYER_SPLIT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str()))) {
      printGraph("trans_layerSplit.dot");
    }
  }
}

std::shared_ptr<Sketch> ConstraintGraph::solveSketch(CGNode &N) const {
  // 1 clone the graph
  ConstraintGraph G = clone();

  // 2. add recall edge to the node.
  G.addEdge(*G.getStartNode(), G.getOrInsertNode(N.key, N.Size),
            RecallBase{.Base = N.key.Base, .V = N.key.SuffixVariance});

  // G.printGraph("sketches1.dot");
  // 3. make all nodes accepting. focus on the recall subgraph, but allow recall
  // base primitive.
  for (auto &Ent : G.Nodes) {
    auto &Source = Ent.second;
    // 3.1 remove all forget label edge
    for (auto &Edge : Source.outEdges) {
      auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      if (std::holds_alternative<ForgetLabel>(Edge.Label)) {
        G.removeEdge(Source, Target, Edge.Label);
      }
    }
    if (&Source == G.Start || &Source == G.End) {
      continue;
    }
    // 3.2 if the node has not forget prim edge, add one to top.
    bool HasForgetPrim = false;
    for (auto &Edge : Source.outEdges) {
      if (std::holds_alternative<ForgetBase>(Edge.Label)) {
        if (std::get<ForgetBase>(Edge.Label).Base.isPrimitive()) {
          HasForgetPrim = true;
          break;
        }
      }
    }
    if (!HasForgetPrim) {
      G.addEdge(Source, *G.getEndNode(),
                ForgetBase{.Base = TypeVariable::CreatePrimitive(Ctx, "top"),
                           .V = Source.key.SuffixVariance});
    }
  }
  // 4. Link vars from primitives to #End.
  std::set<std::string> Vars = {};
  G.linkEndVars(Vars);

  // 5. solve the sketch
  auto G2 = G.simplify();
  // G2.printGraph("before-sketch.dot");

  G2.markVariance();
  auto Sk = Sketch::fromConstraintGraph(G2, Name + "-" + N.key.str());
  // Sk->printGraph("sketch.dot");
  return Sk;
}

// utility function for debugging
[[nodiscard]] std::string
toString(const std::set<std::pair<FieldLabel, CGNode *>> &S) {
  std::string ret = "{";
  for (auto &Ent : S) {
    ret += "(" + toString(Ent.first) + ", " + Ent.second->key.str() + "), ";
  }
  ret += "}";
  return ret;
}

/// Algorithm D.2 Saturation algorithm
void ConstraintGraph::saturate() {
  bool Changed = true;
  // solve PNI first.
  if (PG) {
    PG->solve();
  }
  // Initial reaching push set is maintained during edge insertion.
  // 1. add forget edges to reaching set
  // for (auto &Ent : Nodes) {
  //   auto &Source = Ent.second;
  //   for (auto &Edge : Source.outEdges) {
  //     // For each edge, check if is forget edge.
  //     if (std::holds_alternative<ForgetLabel>(Edge.Label)) {
  //       auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
  //       auto Capa = std::get<ForgetLabel>(Edge.Label);
  //       auto Res = ReachingSet[&Target].insert({
  //           Capa.label,
  //           &Source,
  //       });
  //       Changed |= Res.second;
  //     }
  //   }
  // }

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
            // LLVM_DEBUG(llvm::dbgs()
            //            << "node " << Reach.second->key.str() << " can reach "
            //            << Ent.first->key.str() << " with \".store\" \n");
            Label = LoadLabel{.Size = std::get<StoreLabel>(Reach.first).Size};
          } else if (std::holds_alternative<LoadLabel>(Reach.first)) {
            // LLVM_DEBUG(llvm::dbgs()
            //            << "node " << Reach.second->key.str() << " can reach "
            //            << Ent.first->key.str() << " with \".load\" \n");
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
    // Run PNI solving again.
    if (PG) {
      Changed |= PG->solve();
    }
  }

  if (const char *path = std::getenv("DEBUG_TRANS_SAT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str()))) {
      printGraph("trans_sat.dot");
    }
  }
}

ConstraintGraph
ConstraintGraph::fromConstraints(TRContext &Ctx, std::string FuncName,
                                 std::vector<Constraint> &Cons) {
  ConstraintGraph G(Ctx, FuncName);
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
  if (&NodeL != &NodeR) {
    addEdge(NodeL, NodeR, One{});
  }
  // 2. add each sub var node and edges.
  // 2.1 left
  addRecalls(NodeL);
  addForgets(NodeL);
  // 2.2 right
  addForgets(NodeR);
  addRecalls(NodeR);

  // 3-4 the inverse of the above
  // 3. inverse node and 1-labeled edge
  auto &RNodeL = getOrInsertNode(NodeKey(sub, Contravariant));
  auto &RNodeR = getOrInsertNode(NodeKey(sup, Contravariant));
  // add 1-labeled edge between them
  addEdge(RNodeR, RNodeL, One{});
  // 4.1 inverse left
  addRecalls(RNodeL);
  addForgets(RNodeL);
  // 4.2 inverse right
  addForgets(RNodeR);
  addRecalls(RNodeR);
}

std::optional<std::pair<FieldLabel, NodeKey>> NodeKey::forgetOnce() const {
  if (!Base.hasLabel()) {
    return std::nullopt;
  }
  NodeKey NewKey(*this);
  FieldLabel Label = NewKey.Base.getLabels().back();
  NewKey.Base = NewKey.Base.popLabel();
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
  // if (N.key.SuffixVariance == Covariant) {
  StartNodes.insert(T);
  // }
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
  // if (N.key.SuffixVariance == Covariant) {
  EndNodes.insert(T);
  // }
}

CGNode &ConstraintGraph::getOrInsertNode(const NodeKey &N, unsigned int Size) {
  auto [it, inserted] = Nodes.try_emplace(N, *this, N, Size);
  return it->second;
}

void ConstraintGraph::printGraph(const char *DotFile) const {
  std::error_code EC;
  llvm::raw_fd_ostream OutStream(DotFile, EC);
  if (EC) {
    llvm::errs() << "Error: " << EC.message() << "\n";
    return;
  }
  llvm::WriteGraph(OutStream, const_cast<ConstraintGraph *>(this), false);
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

std::string toString(const std::set<CGNode *> Set) {
  std::string ret;
  for (auto &N : Set) {
    ret += N->key.str() + ",";
  }
  return "{" + ret + "}";
}

struct ExprToConstraintsContext {
  TRContext &Ctx;
  static const char *tempNamePrefix;
  int TempNameIndex = 0;
  // std::function<std::string()> &tempNameGenerator;
  std::string genTempName() {
    return tempNamePrefix + std::to_string(TempNameIndex++);
  }
  std::vector<std::vector<EdgeLabel>> ConstraintsSequence;

  std::vector<std::vector<EdgeLabel>>
  expToConstraintsSequenceRecursive(rexp::PRExp E);
  static std::pair<
      std::vector<std::tuple<std::pair<PooledTypeVariable, PooledTypeVariable>,
                             Variance, Variance>>,
      Variance>
  normalizePath(const std::vector<EdgeLabel> &ELs);
  static std::vector<SubTypeConstraint> constraintsSequenceToConstraints(
      TRContext &Ctx,
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
        rexp::create(RecallBase{TypeVariable::CreateDtv(Ctx, tempName)}) &
        Inner &
        rexp::create(ForgetBase{TypeVariable::CreateDtv(Ctx, tempName)});
    auto Seq = expToConstraintsSequenceRecursive(NewInner);
    ConstraintsSequence.insert(ConstraintsSequence.end(), Seq.begin(),
                               Seq.end());

    // Split the Rule in the middle.
    // Later in normalizePath, we will detect ForgetBase/RecallBase in the
    // middle.
    std::vector<EdgeLabel> V2;
    V2.push_back(ForgetBase{TypeVariable::CreateDtv(Ctx, tempName)});
    V2.push_back(RecallBase{TypeVariable::CreateDtv(Ctx, tempName)});
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
    TRContext &Ctx,
    const std::vector<std::vector<EdgeLabel>> &ConstraintsSequence) {
  std::vector<SubTypeConstraint> Ret;
  for (auto &CS : ConstraintsSequence) {
    LLVM_DEBUG(llvm::dbgs() << "Normalized path: " << toString(CS) << "\n");

    std::pair<std::vector<
                  std::tuple<std::pair<PooledTypeVariable, PooledTypeVariable>,
                             Variance, Variance>>,
              Variance>
        P = normalizePath(CS);
    auto &Cons = P.first;

    std::cerr << ((P.second == Covariant) ? "" : "Skipped ") << "Cons: \n";
    for (auto &Con : Cons) {
      auto &TVPair = std::get<0>(Con);
      std::cerr << "  " << toString(&TVPair.first) << toString(std::get<1>(Con))
                << " <= " << toString(&TVPair.second)
                << toString(std::get<2>(Con)) << "\n";
      // assert(std::get<1>(Con) == std::get<2>(Con));
      if (std::get<1>(Con) != std::get<2>(Con)) {
        std::cerr << "Warning:!\n";
      }
      TVPair.first.Ctx = &Ctx;
      TVPair.second.Ctx = &Ctx;
      if (P.second == Covariant && TVPair.first != TVPair.second) {
        Ret.push_back(SubTypeConstraint{
            {&Ctx, PooledTypeVariable::intern(Ctx, TVPair.first)},
            {&Ctx, PooledTypeVariable::intern(Ctx, TVPair.second)}});
      }
    }
  }
  return Ret;
}

std::pair<
    std::vector<std::tuple<std::pair<PooledTypeVariable, PooledTypeVariable>,
                           Variance, Variance>>,
    Variance>
ExprToConstraintsContext::normalizePath(const std::vector<EdgeLabel> &ELs) {
  // The variance between recalls and forgets.
  std::optional<Variance> PathVariance;
  // the pexp must be in (recall _)*(forget _)*
  std::vector<std::tuple<std::pair<PooledTypeVariable, PooledTypeVariable>,
                         Variance, Variance>>
      Ret;
  if (ELs.size() < 2) {
    assert(false && "normalize_path: path size < 2!");
  }
  auto &First = ELs[0];
  if (!std::holds_alternative<RecallBase>(First)) {
    assert(false && "normalize_path: first label is not RecallBase!");
  }
  Ret.emplace_back();
  auto *Back = &std::get<0>(Ret.back());

  bool isForget = false;
  for (std::size_t Ind = 0; Ind < ELs.size(); Ind++) {
    auto &EL = ELs[Ind];
    if (std::holds_alternative<RecallBase>(EL)) {
      auto Name = std::get<RecallBase>(EL).Base;
      if (Ind == 0) {
        // OK
        std::get<1>(Ret.back()) = std::get<RecallBase>(EL).V;
      } else {
        assert(Back->first.getBaseName().empty());
        assert(!Back->first.hasLabel());
        assert(Back->second.getBaseName().empty());
        assert(!Back->second.hasLabel());
      }
      Back->first = *Name.Var;
    } else if (std::holds_alternative<ForgetBase>(EL)) {
      auto Name = std::get<ForgetBase>(EL).Base;
      if (Back->second.hasLabel()) {
        Back->second.setBase(Name.getBase());
      } else {
        Back->second = *Name.Var;
      }
      if (Ind == ELs.size() - 1) {
        // Collect the variance between recalls and forgets.
        if (!isForget) {
          assert(!PathVariance);
          PathVariance =
              combine(std::get<1>(Ret.back()), Back->first.pathVariance());
        }
        isForget = true;
        // OK
        std::get<2>(Ret.back()) = std::get<ForgetBase>(EL).V;
      } else {
        // Must be a temp name.
        // https://stackoverflow.com/a/40441240/13798540
        assert(Name.getBaseName().rfind(tempNamePrefix, 0) == 0);
        // Compute current variance.
        Variance Current =
            combine(std::get<1>(Ret.back()), Back->first.pathVariance());
        Current = combine(Current, Back->second.pathVariance());
        std::get<2>(Ret.back()) = Current;

        Ret.emplace_back();
        Back = &std::get<0>(Ret.back());
        std::get<1>(Ret.back()) = Current;
      }
    } else if (std::holds_alternative<RecallLabel>(EL)) {
      if (isForget) {
        assert(false && "normalize_path: Path has Recall after Forget!");
      }
      Back->first.getLabels().push_back(std::get<RecallLabel>(EL).label);
    } else if (std::holds_alternative<ForgetLabel>(EL)) {
      // Collect the variance between recalls and forgets.
      if (!isForget) {
        assert(!PathVariance);
        PathVariance =
            combine(std::get<1>(Ret.back()), Back->first.pathVariance());
      }
      isForget = true;
      Back->second.getLabels().push_front(std::get<ForgetLabel>(EL).label);
    } else {
      assert(false && "normalize_path: Base label type in the middle!");
    }
  }

  // combine the variance with label variance.
  for (auto &Ent : Ret) {
    auto &Con = std::get<0>(Ent);
    std::get<1>(Ent) = combine(std::get<1>(Ent), Con.first.pathVariance());
    std::get<2>(Ent) = combine(std::get<2>(Ent), Con.second.pathVariance());
  }
  return std::make_pair(Ret, PathVariance.value());
}

/// the pexp must be in (recall _)*(forget _)*
std::vector<SubTypeConstraint> expToConstraints(TRContext &TRCtx,
                                                rexp::PRExp E) {
  if (rexp::isEmpty(E)) {
    assert(false && "pexp_to_constraints: Empty path!");
    // return {};
  } else if (rexp::isNull(E)) {
    return {};
    // assert(false && "pexp_to_constraints: Null path!");
  }
  ExprToConstraintsContext Ctx{TRCtx};
  auto Seq = Ctx.expToConstraintsSequenceRecursive(E);
  Ctx.ConstraintsSequence.insert(Ctx.ConstraintsSequence.end(), Seq.begin(),
                                 Seq.end());
  return Ctx.constraintsSequenceToConstraints(TRCtx, Ctx.ConstraintsSequence);
}

CGNode *ConstraintGraph::getMemoryNode() {
  if (MemoryNode != nullptr) {
    return MemoryNode;
  }
  MemoryNode = &getOrInsertNode(NodeKey{TypeVariable::CreateDtv(Ctx, Memory)});
  if (PG) {
    setPointer(*MemoryNode);
  }
  return MemoryNode;
}

CGNode *ConstraintGraph::getStartNode() {
  if (Start != nullptr) {
    return Start;
  }
  Start =
      &getOrInsertNode(NodeKey{TypeVariable::CreatePrimitive(Ctx, "#Start")});
  return Start;
}

CGNode *ConstraintGraph::getEndNode() {
  if (End != nullptr) {
    return End;
  }
  End = &getOrInsertNode(NodeKey{TypeVariable::CreatePrimitive(Ctx, "#End")});
  return End;
}

ConstraintGraph::ConstraintGraph(TRContext &Ctx, std::string Name,
                                 bool disablePNI)
    : Ctx(Ctx), Name(Name) {
  if (!disablePNI) {
    PG = std::make_unique<PNIGraph>(Name);
  }
}

ConstraintGraph ConstraintGraph::clone(bool removePNI) const {
  std::map<const CGNode *, CGNode *> Old2New;
  // loses CG pointer when cloning.
  ConstraintGraph G(Ctx, Name, (!(bool)PG) || removePNI);

  // clone all nodes
  for (auto &Ent : Nodes) {
    auto &Node = Ent.second;
    auto &NewNode = G.getOrInsertNode(Node.key, Node.Size);
    auto Pair = Old2New.emplace(&Node, &NewNode);
    assert(Pair.second && "clone: Node already cloned!?");
  }

  // clone all edges
  for (auto &Ent : Nodes) {
    auto &Node = Ent.second;
    auto NewNode = Old2New.at(&Node);
    for (auto &Edge : Node.outEdges) {
      auto &Target = Edge.getTargetNode();
      auto NewTarget = Old2New.at(&Target);
      G.onlyAddEdge(*NewNode, *NewTarget, Edge.Label);
    }
  }

  // clone PNI Graph
  if (PG && !removePNI) {
    G.PG = std::make_unique<PNIGraph>(PG->Name);
    G.PG->cloneFrom(*PG, Old2New);
  }

  // handle fields
  G.isLayerSplit = isLayerSplit;
  if (MemoryNode) {
    G.MemoryNode = Old2New.at(MemoryNode);
  }
  if (Start) {
    G.Start = Old2New.at(Start);
  }
  if (End) {
    G.End = Old2New.at(End);
  }
  for (auto &N : StartNodes) {
    G.StartNodes.insert(Old2New.at(N));
  }
  for (auto &N : EndNodes) {
    G.EndNodes.insert(Old2New.at(N));
  }

  return G;
}

CGNode::CGNode(ConstraintGraph &Parent, NodeKey key, unsigned int Size)
    : Parent(Parent), key(key), Size(Size), PNIVar(nullptr) {
  if (Parent.PG) {
    // for contravariant/new layer, we reuse the PNINode.
    if (key.SuffixVariance == Covariant && key.IsNewLayer == false) {
      this->PNIVar = Parent.PG->createPNINode(this);
    } else {
      auto NewKey = key;
      NewKey.SuffixVariance = Covariant;
      NewKey.IsNewLayer = false;
      this->PNIVar = Parent.getOrInsertNode(NewKey).getPNIVar();
      this->PNIVar->addUser(this);
    }

    if (key.Base.isPrimitive()) {
      PNIVar->setPtrOrNum(NonPtr);
    }
  }
}

} // namespace notdec::retypd
