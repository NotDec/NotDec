#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
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

#include "TypeRecovery/LowTy.h"
#include "TypeRecovery/NFAMinimize.h"
#include "TypeRecovery/Parser.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/retypd/Schema.h"
#include "TypeRecovery/TRContext.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"
#include "TypeRecovery/mlsub/MLsubGraph.h"
#include "TypeRecovery/mlsub/PNDiff.h"
#include "Utils/Utils.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"

#define DEBUG_TYPE "mlsub_graph"

using namespace llvm;

namespace notdec::mlsub {

using retypd::OffsetLabel;
using retypd::StoreLabel;
using retypd::LoadLabel;
using retypd::Contravariant;
using retypd::rexp::PRExp;
using retypd::rexp::createEmpty;
using retypd::rexp::createNull;
using retypd::rexp::Or;
using retypd::rexp::Empty;
using retypd::rexp::Null;
using retypd::combine;
using retypd::OffsetLabel;
using retypd::OffsetLabel;


const CGEdge *ConstraintGraph::addEdge(CGNode &From, CGNode &To,
                                       FieldLabel Label) {
  // do not maintain PNI during layer split.
  if (PG && !isNotSymmetry) {
    if (auto F = Label.getAs<ForgetLabel>()) {
      if (auto O = F->label.getAs<OffsetLabel>()) {
        // unify PN
        From.getPNIVar()->unify(*To.getPNIVar());
        // also should be pointer, TODO: set or assert?
        assert(From.getPNIVar()->isPointer());
      }
    } else if (auto R = Label.getAs<RecallLabel>()) {
      if (auto O = R->label.getAs<OffsetLabel>()) {
        // unify PN
        From.getPNIVar()->unify(*To.getPNIVar());
        // also should be pointer, TODO: set or assert?
        assert(From.getPNIVar()->isPointer());
      }
    } else if (Label.isOne()) {
      // unify PN
      From.getPNIVar()->unify(*To.getPNIVar());
    }
  }
  return onlyAddEdge(From, To, Label);
}

std::map<const CGEdge *, const CGEdge *>
ConstraintGraph::mergeNodeTo(CGNode &From, CGNode &To, bool NoSelfLoop) {
  std::map<const CGEdge *, const CGEdge *> EdgeMap;
  assert(&From.Parent == this && &To.Parent == this);
  if (PG) {
    PG->mergePNINodes(To.getPNIVar(), From.getPNIVar());
  }
  // Move all out edges from From to To
  std::vector<std::tuple<CGNode *, CGNode *, EdgeLabel>> toRemove;
  for (auto &Edge : From.outEdges) {
    auto *Target = &Edge.TargetNode;
    // for self loop
    if (Target == &To || Target == &From) {
      // only keep non-one edge
      if (!Edge.Label.isOne()) {
        assert(!NoSelfLoop);
        auto NE = onlyAddEdge(To, To, Edge.Label);
        EdgeMap.emplace(&Edge, NE);
      }
      // removeEdge(From, *Target, Edge.Label);
      toRemove.push_back({&From, Target, Edge.Label});
    } else {
      auto NE = onlyAddEdge(To, Edge.TargetNode, Edge.Label);
      EdgeMap.emplace(&Edge, NE);
      // removeEdge(From, Edge.TargetNode, Edge.Label);
      toRemove.push_back({&From, &Edge.TargetNode, Edge.Label});
    }
  }
  // Move all in edges
  for (auto *Edge : From.inEdges) {
    auto *Source = &Edge->getSourceNode();
    if (Source == &From || Source == &To) {
      // only keep non-one edge
      if (!Edge->Label.isOne()) {
        assert(!NoSelfLoop);
        auto NE = onlyAddEdge(To, To, Edge->Label);
        EdgeMap.emplace(Edge, NE);
      }
      // removeEdge(To, From, Edge->getLabel());
      toRemove.push_back({Source, &From, Edge->Label});
    } else {
      auto NE = onlyAddEdge(Edge->getSourceNode(), To, Edge->getLabel());
      EdgeMap.emplace(Edge, NE);
      // removeEdge(Edge->getSourceNode(), From, Edge->getLabel());
      toRemove.push_back({&Edge->getSourceNode(), &From, Edge->Label});
    }
  }
  for (auto &[From, Target, Label] : toRemove) {
    if (hasEdge(*From, *Target, Label)) {
      removeEdge(*From, *Target, Label);
    }
  }
  // erase the node
  removeNode(From);
  return EdgeMap;
}

CGNode *CGNode::getLabelTarget(const EdgeLabel &L) const {
  CGNode *Ret = nullptr;
  for (auto &E : outEdges) {
    if (E.getLabel() == L) {
      assert(Ret == nullptr && "Multiple possible target");
      Ret = const_cast<CGNode *>(&E.getTargetNode());
    }
  }
  return Ret;
}

CGNode *CGNode::getLabelSource(const EdgeLabel &L) const {
  CGNode *Ret = nullptr;
  for (auto *E : inEdges) {
    if (E->getLabel() == L) {
      assert(Ret == nullptr && "Multiple possible target");
      Ret = const_cast<CGNode *>(&E->getSourceNode());
    }
  }
  return Ret;
}


void ConstraintGraph::linkConstantPtr2Memory() {
  // for each node, if it is a constant pointer, link it to memory.
  for (auto &Node : Nodes) {
    if (Node.key.Base.isIntConstant() &&
        Node.key.Base.getIntConstant().offset != 0 &&
        Node.getPNIVar()->isPointer() && (Node.getVariance() == Covariant)) {
      auto MN = getMemoryNode(Covariant);
      // auto TV = MN->key;
      // auto Label = OffsetLabel{Node.key.Base.getIntConstant()};
      // TV = TV.Base.pushLabel({Label});
      // auto &MON = getOrInsertNodeWithPNI(TV, MN->getPNIVar());
      addRecallEdge(*MN, Node, {OffsetLabel{Node.key.Base.getIntConstant()}});
    }
  }
}

void ConstraintGraph::changeStoreToLoad() {
  // for every recall store, add recall load.
  for (auto &Node : Nodes) {
    for (auto &Edge : Node.outEdges) {
      auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      if (auto Rec = Edge.getLabel().getAs<RecallLabel>()) {
        if (auto Store = Rec->label.getAs<StoreLabel>()) {
          addEdge(Node, Target, {RecallLabel{LoadLabel{Store->Size}}});
        }
      }
    }
  }
  // remove every recall store.
  std::vector<CGEdge *> toRemove;
  for (auto &Node : Nodes) {
    for (auto &Edge : Node.outEdges) {
      if (auto Rec = Edge.getLabel().getAs<RecallLabel>()) {
        if (Rec->label.isStore()) {
          toRemove.push_back(&const_cast<CGEdge &>(Edge));
        }
      }
    }
  }
  for (auto Edge : toRemove) {
    removeEdge(Edge->getSourceNode(), Edge->getTargetNode(), Edge->getLabel());
  }
}

void ConstraintGraph::aggressiveSimplify() {
  // if has recall load N and recall store N, then remove recall store N.
  for (auto &Node : Nodes) {
    for (auto &Edge : Node.outEdges) {
      auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      if (auto Recall = Edge.getLabel().getAs<RecallLabel>()) {
        if (auto Load = Recall->label.getAs<LoadLabel>()) {
          if (hasEdge(Node, Target, {RecallLabel{StoreLabel{Load->Size}}})) {
            // break, no need to defer the removal out of the loop.
            removeEdge(Node, Target, {RecallLabel{StoreLabel{Load->Size}}});
            break;
          }
        }
      }
    }
  }

  // do not include path with size 8.
  // 1. remove all load8/store8 edges.
  // for (auto &Ent : Nodes) {
  //   auto &Node = Ent.second;
  //   std::vector<const CGEdge *> toRemove;
  //   for (auto &Edge : Node.outEdges) {
  //     if (auto Rec = Edge.getLabel().getAs<RecallLabel>()) {
  //       if (Rec->label.isLoad() &&
  //           std::get<LoadLabel>(Rec->label).Size == 8) {
  //         toRemove.push_back(&Edge);
  //       } else if (Rec->label.isStore() &&
  //                  std::get<StoreLabel>(Rec->label).Size == 8) {
  //         toRemove.push_back(&Edge);
  //       }
  //     }
  //     if (auto Rec = Edge.getLabel().getAs<ForgetLabel>()) {
  //       if (Rec->label.isLoad() &&
  //           std::get<LoadLabel>(Rec->label).Size == 8) {
  //         toRemove.push_back(&Edge);
  //       } else if (Rec->label.isStore() &&
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

void CGNode::onUpdatePNType() {
  // if (this->key.Base.isIntConstant()) {
  //   if (getPNIVar()->isPointer()) {
  //     // convert to offset of memory
  //     auto TV = Parent.getMemoryNode(Covariant)->key.Base;
  //     auto Label = OffsetLabel{key.Base.getIntConstant()};
  //     TV = TV.pushLabel(Label);
  //     std::string name =
  //         "intptr_" + key.Base.getIntConstant().str().substr(1) + '_';
  //     TypeVariable NewTV = TypeVariable::CreateDtv(
  //         *Parent.Ctx, ValueNamer::getName(name.c_str()));
  //     Parent.replaceNodeKey(this->key.Base, NewTV);
  //     // keep PNI untouched. Lowtype is still int.
  //     // this->setPointer();
  //     // Fix links with memory node.
  //     Parent.addConstraint(*this, *this);
  //   } else if (getPNIVar()->isNumber()) {
  //     // do nothing in case of conflict
  //     // view as int later lazily.
  //   } else {
  //     assert(false && "onUpdatePNType: Unknown PNType");
  //   }
  // }
}

void CGNode::setAsPtrAdd(CGNode &Other, OffsetRange Off) {
  // auto TV = Other->key.Base;
  // TV = TV.pushLabel({OffsetLabel{Off}});
  // // 这里没有插入反向PNI的图？
  // auto &PtrAdd = Parent.getOrInsertNodeWithPNI(TV, Other->getPNIVar());
  // assert(PtrAdd.getPNIVar() == Other->getPNIVar());
  // Parent.addConstraint(PtrAdd, *this);
  // assert(PtrAdd.getPNIVar() == this->getPNIVar());
  Parent.addRecallEdge(Other, *this, {OffsetLabel{Off}});
  if (Parent.PG) {
    Parent.PG->mergePNINodes(Other.getPNIVar(), this->getPNIVar());
  }
}

void ConstraintGraph::solve() { saturate(); }

std::vector<SubTypeConstraint> ConstraintGraph::toConstraints() {
  assert(!isNotSymmetry && "printConstraints: graph is already split!?");

  std::vector<SubTypeConstraint> ret;
  for (auto &Source : Nodes) {
    for (auto &Edge : Source.outEdges) {
      auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      if (Edge.Label.isOne()) {
        ret.push_back(
            SubTypeConstraint{.sub = Source.key.Base, .sup = Target.key.Base});
      }
    }
  }
  return ret;
}

std::vector<SubTypeConstraint> ConstraintGraph::solve_constraints_between() {

  std::map<std::pair<CGNode *, CGNode *>, PRExp> P =
      solve_constraints(getStartNode(), PathSeq);

  auto getPexp = [&](CGNode *From, CGNode *To) -> PRExp {
    auto Key = std::make_pair(From, To);
    if (P.count(Key)) {
      return P[Key];
    }
    if (From == To) {
      return createEmpty();
    } else {
      return createNull();
    }
  };
  auto finalExp = getPexp(getStartNode(), getEndNode());
  if ((::llvm::DebugFlag && ::llvm::isCurrentDebugType(DEBUG_TYPE))) {
    // flatten the outer most Or
    if (std::holds_alternative<Or>(*finalExp)) {
      auto &Inner = std::get<Or>(*finalExp).E;
      llvm::dbgs() << "Final Expression: \n";
      for (auto Exp : Inner) {
        llvm::dbgs() << "  " << toString(Exp) << "\n";
      }
    } else {
      llvm::dbgs() << "Final Expression: " << toString(finalExp) << "\n";
    }
  }

  // 2. adjust the expr to create type schemes.
  auto ret = mlsub::expToConstraints(Ctx, finalExp);
  // 3. delete invalid constraints like: int64.in_0
  // 4. substitute temp vars to prevent name collision.
  return ret;
}

void ConstraintGraph::linkVars(std::set<std::string> &InterestingVars,
                               bool LinkLoadStores) {
  // Link nodes to "#Start"
  for (auto &Node : Nodes) {
    auto *N = &Node;
    if (N->isStartOrEnd()) {
      continue;
    }
    // is an interesting var or is a primitive type
    if (!N->key.Base.hasLabel() && N->key.Base.hasBaseName() &&
        InterestingVars.count(N->key.Base.getBaseName()) != 0) {
      // LLVM_DEBUG(llvm::dbgs()
      //            << "Adding an edge from #Start to " << N->key.str() <<
      //            "\n");
      auto &From = *getStartNode();
      auto &To = *N;
      if (TraceIds.count(From.getId())) {
        PRINT_TRACE(From.getId())
            << "Adding edge #Start(ID=" << From.getId() << ") to "
            << To.key.str() << "(ID=" << To.getId() << ")\n";
      }
      if (TraceIds.count(To.getId())) {
        PRINT_TRACE(To.getId())
            << "Adding edge #Start(ID=" << From.getId() << ") to "
            << To.key.str() << "(ID=" << To.getId() << ")\n";
      }
      assert(!To.key.Base.hasLabel());
      addEdge(From, To,
              {RecallBase{.Base = N->key.Base, .V = N->getVariance()}});
    }
  }
  linkPrimitives();
  linkEndVars(InterestingVars);
  if (LinkLoadStores) {
    // mark load/store as accepting.
    linkLoadStore();
  }
}

void ConstraintGraph::linkEndVars(std::set<std::string> &InterestingVars) {
  // Link nodes to "#End"
  for (auto &Node : Nodes) {
    auto *N = &Node;
    if (N->isStartOrEnd()) {
      continue;
    }
    // is an interesting var or is a primitive type
    if (!N->key.Base.hasLabel() && N->key.Base.hasBaseName() &&
        InterestingVars.count(N->key.Base.getBaseName()) != 0) {
      // LLVM_DEBUG(llvm::dbgs()
      //            << "Adding an edge from " << N->key.str() << " to #End\n");
      auto &From = *N;
      auto &To = *getEndNode();
      if (TraceIds.count(From.getId())) {
        PRINT_TRACE(From.getId())
            << "Adding edge " << From.key.str() << "(ID=" << From.getId()
            << ") to " << toString(To.key) << "(ID=" << To.getId() << ")\n";
      }
      if (TraceIds.count(To.getId())) {
        PRINT_TRACE(To.getId())
            << "Adding edge " << From.key.str() << "(ID=" << From.getId()
            << ") to " << toString(To.key) << "(ID=" << To.getId() << ")\n";
      }
      addEdge(
          *N, *getEndNode(),
          {ForgetBase{.Base = N->key.Base.toBase(), .V = N->getVariance()}});
    }
  }
}

void ConstraintGraph::linkPrimitives() {
  // // Link primitive nodes to "#Start"
  // for (auto &Node : Nodes) {
  //   auto *N = &Node;
  //   if (N->isStartOrEnd()) {
  //     continue;
  //   }
  //   if (N->key.Base.isPrimitive()) {
  //     // LLVM_DEBUG(llvm::dbgs()
  //     //            << "Adding an edge from #Start to " << N->key.str() <<
  //     //            "\n");
  //     auto &From = *getStartNode();
  //     auto &To = *N;
  //     if (TraceIds.count(From.getId())) {
  //       PRINT_TRACE(From.getId())
  //           << "Adding edge #Start(ID=" << From.getId() << ") to "
  //           << To.key.str() << "(ID=" << To.getId() << ")\n";
  //     }
  //     if (TraceIds.count(To.getId())) {
  //       PRINT_TRACE(To.getId())
  //           << "Adding edge #Start(ID=" << From.getId() << ") to "
  //           << To.key.str() << "(ID=" << To.getId() << ")\n";
  //     }
  //     assert(!To.key.Base.hasLabel());
  //     addEdge(From, To,
  //             {RecallBase{.Base = N->key.Base, .V = N->getVariance()}});
  //   }
  // }
  // Link primitive nodes to "#End"
  for (auto &Node : Nodes) {
    auto *N = &Node;
    if (N->isStartOrEnd()) {
      continue;
    }
    if (N->key.Base.isPrimitive()) {
      // LLVM_DEBUG(llvm::dbgs()
      //            << "Adding an edge from " << N->key.str() << " to #End\n");
      auto &From = *N;
      auto &To = *getEndNode();
      if (TraceIds.count(From.getId())) {
        PRINT_TRACE(From.getId())
            << "Adding edge " << From.key.str() << "(ID=" << From.getId()
            << ") to " << toString(To.key) << "(ID=" << To.getId() << ")\n";
      }
      if (TraceIds.count(To.getId())) {
        PRINT_TRACE(To.getId())
            << "Adding edge " << From.key.str() << "(ID=" << From.getId()
            << ") to " << toString(To.key) << "(ID=" << To.getId() << ")\n";
      }
      addEdge(
          *N, *getEndNode(),
          {ForgetBase{.Base = N->key.Base.toBase(), .V = N->getVariance()}});
    }
  }
}

void ConstraintGraph::recoverBaseVars() {
  // Fix RecallBase and ForgetBase edges
  std::vector<std::tuple<CGNode *, CGNode *, EdgeLabel>> toRemove;
  std::map<NodeKey, CGNode *> RecNodeMap;
  for (auto &OE : Start->outEdges) {
    auto RE = OE.getLabel().getAs<RecallBase>();
    assert(RE != nullptr);
    NodeKey NewKey(RE->Base, RE->V);
    auto &Target = const_cast<CGNode &>(OE.getTargetNode());

    CGNode *NewTarget = nullptr;
    if (RecNodeMap.count(NewKey)) {
      NewTarget = RecNodeMap.at(NewKey);
    } else {
      NewTarget = &createNodeWithPNI(NewKey, Target.getPNIVar());
      RecNodeMap.insert(std::make_pair(NewKey, NewTarget));
    }
    addEdge(*NewTarget, Target, {One{}});
    toRemove.push_back(std::make_tuple(Start, &Target, OE.getLabel()));
  }
  for (auto IE : End->inEdges) {
    auto FB = IE->getLabel().getAs<ForgetBase>();
    assert(FB != nullptr);
    NodeKey NewKey(FB->Base, FB->V);
    auto &Source = const_cast<CGNode &>(IE->getSourceNode());

    CGNode *NewSource = nullptr;
    if (RecNodeMap.count(NewKey)) {
      NewSource = RecNodeMap.at(NewKey);
    } else {
      NewSource = &createNodeWithPNI(NewKey, Source.getPNIVar());
      RecNodeMap.insert(std::make_pair(NewKey, NewSource));
    }
    addEdge(Source, *NewSource, {One{}});
    toRemove.push_back(std::make_tuple(&Source, End, IE->getLabel()));
  }
  for (auto &[From, To, Label] : toRemove) {
    removeEdge(*From, *To, Label);
  }
}

void ConstraintGraph::contraVariantSplit() {
  markVariance();
  std::vector<CGNode *> toHandle;
  // for all contra-variant nodes
  for (auto &Source : Nodes) {
    if (Source.getVariance() != Contravariant) {
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
        NodeKey{TypeVariable::CreateDtv(*Ctx, ValueNamer::getName("split_"))},
        N->getPNIVar());
    // Move all incoming recall edge to the new node.
    std::set<std::tuple<CGNode *, CGNode *, EdgeLabel>> toRemove;
    for (auto InEdge2 : N->inEdges) {
      if (isRecall(InEdge2->Label)) {
        addEdge(InEdge2->getSourceNode(), NewNode, InEdge2->Label);
        // removeEdge(InEdge2->getSourceNode(), *N, InEdge2->Label);
        toRemove.insert({&InEdge2->getSourceNode(), N, InEdge2->Label});
      }
    }
    // Move all outgoing recall edge to the new node.
    for (auto &Edge : N->outEdges) {
      if (isRecall(Edge.Label)) {
        auto &Target2 = const_cast<CGNode &>(Edge.getTargetNode());
        addEdge(NewNode, Target2, Edge.Label);
        // removeEdge(*N, Target2, Edge.Label);
        toRemove.insert({N, &Target2, Edge.Label});
      }
    }
    for (auto &[From, To, Label] : toRemove) {
      removeEdge(*From, *To, Label);
    }
  }
}

/// Mark the variance of the node according to the edges.
/// DFS traversal.
void ConstraintGraph::markVariance(std::map<CGNode *, Variance> *Initial) {
  std::map<CGNode *, Variance> Visited;
  std::queue<std::pair<CGNode *, Variance>> Worklist;
  if (Initial) {
    for (auto &Ent : *Initial) {
      Worklist.push(Ent);
    }
  } else {
    Worklist.push(std::make_pair(getStartNode(), Covariant));
  }

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
      if (Label.isOne()) {
        Worklist.push(std::make_pair(&Target, N.second));
      } else if (auto *RL = Label.getAs<RecallLabel>()) {
        Worklist.push(
            std::make_pair(&Target, combine(N.second, getVariance(RL->label))));
      } else if (auto *FL = Label.getAs<ForgetLabel>()) {
        Worklist.push(
            std::make_pair(&Target, combine(N.second, getVariance(FL->label))));
      } else if (auto *RB = Label.getAs<RecallBase>()) {
        Worklist.push(std::make_pair(&Target, combine(N.second, RB->V)));
      } else if (auto *FB = Label.getAs<ForgetBase>()) {
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
  for (auto &Node : Nodes) {
    if (&Node == Start || &Node == End) {
      continue;
    }
    if (Visited.count(&Node) == 0) {
      std::cerr << "Error: labelVariance: node is not visited: "
                << toString(Node.key) << "\n";
      std::abort();
    }
    //
    // // use map extract
    // auto Ent2 = Nodes.extract(Node.key);
    // const_cast<NodeKey &>(Ent2.mapped().key).SuffixVariance = Visited[&Node];
    // Ent2.key() = Ent2.mapped().key;
    // Nodes.insert(std::move(Ent2));

    Node.key.SuffixVariance = Visited[&Node];

    // std::cerr << "Node: " << toString(Node.key)
    //           << " Variance: " << toString(Visited[&Node]) << "\n";
  }
  // std::cerr << " OK\n";

  return;
}

// disallow recall and forget the same thing, which is a no-op.
// for each node, create a duplicate node, move all non-forget edge to the new
void ConstraintGraph::pushSplit() {
  std::vector<std::tuple<CGNode *, CGNode *, EdgeLabel>> toRemove;
  std::set<std::tuple<CGNode *, EdgeLabel, EdgeLabel>> toIsolate;

  // first scan all nodes with only useless path, delete them.
  // and find nodes to isolate.
  for (auto &Node : Nodes) {
    if (&Node == Start || &Node == End) {
      continue;
    }
    CGNode *Current = &Node;
    for (auto InEdge : Current->inEdges) {
      if (InEdge->getLabel().isOne()) {
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
        NodeKey{TypeVariable::CreateDtv(*Ctx, ValueNamer::getName("split_"))},
        Current->getPNIVar());
    // copy all out edges except the forget edge.
    for (auto &Edge : Current->outEdges) {
      auto &Target2 = const_cast<CGNode &>(Edge.getTargetNode());
      if (Edge.Label != Forget) {
        addEdge(NewNode, Target2, Edge.getLabel());
      }
    }
    // Move all same recall edge to the new node.
    std::vector<std::tuple<CGNode *, CGNode *, EdgeLabel>> toRemove;
    for (auto InEdge2 : Current->inEdges) {
      if (InEdge2->getLabel() == Recall) {
        addEdge(InEdge2->getSourceNode(), NewNode, InEdge2->getLabel());
        // removeEdge(InEdge2->getSourceNode(), *Current, InEdge2->getLabel());
        toRemove.emplace_back(&InEdge2->getSourceNode(), Current,
                              InEdge2->getLabel());
      }
    }
    for (auto &Ent : toRemove) {
      auto [Source, Target, Label] = Ent;
      removeEdge(*Source, *Target, Label);
    }
  }
}

// Simplify graph and build path expr.
std::vector<SubTypeConstraint>
ConstraintGraph::simplifiedExpr(std::set<std::string> &InterestingVars) const {
  // TODO: eliminate this clone by removing Start and End nodes later.
  std::map<const CGNode *, CGNode *> Old2New;
  auto G = clone(Old2New);
  // G.lowTypeToSubType();
  G.linkVars(InterestingVars, false);
  auto G2 = G.simplify();
  G2.aggressiveSimplify();

  G2.linkVars(InterestingVars, false);
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

unsigned countLoadOrStoreEdge(CGNode &N) {
  unsigned Count = 0;
  for (auto &Edge : N.outEdges) {
    if (isLoadOrStore(Edge.Label)) {
      Count += 1;
    }
  }
  return Count;
}

const CGEdge *getOnlyLoadOrStoreEdge(CGNode &N) {
  const CGEdge *LoadEdge = nullptr;
  for (auto &Edge : N.outEdges) {
    if (isLoadOrStore(Edge.Label)) {
      assert(LoadEdge == nullptr);
      LoadEdge = &Edge;
    }
  }
  return LoadEdge;
}

bool hasOffsetEdge(CGNode &Start) {
  // fast path, if there is no outgoing or incoming offset edge, only consider
  // one(subtype) edge.
  bool HasOffset = false;
  // Outgoing
  for (auto &Edge : Start.outEdges) {
    if (auto EL1 = Edge.Label.getAs<RecallLabel>()) {
      if (EL1->label.isOffset()) {
        HasOffset = true;
        break;
      }
    } else if (auto EL2 = Edge.Label.getAs<ForgetLabel>()) {
      if (EL2->label.isOffset()) {
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


/// Intersect the language with recall*forget*.
/// Support null/epsilon moves.
void ConstraintGraph::layerSplit() {
  // using ND = NFADeterminizer<>;
  assert(!isNotSymmetry && "layerSplit: already not Symmetry");
  isNotSymmetry = true;

  std::map<const CGNode *, CGNode *> Old2New;

  // 1. clone all nodes to new layer
  for (auto &Node : Nodes) {
    // Do not copy edges from start.
    if (Node.key.IsNewLayer || &Node == Start) {
      continue;
    }
    auto &Source = Node;
    NodeKey NewSrc = Source.key;
    NewSrc.IsNewLayer = true;
    auto &New = (&Node != End && !Node.isTop())
                    ? createNodeWithPNI(NewSrc, Source.getPNIVar())
                    : createNodeNoPNI(NewSrc, End->getSize());
    Old2New.insert({&Source, &New});
  }

  // 2. perform layer split
  std::vector<std::tuple<CGNode *, CGNode *, CGNode *, EdgeLabel>> toChange;
  for (auto &Node : Nodes) {
    // Do not copy edges from start.
    if (Node.key.IsNewLayer || &Node == Start) {
      continue;
    }
    auto &Source = Node;
    for (auto &Edge : Source.outEdges) {
      auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      if (Target.key.IsNewLayer) {
        continue;
      }
      auto &NewSrcNode = *Old2New.at(&Source);
      auto &NewDstNode = *Old2New.at(&Target);
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
  std::set<CGNode *> NewEndNodes;
  for (CGNode *Ent : EndNodes) {
    if (Ent->key.IsNewLayer) {
      continue;
    }
    auto &NewNode = *Old2New.at(Ent);
    NewEndNodes.insert(&NewNode);
  }
  // remove old end nodes that is not in the new layer.
  EndNodes = std::move(NewEndNodes);
  // Update the end node to the new layer.
  // Because there will only be forget base to End, old end node must has no in
  // edges, which is moved to new end node in new layer.
  if (End != nullptr) {
    auto *OldEnd = End;
    assert(OldEnd->inEdges.size() == 0);
    auto &NewNode = *Old2New.at(End);
    End = &NewNode;
    // remove the old end node.
    removeNode(*OldEnd);
  }

  if (const char *path = std::getenv("DEBUG_TRANS_LAYER_SPLIT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str()))) {
      printGraph("trans_layerSplit.dot");
    }
  }
}

void ConstraintGraph::linkLoadStore() {
  for (auto &Node : Nodes) {
    auto &Source = Node;
    if (Source.isStartOrEnd()) {
      continue;
    }
    // for load/store nodes, add forget base or recall base.
    if (Source.key.Base.hasLabel()) {
      auto &Back = Source.key.Base.getLabels().back();
      if (Back.isLoad()) {
        if (Source.getVariance() == Covariant) {
          // covariant load can only have subtype (one) edge out
          onlyAddEdge(
              Source, *getEndNode(),
              {ForgetBase{.Base = Source.key.Base, .V = Source.getVariance()}});
        } else {
          onlyAddEdge(
              *getStartNode(), Source,
              {RecallBase{.Base = Source.key.Base, .V = Source.getVariance()}});
        }
      } else if (Back.isStore()) {
        if (Source.getVariance() == Covariant) {
          // covariant store can only have subtype (one) edge in
          onlyAddEdge(
              *getStartNode(), Source,
              {RecallBase{.Base = Source.key.Base, .V = Source.getVariance()}});
        } else {
          onlyAddEdge(
              Source, *getEndNode(),
              {ForgetBase{.Base = Source.key.Base, .V = Source.getVariance()}});
        }
      }
    }
  }
}

void ConstraintGraph::ensureNoForgetLabel() {
  for (auto &Node : Nodes) {
    auto &Source = Node;
    if (Source.isStartOrEnd()) {
      continue;
    }
    for (auto &Edge : Source.outEdges) {
      if (Edge.Label.isForgetLabel()) {
        std::cerr << "Error: ensureNoForgetLabel: forget label found: "
                  << toString(Source.key) << " -> "
                  << toString(Edge.getTargetNode().key) << "\n";
        std::abort();
      }
    }
  }
}

void ConstraintGraph::sketchSplit() {
  linkPrimitives();
  std::vector<const CGEdge *> toRemove;
  // 1. focus on the recall subgraph, but allow ForgetBase edge.
  for (auto &Node : Nodes) {
    auto &Source = Node;
    // 1.1 remove all forgetLabel and RecallBase edge
    for (auto &Edge : Source.outEdges) {
      // auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      if (Edge.Label.isForgetLabel()) {
        toRemove.push_back(&Edge);
        // removeEdge(Source, Target, Edge.Label);
        // continue;
      } else if (Edge.Label.isRecallBase()) {
        toRemove.push_back(&Edge);
        // removeEdge(Source, Target, Edge.Label);
        // continue;
      }
    }
  }
  for (auto &Edge : toRemove) {
    removeEdge(const_cast<CGNode &>(Edge->getSourceNode()),
               const_cast<CGNode &>(Edge->getTargetNode()), Edge->getLabel());
  }
  isSketchSplit = true;
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

std::set<CGNode *> getOneReachable(CGNode &N) {
  std::set<CGNode *> Visited;
  std::queue<CGNode *> Q;
  Q.push(&N);
  while (!Q.empty()) {
    auto *Current = Q.front();
    Q.pop();
    if (Visited.count(Current) > 0) {
      continue;
    }
    Visited.insert(Current);
    for (auto &Edge : Current->outEdges) {
      if (Edge.Label.isOne()) {
        Q.push(const_cast<CGNode *>(&Edge.getTargetNode()));
      }
    }
  }
  return Visited;
}

bool canReach(CGNode &From, CGNode &To) {
  if (&From == &To) {
    return true;
  }
  std::set<CGNode *> Visited;
  std::queue<CGNode *> Q;
  Q.push(&From);
  while (!Q.empty()) {
    auto *Current = Q.front();
    Q.pop();
    if (Visited.count(Current) > 0) {
      continue;
    }
    Visited.insert(Current);
    if (Current == &To) {
      return true;
    }
    for (auto &Edge : Current->outEdges) {
      if (Edge.Label.isOne()) {
        Q.push(const_cast<CGNode *>(&Edge.getTargetNode()));
      }
    }
  }
  return false;
}

void ConstraintGraph::applyPNIPolicy() {
  for (auto &N : Nodes) {
    if (!N.isPNIUnknown()) {
      continue;
    }
    if (N.key.Base.isIntConstant()) {
      auto C = N.key.Base.getIntConstant();
      if (C.offset != 0 && C.offset < 900 && C.offset > -900) {
        N.setPNINonPtr();
      }
    }
  }
}

/// Algorithm D.2 Saturation algorithm
void ConstraintGraph::saturate() {

  if (const char *val = std::getenv("NOTDEC_SAT_DISABLE")) {
    if ((std::strcmp(val, "1") == 0)) {
      return;
    }
  }

  long Timeout = 0;
  if (const char *val = std::getenv("NOTDEC_SAT_TIMEOUT")) {
    Timeout = std::stol(val);
  }
  const clock_t begin_time = clock();

  bool DenseSubtype = false;
  if (const char *val = std::getenv("NOTDEC_SAT_DENSESUBTYPE")) {
    if ((std::strcmp(val, "1") == 0)) {
      DenseSubtype = true;
    }
  }

  bool NoPtrRule = false;
  if (const char *val = std::getenv("NOTDEC_SAT_NOPTRRULE")) {
    if ((std::strcmp(val, "1") == 0)) {
      NoPtrRule = true;
    }
  }

  bool Changed = true;
  // solve PNI first.
  if (PG) {
    PG->solve();
  }

  std::set<CGNode *> WorklistSet;
  std::deque<CGNode *> Worklist;
  auto AddWorklist = [&](CGNode *N) {
    auto Ent = WorklistSet.insert(N);
    if (Ent.second) {
      Worklist.push_back(N);
    }
  };
  auto PopWorklist = [&]() -> CGNode * {
    auto N = Worklist.front();
    Worklist.pop_front();
    WorklistSet.erase(N);
    return N;
  };
  auto SolveWorklist = [&]() -> bool {
    bool Changed = false;
    if (DenseSubtype) {
      // Add transitive subtypeing and cancel out offset edges.
      // TODO: fix probably has one edge and is still offset related
      assert(false && "TODO");
    }
    while (!Worklist.empty()) {
      auto N = PopWorklist();
      // 传递ReachingSet
      for (auto &Edge : N->outEdges) {
        if (Edge.Label.isOne()) {
          auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
          // For each One edge.
          if (ReachingSet.count(N)) {
            for (auto &Reach : ReachingSet[N]) {
              auto Res = ReachingSet[&Target].insert(Reach);
              // 如果更新了set，加入Worklist
              if (Res.second) {
                Changed = true;
                AddWorklist(&Target);
              }
            }
          }
        }
      }
    }
    return Changed;
  };
  auto HandleNewSubtype = [&](CGNode *From, CGNode *To) {
    if (ReachingSet.count(From)) {
      for (auto &Reach : ReachingSet[From]) {
        auto Res = ReachingSet[To].insert(Reach);
        // 如果更新了set，加入Worklist
        if (Res.second) {
          AddWorklist(To);
        }
      }
    }
  };

  // 1. add forget edges to reaching set
  for (auto &Source : Nodes) {
    for (auto &Edge : Source.outEdges) {
      // For each edge, check if is forget edge.
      if (auto Capa = Edge.Label.getAs<ForgetLabel>()) {
        auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
        if (DenseSubtype) {
          if (!Capa->label.isOffset()) {
            auto Res = ReachingSet[&Target].insert({Capa->label, &Source});
            if (Res.second) {
              AddWorklist(&Target);
            }
          }
        } else {
          auto Res = ReachingSet[&Target].insert({Capa->label, &Source});
          if (Res.second) {
            AddWorklist(&Target);
          }
        }
      }
    }
  }
  Changed |= SolveWorklist();

  while (Changed) {
    Changed = false;

    // The standard saturation rule.
    // begin: For each recall edge,
    for (auto &Source : Nodes) {
      for (auto &Edge : Source.outEdges) {
        if (auto Capa = Edge.Label.getAs<RecallLabel>()) {
          auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
          // end: for each recall edge.
          if (ReachingSet.count(&Source)) {
            // non-lazy rule: if it is recall load, we allow forget store.
            std::optional<FieldLabel> RecallStore = std::nullopt;
            if (auto *Load = Capa->label.getAs<LoadLabel>()) {
              RecallStore = {toStore(*Load)};
            }
            for (auto &Reach : ReachingSet[&Source]) {
              // 相同的label，增加subtype
              if (Reach.first == Capa->label && Reach.second != &Target) {
                // We are iterating through Recall edges, and we insert One
                // edge, so it is ok to modify edge during iterating.
                if (!canReach(*Reach.second, Target)) {
                  LLVM_DEBUG(llvm::dbgs()
                             << "Adding Edge From " << Reach.second->key.str()
                             << " to " << Target.key.str() << " with _1_ \n");
                  auto NewEdge = addEdge(*Reach.second, Target, {One{}});
                  if (NewEdge) {
                    HandleNewSubtype(Reach.second, &Target);
                  }
                  Changed |= (NewEdge != nullptr);
                }
              }
              // non-lazy rule: if it is recall load, we also allow forget
              // store.
              if (RecallStore && (Reach.first == *RecallStore) &&
                  Reach.second != &Target) {
                // Changed |= (addEdge(*Reach.second, Target, {One{}}) !=
                // nullptr);
                auto &From = *Reach.second;
                auto &To = Target;
                From.getPNIVar()->unify(*To.getPNIVar());
              }
            }
            // solve outside of the ReachingSet loop
            Changed |= SolveWorklist();
          }
        }

        if (Timeout) {
          auto DurationMS =
              (float(clock() - begin_time) * 1000 / CLOCKS_PER_SEC);
          if (DurationMS > Timeout) {
            goto end;
          }
        }
      }
    }

    // Lazily apply saturation rules corresponding to S-POINTER.
    if (!NoPtrRule) {
      for (auto &Ent : ReachingSet) {
        if (Ent.first->getVariance() == Contravariant) {
          for (auto &Reach : Ent.second) {
            std::optional<FieldLabel> Label;
            if (auto S = Reach.first.getAs<StoreLabel>()) {
              // LLVM_DEBUG(llvm::dbgs()
              //            << "node " << Reach.second->key.str() << " can reach
              //            "
              //            << Ent.first->key.str() << " with \".store\" \n");
              Label = {LoadLabel{.Size = S->Size}};
            } else if (auto L = Reach.first.getAs<LoadLabel>()) {
              // LLVM_DEBUG(llvm::dbgs()
              //            << "node " << Reach.second->key.str() << " can reach
              //            "
              //            << Ent.first->key.str() << " with \".load\" \n");
              Label = {StoreLabel{.Size = L->Size}};
            } else {
              continue;
            }
            // find the node with opposite variance.
            auto &OppositeNode = getReverseVariant(*Ent.first);
            auto Res = ReachingSet[&OppositeNode].insert(
                {Label.value(), Reach.second});
            // Changed |= Res.second;
            if (Res.second) {
              AddWorklist(&OppositeNode);
            }

            if (Timeout) {
              auto DurationMS =
                  (float(clock() - begin_time) * 1000 / CLOCKS_PER_SEC);
              if (DurationMS > Timeout) {
                goto end;
              }
            }
          }
        }
      }
    }

    // solve outside of the ReachingSet loop
    Changed |= SolveWorklist();
    // Run PNI solving again.
    if (PG) {
      Changed |= PG->solve();
    }

    if (Timeout) {
      auto DurationMS = (float(clock() - begin_time) * 1000 / CLOCKS_PER_SEC);
      if (DurationMS > Timeout) {
        break;
      }
    }
  }

end:

  auto DurationMS = (float(clock() - begin_time) * 1000 / CLOCKS_PER_SEC);
  if (Timeout && DurationMS > Timeout) {
    std::cerr << "ConstraintGraph::saturate: " << DurationMS << "ms for "
              << Name << ".(Timed out)\n";
  } else if (DurationMS > 100) {
    std::cerr << "ConstraintGraph::saturate: " << DurationMS << "ms for "
              << Name << ".\n";
  }

  if (const char *path = std::getenv("DEBUG_TRANS_SAT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str()))) {
      printGraph("trans_sat.dot");
    }
  }
}

void ConstraintSummaryInstance::addRecalls(CGNode &N) {
  CGNode *T = &N;
  auto V1 = T->key.forgetOnce();
  while (V1.has_value()) {
    auto [Cap, Next] = V1.value();
    auto &NNext = getNode(Next);
    CG.addEdgeDualVariance(NNext, *T, {RecallLabel{Cap}});
    V1 = Next.forgetOnce();
    T = &NNext;
  }
}

void ConstraintSummaryInstance::addForgets(CGNode &N) {
  CGNode *T = &N;
  auto V1 = T->key.forgetOnce();
  while (V1.has_value()) {
    auto [Cap, Next] = V1.value();
    auto &NNext = getNode(Next);
    CG.addEdgeDualVariance(*T, NNext, {ForgetLabel{Cap}});
    V1 = Next.forgetOnce();
    T = &NNext;
  }
}

void ConstraintGraph::instantiateConstraints(const ConstraintSummary &Summary) {
  ConstraintSummaryInstance CSI{.CG = *this, .Summary = Summary};
  CSI.instantiateConstraints();
}

void ConstraintSummaryInstance::instantiateConstraints() {
  for (auto &Ent : Summary.PNIMap) {
    const std::string &SerializedPNI = Ent.second;
    auto Pos = SerializedPNI.rfind(" ");
    assert(Pos != std::string::npos);
    auto IDStr = SerializedPNI.substr(Pos + 1);
    assert(IDStr.front() == '#');
    auto ID = std::stoul(IDStr.substr(1));
    PNINode *PN = nullptr;
    if (ID2PNI.count(ID)) {
      PN = ID2PNI[ID];
      assert(ID2PNIStr[ID] == SerializedPNI && "Inconsistent PNI!");
    } else {
      PN = CG.PG->createPNINode(SerializedPNI.substr(0, Pos));
      ID2PNI[ID] = PN;
      ID2PNIStr[ID] = SerializedPNI;
    }
    getOrInsertNodeWithPNI(Ent.first, PN);
  }
  for (auto &C : Summary.Cons) {
    if (std::holds_alternative<SubTypeConstraint>(C)) {
      // TODO 恢复之前的函数，基于typevar创建节点，但是放到Constraint
      // Summary里面。
      auto &SCons = std::get<SubTypeConstraint>(C);
      addConstraint(getNode(SCons.sub), getNode(SCons.sup));
    } else {
      assert(false && "buildInitialGraph: Unimplemented constraint type!");
    }
  }
}

void ConstraintSummaryInstance::addConstraint(CGNode &From, CGNode &To) {
  addRecalls(From);
  addForgets(From);
  addRecalls(To);
  addForgets(To);
  CG.addEdgeDualVariance(From, To, {One{}});
}

ConstraintGraph
ConstraintGraph::fromConstraints(std::shared_ptr<TRContext> Ctx,
                                 std::string FuncName,
                                 const ConstraintSummary &Summary) {
  ConstraintGraph G(Ctx, Summary.PointerSize, FuncName);
  G.instantiateConstraints(Summary);
  return G;
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

const CGEdge *ConstraintGraph::addEdgeDualVariance(CGNode &From, CGNode &To,
                                                   EdgeLabel Label) {
  assert(!isNotSymmetry && "addEdgeDualVariance: not symmetry!");
  if (&From == &To) {
    if (Label.isOne()) {
      return nullptr;
    } else {
      auto &CN = getReverseVariant(From);
      addEdge(CN, CN, Label);
      return addEdge(From, To, Label);
    }
  }
  auto Ret = addEdge(From, To, Label);
  auto &FromC = getReverseVariant(From);
  auto &ToC = getReverseVariant(To);
  if (Label.isOne()) {
    addEdge(ToC, FromC, Label);
  } else {
    addEdge(FromC, ToC, Label);
  }
  return Ret;
}


CGNode &ConstraintGraph::emplace(const NodeKey &N, llvm::Type *LowType) {
  return Nodes.emplace_back(*this, N, LowType);
}

CGNode &ConstraintGraph::createNodeNoPNI(const NodeKey &N, unsigned Size) {
  return Nodes.emplace_back(*this, N, Size);
}

CGNode &ConstraintGraph::createNode(const NodeKey &N, llvm::Type *LowType) {
  return emplace(N, LowType);
}

CGNode &ConstraintGraph::createNodeWithPNI(const NodeKey &N, PNINode *PNI) {
  return Nodes.emplace_back(*this, N, PNI);
}

CGNode &ConstraintGraph::createNodeClonePNI(const NodeKey &N, PNINode *ON) {
  CGNode *NewN;
  if (ON != nullptr) {
    auto PN = PG->clonePNINode(*ON);
    NewN = &createNodeWithPNI(N, PN);
  } else {
    NewN = &createNodeNoPNI(N, 0);
  }
  return *NewN;
}

std::pair<CGNode &, CGNode &>
ConstraintGraph::createNodePairWithPNI(NodeKey K, PNINode *PNI) {
  auto &N = createNodeWithPNI(K, PNI);
  K.SuffixVariance = !K.SuffixVariance;
  auto &NC = createNodeWithPNI(K, N.getPNIVar());
  RevVariance.insert(std::make_pair(&N, &NC));
  RevVariance.insert(std::make_pair(&NC, &N));
  return {N, NC};
}

std::pair<CGNode &, CGNode &>
ConstraintGraph::createNodePair(NodeKey K, llvm::Type *LowType) {
  auto &N = createNode(K, LowType);
  K.SuffixVariance = !K.SuffixVariance;
  auto &NC = createNodeWithPNI(K, N.getPNIVar());
  RevVariance.insert(std::make_pair(&N, &NC));
  RevVariance.insert(std::make_pair(&NC, &N));
  return {N, NC};
}

CGNode &ConstraintGraph::cloneNode(const CGNode &Other) {
  assert(&Other.Parent != this);
  auto &N = createNodeWithPNI(Other.key, nullptr);
  return N;
}

void ConstraintGraph::removeNode(CGNode &Node) {
  assert(&Node.Parent == this && "removeNode: node not found");
  assert(!Node.isSpecial());
  assert(Node.outEdges.empty() && "removeNode: node has out edges");
  assert(Node.inEdges.empty() && "removeNode: node has in edges");
  PG->markRemoved(Node);
  if (StartNodes.count(&Node) != 0) {
    StartNodes.erase(&Node);
  }
  if (EndNodes.count(&Node) != 0) {
    EndNodes.erase(&Node);
  }
  auto It = RevVariance.find(&Node);
  if (It != RevVariance.end()) {
    auto N = It->first;
    auto NC = It->second;
    RevVariance.erase(N);
    RevVariance.erase(NC);
  }
  Nodes.erase(Node.getIterator());
}

void ConstraintGraph::printGraph(const char *DotFile) const {
  std::error_code EC;
  llvm::raw_fd_ostream OutStream(DotFile, EC);
  if (EC) {
    llvm::errs() << "ConstraintGraph::printGraph: Error printing to " << DotFile
                 << ", " << EC.message() << "\n";
    return;
  }
  assert(false && "TODO fix graph trait");
  // llvm::WriteGraph(OutStream, const_cast<ConstraintGraph *>(this), false);
  OutStream.flush();
  OutStream.close();
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
  ConstraintGraph Temp(Ctx, PointerSize, this->Name, true);

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

  for (auto &Node : Nodes) {
    if (ReachableNodes.count(&Node) != 0) {
      auto *NewNode = Old2New.at(&Node);
      for (auto &Edge : Node.outEdges) {
        if (ReachableNodes.count(&Edge.getTargetNode()) != 0) {
          auto *Target = Old2New.at(&Edge.getTargetNode());
          Temp.onlyAddEdge(*NewNode, *Target, Edge.Label);
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

CGNode *ConstraintGraph::getMemoryNodeOrNull(Variance V) {
  if (V == Covariant) {
    return Memory;
  } else {
    return MemoryC;
  }
}

CGNode *ConstraintGraph::getMemoryNode(Variance V) {
  if (Memory == nullptr) {
    assert(MemoryC == nullptr);
    auto PN = PG->createPNINode("ptr " + std::to_string(PointerSize));
    auto [MemoryN, MemoryNC] = createNodePairWithPNI(
        NodeKey{TypeVariable::CreateDtv(*Ctx, TypeVariable::Memory), Covariant},
        PN);
    Memory = &MemoryN;
    MemoryC = &MemoryNC;
  }
  auto Ret = getMemoryNodeOrNull(V);
  assert(Ret != nullptr);
  return Ret;
}

CGNode *ConstraintGraph::getStartNode() {
  if (Start != nullptr) {
    return Start;
  }
  Start = &createNodeNoPNI(
      NodeKey{TypeVariable::CreatePrimitive(*Ctx, "#Start")}, 0);
  return Start;
}

CGNode *ConstraintGraph::getEndNode() {
  if (End != nullptr) {
    return End;
  }
  End =
      &createNodeNoPNI(NodeKey{TypeVariable::CreatePrimitive(*Ctx, "#End")}, 0);
  return End;
}

ConstraintGraph::ConstraintGraph(std::shared_ptr<TRContext> Ctx,
                                 long PointerSize, std::string Name,
                                 bool disablePNI)
    : Ctx(Ctx), Name(Name), PointerSize(PointerSize) {
  if (!disablePNI) {
    PG = std::make_unique<PNIGraph>(*this, Name, PointerSize);
  }
}

void ConstraintGraph::clone(
    std::map<const CGNode *, CGNode *> &Old2New, const ConstraintGraph &From,
    ConstraintGraph &To, bool isMergeClone,
    std::function<NodeKey(const NodeKey &)> TransformKey,
    std::function<SubtypeRelation(CGNode &, CGNode &)> ConflictKeyRelation) {
  assert(Old2New.empty() && "clone: Old2New is not empty!");
  assert(From.Ctx.get() == To.Ctx.get() && "clone: Ctx mismatch!");
  assert(From.PointerSize == To.PointerSize);
  if (!isMergeClone) {
    To.Name = From.Name;
  }

  // handle fields
  if (!isMergeClone) {
    To.isNotSymmetry = From.isNotSymmetry;
    To.isSketchSplit = From.isSketchSplit;
  }

  if (From.Start) {
    Old2New.insert({From.Start, To.getStartNode()});
  }
  if (From.End) {
    Old2New.insert({From.End, To.getEndNode()});
  }
  if (From.Memory) {
    Old2New.insert({From.Memory, To.getMemoryNode(Covariant)});
  }
  if (From.MemoryC) {
    Old2New.insert({From.MemoryC, To.getMemoryNode(Contravariant)});
  }

  // for partial cloning, we need to merge the primitive nodes.
  std::vector<std::pair<CGNode *, CGNode *>> toMerge;

  // clone all nodes. Clone PNINodes later.
  for (auto &NodeConst : From.Nodes) {
    if (NodeConst.isSpecial()) {
      continue;
    }
    auto &Node = const_cast<CGNode &>(NodeConst);
    NodeKey NewKey = TransformKey ? TransformKey(Node.key) : Node.key;
    CGNode *NewNode;
    NewNode = &To.createNodeNoPNI(NewKey, Node.Size);

    auto Pair = Old2New.insert({&Node, NewNode});
    assert(Pair.second && "clone: Node already cloned!?");
  }

  // clone all edges
  for (auto &Node : From.Nodes) {
    auto NewNode = Old2New.at(&Node);
    for (auto &Edge : Node.outEdges) {
      auto &Target = Edge.getTargetNode();
      auto NewTarget = Old2New.at(&Target);
      To.onlyAddEdge(*NewNode, *NewTarget, Edge.Label);
    }
  }

  // clone PNI Graph
  if (From.PG) {
    assert(From.PointerSize == From.PG->PointerSize);
    assert(To.PG);
    assert(To.PointerSize == To.PG->PointerSize);
    To.PG->cloneFrom(*From.PG, Old2New);
  }

  // merge primitive nodes
  for (auto &Ent : toMerge) {
    To.mergeNodeTo(*Ent.first, *Ent.second);
  }

  for (auto &N : From.StartNodes) {
    To.StartNodes.insert(Old2New.at(N));
  }
  for (auto &N : From.EndNodes) {
    To.EndNodes.insert(Old2New.at(N));
  }

  // clone the RevVariance map
  for (auto &Ent : From.RevVariance) {
    To.RevVariance.insert({Old2New.at(Ent.first), Old2New.at(Ent.second)});
  }
}

ConstraintGraph
ConstraintGraph::clone(std::map<const CGNode *, CGNode *> &Old2New) const {
  ConstraintGraph G(Ctx, PointerSize, Name, !(bool)PG);
  clone(Old2New, *this, G);
  return G;
}

CGNode::CGNode(ConstraintGraph &Parent, NodeKey key, unsigned Size)
    : Parent(Parent), Id(ValueNamer::getId()), key(key), Size(Size) {}

CGNode::CGNode(ConstraintGraph &Parent, NodeKey key, llvm::Type *LowTy)
    : Parent(Parent), Id(ValueNamer::getId()), key(key),
      Size(Parent.PG ? ::notdec::getSize(LowTy, Parent.PointerSize)
                     : 0) {
  if (Parent.PG) {
    auto N = Parent.PG->createPNINode(LowTy);
    PNIGraph::addPNINodeTarget(*this, *N);
  }
}

CGNode::CGNode(ConstraintGraph &Parent, NodeKey key, PNINode *N)
    : Parent(Parent), Id(ValueNamer::getId()), key(key),
      Size(Parent.PG ? N->getSize() : 0), PNIVar(nullptr) {
  if (N != nullptr) {
    assert(Parent.PG != nullptr);
  }
  if (Parent.PG) {
    assert(N != nullptr);
    if (N != nullptr) {
      assert(&N->getParent() == Parent.PG.get());
      PNIGraph::addPNINodeTarget(*this, *N);
    }
  }
}

bool isPointerRelated(const FieldLabel &FL) {
  if (auto *OL = FL.getAs<OffsetLabel>()) {
    return true;
  } else if (auto *LL = FL.getAs<LoadLabel>()) {
    return true;
  } else if (auto *SL = FL.getAs<StoreLabel>()) {
    return true;
  }
  return false;
}

bool CGNode::hasIncomingLoadOrStore() const {
  for (auto *Edge : inEdges) {
    if (isLoadOrStore(Edge->getLabel())) {
      return true;
    }
  }
  return false;
}

bool CGNode::hasPointerEdge() const {
  for (auto &Edge : outEdges) {
    if (auto *FL = Edge.getLabel().getAs<ForgetLabel>()) {
      if (isPointerRelated(FL->label)) {
        return true;
      }
    } else if (auto *RL = Edge.getLabel().getAs<RecallLabel>()) {
      if (isPointerRelated(RL->label)) {
        return true;
      }
    }
  }
  return false;
}

bool CGNode::isSpecial() const { return isStartOrEnd() || isMemory(); }

bool CGNode::isMemory() const {
  return Parent.getMemoryNodeOrNull(Covariant) == this ||
         Parent.getMemoryNodeOrNull(Contravariant) == this;
}

bool CGNode::isStartOrEnd() const {
  return this == Parent.Start || this == Parent.End;
}

bool CGNode::isPNIAndEdgeMatch() const {
  if (!getPNIVar()) {
    return true;
  }
  auto &PN = *getPNIVar();
  auto Ty = PN.getPtrOrNum();
  switch (Ty) {
  case Number:
    if (hasPointerEdge()) {
      assert(false && "PNI is number type, but has pointer edge");
    }
    return true;
  case Pointer: { // error if all edges are forget prim.
    bool AllForgetPrim = true;
    // bool hasPrim = false;
    if (outEdges.empty()) {
      AllForgetPrim = false;
    }
    for (auto &Edge : outEdges) {
      bool IsForgetPrim = false;
      if (const auto *FB = Edge.getLabel().getAs<ForgetBase>()) {
        if (FB->Base.isPrimitive()) {
          IsForgetPrim = true;
        }
      }
      AllForgetPrim &= IsForgetPrim;
      // hasPrim |= IsForgetPrim;
    }
    if (AllForgetPrim) {
      assert(false &&
             "PNI is pointer type, but all edges are forget primitive");
    }
    // warn if there is forget primitive
    for (auto &Edge : outEdges) {
      if (auto *FB = Edge.getLabel().getAs<ForgetBase>()) {
        if (FB->Base.isPrimitive()) {
          llvm::errs() << "Warning: TypeBuilderImpl::visitType: Pointer node "
                          "has forget primitive edge.: "
                       << toString(Edge.getLabel()) << "\n";
        }
      }
    }
    return true;
  }
  default:
    return true;
  }
}

} // namespace notdec::mlsub
