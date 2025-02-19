#include "TypeRecovery/Sketch.h"
#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/LowTy.h"
#include "TypeRecovery/Schema.h"
#include <cassert>
#include <map>
#include <memory>
#include <queue>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace notdec::retypd {

static inline void
insertNodeMap(std::map<const SketchNode *, SketchNode *> &Old2New,
              const SketchNode *Old, SketchNode *New) {
  if (Old2New.count(Old) == 0) {
    Old2New.insert(std::make_pair(Old, New));
  } else {
    assert(Old2New[Old] == New);
  }
}

static std::map<FieldLabel, const SketchNode *>
buildLabelMap(const SketchNode &N) {
  std::map<FieldLabel, const SketchNode *> LabelMap;
  for (auto &E : N.outEdges) {
    LabelMap.emplace(E.getLabel(), &E.getTargetNode());
  }
  return LabelMap;
}

static std::map<FieldLabel, SketchNode *> buildLabelMap(SketchNode &N) {
  std::map<FieldLabel, SketchNode *> LabelMap;
  for (auto &E : N.outEdges) {
    auto &Target = const_cast<SketchNode &>(E.getTargetNode());
    LabelMap.emplace(E.getLabel(), &Target);
  }
  return LabelMap;
}

/// We may need to remove node because it is unreachable.
llvm::iplist<SketchNode>::iterator SketchNode::eraseFromParent() {
  assert(inEdges.empty());
  // release all out edges
  for (auto &E : outEdges) {
    auto Target = const_cast<SketchNode &>(E.getTargetNode());
    Parent->removeEdge(*this, Target, E.getLabel());
  }
  return node_with_erase<SketchNode, Sketch>::eraseFromParent();
}

// join = supremum (least upper bound V)
// prune non-common edges
void SketchNode::join(std::map<const SketchNode *, SketchNode *> &Old2New,
                      const SketchNode &Other) {
  assert(V == Covariant);
  if (Old2New.count(&Other)) {
    // already merged
    return;
  }
  Element = notdec::retypd::join(Element, Other.Element);
  std::map<FieldLabel, const SketchNode *> OtherLabels = buildLabelMap(Other);
  std::vector<std::tuple<SketchNode *, SketchNode *, FieldLabel>> toRemove;
  std::vector<std::tuple<SketchNode *, const SketchNode *, Variance>> toMerge;

  // For each edge, if is a common edge, merge. Otherwise, remove.
  for (auto It = outEdges.begin(); It != outEdges.end(); ++It) {
    auto &Child = const_cast<SketchNode &>(It->getTargetNode());
    if (OtherLabels.count(It->getLabel()) == 0) {
      std::cerr << "Removing Sketch edge " << Name << " -> " << Child.Name
                << " (" << toString(It->getLabel()) << ")\n";
      toRemove.emplace_back(this, &Child, It->getLabel());
    } else {
      auto *Oth = OtherLabels.at(It->getLabel());
      toMerge.emplace_back(&Child, Oth, getVariance(It->getLabel()));
      insertNodeMap(Old2New, Oth, &Child);
    }
  }
  // handle toMerge
  for (auto &Ent : toMerge) {
    auto *Child = std::get<0>(Ent);
    auto *Oth = std::get<1>(Ent);

    Variance V = std::get<2>(Ent);
    V = combine(V, this->V);
    if (V == Covariant) {
      Child->join(Old2New, *Oth);
    } else if (V == Contravariant) {
      Child->meet(Old2New, *Oth);
    }
  }

  // handle toRemove
  for (auto &Ent : toRemove) {
    auto *From = std::get<0>(Ent);
    auto *To = std::get<1>(Ent);
    auto &Label = std::get<2>(Ent);
    Parent->removeEdge(*From, *To, Label);
  }
}

// meet = infimum (greatest lower bound)
// collect both edges
void SketchNode::meet(std::map<const SketchNode *, SketchNode *> &Old2New,
                      const SketchNode &Other) {
  assert(V == Contravariant);
  if (Old2New.count(&Other)) {
    // already merged
    return;
  }
  Element = notdec::retypd::meet(Element, Other.Element);
  std::map<FieldLabel, const SketchNode *> OtherLabels = buildLabelMap(Other);
  std::map<FieldLabel, SketchNode *> ThisLabels = buildLabelMap(*this);

  std::vector<std::tuple<SketchNode *, const SketchNode *, FieldLabel>> toAdd;
  std::vector<std::tuple<SketchNode *, const SketchNode *, Variance>> toMerge;

  // For each edge, if is a common edge, merge. Otherwise, add.
  for (auto &Ent : OtherLabels) {
    auto &Label = Ent.first;
    auto *OthChild = Ent.second;
    if (ThisLabels.count(Label) == 0) {
      toAdd.emplace_back(this, OthChild, Label);
    } else {
      auto *Child = ThisLabels.at(Label);
      toMerge.emplace_back(Child, OthChild, getVariance(Label));
    }
  }

  // handle toAdd
  for (auto &Ent : toAdd) {
    auto *From = std::get<0>(Ent);
    auto *OthChild = std::get<1>(Ent);
    auto &Label = std::get<2>(Ent);

    Variance Va = combine(getVariance(Label), this->V);
    assert(Va == OthChild->V);
    // If the node is not in the map, create it.
    SketchNode *New;
    if (Old2New.count(OthChild) == 0) {
      New = Parent->createNode(OthChild->V);
      New->setName(OthChild->Name);
      Old2New.insert(std::make_pair(OthChild, New));
    } else {
      New = Old2New.at(OthChild);
      assert(New->V == Va);
    }
    Parent->addEdge(*From, *New, Label);
  }

  // handle toMerge
  for (auto &Ent : toMerge) {
    auto *Child = std::get<0>(Ent);
    auto *Oth = std::get<1>(Ent);
    insertNodeMap(Old2New, Oth, Child);

    Variance Va = std::get<2>(Ent);
    Va = combine(Va, this->V);
    if (Va == Covariant) {
      Child->join(Old2New, *Oth);
    } else if (Va == Contravariant) {
      Child->meet(Old2New, *Oth);
    }
  }
}

void Sketch::join(const Sketch &Other) {
  std::map<const SketchNode *, SketchNode *> Old2New;
  assert(getRoot()->V == Covariant);
  getRoot()->join(Old2New, *Other.getRoot());
}

void Sketch::meet(const Sketch &Other) {
  std::map<const SketchNode *, SketchNode *> Old2New;
  assert(getRoot()->V == Contravariant);
  getRoot()->meet(Old2New, *Other.getRoot());
}

std::shared_ptr<Sketch> Sketch::fromConstraintGraph(const ConstraintGraph &CG1,
                                                    std::string Name) {
  auto &CG = const_cast<ConstraintGraph &>(CG1);

  assert(CG.getStartNode()->outEdges.size() == 1);
  auto &Init = CG.getStartNode()->outEdges.begin()->getTargetNode();

  std::shared_ptr<Sketch> S =
      std::make_shared<Sketch>(Name.empty() ? toString(Init.key) : Name);
  std::map<const CGNode *, SketchNode *> NodeMap;

  auto getNode = [&](const CGNode &N) -> SketchNode * {
    auto It = NodeMap.find(&N);
    if (It != NodeMap.end()) {
      return It->second;
    }
    auto *SN = S->createNode(N.key.SuffixVariance);
    SN->setName(toString(N.key));
    NodeMap[&N] = SN;
    return SN;
  };

  // for each edge
  for (auto &Ent : CG.Nodes) {
    auto &Node = Ent.second;
    if (&Node == CG.getStartNode() || &Node == CG.getEndNode()) {
      continue;
    }
    for (auto Edge : Node.outEdges) {
      auto &Target = Edge.getTargetNode();

      auto &Label = Edge.getLabel();
      if (std::holds_alternative<RecallBase, One, ForgetLabel>(Label)) {
        assert(
            false &&
            "Sketch::fromConstraintGraph: unexpected RecallBase or One edge");
      } else if (auto *L = std::get_if<RecallLabel>(&Label)) {
        auto *SN = getNode(Node);
        auto *TN = getNode(Target);
        S->addEdge(*SN, *TN, L->label);
      } else if (auto *B = std::get_if<ForgetBase>(&Label)) {
        assert(&Target == CG.getEndNode());
        assert(B->Base.isPrimitive());
        auto *SN = getNode(Node);
        SN->merge(B->Base.getPrimitiveName());
      } else {
        assert(false && "Sketch::fromConstraintGraph: unexpected edge");
      }
    }
  }
  S->setRoot(NodeMap.at(&Init));
  return S;
}

void Sketch::printGraph(const char *DotFile) const {
  std::error_code EC;
  llvm::raw_fd_ostream OutStream(DotFile, EC);
  if (EC) {
    llvm::errs() << "Error: " << EC.message() << "\n";
    return;
  }
  llvm::WriteGraph(OutStream, const_cast<Sketch *>(this), false);
  OutStream.flush();
  OutStream.close();
}

} // namespace notdec::retypd
