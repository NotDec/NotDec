#ifndef _NOTDEC_RETYPD_GRAPH_H_
#define _NOTDEC_RETYPD_GRAPH_H_

#include <list>
#include <llvm/IR/AssemblyAnnotationWriter.h>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <llvm/ADT/DirectedGraph.h>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/simple_ilist.h>
#include <llvm/Support/GraphWriter.h>

#include "TypeRecovery/PointerNumberIdentification.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include "Utils/Range.h"

namespace notdec {
struct ConstraintsGenerator;
}

namespace notdec::retypd {

struct CGNode;
struct ConstraintGraph;

struct NodeKey {
  TypeVariable Base;
  Variance SuffixVariance = Covariant;
  bool IsNewLayer = false;
  NodeKey(const TypeVariable &Base) : Base(Base) {}
  NodeKey(const TypeVariable &Base, Variance V)
      : Base(Base), SuffixVariance(V) {}

  std::optional<std::pair<FieldLabel, NodeKey>> forgetOnce() const;
  std::string str() const {
    return (IsNewLayer ? "F: " : "") + toString(Base) +
           toString(SuffixVariance);
  }

  // Comparator for stored in a std::map
  // https://stackoverflow.com/questions/26918912/efficient-operator-with-multiple-members
  bool operator<(const NodeKey &rhs) const {
    return std::tie(Base, SuffixVariance, IsNewLayer) <
           std::tie(rhs.Base, rhs.SuffixVariance, rhs.IsNewLayer);
  }
};

std::string toString(const NodeKey &K);

// Follows llvm/unittests/ADT/DirectedGraphTest.cpp
struct CGNode;
struct CGEdge;
using CGNodeBase = llvm::DGNode<CGNode, CGEdge>;
using CGEdgeBase = llvm::DGEdge<CGNode, CGEdge>;
using CGBase = llvm::DirectedGraph<CGNode, CGEdge>;

struct CGNode : CGNodeBase {
  ConstraintGraph &Parent;
  const NodeKey key;
  std::set<CGEdge> outEdges;
  unsigned int Size;

  // Map from CGNode to SSGNode using union-find
  // We will not remove CGNode from the graph, but just update, so it is safe to
  // use raw pointer here.
  PNINode *PNIVar = nullptr;
  PNINode *getPNIVar() { return PNIVar; }

protected:
  friend struct PNIGraph;
  void setPNIVar(PNINode *N) { PNIVar = N; }

public:
  CGNode(ConstraintGraph &Parent, NodeKey key, unsigned int Size);
  std::string str() { return key.str() + "-" + PNIVar->str(); }
  void onUpdatePNType();
  void setAsPtrAdd(CGNode *Other, OffsetRange Off);
};

struct CGEdge : CGEdgeBase {
  EdgeLabel Label;
  CGEdge(CGNode &Target, EdgeLabel &L) : CGEdgeBase(Target), Label(L) {}
  bool operator<(const CGEdge &rhs) const {
    auto p1 = &TargetNode;
    auto p2 = &rhs.TargetNode;
    return std::tie(Label, p1) < std::tie(rhs.Label, p2);
  }
};

struct ConstraintGraph : CGBase {
  std::string Name;
  PNIGraph PG;
  std::map<NodeKey, CGNode> Nodes;
  // TODO split constraint generation and solving
  // std::vector<Constraint> AddConstraints;
  std::set<CGNode *> StartNodes;
  std::set<CGNode *> EndNodes;
  std::vector<std::tuple<CGNode *, CGNode *, rexp::PRExp>> PathSeq;
  std::map<CGNode *, std::set<std::pair<FieldLabel, CGNode *>>> ReachingSet;
  CGNode *Start = nullptr;
  CGNode *End = nullptr;
  bool isLayerSplit = false;
  // TODO prevent name collision
  static const char *Memory;
  CGNode *MemoryNode = nullptr;

  ConstraintGraph(ConstraintsGenerator *CG, std::string FuncName);
  CGNode &getOrInsertNode(const NodeKey &N, unsigned int Size = 0);
  // Interface for initial constraint insertion
  void addConstraint(const TypeVariable &sub, const TypeVariable &sup);

  // Main interface for constraint simplification
  std::vector<SubTypeConstraint>
  simplify(std::set<std::string> &InterestingVars);

  void solve();

  // internal steps
  void saturate();
  void layerSplit();
  void buildPathSequence();
  std::vector<SubTypeConstraint> solve_constraints_between();
  void addRecalls(CGNode &N);
  void addForgets(CGNode &N);
  void printGraph(const char *DotFile);
  std::vector<SubTypeConstraint> toConstraints();
  static ConstraintGraph fromConstraints(std::string FuncName,
                                         std::vector<Constraint> &Cons);

protected:
  // Graph related operations
  void removeEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    assert(isLayerSplit);
    auto it = From.outEdges.find(CGEdge(To, Label));
    assert(it != From.outEdges.end());
    From.removeEdge(const_cast<CGEdge &>(*it));
    From.outEdges.erase(it);
  }
  bool addEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    if (&From == &To) {
      assert(std::holds_alternative<One>(Label));
      return false;
    }
    if (!isLayerSplit) {
      // maintain the initial reaching push/forget set.
      if (std::holds_alternative<ForgetLabel>(Label)) {
        auto Capa = std::get<ForgetLabel>(Label);
        ReachingSet[&To].insert({Capa.label, &From});
      } else if (std::holds_alternative<One>(Label)) {
        // unify PN
        From.getPNIVar()->unifyPN(*To.getPNIVar());
      }
    }
    auto it = From.outEdges.emplace(To, Label);
    if (it.second) {
      connect(From, To, const_cast<CGEdge &>(*it.first));
    }
    return it.second;
  }
  void replaceTypeVarWith(CGNode &Node, const TypeVariable &New);
  friend struct CGNode;
  void replaceNodeKey(CGNode &Node, const TypeVariable &NewVar);
  void addLeftRecalls(const TypeVariable &sub);
  // void addRightForgets(const TypeVariable &sup);

public:
  void setPointer(CGNode &Node) {
    Node.getPNIVar()->setPtrOrNum(retypd::Pointer);
  }
};

std::vector<SubTypeConstraint> expToConstraints(rexp::PRExp E);

} // namespace notdec::retypd

namespace llvm {

//===----------------------------------------------------------------------===//
// GraphTraits specializations for ConstraintGraph
//===----------------------------------------------------------------------===//
using notdec::retypd::CGEdge;
using notdec::retypd::CGNode;
using notdec::retypd::ConstraintGraph;
template <> struct llvm::GraphTraits<CGNode *> {
  using NodeRef = CGNode *;

  static CGNode *DGTestGetTargetNode(DGEdge<CGNode, CGEdge> *P) {
    return &P->getTargetNode();
  }

  // Provide a mapped iterator so that the GraphTrait-based implementations can
  // find the target nodes without having to explicitly go through the edges.
  using ChildIteratorType =
      mapped_iterator<CGNode::iterator, decltype(&DGTestGetTargetNode)>;
  using ChildEdgeIteratorType = CGNode::iterator;

  static NodeRef getEntryNode(NodeRef N) { std::abort(); }
  static ChildIteratorType child_begin(NodeRef N) {
    return ChildIteratorType(N->begin(), &DGTestGetTargetNode);
  }
  static ChildIteratorType child_end(NodeRef N) {
    return ChildIteratorType(N->end(), &DGTestGetTargetNode);
  }

  static ChildEdgeIteratorType child_edge_begin(NodeRef N) {
    return N->begin();
  }
  static ChildEdgeIteratorType child_edge_end(NodeRef N) { return N->end(); }
};

template <>
struct GraphTraits<ConstraintGraph *> : public GraphTraits<CGNode *> {
  using nodes_iterator = ConstraintGraph::iterator;
  static NodeRef getEntryNode(ConstraintGraph *DG) { return DG->Start; }
  static nodes_iterator nodes_begin(ConstraintGraph *DG) { return DG->begin(); }
  static nodes_iterator nodes_end(ConstraintGraph *DG) { return DG->end(); }
};

template <>
struct DOTGraphTraits<ConstraintGraph *> : public DefaultDOTGraphTraits {
  using GraphRef = ConstraintGraph *;
  using NodeRef = CGNode *;
  DOTGraphTraits(bool isSimple = false) : DefaultDOTGraphTraits(isSimple) {}
  static std::string getGraphName(GraphRef CG) { return CG->Name; }
  static std::string getNodeLabel(const NodeRef Node, GraphRef CG) {
    return Node->key.str();
  }

  std::string
  getEdgeAttributes(const NodeRef Node,
                    llvm::GraphTraits<CGNode *>::ChildIteratorType I,
                    GraphRef CG) {
    return std::string("label=\"") +
           notdec::retypd::toString((*I.getCurrent())->Label) + "\"";
  }
};

} // namespace llvm

#endif
