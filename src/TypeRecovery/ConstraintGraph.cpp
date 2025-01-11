#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Type.h>
#include <map>
#include <memory>
#include <optional>
#include <queue>
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
#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/NFAMinimize.h"
#include "TypeRecovery/PointerNumberIdentification.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/Sketch.h"
#include "TypeRecovery/TRContext.h"
#include "Utils/Range.h"
#include "optimizers/ConstraintGenerator.h"
#include "utils.h"

#define DEBUG_TYPE "retypd_graph"

namespace notdec::retypd {

llvm::Optional<std::pair<bool, OffsetRange>>
EdgeLabel2Offset(const EdgeLabel &E) {
  if (auto OL = std::get_if<ForgetLabel>(&E)) {
    if (auto OL2 = std::get_if<OffsetLabel>(&OL->label)) {
      return std::make_pair(false, OL2->range);
    }
  }
  if (auto OL = std::get_if<RecallLabel>(&E)) {
    if (auto OL2 = std::get_if<OffsetLabel>(&OL->label)) {
      return std::make_pair(true, OL2->range);
    }
  }
  return llvm::None;
};

void ConstraintGraph::lowTypeToSubType() {
  // Check the low type of each node.
  // If non-PNI related, add subtype relation.
  // if number, add subtype relation.
  // if Unknown/pointer, TODO do nothing for now.
  // Also check for existing subtypes. Skip if already exists.
  std::set<CGNode *> Visited;
  for (auto &Ent : Nodes) {
    auto &Node = Ent.second;
    if (Node.key.Base.isPrimitive()) {
      continue;
    }
    if (Visited.count(&Node)) {
      continue;
    }
    if (Node.getPNIVar() == nullptr || Node.getPNIVar()->isNull()) {
      continue;
    }
    if (Node.getPNIVar()->isUnknown() || Node.getPNIVar()->isPointer()) {
      continue;
    }
    std::string LowTyStr = fromLLVMType(Node.getLowTy());
    if (startswith(LowTyStr, "int")) {
      // TODO: if already has sint or uint subtype, skip.
    }
    auto &Prim = getOrInsertNodeWithPNI(
        NodeKey(TypeVariable::CreatePrimitive(Ctx, LowTyStr),
                Node.key.SuffixVariance),
        Node.getPNIVar());
    StartNodes.insert(&Prim);
    EndNodes.insert(&Prim);
    Visited.insert(&Node);
    if (Node.key.SuffixVariance == Covariant) {
      onlyAddEdge(Node, Prim, One{});
    } else {
      onlyAddEdge(Prim, Node, One{});
    }
  }
}

void ConstraintGraph::mergeNodeTo(CGNode &From, CGNode &To, bool NoSelfLoop) {
  assert(&From.Parent == this && &To.Parent == this);
  // Move all edges from From to To
  for (auto &Edge : From.outEdges) {
    auto *Target = &Edge.TargetNode;
    // for self loop
    if (Target == &To || Target == &From) {
      // only keep non-one edge
      if (!std::holds_alternative<retypd::One>(Edge.Label)) {
        assert(!NoSelfLoop);
        onlyAddEdge(To, To, Edge.Label);
      }
      removeEdge(From, *Target, Edge.Label);
    } else {
      onlyAddEdge(To, Edge.TargetNode, Edge.Label);
      removeEdge(From, Edge.TargetNode, Edge.Label);
    }
  }
  for (auto *Edge : From.inEdges) {
    auto *Source = &Edge->getSourceNode();
    if (Source == &From)
      assert(false); // should be removed
    if (Source == &To) {
      // only keep non-one edge
      if (!std::holds_alternative<retypd::One>(Edge->Label)) {
        assert(!NoSelfLoop);
        onlyAddEdge(To, To, Edge->Label);
      }
      removeEdge(To, From, Edge->getLabel());
    } else {
      onlyAddEdge(Edge->getSourceNode(), To, Edge->getLabel());
      removeEdge(Edge->getSourceNode(), From, Edge->getLabel());
    }
  }
  // erase the node
  removeNode(From.key);
}

void ConstraintGraph::aggressiveSimplify() {
  // do not include path with size 8.
  // 1. remove all load8/store8 edges.
  // for (auto &Ent : Nodes) {
  //   auto &Node = Ent.second;
  //   std::vector<const CGEdge *> toRemove;
  //   for (auto &Edge : Node.outEdges) {
  //     if (auto Rec = std::get_if<RecallLabel>(&Edge.getLabel())) {
  //       if (std::holds_alternative<LoadLabel>(Rec->label) &&
  //           std::get<LoadLabel>(Rec->label).Size == 8) {
  //         toRemove.push_back(&Edge);
  //       } else if (std::holds_alternative<StoreLabel>(Rec->label) &&
  //                  std::get<StoreLabel>(Rec->label).Size == 8) {
  //         toRemove.push_back(&Edge);
  //       }
  //     }
  //     if (auto Rec = std::get_if<ForgetLabel>(&Edge.getLabel())) {
  //       if (std::holds_alternative<LoadLabel>(Rec->label) &&
  //           std::get<LoadLabel>(Rec->label).Size == 8) {
  //         toRemove.push_back(&Edge);
  //       } else if (std::holds_alternative<StoreLabel>(Rec->label) &&
  //                  std::get<StoreLabel>(Rec->label).Size == 8) {
  //         toRemove.push_back(&Edge);
  //       }
  //     }
  //   }
  //   for (auto Edge : toRemove) {
  //     removeEdge(Node, const_cast<CGNode &>(Edge->getTargetNode()),
  //                Edge->getLabel());
  //   }
  // }

  // for each node's offset edges.
  // for (auto &Ent : Nodes) {
  //   auto &Node = Ent.second;
  //   std::vector<std::pair<std::pair<bool, OffsetRange>, CGEdge *>>
  //       outOffsetEdges;
  //   for (auto &Edge : Node.outEdges) {
  //     auto O1 = EdgeLabel2Offset(Edge.getLabel());
  //     if (O1.hasValue()) {
  //       outOffsetEdges.emplace_back(*O1, &Edge);
  //     }
  //   }
  //   for (auto &Ent : outOffsetEdges) {
  //     auto &[O1, Edge] = Ent;
  //     if (O1.second.has1x()) {
  //       int64_t Start = O1.first ? O1.second.offset : (-O1.second.offset);
  //       int64_t End = Start + 4;
  //       std::cerr << "ConstraintGraph::aggressiveSimplify: Merge Off Edges "
  //                 << toString(Node.key) << toString(Edge->getLabel())
  //                 << " from offset " << Start << " To " << End << "\n";
  //       for (auto &Ent2 : outOffsetEdges) {
  //         if (Ent2.second == Edge) {
  //           continue;
  //         }
  //         auto &[O2, Edge2] = Ent2;
  //         OffsetRange Adj = O2.first ? O2.second : (-O2.second);
  //         if (Adj.offset >= Start && Adj.offset < End) {
  //           if (Adj.access.empty()) {
  //             // merge
  //             std::cerr
  //                 << "ConstraintGraph::aggressiveSimplify: Merge Off Edges "
  //                 << toString(Node.key) << toString(Edge2->getLabel())
  //                 << " from offset " << Adj.offset << " To " << End << "\n";
  //             // remove the edge
  //             removeEdge(Node, Edge2->getTargetNode(), Edge2->getLabel());
  //             // merge node to target.
  //             mergeNodeTo(Edge2->getTargetNode(), Edge->getTargetNode());
  //           } else if (Adj.has1x()) {
  //           }
  //         }
  //       }
  //     }
  //   }
  // }
}

// CGNode &ConstraintGraph::instantiateSketch(std::shared_ptr<retypd::Sketch>
// Sk) {
//   std::map<retypd::SketchNode *, CGNode *> NodeMap;
//   // 1. clone the graph nodes
//   for (auto &Node : llvm::make_range(Sk->begin(), Sk->end())) {
//     auto &NewNode = getOrInsertNode(NodeKey(
//         TypeVariable::CreateDtv(Ctx, ValueNamer::getName("sketch_")),
//         Node.V));
//     NodeMap.emplace(&Node, &NewNode);
//   }
//   // 2. clone all edges as recall edges. Also add reverse forget edges.
//   for (auto &Node : llvm::make_range(Sk->begin(), Sk->end())) {
//     auto *Source = NodeMap[&Node];
//     for (auto &Edge : Node) {
//       auto &TargetSk = const_cast<SketchNode &>(Edge.getTargetNode());
//       auto *Target = NodeMap.at(&TargetSk);
//       addEdge(*Source, *Target, RecallLabel{Edge.getLabel()});
//       addEdge(*Target, *Source, ForgetLabel{Edge.getLabel()});
//     }
//   }
//   return *NodeMap.at(Sk->getRoot());
// }

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
      // keep PNI untouched. Lowtype is still int.
      // this->setPointer();
      // Fix links with memory node.
      Parent.addConstraint(*this, *this);
    } else if (getPNIVar()->isNumber()) {
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
  // 这里没有插入反向PNI的图？
  auto &PtrAdd = Parent.getOrInsertNodeWithPNI(TV, Other->getPNIVar());
  assert(PtrAdd.getPNIVar() == Other->getPNIVar());
  Parent.addConstraint(PtrAdd, *this);
  assert(PtrAdd.getPNIVar() == this->getPNIVar());
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
    // TODO set PtrOrNum.
    if (LLCtx) {
      if (Sub.isPrimitive()) {
        if (auto LowTy = ToLLVMType(*LLCtx, Sub.getBaseName())) {
          assert(!Sup.isPrimitive());
          auto &SupNode = getOrInsertNode(Sup, nullptr);
          SupNode.getPNIVar()->updateLowTy(LowTy);
          continue;
        }
      } else if (Sup.isPrimitive()) {
        if (auto LowTy = ToLLVMType(*LLCtx, Sup.getBaseName())) {
          assert(!Sub.isPrimitive());
          auto &SubNode = getOrInsertNode(Sub, nullptr);
          SubNode.getPNIVar()->updateLowTy(LowTy);
          continue;
        }
      }
    }

    addConstraint(getOrInsertNode(Sub, nullptr), getOrInsertNode(Sup, nullptr));
  }
  // TODO What about PNI
}

std::vector<SubTypeConstraint> ConstraintGraph::solve_constraints_between() {

  std::map<std::pair<CGNode *, CGNode *>, rexp::PRExp> P =
      rexp::solve_constraints(getStartNode(), PathSeq);

  auto getPexp = [&](CGNode *From, CGNode *To) -> rexp::PRExp {
    auto Key = std::make_pair(From, To);
    if (P.count(Key)) {
      return P[Key];
    }
    if (From == To) {
      return rexp::createEmpty();
    } else {
      return rexp::createNull();
    }
  };
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
  std::map<const CGNode *, CGNode *> Old2New;
  auto G = clone(Old2New);
  return G.simplify();
}

ConstraintGraph ConstraintGraph::simplify() {
  layerSplit();

  // auto G2 = minimize(this);

  // // G2.printGraph("trans_before_push_split.dot");

  // G2.pushSplit();
  // if (const char *path = std::getenv("DEBUG_TRANS_PUSH_SPLIT_GRAPH")) {
  //   if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str()))) {
  //     G2.printGraph("trans_push_split.dot");
  //   }
  // }

  auto G3 = minimize(this);

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
    auto &NewNode = createNodeWithPNI(
        NodeKey{TypeVariable::CreateDtv(Ctx, ValueNamer::getName("split_"))},
        N->getPNIVar());
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

void ConstraintGraph::linkContraToCovariant() {
  // add subtype edge from Contravariant nodes to Covariant nodes
  for (auto &Ent : Nodes) {
    if (&Ent.second == getStartNode() || &Ent.second == getEndNode()) {
      continue;
    }
    if (Ent.second.key.Base.isPrimitive()) {
      continue;
    }
    if (Ent.second.key.SuffixVariance == retypd::Contravariant) {
      continue;
    }
    auto *Node = &Ent.second;
    if (Nodes.count(MakeContraVariant(Node->key)) != 0) {
      auto &CN = Nodes.at(MakeContraVariant(Node->key));
      assert(CN.getPNIVar() == Node->getPNIVar());
      addEdge(CN, *Node, retypd::One{});
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
    auto &NewNode = createNodeWithPNI(
        NodeKey{TypeVariable::CreateDtv(Ctx, ValueNamer::getName("split_"))},
        Current->getPNIVar());
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
  std::map<const CGNode *, CGNode *> Old2New;
  auto G = clone(Old2New);
  G.lowTypeToSubType();
  G.linkVars(InterestingVars);
  auto G2 = G.simplify();
  G2.aggressiveSimplify();

  G2.buildPathSequence();
  return G2.solve_constraints_between();
}

// bool -> isRecall
OffsetRange
mergeOffsetLabels(const std::vector<std::pair<bool, OffsetRange>> &V) {
  OffsetRange Val;
  std::map<OffsetRange, int64_t> Counts;
  for (auto &Ent : V) {
    if (Ent.first) {
      Counts[Ent.second] += 1;
    } else {
      Counts[Ent.second] -= 1;
    }
  }
  for (auto &Ent : Counts) {
    while (Ent.second > 0) {
      Val = Val + Ent.first;
      Ent.second--;
    }
    while (Ent.second < 0) {
      Val = Val + (-Ent.first);
      Ent.second++;
    }
  }
  return Val;
}

std::set<OffsetRange> calcOffset(rexp::PRExp E) {
  if (auto *Empty = std::get_if<rexp::Empty>(&*E)) {
    return {OffsetRange()};
  }
  std::function<std::vector<std::vector<std::pair<bool, OffsetRange>>>(
      rexp::PRExp)>
      FlattenRec = [&](rexp::PRExp E)
      -> std::vector<std::vector<std::pair<bool, OffsetRange>>> {
    std::vector<std::vector<std::pair<bool, OffsetRange>>> Ret;
    if (auto *Null = std::get_if<rexp::Null>(&*E)) {
      assert(false);
    } else if (auto *Empty = std::get_if<rexp::Empty>(&*E)) {
      return {{{false, OffsetRange()}}};
    } else if (auto *Or = std::get_if<rexp::Or>(&*E)) {
      for (auto &Inner : Or->E) {
        auto R = FlattenRec(Inner);
        Ret.insert(Ret.end(), R.begin(), R.end());
      }
    } else if (auto *Star = std::get_if<rexp::Star>(&*E)) {
      auto Rs = calcOffset(Star->E);
      Ret.emplace_back();
      for (auto R : Rs) {
        auto R1 = R.mulx();
        Ret.back().push_back({true, R1});
      }
    } else if (auto *And = std::get_if<rexp::And>(&*E)) {
      // try to eliminate the forget and recall node.
      // first bool: true for recall.
      std::vector<std::pair<bool, std::set<OffsetRange>>> Recalls;
      for (auto &Inner : And->E) {
        if (auto *N1 = std::get_if<rexp::Node>(&*Inner)) {
          // handle forget offset and recall offset
          if (auto *RL = std::get_if<RecallLabel>(&N1->E)) {
            if (auto OL1 = std::get_if<OffsetLabel>(&RL->label)) {
              Recalls.push_back({true, {OL1->range}});
            }
          } else if (auto *FL = std::get_if<ForgetLabel>(&N1->E)) {
            if (auto OL2 = std::get_if<OffsetLabel>(&FL->label)) {
              Recalls.push_back({false, {OL2->range}});
            }
          }
        } else {
          auto R = calcOffset(Inner);
          Recalls.push_back({true, R});
        }
      }
      // for each possible combination (Cartesian product)
      // note: this creates a vector containing one empty vector
      std::vector<std::vector<std::pair<bool, OffsetRange>>> Results = {{}};
      for (const auto &Ents : Recalls) {
        // helper vector so that we don't modify results while iterating
        std::vector<std::vector<std::pair<bool, OffsetRange>>> next_results;
        for (const auto &result : Results) {
          auto [isRecall, NewValues] = Ents;
          for (const auto &value : NewValues) {
            next_results.push_back(result);
            next_results.back().push_back({isRecall, value});
          }
        }
        Results = std::move(next_results);
      }
      Ret = std::move(Results);
    } else if (auto *Node = std::get_if<rexp::Node>(&*E)) {
      // handle forget offset and recall offset
      if (auto *RL = std::get_if<RecallLabel>(&Node->E)) {
        if (auto OL1 = std::get_if<OffsetLabel>(&RL->label)) {
          Ret.push_back({{true, OL1->range}});
        }
        return Ret;
      } else if (auto *FL = std::get_if<ForgetLabel>(&Node->E)) {
        if (auto OL2 = std::get_if<OffsetLabel>(&FL->label)) {
          Ret.push_back({{false, OL2->range}});
        }
        return Ret;
      }
      std::cerr << "Error: calcOffset: unknown Node: " << toString(Node->E)
                << "\n";
      std::abort();
    } else {
      std::cerr << "Error: calcOffset: unknown PRExp: " << toString(E) << "\n";
      std::abort();
    }
    assert(!Ret.empty());
    return Ret;
  };
  std::set<OffsetRange> Ret;
  auto Results = FlattenRec(E);

  // for each recall, check if there is a forget, and eliminate them.
  for (auto &Result : Results) {
    OffsetRange Val = mergeOffsetLabels(Result);
    Ret.insert(Val);
  }
  return Ret;
}

bool isOffsetRelated(CGNode &Start) {
  // fast path, if there is no outgoing or incoming offset edge, only consider
  // one edge.
  bool HasOffset = false;
  // Outgoing
  for (auto &Edge : Start.outEdges) {
    if (auto EL1 = std::get_if<RecallLabel>(&Edge.Label)) {
      if (std::holds_alternative<OffsetLabel>(EL1->label)) {
        HasOffset = true;
        break;
      }
    } else if (auto EL2 = std::get_if<ForgetLabel>(&Edge.Label)) {
      if (std::holds_alternative<OffsetLabel>(EL2->label)) {
        HasOffset = true;
        break;
      }
    }
  }
  // Because of symmetrical, there must be outgoing offset if there is incoming
  // offset.
  return HasOffset;
}

std::string toString(const std::set<OffsetRange> &S) {
  std::string Ret = "{";
  for (auto &R : S) {
    Ret += R.str() + ", ";
  }
  return Ret + "}";
};

std::set<std::pair<CGNode *, OffsetRange>>
ConstraintGraph::getNodeReachableOffset(CGNode &Start) {
  std::set<std::pair<CGNode *, OffsetRange>> Ret;
  // fast path, if there is no outgoing or incoming offset edge, only consider
  // one edge.
  bool HasOffset = isOffsetRelated(Start);

  if (!HasOffset) {
    // only check for outgoing one edge
    for (auto &Edge : Start.outEdges) {
      if (std::holds_alternative<One>(Edge.Label)) {
        auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
        Ret.emplace(&Target, OffsetRange());
      }
    }
    return Ret;
  }
  // Has at least one offset edge.
  std::vector<std::tuple<CGNode *, CGNode *, rexp::PRExp>> PathSeq;

  // Focus on the offset edge and one edge, first build pathseq by iterating the
  // SCC.
  using GT = llvm::GraphTraits<OffsetOnly<CGNode *>>;
  auto convertSet = [&](const std::set<OffsetOnly<CGNode *>> &Set) {
    std::set<CGNode *> Ret;
    for (auto N : Set) {
      Ret.insert(N);
    }
    return Ret;
  };
  std::vector<std::set<OffsetOnly<CGNode *>>> SCCs;
  for (auto I = llvm::scc_begin(OffsetOnly<CGNode *>(&Start)); !I.isAtEnd();
       ++I) {
    auto &SCC = *I;
    assert(SCC.size() > 0);
    SCCs.emplace_back(SCC.begin(), SCC.end());
  }
  for (auto &SCC : llvm::reverse(SCCs)) {
    // run eliminate for each SCC
    if (SCC.size() > 1) {
      // Cache the path sequence for each SCC.
      if (ElimCache.count(SCC) == 0) {
        auto Seqs = rexp::eliminate(SCC);
        LLVM_DEBUG(llvm::dbgs() << "(offonly) PathSeq for SCC: "
                                << toString(convertSet(SCC)) << ":\n");
        for (auto &Seq : Seqs) {
          LLVM_DEBUG(llvm::dbgs()
                     << "  From " << toString(std::get<0>(Seq).Graph->key)
                     << " To " << toString(std::get<1>(Seq).Graph->key) << ": "
                     << toString(std::get<2>(Seq)) << "\n");
        }
        ElimCache[SCC] = Seqs;
      } else {
        auto &Seqs = ElimCache[SCC];
        PathSeq.insert(PathSeq.end(), Seqs.begin(), Seqs.end());
      }
    }
    // Add all edges out of the current SCC to the path sequence.
    for (auto N : SCC) {
      for (auto E :
           llvm::make_range(GT::child_edge_begin(N), GT::child_edge_end(N))) {
        assert(isOffsetOrOne(*E));
        auto &Target = const_cast<CGNode &>(E->getTargetNode());
        // Add the edge to the path sequence.
        if (SCC.count(&Target) == 0) {
          PathSeq.emplace_back(N, &Target, rexp::create(E->getLabel()));
        }
      }
    }
  }

  // Use the PathSeq to build the reachable offset.
  std::map<std::pair<CGNode *, CGNode *>, rexp::PRExp> P =
      rexp::solve_constraints(&Start, PathSeq);
  LLVM_DEBUG(llvm::dbgs() << "Final result for " << toString(Start.key)
                          << " : \n");
  for (auto &Ent : P) {
    auto StartN = Ent.first.first;
    auto TargetN = Ent.first.second;
    if (StartN != &Start) {
      continue;
    }
    if (StartN == TargetN) {
      continue;
    }

    auto &Exp = Ent.second;
    std::set<OffsetRange> Calced = calcOffset(Exp);
    // debug print
    LLVM_DEBUG(llvm::dbgs()
               << "  can reach " << toString(TargetN->key) << " With "
               << toString(Exp) << ": " << toString(Calced) << "\n");
    for (auto &R : Calced) {
      Ret.emplace(TargetN, R);
    }
  }
  return Ret;
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
      auto Seqs = rexp::eliminate(SCC);
      LLVM_DEBUG(llvm::dbgs()
                 << "Path Sequence for SCC: " << toString(SCC) << ":\n");
      for (auto &Seq : Seqs) {
        LLVM_DEBUG(llvm::dbgs() << "  From " << toString(std::get<0>(Seq)->key)
                                << " To " << toString(std::get<1>(Seq)->key)
                                << ": " << toString(std::get<2>(Seq)) << "\n");
      }
      PathSeq.insert(PathSeq.end(), Seqs.begin(), Seqs.end());
    } else {
      // if has self loop, add a star to the path sequence.
      auto N = *SCC.begin();
      std::set<rexp::PRExp> OrInner;
      // auto Or = std::make_shared<rexp::RExp>(rexp::Or{});
      for (auto &Edge : N->outEdges) {
        if (&Edge.getTargetNode() == N) {
          OrInner.insert(rexp::create(Edge.Label));
        }
      }
      if (OrInner.size() > 0) {
        auto Or = std::make_shared<rexp::RExp>(rexp::Or{.E = OrInner});
        PathSeq.emplace_back(N, N,
                             std::make_shared<rexp::RExp>(rexp::Star{.E = Or}));
      }
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

  std::map<const CGNode *, CGNode *> Old2New;

  // 1. clone all nodes to new layer
  for (auto &Ent : Nodes) {
    // Do not copy edges from start.
    if (Ent.second.key.IsNewLayer || &Ent.second == Start) {
      continue;
    }
    auto &Source = Ent.second;
    NodeKey NewSrc = Source.key;
    NewSrc.IsNewLayer = true;
    auto &New = createNodeWithPNI(NewSrc, Source.getPNIVar());
    Old2New.insert({&Source, &New});
  }

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
      auto &NewSrcNode = getNode(NewSrc);
      NodeKey NewDst = Target.key;
      NewDst.IsNewLayer = true;
      auto &NewDstNode = getNode(NewDst);
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
    auto &NewNode = getNode(NewKey);
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
    auto &NewNode = getNode(NewKey);
    End = &NewNode;
  }

  if (const char *path = std::getenv("DEBUG_TRANS_LAYER_SPLIT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str()))) {
      printGraph("trans_layerSplit.dot");
    }
  }
}

void ConstraintGraph::sketchSplit() {
  // 1. make all nodes accepting. focus on the recall subgraph, but allow recall
  // base primitive.
  for (auto &Ent : Nodes) {
    auto &Source = Ent.second;
    // 1.1 remove all forget label edge
    for (auto &Edge : Source.outEdges) {
      auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      if (std::holds_alternative<ForgetLabel>(Edge.Label)) {
        removeEdge(Source, Target, Edge.Label);
      }
    }
    if (&Source == Start || &Source == End) {
      continue;
    }
    // 1.2 if the node has not forget prim edge, add one to top.
    // bool HasForgetPrim = false;
    // for (auto &Edge : Source.outEdges) {
    //   if (std::holds_alternative<ForgetBase>(Edge.Label)) {
    //     if (std::get<ForgetBase>(Edge.Label).Base.isPrimitive()) {
    //       HasForgetPrim = true;
    //       break;
    //     }
    //   }
    // }
    // if (!HasForgetPrim && !Source.key.Base.isPrimitive()) {
    //   addEdge(Source, *getEndNode(),
    //           ForgetBase{.Base = TypeVariable::CreatePrimitive(Ctx, "top"),
    //                      .V = Source.key.SuffixVariance});
    // }
  }
  // 2. Link vars from primitives to #End.
  std::set<std::string> Vars = {};
  linkEndVars(Vars);

  // // 3. remove edge that is parent type of primitives.
  // for (auto &Ent : Nodes) {
  //   if (&Ent.second == Start || &Ent.second == End) {
  //     continue;
  //   }
  //   if (Ent.second.key.Base.isPrimitive()) {
  //     assert(!Ent.second.key.Base.hasLabel());
  //     std::vector<CGEdge *> toRemove;
  //     for (auto &Edge : Ent.second.outEdges) {
  //       if (std::holds_alternative<One>(Edge.Label)) {
  //         toRemove.push_back(const_cast<CGEdge *>(&Edge));
  //       }
  //     }
  //     for (auto Edge : toRemove) {
  //       removeEdge(Edge->getSourceNode(), Edge->getTargetNode(),
  //       Edge->Label);
  //     }
  //   }
  // }
}

// std::shared_ptr<Sketch> ConstraintGraph::solveSketch(CGNode &N) const {
//   assert(&N.Parent == this && "solveSketch: node is not in the graph");
//   // 1 clone the graph
//   std::map<const CGNode *, CGNode *> Old2New;
//   ConstraintGraph G = clone(Old2New);

//   // 2. add recall edge to the node.
//   G.addEdge(*G.getStartNode(), G.getOrInsertNode(N.key, N.Size),
//             RecallBase{.Base = N.key.Base, .V = N.key.SuffixVariance});

//   // G.printGraph("sketches1.dot");
//   // 3. focus on the recall subgraph.
//   G.sketchSplit();

//   // 4. solve the sketch
//   auto G2 = G.simplify();
//   // G2.printGraph("before-sketch.dot");

//   G2.markVariance();
//   auto Sk = Sketch::fromConstraintGraph(G2, Name + "-" + N.key.str());
//   // Sk->printGraph("sketch.dot");
//   return Sk;
// }

// void ConstraintGraph::solveSketchQueries(
//     const std::vector<std::pair<
//         TypeVariable, std::function<void(std::shared_ptr<retypd::Sketch>)>>>
//         &Queries) const {
//   // 1 clone the graph
//   std::map<const CGNode *, CGNode *> Old2New;
//   ConstraintGraph G = clone(Old2New);

//   // 2. add recall edges to these nodes.
//   assert(G.getStartNode()->outEdges.empty() && "solveSketchQueries: start
//   node "
//                                                "should not have any out
//                                                edges");
//   for (auto &Ent : Queries) {
//     auto &N = G.getOrInsertNode(Ent.first);
//     assert(N.key.SuffixVariance == Covariant);
//     G.addEdge(*G.getStartNode(), N,
//               RecallBase{.Base = N.key.Base, .V = N.key.SuffixVariance});
//   }

//   // 3. focus on the recall subgraph.
//   G.sketchSplit();

//   // 4. solve the sketch
//   auto G2 = G.simplify();

//   G2.printGraph("sketch-forest.dot");

//   // 5. for each query, relink the start edges.
//   for (auto &Ent : Queries) {
//     std::map<const CGNode *, CGNode *> Old2New;
//     auto G3 = G2.clone(Old2New);
//     std::vector<std::tuple<CGNode *, CGNode *, EdgeLabel>> ToRemove;
//     bool found = false;
//     for (auto &Edge : G3.getStartNode()->outEdges) {
//       auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
//       if (auto *RB = std::get_if<RecallBase>(&Edge.Label)) {
//         if (RB->Base == Ent.first) {
//           found = true;
//           continue;
//         }
//         // G3.removeEdge(*G3.getStartNode(), Target, Edge.Label);
//         ToRemove.emplace_back(G3.getStartNode(), &Target, Edge.Label);
//       }
//     }
//     assert(found); // must found because all nodes are accepting
//     for (auto &Ent : ToRemove) {
//       auto [Source, Target, Label] = Ent;
//       G3.removeEdge(*Source, *Target, Label);
//     }
//     assert(G3.getStartNode()->outEdges.size() == 1);
//     auto G4 = G3.simplify();
//     G4.markVariance();
//     auto Sk = Sketch::fromConstraintGraph(G4, Name + "-" + Ent.first.str());
//     Ent.second(Sk);
//   }
// }

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
  bool DenseSubtype = false;
  if (const char *val = std::getenv("NOTDEC_SAT_DENSESUBTYPE")) {
    if ((std::strcmp(val, "1") == 0)) {
      DenseSubtype = true;
    }
  }

  bool Changed = true;
  // solve PNI first.
  if (PG) {
    PG->solve();
  }
  // Initial reaching push set is maintained during edge insertion.
  // 1. add forget edges to reaching set
  for (auto &Ent : Nodes) {
    auto &Source = Ent.second;
    for (auto &Edge : Source.outEdges) {
      // For each edge, check if is forget edge.
      if (std::holds_alternative<ForgetLabel>(Edge.Label)) {
        auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
        auto Capa = std::get<ForgetLabel>(Edge.Label);
        if (DenseSubtype) {
          if (!std::holds_alternative<OffsetLabel>(Capa.label)) {
            auto Res = ReachingSet[&Target].insert({Capa.label, &Source});
            Changed |= Res.second;
          }
        } else {
          auto Res = ReachingSet[&Target].insert({Capa.label, &Source});
          Changed |= Res.second;
        }
      }
    }
  }

  while (Changed) {
    Changed = false;
    // For each edge, if it is One edge, add reaching set.
    for (auto &Ent : Nodes) {
      auto &Source = Ent.second;

      if (DenseSubtype) {
        // Add transitive subtypeing and cancel out offset edges.
        if (isOffsetRelated(Source)) {
          auto Re = getNodeReachableOffset(Source);
          for (auto &Ent : Re) {
            if (Ent.first == &Source) {
              continue;
            }
            if (Ent.second.isZero()) {
              bool Added = addEdge(Source, *Ent.first, One{});
              if (Added) {
                LLVM_DEBUG(llvm::dbgs()
                           << "(Off) Adding Edge From " << Source.key.str()
                           << " to " << Ent.first->key.str() << " with _1_ \n");
                Changed |= Added;
              }
            }
          }
        }
      }

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
                // LLVM_DEBUG(llvm::dbgs()
                //            << "Adding Edge From " << Reach.second->key.str()
                //            << " to " << Target.key.str() << " with _1_ \n");
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
          auto &OppositeNode = getNode(Opposite);
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

ConstraintGraph ConstraintGraph::fromConstraints(TRContext &Ctx,
                                                 std::string FuncName,
                                                 std::vector<Constraint> &Cons,
                                                 long PointerSize) {
  ConstraintGraph G(Ctx, nullptr, PointerSize, FuncName, true);
  for (auto &C : Cons) {
    if (std::holds_alternative<SubTypeConstraint>(C)) {
      auto &SCons = std::get<SubTypeConstraint>(C);
      G.addConstraint(G.getOrInsertNode(SCons.sub, nullptr),
                      G.getOrInsertNode(SCons.sup, nullptr));
    } else {
      assert(false && "buildInitialGraph: Unimplemented constraint type!");
    }
  }
  return G;
}

/// Interface for initial constraint insertion
/// Also build the initial graph (Algorithm D.1 Transducer)
void ConstraintGraph::addConstraint(CGNode &NodeL, CGNode &NodeR) {
  assert(&NodeL.Parent == this && "addConstraint: NodeL is not in the graph");
  assert(&NodeR.Parent == this && "addConstraint: NodeR is not in the graph");
  // 1. add 1-labeled edge between them
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
  auto &RNodeL = getOrInsertNodeWithPNI(NodeKey(NodeL.key.Base, Contravariant),
                                        NodeL.getPNIVar());
  auto &RNodeR = getOrInsertNodeWithPNI(NodeKey(NodeR.key.Base, Contravariant),
                                        NodeR.getPNIVar());
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
    auto &NNext = getOrInsertNode(Next, nullptr);
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
    auto &NNext = getOrInsertNode(Next, nullptr);
    addEdge(*T, NNext, ForgetLabel{Cap});
    V1 = Next.forgetOnce();
    T = &NNext;
  }
  // We do not really link the node to #Start
  // if (N.key.SuffixVariance == Covariant) {
  EndNodes.insert(T);
  // }
}

CGNode &ConstraintGraph::getOrInsertNodeWithPNI(const NodeKey &N, PNINode *PN) {
  if (Nodes.count(N) == 0) {
    return createNodeWithPNI(N, PN);
  } else {
    return getNode(N);
  }
}

CGNode &ConstraintGraph::getOrInsertNode(const NodeKey &N,
                                         llvm::Type *LowType) {
  auto [it, inserted] = Nodes.try_emplace(N, *this, N, (llvm::Type *)nullptr);
  if (!inserted) {
    if (LowType != nullptr) {
      it->second.getPNIVar()->updateLowTy(LowType);
    }
  }
  return it->second;
}

bool ConstraintGraph::hasNode(const NodeKey &N) { return Nodes.count(N) > 0; }
CGNode &ConstraintGraph::getNode(const NodeKey &N) { return Nodes.at(N); }
CGNode *ConstraintGraph::getNodeOrNull(const NodeKey &N) {
  if (Nodes.count(N)) {
    return &Nodes.at(N);
  }
  return nullptr;
}

std::pair<std::map<NodeKey, CGNode>::iterator, bool>
ConstraintGraph::emplace(const NodeKey &N, llvm::Type *LowType) {
  return Nodes.emplace(std::piecewise_construct, std::forward_as_tuple(N),
                       std::forward_as_tuple(*this, N, LowType));
}

CGNode &ConstraintGraph::createNode(const NodeKey &N, llvm::Type *LowType) {
  auto It = emplace(N, LowType);
  assert(It.second && "createNode failed: node already exists");
  return It.first->second;
}

CGNode &ConstraintGraph::createNodeWithPNI(const NodeKey &N, PNINode *PNI) {
  auto It = Nodes.emplace(std::piecewise_construct, std::forward_as_tuple(N),
                          std::forward_as_tuple(*this, N, PNI));
  assert(It.second && "createNode failed: node already exists");
  return It.first->second;
}

CGNode &ConstraintGraph::createNodeClonePNI(const NodeKey &N, PNINode *ON) {
  CGNode *NewN;
  assert(PG != nullptr);
  if (ON != nullptr) {
    NewN = &createNodeWithPNI(N, nullptr);
    auto PN = PG->clonePNINode(*ON);
    PNIGraph::addPNINodeTarget(*NewN, *PN);
  } else {
    NewN = &createNode(N, nullptr);
  }
  return *NewN;
}

CGNode &ConstraintGraph::cloneNode(const CGNode &Other) {
  assert(&Other.Parent != this);
  auto &N = createNodeWithPNI(Other.key, nullptr);
  return N;
}

void ConstraintGraph::removeNode(const NodeKey &N) {
  assert(Nodes.count(N) && "removeNode: node not found");
  auto &Node = Nodes.at(N);
  assert(!isSpecialNode(Node));
  assert(Node.outEdges.empty() && "removeNode: node has out edges");
  assert(Node.inEdges.empty() && "removeNode: node has in edges");
  PG->markRemoved(Node);
  if (StartNodes.count(&Node) != 0) {
    StartNodes.erase(&Node);
  }
  if (EndNodes.count(&Node) != 0) {
    EndNodes.erase(&Node);
  }
  Nodes.erase(N);
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

void ConstraintGraph::printEpsilonLoop(const char *DotPrefix,
                                       std::set<const CGNode *> Nodes) const {
  ConstraintGraph Temp = getSubGraph(Nodes, false);
  int i = 0;
  for (scc_iterator<ConstraintGraph *> I = scc_begin(&Temp); !I.isAtEnd();
       ++I) {
    const std::vector<notdec::retypd::CGNode *> &SCC = *I;
    if (SCC.size() > 1) {
      std::set<const CGNode *> SCCSet;
      for (auto *N : SCC) {
        SCCSet.insert(N);
      }
      ConstraintGraph Temp1 = getSubGraph(SCCSet, false);
      Temp1.printGraph(
          (std::string(DotPrefix) + std::to_string(++i) + ".dot").c_str());
    }
  }
  std::cerr << "printed " << i << " epsilon SCCs.\n";
}

std::set<const CGNode *>
ConstraintGraph::getReachableNodes(std::set<const CGNode *> Initial) const {
  std::set<const CGNode *> ReachableNodes;
  std::queue<const CGNode *> Worklist;
  for (auto *Node : Initial) {
    assert(&Node->Parent == this && "printSubGraph: node is not in the graph");
    Worklist.push(Node);
  }
  while (!Worklist.empty()) {
    auto *Node = Worklist.front();
    if (ReachableNodes.count(Node) == 0) {
      ReachableNodes.insert(Node);
      for (auto &Edge : Node->outEdges) {
        Worklist.push(&const_cast<CGNode &>(Edge.getTargetNode()));
      }
    }
    Worklist.pop();
  }
  return ReachableNodes;
}

ConstraintGraph
ConstraintGraph::getSubGraph(const std::set<const CGNode *> &Roots,
                             bool AllReachable) const {
  ConstraintGraph Temp(Ctx, LLCtx, PointerSize, this->Name, true);

  std::set<const CGNode *> ReachableNodes;
  if (AllReachable) {
    ReachableNodes = getReachableNodes(Roots);
  } else {
    ReachableNodes = Roots;
  }

  std::map<const CGNode *, CGNode *> Old2New;

  // clone all nodes
  for (auto *Node : ReachableNodes) {
    auto &NewNode = Temp.cloneNode(*Node);
    auto Pair = Old2New.insert({Node, &NewNode});
    assert(Pair.second && "clone: Node already cloned!?");
  }

  for (auto &Ent : Nodes) {
    if (ReachableNodes.count(&Ent.second) != 0) {
      auto &NewNode = Temp.getNode(Ent.second.key);
      for (auto &Edge : Ent.second.outEdges) {
        if (ReachableNodes.count(&Edge.getTargetNode()) != 0) {
          auto &Target = Temp.getNode(Edge.getTargetNode().key);
          Temp.onlyAddEdge(NewNode, Target, Edge.Label);
        } else {
          if (AllReachable) {
            // not possible
            assert(false);
          }
        }
      }
    }
  }

  return Temp;
}

void ConstraintGraph::printSubGraph(const char *DotFile,
                                    std::set<const CGNode *> Roots) const {
  ConstraintGraph Temp = getSubGraph(Roots, true);

  Temp.printGraph(DotFile);
}

void ConstraintGraph::printSubGraph(const char *DotFile,
                                    const CGNode *Root) const {
  std::set<const CGNode *> Roots = {Root};
  printSubGraph(DotFile, Roots);
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

    // try to simplify adjacent Recall/Forget and merge adjacent offset edges.
    auto Simplify = [&](std::vector<EdgeLabel> &V) {
      bool updated = false;
      std::vector<EdgeLabel> NewV;
      for (size_t Ind2 = 0; Ind2 < V.size(); Ind2++) {
        if (Ind2 + 1 < V.size()) {
          auto &E1 = V[Ind2];
          auto &E2 = V[Ind2 + 1];
          // simplify adjacent Recall/Forget
          if (auto RL = std::get_if<RecallLabel>(&E1)) {
            if (auto FL = std::get_if<ForgetLabel>(&E2)) {
              if (RL->label == FL->label) {
                // skip these two elements
                Ind2++;
                continue;
              }
            }
          }
          if (auto RL = std::get_if<RecallBase>(&E1)) {
            if (auto FL = std::get_if<ForgetBase>(&E2)) {
              if (RL->Base == FL->Base) {
                // skip these two elements
                Ind2++;
                continue;
              }
            }
          }
          // eliminate Forget+Recall the same offset.
          if (auto RL = std::get_if<ForgetLabel>(&E1)) {
            if (auto FL = std::get_if<RecallLabel>(&E2)) {
              if (std::holds_alternative<OffsetLabel>(RL->label) &&
                  std::holds_alternative<OffsetLabel>(FL->label)) {
                if (std::get<OffsetLabel>(RL->label).range ==
                    std::get<OffsetLabel>(FL->label).range) {
                  // skip these two elements
                  Ind2++;
                  continue;
                }
              }
            }
          }
        }
        NewV.push_back(V[Ind2]);
      }
      if (NewV.size() < V.size()) {
        LLVM_DEBUG(llvm::dbgs() << "Simplified: From " << toString(V) << " to "
                                << toString(NewV) << "\n");
        V = NewV;
        updated = true;
      }
      return updated;
    };
    auto SimplifyOffset = [&](std::vector<EdgeLabel> &V) {
      bool updated = false;
      std::vector<EdgeLabel> NewV;
      std::vector<EdgeLabel> OffsetBufferL;
      std::vector<std::pair<bool, OffsetRange>> OffsetBuffer;
      for (size_t Ind2 = 0; Ind2 < V.size(); Ind2++) {
        auto &E1 = V[Ind2];
        auto O1 = EdgeLabel2Offset(E1);
        if (O1.hasValue()) {
          OffsetBuffer.push_back(*O1);
          OffsetBufferL.push_back(E1);
          continue;
        } else {
          if (OffsetBuffer.size() > 1) {
            // merge the offset edges.
            OffsetRange R = mergeOffsetLabels(OffsetBuffer);
            if (OffsetBuffer.front().first) {
              NewV.push_back(RecallLabel{OffsetLabel{R}});
            } else {
              NewV.push_back(ForgetLabel{OffsetLabel{-R}});
            }
          } else if (OffsetBuffer.size() == 1) {
            NewV.push_back(OffsetBufferL[0]);
          }
          OffsetBuffer.clear();
          OffsetBufferL.clear();
        }
        NewV.push_back(V[Ind2]);
      }
      if (!OffsetBufferL.empty()) {
        if (OffsetBuffer.size() > 1) {
          // merge the offset edges.
          OffsetRange R = mergeOffsetLabels(OffsetBuffer);
          if (OffsetBuffer.front().first) {
            NewV.push_back(RecallLabel{OffsetLabel{R}});
          } else {
            NewV.push_back(ForgetLabel{OffsetLabel{-R}});
          }
        } else if (OffsetBuffer.size() == 1) {
          NewV.push_back(OffsetBufferL[0]);
        }
        OffsetBuffer.clear();
        OffsetBufferL.clear();
      }
      if (NewV.size() < V.size()) {
        LLVM_DEBUG(llvm::dbgs() << "Simplified: From " << toString(V) << " to "
                                << toString(NewV) << "\n");
        V = NewV;
        updated = true;
      }
      return updated;
    };
    for (size_t Ind = 0; Ind < Current.size(); Ind++) {
      std::vector<EdgeLabel> &V = Current[Ind];
      do {
      } while (Simplify(V));
      SimplifyOffset(V);
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
  std::set<SubTypeConstraint> Ret;
  for (auto &CS : ConstraintsSequence) {
    LLVM_DEBUG(llvm::dbgs() << "Normalized path: " << toString(CS) << "\n");

    std::pair<std::vector<
                  std::tuple<std::pair<PooledTypeVariable, PooledTypeVariable>,
                             Variance, Variance>>,
              Variance>
        P = normalizePath(CS);
    auto &Cons = P.first;

    // std::cerr << ((P.second == Covariant) ? "" : "Skipped ") << "Cons: \n";
    for (auto &Con : Cons) {
      auto &TVPair = std::get<0>(Con);
      // std::cerr << "  " << toString(&TVPair.first) <<
      // toString(std::get<1>(Con))
      //           << " <= " << toString(&TVPair.second)
      //           << toString(std::get<2>(Con)) << "\n";
      // assert(std::get<1>(Con) == std::get<2>(Con));
      // if (std::get<1>(Con) != std::get<2>(Con)) {
      //   std::cerr << "Warning:!\n";
      // }
      TVPair.first.Ctx = &Ctx;
      TVPair.second.Ctx = &Ctx;
      assert(P.second == Covariant);
      if (TVPair.first != TVPair.second) {
        if (TVPair.first.isPrimitive() && TVPair.second.isPrimitive()) {
          LLVM_DEBUG(llvm::dbgs() << "Inferred subtype between primitives: "
                                  << toString(&TVPair.first) << " <= "
                                  << toString(&TVPair.second) << "\n");
          continue;
        }
        Ret.insert(SubTypeConstraint{
            {&Ctx, PooledTypeVariable::intern(Ctx, TVPair.first)},
            {&Ctx, PooledTypeVariable::intern(Ctx, TVPair.second)}});
      }
    }
  }
  std::vector<SubTypeConstraint> RetVec;
  for (auto &Ent : Ret) {
    RetVec.push_back(Ent);
  }
  return RetVec;
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
  if (ELs.empty()) {
    return std::make_pair(Ret, Covariant);
  }
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
  MemoryNode =
      &createNode(NodeKey{TypeVariable::CreateDtv(Ctx, Memory)}, nullptr);
  if (PG) {
    setPointer(*MemoryNode);
  }
  return MemoryNode;
}

CGNode *ConstraintGraph::getStartNode() {
  if (Start != nullptr) {
    return Start;
  }
  Start = &createNode(NodeKey{TypeVariable::CreatePrimitive(Ctx, "#Start")},
                      nullptr);
  return Start;
}

CGNode *ConstraintGraph::getEndNode() {
  if (End != nullptr) {
    return End;
  }
  End =
      &createNode(NodeKey{TypeVariable::CreatePrimitive(Ctx, "#End")}, nullptr);
  return End;
}

ConstraintGraph::ConstraintGraph(TRContext &Ctx, llvm::LLVMContext *LLCtx,
                                 long PointerSize, std::string Name,
                                 bool disablePNI)
    : Ctx(Ctx), LLCtx(LLCtx), Name(Name), PointerSize(PointerSize) {
  if (!disablePNI) {
    assert(LLCtx != nullptr);
    PG = std::make_unique<PNIGraph>(*this, *LLCtx, Name, PointerSize);
  }
}

void ConstraintGraph::clone(std::map<const CGNode *, CGNode *> &Old2New,
                            const ConstraintGraph &From, ConstraintGraph &To,
                            bool removePNI) {
  assert(Old2New.empty() && "clone: Old2New is not empty!");
  assert(&From.Ctx == &To.Ctx && "clone: Ctx mismatch!");
  // loses CG pointer when cloning.
  removePNI = (!(bool)From.PG) || removePNI;
  To.Name = From.Name;

  // clone all nodes
  for (auto &Ent : From.Nodes) {
    auto &Node = Ent.second;
    auto &NewNode = To.createNodeWithPNI(Node.key, (PNINode *)nullptr);
    auto Pair = Old2New.insert({&Node, &NewNode});
    assert(Pair.second && "clone: Node already cloned!?");
  }

  // clone all edges
  for (auto &Ent : From.Nodes) {
    auto &Node = Ent.second;
    auto NewNode = Old2New.at(&Node);
    for (auto &Edge : Node.outEdges) {
      auto &Target = Edge.getTargetNode();
      auto NewTarget = Old2New.at(&Target);
      To.onlyAddEdge(*NewNode, *NewTarget, Edge.Label);
    }
  }

  // clone PNI Graph
  if (From.PG && !removePNI) {
    assert(From.LLCtx != nullptr);
    To.PG = std::make_unique<PNIGraph>(To, *From.LLCtx, From.PG->Name,
                                       From.PG->PointerSize);
    To.PG->cloneFrom(*From.PG, Old2New);
  }

  // handle fields
  To.isLayerSplit = From.isLayerSplit;
  To.isSketchSplit = From.isSketchSplit;
  if (From.MemoryNode) {
    To.MemoryNode = Old2New.at(From.MemoryNode);
  }
  if (From.Start) {
    To.Start = Old2New.at(From.Start);
  }
  if (From.End) {
    To.End = Old2New.at(From.End);
  }
  for (auto &N : From.StartNodes) {
    To.StartNodes.insert(Old2New.at(N));
  }
  for (auto &N : From.EndNodes) {
    To.EndNodes.insert(Old2New.at(N));
  }
}

ConstraintGraph
ConstraintGraph::clone(std::map<const CGNode *, CGNode *> &Old2New,
                       bool removePNI) const {
  ConstraintGraph G(Ctx, LLCtx, PointerSize, Name, (!(bool)PG) || removePNI);
  clone(Old2New, *this, G, removePNI);
  return G;
}

CGNode::CGNode(ConstraintGraph &Parent, NodeKey key, llvm::Type *LowTy)
    : Parent(Parent), key(key) {
  if (Parent.PG) {
    if (Parent.Nodes.count(MakeReverseVariant(key))) {
      PNIGraph::addPNINodeTarget(
          *this, *Parent.Nodes.at(MakeReverseVariant(key)).PNIVar);
    } else {
      PNIVar = Parent.PG->createPNINode(this, LowTy);
    }
  }
}

CGNode::CGNode(ConstraintGraph &Parent, NodeKey key, PNINode *N)
    : Parent(Parent), key(key), PNIVar(nullptr) {
  if (N != nullptr) {
    assert(Parent.PG != nullptr);
  }
  if (Parent.PG) {
    if (N != nullptr) {
      assert(N->getParent() == Parent.PG.get());
      PNIGraph::addPNINodeTarget(*this, *N);
    } else if (Parent.Nodes.count(MakeReverseVariant(key))) {
      PNIVar = Parent.Nodes.at(MakeReverseVariant(key)).PNIVar;
    }
    // // try to reuse the PNINode as possible: other variance or new layer.
    // // for new layer node, non-new-layer node must exist, and reuse the
    // PNINode. if (key.IsNewLayer) {
    //   auto NewKey = key;
    //   NewKey.IsNewLayer = false;
    //   this->PNIVar = Parent.getNode(NewKey).getPNIVar();
    //   this->PNIVar->addUser(this);
    //   if (LowType != nullptr) {
    //     this->PNIVar->updateLowTy(LowType);
    //   }
    // } else {
    //   // if the other variance node exist, use that node's PNINode.
    //   NodeKey NewKey = key;
    //   if (key.SuffixVariance == Covariant) {
    //     NewKey.SuffixVariance = Contravariant;
    //   } else {
    //     NewKey.SuffixVariance = Covariant;
    //   }
    //   if (Parent.hasNode(NewKey)) {
    //     this->PNIVar = Parent.getNode(NewKey).getPNIVar();
    //     this->PNIVar->addUser(this);
    //     if (LowType != nullptr) {
    //       this->PNIVar->updateLowTy(LowType);
    //     }
    //   } else {
    //     // create a new PNINode.
    //     this->PNIVar = Parent.PG->createPNINode(this, LowType);
    //   }
    // }
  }
}

} // namespace notdec::retypd
