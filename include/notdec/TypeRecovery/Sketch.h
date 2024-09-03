#ifndef _NOTDEC_RETYPD_SKETCH_H
#define _NOTDEC_RETYPD_SKETCH_H

#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/Schema.h"
#include <llvm/ADT/ilist_node.h>
#include <llvm/Support/GraphWriter.h>
#include <memory>
#include <set>
#include <string>

namespace notdec::retypd {

struct ConstraintGraph;
struct Sketch;
struct SketchNode;
struct SketchEdge;

struct SketchNode : public node_with_erase<SketchNode, Sketch> {
  std::string Name;
  // Lattice Element
  Variance V = Covariant;
  std::string Element;
  std::set<SketchEdge *> inEdges;
  std::set<SketchEdge> outEdges;

  SketchNode(Sketch &Parent, Variance V) : node_with_erase(Parent), V(V) {
    if (V == Covariant) {
      Element = "top";
    } else if (V == Contravariant) {
      Element = "bottom";
    }
  }
  llvm::iplist<SketchNode>::iterator eraseFromParent();

  void setName(std::string Name) { this->Name = Name; }
  std::string getElement() const { return Element; }

  void join(std::map<const SketchNode *, SketchNode *> &Old2New,
            const SketchNode &Other);
  void meet(std::map<const SketchNode *, SketchNode *> &Old2New,
            const SketchNode &Other);
  void merge(std::string Element) {
    if (V == Covariant) {
      // get highest common sub type
      this->Element = notdec::retypd::meet(this->Element, Element);
    } else if (V == Contravariant) {
      // get lowest common parent type
      this->Element = notdec::retypd::join(this->Element, Element);
    } else {
      assert(false && "SketchNode::merge: unexpected variance");
    }
  }

  using iterator = std::set<SketchEdge>::iterator;
  iterator begin() { return outEdges.begin(); }
  iterator end() { return outEdges.end(); }
};

struct SketchEdge {
  SketchNode &From;
  SketchNode &To;
  FieldLabel Label;

  SketchEdge(SketchNode &From, SketchNode &To, const FieldLabel &Label)
      : From(From), To(To), Label(Label) {}

  const FieldLabel &getLabel() const { return Label; }

  const SketchNode &getTargetNode() const { return To; }
  SketchNode &getTargetNode() { return To; }
  const SketchNode &getSourceNode() const { return From; }
  SketchNode &getSourceNode() { return From; }

  bool operator<(const SketchEdge &rhs) const {
    auto p1 = std::addressof(To);
    auto p2 = std::addressof(rhs.To);
    return std::tie(Label, p1) < std::tie(rhs.Label, p2);
  }
};

struct Sketch {
  std::string Name;
  SketchNode *Root;

  // list for SketchNode
  using NodeListType = llvm::ilist<SketchNode>;
  NodeListType SketchNodes;
  static NodeListType Sketch::*getSublistAccess(SketchNode *) {
    return &Sketch::SketchNodes;
  }
  using iterator = NodeListType::iterator;
  iterator begin() { return SketchNodes.begin(); }
  iterator end() { return SketchNodes.end(); }
  SketchNode *getRoot() { return Root; }
  const SketchNode *getRoot() const { return Root; }

  Sketch(std::string Name) : Name(Name) {}
  static std::shared_ptr<Sketch> fromConstraintGraph(const ConstraintGraph &CG,
                                                     std::string Name);
  SketchNode *createNode(Variance V) {
    SketchNode *N = new SketchNode(*this, V);
    SketchNodes.push_back(N);
    return N;
  }
  void join(const Sketch &Other);
  void meet(const Sketch &Other);
  bool addEdge(SketchNode &From, SketchNode &To, const FieldLabel &Label) {
    auto It = From.outEdges.emplace(SketchEdge(From, To, Label));
    if (It.second) {
      To.inEdges.insert(const_cast<SketchEdge *>(&*It.first));
    }
    return It.second;
  }
  void removeEdge(SketchNode &From, SketchNode &To, const FieldLabel &Label) {
    auto It = From.outEdges.find(SketchEdge(From, To, Label));
    assert(It != From.outEdges.end());
    To.inEdges.erase(const_cast<SketchEdge *>(&*It));
    From.outEdges.erase(It);
    if (To.inEdges.empty() && (&To) != Root) {
      To.eraseFromParent();
    }
  }
  void printGraph(const char *DotFile) const;
};

} // namespace notdec::retypd

namespace llvm {

//===----------------------------------------------------------------------===//
// GraphTraits specializations for ConstraintGraph
//===----------------------------------------------------------------------===//
using notdec::retypd::Sketch;
using notdec::retypd::SketchEdge;
using notdec::retypd::SketchNode;

template <> struct GraphTraits<SketchNode *> {
  using NodeRef = SketchNode *;
  using EdgeRef = SketchEdge *;

  static NodeRef SkEdgeToTarget(const SketchEdge &P) {
    return const_cast<NodeRef>(&P.getTargetNode());
  }

  static EdgeRef SkEdgeToPtr(const SketchEdge &P) {
    return const_cast<EdgeRef>(&P);
  }

  using ChildIteratorType =
      mapped_iterator<SketchNode::iterator, decltype(&SkEdgeToTarget)>;
  using ChildEdgeIteratorType =
      mapped_iterator<SketchNode::iterator, decltype(&SkEdgeToPtr)>;

  static ChildIteratorType child_begin(NodeRef N) {
    return ChildIteratorType(N->begin(), &SkEdgeToTarget);
  }
  static ChildIteratorType child_end(NodeRef N) {
    return ChildIteratorType(N->end(), &SkEdgeToTarget);
  }

  static ChildEdgeIteratorType child_edge_begin(NodeRef N) {
    return ChildEdgeIteratorType(N->begin(), &SkEdgeToPtr);
  }
  static ChildEdgeIteratorType child_edge_end(NodeRef N) {
    return ChildEdgeIteratorType(N->end(), &SkEdgeToPtr);
  }
};

template <> struct GraphTraits<Sketch *> : public GraphTraits<SketchNode *> {
  using GraphRef = Sketch *;
  static NodeRef SkGetNode(Sketch::iterator::reference Entry) { return &Entry; }
  using nodes_iterator =
      mapped_iterator<Sketch::iterator, decltype(&SkGetNode)>;

  static NodeRef getEntryNode(GraphRef G) { return G->getRoot(); }

  static nodes_iterator nodes_begin(GraphRef DG) {
    return nodes_iterator(DG->begin(), &SkGetNode);
  }
  static nodes_iterator nodes_end(GraphRef DG) {
    return nodes_iterator(DG->end(), &SkGetNode);
  }
};

template <> struct DOTGraphTraits<Sketch *> : public DefaultDOTGraphTraits {
  using GraphRef = Sketch *;
  using NodeRef = SketchNode *;
  DOTGraphTraits(bool isSimple = false) : DefaultDOTGraphTraits(isSimple) {}
  static std::string getGraphName(GraphRef CG) { return CG->Name; }
  static std::string getNodeLabel(const NodeRef Node, GraphRef CG) {
    return Node->getElement();
  }

  std::string getEdgeAttributes(const NodeRef Node,
                                llvm::GraphTraits<NodeRef>::ChildIteratorType I,
                                GraphRef CG) {
    return std::string("label=\"") +
           notdec::retypd::toString((*I.getCurrent()).Label) + "\"";
  }
};

} // namespace llvm

#endif
