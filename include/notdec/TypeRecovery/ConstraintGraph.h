#ifndef _NOTDEC_RETYPD_GRAPH_H_
#define _NOTDEC_RETYPD_GRAPH_H_

#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <llvm/ADT/ilist.h>
#include <llvm/ADT/simple_ilist.h>
#include <llvm/IR/AssemblyAnnotationWriter.h>
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

struct CGNode;
struct CGEdge;

struct CGNode {
  ConstraintGraph &Parent;
  const NodeKey key;
  std::set<CGEdge *> inEdges;
  std::set<CGEdge> outEdges;
  unsigned int Size;

  using iterator = std::set<CGEdge>::iterator;
  using pred_iterator = std::set<CGEdge *>::iterator;
  iterator begin() { return outEdges.begin(); }
  iterator end() { return outEdges.end(); }
  pred_iterator pred_begin() { return inEdges.begin(); }
  pred_iterator pred_end() { return inEdges.end(); }

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

struct CGEdge {
  CGNode &FromNode;
  CGNode &TargetNode;
  EdgeLabel Label;
  CGEdge(CGNode &From, CGNode &Target, EdgeLabel &L)
      : FromNode(From), TargetNode(Target), Label(L) {}

  EdgeLabel getLabel() const { return Label; }

  const CGNode &getTargetNode() const { return TargetNode; }
  CGNode &getTargetNode() { return TargetNode; }
  const CGNode &getSourceNode() const { return FromNode; }
  CGNode &getSourceNode() { return FromNode; }

  bool operator<(const CGEdge &rhs) const {
    auto p1 = &TargetNode;
    auto p2 = &rhs.TargetNode;
    return std::tie(Label, p1) < std::tie(rhs.Label, p2);
  }
};

struct RevEdge {
  CGEdge &Edge;
  RevEdge(CGEdge &E) : Edge(E) {}
  CGNode &getTargetNode() { return Edge.getSourceNode(); }
  CGNode &getSourceNode() { return Edge.getTargetNode(); }
  EdgeLabel getLabel() const { return Edge.getLabel(); }

  // Act as a pointer
  RevEdge &operator*() { return *this; }
  RevEdge *operator->() { return this; }
};

struct ConstraintGraph {
  std::string Name;
  std::unique_ptr<PNIGraph> PG;
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

  ConstraintGraph(ConstraintsGenerator *CG, std::string Name,
                  bool disablePNI = false);
  ConstraintGraph clone(bool removePNI = false) const;
  CGNode &getOrInsertNode(const NodeKey &N, unsigned int Size = 0);

  std::string getName() { return Name; }
  using iterator = std::map<NodeKey, CGNode>::iterator;
  iterator begin() { return Nodes.begin(); }
  iterator end() { return Nodes.end(); }
  bool empty() { return Nodes.empty(); }

  // Interface for initial constraint insertion
  void addConstraint(const TypeVariable &sub, const TypeVariable &sup);

  // Main interface for constraint simplification
  std::vector<SubTypeConstraint>
  simplifiedExpr(std::set<std::string> &InterestingVars);
  ConstraintGraph simplify(std::set<std::string> &InterestingVars);
  void instantiate(const ConstraintGraph &Sum, size_t ID);

protected:
  // Create a new simplified.
  ConstraintGraph simplifyImpl(std::set<std::string> &InterestingVars) const;

public:
  void solve();

  // Lazy initialization of special nodes.
  CGNode *getStartNode();
  CGNode *getEndNode();
  CGNode *getMemoryNode();

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
    auto it = From.outEdges.find(CGEdge(From, To, Label));
    assert(it != From.outEdges.end());
    To.inEdges.erase(const_cast<CGEdge *>(&*it));
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
    return onlyAddEdge(From, To, Label);
  }
  bool onlyAddEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    auto it = From.outEdges.emplace(From, To, Label);
    if (it.second) {
      To.inEdges.insert(const_cast<CGEdge *>(&*it.first));
    }
    return it.second;
  }
  void replaceTypeVarWith(CGNode &Node, const TypeVariable &New);
  friend struct CGNode;
  template <typename GraphTy, typename NodeTy> friend struct NFADeterminizer;
  void replaceNodeKey(CGNode &Node, const TypeVariable &NewVar);
  // void addLeftRecalls(const TypeVariable &sub);
  // void addRightForgets(const TypeVariable &sup);

public:
  void setPointer(CGNode &Node) {
    Node.getPNIVar()->setPtrOrNum(retypd::Pointer);
  }
};

std::vector<SubTypeConstraint> expToConstraints(rexp::PRExp E);
std::string toString(const std::set<CGNode *> Set);

} // namespace notdec::retypd

namespace llvm {

//===----------------------------------------------------------------------===//
// GraphTraits specializations for ConstraintGraph
//===----------------------------------------------------------------------===//
using notdec::retypd::CGEdge;
using notdec::retypd::CGNode;
using notdec::retypd::ConstraintGraph;
using notdec::retypd::RevEdge;

template <> struct llvm::GraphTraits<CGNode *> {
  using NodeRef = CGNode *;

  static CGNode *CGEdgeToTarget(const CGEdge &P) {
    return const_cast<CGNode *>(&P.getTargetNode());
  }

  static CGEdge *CGEdgeToPtr(const CGEdge &P) {
    return const_cast<CGEdge *>(&P);
  }

  // Provide a mapped iterator so that the GraphTrait-based implementations can
  // find the target nodes without having to explicitly go through the edges.
  using ChildIteratorType =
      mapped_iterator<CGNode::iterator, decltype(&CGEdgeToTarget)>;
  using ChildEdgeIteratorType =
      mapped_iterator<CGNode::iterator, decltype(&CGEdgeToPtr)>;

  // static NodeRef getEntryNode(NodeRef N) { std::abort(); }
  static ChildIteratorType child_begin(NodeRef N) {
    return ChildIteratorType(N->begin(), &CGEdgeToTarget);
  }
  static ChildIteratorType child_end(NodeRef N) {
    return ChildIteratorType(N->end(), &CGEdgeToTarget);
  }

  static ChildEdgeIteratorType child_edge_begin(NodeRef N) {
    return ChildEdgeIteratorType(N->begin(), &CGEdgeToPtr);
  }
  static ChildEdgeIteratorType child_edge_end(NodeRef N) {
    return ChildEdgeIteratorType(N->end(), &CGEdgeToPtr);
  }
};

template <>
struct GraphTraits<ConstraintGraph *> : public GraphTraits<CGNode *> {
  static CGNode *CGGetNode(ConstraintGraph::iterator::reference Entry) {
    return &Entry.second;
  }
  using nodes_iterator =
      mapped_iterator<ConstraintGraph::iterator, decltype(&CGGetNode)>;

  static NodeRef getEntryNode(ConstraintGraph *DG) {
    return DG->getStartNode();
  }
  static NodeRef getExitNode(ConstraintGraph *DG) { return DG->getEndNode(); }

  static nodes_iterator nodes_begin(ConstraintGraph *DG) {
    return nodes_iterator(DG->begin(), &CGGetNode);
  }
  static nodes_iterator nodes_end(ConstraintGraph *DG) {
    return nodes_iterator(DG->end(), &CGGetNode);
  }
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
           notdec::retypd::toString((*I.getCurrent()).Label) + "\"";
  }
};

//===----------------------------------------------------------------------===//
// Inverse GraphTraits specializations for ConstraintGraph
//===----------------------------------------------------------------------===//

// Inverse by value
template <class GraphType> struct InverseVal {
  const GraphType Graph;

  inline InverseVal(const GraphType G) : Graph(G) {}
};

template <> struct GraphTraits<InverseVal<CGNode *>> {
  using NodeRef = InverseVal<CGNode *>;
  static CGNode *CGEdgeToSource(const CGEdge *P) {
    return const_cast<CGNode *>(&P->getSourceNode());
  }

  static RevEdge CGEdgeToRevEdge(const CGEdge *P) {
    return RevEdge(*const_cast<CGEdge *>(P));
  }

  // Provide a mapped iterator so that the GraphTrait-based implementations can
  // find the souce nodes without having to explicitly go through the edges.
  using ChildIteratorType =
      mapped_iterator<CGNode::pred_iterator, decltype(&CGEdgeToSource)>;
  using ChildEdgeIteratorType =
      mapped_iterator<CGNode::pred_iterator, decltype(&CGEdgeToRevEdge)>;

  // static NodeRef getEntryNode(InverseVal<CGNode *> G) { std::abort(); }
  static ChildIteratorType child_begin(NodeRef N) {
    return ChildIteratorType(N.Graph->pred_begin(), &CGEdgeToSource);
  }
  static ChildIteratorType child_end(NodeRef N) {
    return ChildIteratorType(N.Graph->pred_end(), &CGEdgeToSource);
  }
  static ChildEdgeIteratorType child_edge_begin(NodeRef N) {
    return ChildEdgeIteratorType(N.Graph->pred_begin(), &CGEdgeToRevEdge);
  }
  static ChildEdgeIteratorType child_edge_end(NodeRef N) {
    return ChildEdgeIteratorType(N.Graph->pred_end(), &CGEdgeToRevEdge);
  }
};

template <>
struct GraphTraits<InverseVal<ConstraintGraph *>>
    : public GraphTraits<InverseVal<CGNode *>> {

  static NodeRef getExitNode(InverseVal<ConstraintGraph *> G) {
    return G.Graph->getStartNode();
  }

  static NodeRef getEntryNode(InverseVal<ConstraintGraph *> G) {
    return G.Graph->getEndNode();
  }
};

inline bool operator<(const InverseVal<CGNode *> &N1,
                      const InverseVal<CGNode *> &N2) {
  return N1.Graph < N2.Graph;
}

} // namespace llvm

#endif
