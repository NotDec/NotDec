#ifndef _NOTDEC_RETYPD_GRAPH_H_
#define _NOTDEC_RETYPD_GRAPH_H_

#include <cassert>
#include <list>
#include <llvm/ADT/Optional.h>
#include <llvm/IR/Type.h>
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
#include <llvm/Support/JSON.h>

#include "TypeRecovery/LowTy.h"
#include "TypeRecovery/PointerNumberIdentification.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/Sketch.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"

namespace notdec {
struct ConstraintsGenerator;
}

namespace llvm {
template <class GraphType> struct OffsetOnly;
}

namespace notdec::retypd {

struct ConstraintSummary {
  std::vector<Constraint> Cons;
  long PointerSize = 0;
  std::map<TypeVariable, std::string> PNIMap;

  void fromJSON(TRContext &Ctx, const llvm::json::Object &Obj);
};

std::shared_ptr<retypd::ConstraintSummary>
buildPrintfSummary(retypd::TRContext &Ctx, uint32_t PointerSize,
                   llvm::StringRef FormatStr);

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
  bool operator==(const NodeKey &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }

  static NodeKey fromLabel(const EdgeLabel &L) {
    if (auto FB = std::get_if<ForgetBase>(&L)) {
      return NodeKey(FB->Base, FB->V);
    } else if (auto RB = std::get_if<RecallBase>(&L)) {
      return NodeKey(RB->Base, RB->V);
    } else {
      assert(false && "fromLabel: Not a ForgetBase or RecallBase");
    }
  }
};

std::string toString(const NodeKey &K);

struct CGNode;
struct CGEdge;

struct CGNode {
  ConstraintGraph &Parent;
  unsigned long Id = 0;
  const NodeKey key;
  unsigned Size;
  std::set<CGEdge *> inEdges;
  std::set<CGEdge> outEdges;

  using iterator = std::set<CGEdge>::iterator;
  using pred_iterator = std::set<CGEdge *>::iterator;
  iterator begin() { return outEdges.begin(); }
  iterator end() { return outEdges.end(); }
  pred_iterator pred_begin() { return inEdges.begin(); }
  pred_iterator pred_end() { return inEdges.end(); }

  unsigned long getId() const { return Id; }
  // Map from CGNode to SSGNode using union-find
  // We will not remove CGNode from the graph, but just update, so it is safe to
  // use raw pointer here.
  PNINode *PNIVar = nullptr;
  PNINode *getPNIVar() { return PNIVar; }
  const PNINode *getPNIVar() const { return PNIVar; }
  bool isSpecial() const;
  bool isStartOrEnd() const;
  bool hasNoPNI() const { return PNIVar == nullptr || PNIVar->isNull(); }
  bool isTop() const { return key.Base.isTop(); }
  bool isMemory() const { return !key.Base.hasLabel() && key.Base.isMemory(); }
  bool isPNIPtr() const { return PNIVar != nullptr && PNIVar->isPointer(); }

protected:
  friend struct PNIGraph;
  friend struct ConstraintGraph;
  void setPNIVar(PNINode *N) { PNIVar = N; }
  void setPNIPointer() {
    assert(PNIVar != nullptr);
    PNIVar->setPtr();
  }

public:
  CGNode(const CGNode &) = delete;
  CGNode(ConstraintGraph &Parent, NodeKey key, llvm::Type *LowTy);
  // Creating a new node with low type and set PNI accordingly.
  CGNode(ConstraintGraph &Parent, NodeKey key, PNINode *N);

  // Create node with no PNINode, regardless of PG != nullptr.
  // 1. for special nodes like #Start, #End
  // 2. for clone
  CGNode(ConstraintGraph &Parent, NodeKey key, unsigned Size);

  std::string str() { return key.str() + "-" + PNIVar->str(); }
  // handle update from PNI
  void onUpdatePNType();
  void setAsPtrAdd(CGNode *Other, OffsetRange Off);
  TypeVariable getTypeVar() { return key.Base; }
  bool hasPointerEdge() const;
  // Some sanity check
  bool isPNIAndEdgeMatch() const;
  bool isPNIPointer() const {
    return getPNIVar() != nullptr && getPNIVar()->isPointer();
  }
  unsigned getSize() const { return Size; }
};

struct CGEdge {
  CGNode &FromNode;
  CGNode &TargetNode;
  EdgeLabel Label;
  CGEdge(CGNode &From, CGNode &Target, const EdgeLabel &L)
      : FromNode(From), TargetNode(Target), Label(L) {}

  const EdgeLabel &getLabel() const { return Label; }

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
  const EdgeLabel &getLabel() const { return Edge.getLabel(); }

  // Act as a pointer
  RevEdge &operator*() { return *this; }
  RevEdge *operator->() { return this; }
};

struct ConstraintGraph {
  std::shared_ptr<retypd::TRContext> Ctx;
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
  // If the graph has eliminated the forget-label edges for building sketches.
  // TODO: update set and check of the flag, or use a enum for state.
  bool isSketchSplit = false;
  // TODO replace with datalayout?
  long PointerSize = 0;

  ConstraintGraph(std::shared_ptr<retypd::TRContext> Ctx, long PointerSize,
                  std::string Name, bool disablePNI = false);
  static void
  clone(std::map<const CGNode *, CGNode *> &Old2New,
        const ConstraintGraph &From, ConstraintGraph &To,
        std::function<NodeKey(const NodeKey &)> TransformKey = nullptr,
        bool Partial = false);
  // Node map is the core data of cloning.
  ConstraintGraph clone(std::map<const CGNode *, CGNode *> &Old2New) const;

  CGNode &getOrCreatePrim(std::string Name, llvm::Type *LowType) {
    auto &Ret =
        getOrInsertNode(TypeVariable::CreatePrimitive(*Ctx, Name), LowType);
    return Ret;
  }

  bool hasNode(const NodeKey &N);
  // must get the node
  CGNode &getNode(const NodeKey &N);
  CGNode *getNodeOrNull(const NodeKey &N);
  CGNode &createNodeNoPNI(const NodeKey &N, unsigned Size);
  CGNode &createNode(const NodeKey &N, llvm::Type *LowType);
  CGNode &createNodeWithPNI(const NodeKey &N, PNINode *PNI);
  CGNode &createNodeClonePNI(const NodeKey &N, PNINode *ON);
  CGNode &cloneNode(const CGNode &Other);
  void removeNode(const NodeKey &N);
  CGNode &getOrInsertNodeWithPNI(const NodeKey &N, PNINode *ON);
  CGNode &getOrInsertNode(const NodeKey &N, llvm::Type *LowType);

  std::string getName() const { return Name; }
  using iterator = decltype(Nodes)::iterator;
  iterator begin() { return Nodes.begin(); }
  iterator end() { return Nodes.end(); }
  bool empty() { return Nodes.empty(); }

  // Interface for initial constraint insertion
  void addConstraint(CGNode &NodeL, CGNode &NodeR);

  // Main interface for constraint simplification
  std::vector<SubTypeConstraint>
  simplifiedExpr(std::set<std::string> &InterestingVars) const;
  void linkPrimitives();
  void linkVars(std::set<std::string> &InterestingVars);
  void linkEndVars(std::set<std::string> &InterestingVars);
  ConstraintGraph simplify();
  void recoverBaseVars();
  void aggressiveSimplify();
  void linkConstantPtr2Memory();
  void changeStoreToLoad();
  // void lowTypeToSubType();

public:
  void solve();

  // Lazy initialization of special nodes.
  CGNode *getStartNode();
  CGNode *getEndNode();
  CGNode *getMemoryNodeOrNull(Variance V);
  CGNode *getMemoryNode(Variance V /*= Covariant*/);
  CGNode *getTopNode(Variance V);
  CGNode *getTopNodeOrNull(Variance V);

  // void solveSketchQueries(
  //     const std::vector<std::pair<
  //         TypeVariable,
  //         std::function<void(std::shared_ptr<retypd::Sketch>)>>> &Queries)
  //         const;
  // std::shared_ptr<Sketch> solveSketch(CGNode &N) const;
  
  // internal steps
  void saturate();
  void layerSplit();
  /// Intersect the language, that disallow recall and forget the same thing,
  /// which is a no-op. Must not have null/epsilon moves.
  void pushSplit();
  /// Intersect the language, that disallow contravariant node have both recall
  /// in and forget out. Only care about the relation where the recall and
  /// forget conjunction is covariant.
  void contraVariantSplit();
  void buildPathSequence();

  std::map<std::set<llvm::OffsetOnly<CGNode *>>,
           std::vector<std::tuple<llvm::OffsetOnly<CGNode *>,
                                  llvm::OffsetOnly<CGNode *>, rexp::PRExp>>>
      ElimCache;
  std::set<std::pair<CGNode *, OffsetRange>>
  getNodeReachableOffset(CGNode &Start);

  void linkLoadStore();
  void markVariance(std::map<CGNode *, Variance> *Initial = nullptr);
  /// Focus on the recall subgraph and use forgetBase to mark primitives. all
  /// nodes as accepting because our determinization algorithm keeps all
  /// reachable nodes.
  void sketchSplit();
  void ensureNoForgetLabel();
  std::vector<SubTypeConstraint> solve_constraints_between();
  void addRecalls(CGNode &N);
  void addForgets(CGNode &N);
  void printGraph(const char *DotFile) const;
  ConstraintGraph getSubGraph(const std::set<const CGNode *> &Roots,
                              bool AllReachable) const;
  std::set<const CGNode *>
  getReachableNodes(std::set<const CGNode *> Initial) const;
  void printSubGraph(const char *DotFile, std::set<const CGNode *> Roots) const;
  void printSubGraph(const char *DotFile, const CGNode *Root) const;
  void printEpsilonLoop(const char *DotFile,
                        std::set<const CGNode *> Nodes) const;
  std::vector<SubTypeConstraint> toConstraints();

  void instantiateConstraints(const ConstraintSummary &Summary);

  static ConstraintGraph fromConstraints(std::shared_ptr<retypd::TRContext> Ctx,
                                         std::string FuncName,
                                         const ConstraintSummary &Summary);

  bool hasEdge(const CGNode &From, const CGNode &To, EdgeLabel Label) const {
    assert(&From.Parent == this && &To.Parent == this);
    return From.outEdges.count(
        CGEdge(const_cast<CGNode &>(From), const_cast<CGNode &>(To), Label));
  }
  bool onlyAddEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    assert(&From.Parent == this && &To.Parent == this);
    if (TraceIds.count(From.getId()) || TraceIds.count(To.getId())) {
      std::cerr << "Add edge: " << From.str() << "(ID=" << From.getId()
                << ") -> " << To.str() << "(ID=" << From.getId()
                << ") Label: " << toString(Label) << "\n";
    }
    auto it = From.outEdges.emplace(From, To, Label);
    if (it.second) {
      To.inEdges.insert(const_cast<CGEdge *>(&*it.first));
    }
    return it.second;
  }
  void mergeNodeTo(CGNode &From, CGNode &To, bool NoSelfLoop = false);

  // Graph operations
  void removeEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    assert(&From.Parent == this && &To.Parent == this);
    auto it = From.outEdges.find(CGEdge(From, To, Label));
    assert(it != From.outEdges.end());
    To.inEdges.erase(const_cast<CGEdge *>(&*it));
    From.outEdges.erase(it);
  }

  bool addEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    if (&From == &To) {
      if (std::holds_alternative<One>(Label)) {
        return false;
      }
      std::cerr << "Warning: Non-null self edge: " << toString(From.key)
                << " To " << toString(To.key) << " Label: " << toString(Label)
                << "\n";
    }
    // do not maintain PNI during layer split.
    if (PG && !isLayerSplit) {
      if (auto F = std::get_if<ForgetLabel>(&Label)) {
        if (auto O = std::get_if<OffsetLabel>(&F->label)) {
          // unify PN
          From.getPNIVar()->unify(*To.getPNIVar());
          // also should be pointer, TODO: set or assert?
          assert(From.getPNIVar()->isPointer());
        }
      } else if (auto R = std::get_if<RecallLabel>(&Label)) {
        if (auto O = std::get_if<OffsetLabel>(&R->label)) {
          // unify PN
          From.getPNIVar()->unify(*To.getPNIVar());
          // also should be pointer, TODO: set or assert?
          assert(From.getPNIVar()->isPointer());
        }
      } else if (std::holds_alternative<One>(Label)) {
        // unify PN
        From.getPNIVar()->unify(*To.getPNIVar());
      }
    }
    return onlyAddEdge(From, To, Label);
  }

protected:
  std::pair<std::map<NodeKey, CGNode>::iterator, bool>
  emplace(const NodeKey &N, llvm::Type *LowTy);

  friend struct CGNode;
  template <typename GraphTy, typename NodeTy> friend struct NFADeterminizer;
  void replaceNodeKey(const TypeVariable &Old, const TypeVariable &New);
  void replaceNodeKeyImpl(const CGNode &Node, const NodeKey &Key);

public:
  // void reTagBaseTV();
  void setPointer(CGNode &Node) {
    Node.getPNIVar()->setPtrOrNum(retypd::Pointer);
  }
};

std::vector<SubTypeConstraint>
expToConstraints(std::shared_ptr<retypd::TRContext> Ctx, rexp::PRExp E);
std::string toString(const std::set<CGNode *> Set);

// inline NodeKey MakeContraVariant(NodeKey Key) {
//   assert(Key.SuffixVariance == Covariant);
//   Key.SuffixVariance = Contravariant;
//   return Key;
// }

inline NodeKey MakeReverseVariant(NodeKey Key) {
  Key.SuffixVariance = !Key.SuffixVariance;
  return Key;
}

inline bool isOffsetOrOne(const CGEdge &E) {
  if (auto Rec = std::get_if<notdec::retypd::RecallLabel>(&E.Label)) {
    return std::holds_alternative<notdec::retypd::OffsetLabel>(Rec->label);
  } else if (auto Rec = std::get_if<notdec::retypd::ForgetLabel>(&E.Label)) {
    return std::holds_alternative<notdec::retypd::OffsetLabel>(Rec->label);
  } else if (std::holds_alternative<notdec::retypd::One>(E.Label)) {
    return true;
  }
  return false;
}



bool hasOffsetEdge(CGNode &Start);

} // namespace notdec::retypd

namespace llvm {

//===----------------------------------------------------------------------===//
// GraphTraits specializations for ConstraintGraph
//===----------------------------------------------------------------------===//
using notdec::retypd::CGEdge;
using notdec::retypd::CGNode;
using notdec::retypd::ConstraintGraph;
using notdec::retypd::RevEdge;

template <> struct GraphTraits<CGNode *> {
  using NodeRef = CGNode *;

  static CGNode *CGEdgeToTarget(const CGEdge &P) {
    return const_cast<CGNode *>(&P.getTargetNode());
  }

  static CGEdge *CGEdgeToPtr(const CGEdge &P) {
    return const_cast<CGEdge *>(&P);
  }
  static NodeRef getEdgeTarget(const CGEdge *Edge) {
    return const_cast<CGNode *>(&Edge->getTargetNode());
  }
  static notdec::retypd::EdgeLabel getEdgeLabel(const CGEdge *Edge) {
    return Edge->Label;
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

  static bool isPrimitive(NodeRef N) { return N->key.Base.isPrimitive(); }
};

template <>
struct GraphTraits<ConstraintGraph *> : public GraphTraits<CGNode *> {
  using GraphRef = ConstraintGraph *;
  static CGNode *CGGetNode(ConstraintGraph::iterator::reference Entry) {
    return &Entry.second;
  }
  using nodes_iterator =
      mapped_iterator<ConstraintGraph::iterator, decltype(&CGGetNode)>;

  static NodeRef getEntryNode(GraphRef DG) { return DG->getStartNode(); }
  static NodeRef getExitNode(GraphRef DG) { return DG->getEndNode(); }

  static nodes_iterator nodes_begin(GraphRef DG) {
    return nodes_iterator(DG->begin(), &CGGetNode);
  }
  static nodes_iterator nodes_end(GraphRef DG) {
    return nodes_iterator(DG->end(), &CGGetNode);
  }

  static CGNode *getInner(NodeRef N) { return N; }
  static notdec::retypd::TypeVariable getTypeVar(NodeRef N) {
    return N->getTypeVar();
  }
  static std::string toString(NodeRef N) {
    return notdec::retypd::toString(N->key);
  }
};

template <>
struct DOTGraphTraits<ConstraintGraph *> : public DefaultDOTGraphTraits {
  using GraphRef = ConstraintGraph *;
  using NodeRef = CGNode *;
  DOTGraphTraits(bool isSimple = false) : DefaultDOTGraphTraits(isSimple) {}
  static std::string getGraphName(GraphRef CG) { return CG->Name; }
  static std::string getNodeLabel(const NodeRef Node, GraphRef CG) {
    return Node->key.str() +
           (Node->getPNIVar() != nullptr
                ? "\n" + Node->getPNIVar()->str() + " #" +
                      std::to_string(Node->getPNIVar()->getId())
                : "");
  }

  std::string getEdgeAttributes(const NodeRef Node,
                                llvm::GraphTraits<NodeRef>::ChildIteratorType I,
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
  static notdec::retypd::TypeVariable getTypeVar(NodeRef N) {
    return N.Graph->getTypeVar();
  }
  static CGNode *getInner(NodeRef N) { return N.Graph; }
  static std::string toString(NodeRef N) {
    return notdec::retypd::toString(N.Graph->key);
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

// Only look for offset edges
template <class GraphType> struct OffsetOnly {
  GraphType Graph;

  inline OffsetOnly(const GraphType G) : Graph(G) {}
  OffsetOnly &operator=(const OffsetOnly &other) {
    this->Graph = other.Graph;
    return *this;
  }
  // conversion
  operator GraphType() const { return Graph; }
};

template <typename BaseIt> struct FilteredIterator : BaseIt {
  typedef std::function<bool(const typename BaseIt::value_type &)> FilterType;
  FilteredIterator() = default;
  FilteredIterator(FilterType filter, BaseIt base, BaseIt end = {})
      : BaseIt(base), End(end), Filter(filter) {
    while (*this != End && !Filter(**this)) {
      ++*this;
    }
  }

  FilteredIterator &operator++() {
    do {
      BaseIt::operator++();
    } while (*this != End && !Filter(**this));
    return *this;
  }

  FilteredIterator operator++(int) {
    FilteredIterator copy = *this;
    ++*this;
    return copy;
  }

private:
  BaseIt End;
  FilterType Filter;
};

using notdec::retypd::isOffsetOrOne;

template <> struct GraphTraits<OffsetOnly<CGNode *>> {
  using NodeRef = OffsetOnly<CGNode *>;

  static CGNode *CGEdgeToTarget(const CGEdge &P) {
    return const_cast<CGNode *>(&P.getTargetNode());
  }

  static CGEdge *CGEdgeToPtr(const CGEdge &P) {
    return const_cast<CGEdge *>(&P);
  }
  static NodeRef getEdgeTarget(const CGEdge *Edge) {
    return const_cast<CGNode *>(&Edge->getTargetNode());
  }
  static notdec::retypd::EdgeLabel getEdgeLabel(const CGEdge *Edge) {
    return Edge->Label;
  }

  using FilteredIt = FilteredIterator<CGNode::iterator>;
  using ChildIteratorType =
      mapped_iterator<FilteredIt, decltype(&CGEdgeToTarget)>;

  using ChildEdgeIteratorType =
      mapped_iterator<FilteredIt, decltype(&CGEdgeToPtr)>;

  static ChildIteratorType child_begin(NodeRef N) {
    return ChildIteratorType(
        FilteredIt(isOffsetOrOne, N.Graph->begin(), N.Graph->end()),
        &CGEdgeToTarget);
  }
  static ChildIteratorType child_end(NodeRef N) {
    return ChildIteratorType(
        FilteredIt(isOffsetOrOne, N.Graph->end(), N.Graph->end()),
        &CGEdgeToTarget);
  }
  static ChildEdgeIteratorType child_edge_begin(NodeRef N) {
    return ChildEdgeIteratorType(
        FilteredIt(isOffsetOrOne, N.Graph->begin(), N.Graph->end()),
        &CGEdgeToPtr);
  }
  static ChildEdgeIteratorType child_edge_end(NodeRef N) {
    return ChildEdgeIteratorType(
        FilteredIt(isOffsetOrOne, N.Graph->end(), N.Graph->end()),
        &CGEdgeToPtr);
  }
  static notdec::retypd::TypeVariable getTypeVar(NodeRef N) {
    return N.Graph->getTypeVar();
  }
  static CGNode *getInner(NodeRef N) { return N.Graph; }
  static std::string toString(NodeRef N) {
    return notdec::retypd::toString(N.Graph->key);
  }

  static NodeRef getEntryNode(OffsetOnly<NodeRef> G) { return G.Graph; }
};

inline bool operator<(const OffsetOnly<CGNode *> &N1,
                      const OffsetOnly<CGNode *> &N2) {
  return N1.Graph < N2.Graph;
}

inline bool operator!=(const OffsetOnly<CGNode *> &N1,
                       const OffsetOnly<CGNode *> &N2) {
  return N1.Graph != N2.Graph;
}

template <> struct DenseMapInfo<OffsetOnly<CGNode *>> {
  using T = OffsetOnly<CGNode *>;
  using DIP = DenseMapInfo<CGNode *>;

  static inline T getEmptyKey() { return DIP::getEmptyKey(); }

  static inline T getTombstoneKey() { return DIP::getTombstoneKey(); }

  static unsigned getHashValue(const T PtrVal) {
    return DIP::getHashValue(PtrVal.Graph);
  }

  static bool isEqual(const T LHS, const T RHS) {
    return LHS.Graph == RHS.Graph;
  }
};

} // namespace llvm

#endif
