#ifndef _BINARYSUB_MLSUBGRAPH_H_
#define _BINARYSUB_MLSUBGRAPH_H_

#include <cassert>
#include <list>
#include <llvm/ADT/Optional.h>
#include <llvm/IR/Type.h>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include <llvm/ADT/ilist.h>
#include <llvm/ADT/simple_ilist.h>
#include <llvm/IR/AssemblyAnnotationWriter.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Support/JSON.h>

#include "TypeRecovery/LowTy.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/mlsub/PNDiff.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"

namespace notdec::mlsub {
struct ConstraintsGenerator;
}

namespace notdec::mlsub {

using retypd::Constraint;
using retypd::TypeVariable;
using retypd::TRContext;
using retypd::Variance;
using retypd::Covariant;
using retypd::FieldLabel;
using retypd::toString;
using retypd::EdgeLabel;
using retypd::ForgetBase;
using retypd::RecallBase;
using retypd::rexp::PRExp;
using retypd::SubTypeConstraint;
using retypd::RecallLabel;
using retypd::ForgetLabel;

struct ConstraintSummary {
  std::vector<Constraint> Cons;
  long PointerSize = 0;
  std::map<TypeVariable, std::string> PNIMap;

  void fromJSON(TRContext &Ctx, const llvm::json::Object &Obj);
};

struct CGNode;
struct CGEdge;
struct ConstraintGraph;

struct CGNode {
  ConstraintGraph &Parent;
  unsigned long Id = 0;
  Variance Vari = Covariant;
  unsigned Size;
  std::set<CGEdge *> inEdges;
  std::set<CGEdge> outEdges;
  std::set<CGNode *> flowEdgesOut;
  std::set<CGNode *> flowEdgesIn;

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
  bool isPNIPtr() const { return PNIVar != nullptr && PNIVar->isPointer(); }
  bool isPNIUnknown() const { return PNIVar != nullptr && PNIVar->isUnknown(); }
  bool isMemory() const;

protected:
  friend struct PNIGraph;
  friend struct ConstraintGraph;
  void setPNIVar(PNINode *N) { PNIVar = N; }
  void setPNIPointer() {
    assert(PNIVar != nullptr);
    PNIVar->setPtr();
  }
  void setPNINonPtr() {
    assert(PNIVar != nullptr);
    PNIVar->setNonPtr();
  }

public:
  CGNode(const CGNode &) = delete;
  CGNode(ConstraintGraph &Parent, Variance V, llvm::Type *LowTy);
  // Creating a new node with low type and set PNI accordingly.
  CGNode(ConstraintGraph &Parent, Variance V, PNINode *N);

  // Create node with no PNINode, regardless of PG != nullptr.
  // 1. for special nodes like #Start, #End
  // 2. for clone
  CGNode(ConstraintGraph &Parent, Variance V, unsigned Size);

  std::string str() const {
    return std::to_string(Id) + toString(Vari) +
           (PNIVar != nullptr ? "-" + PNIVar->serialize() : "");
  }

  Variance getVariance() const { return Vari; }
  // handle update from PNI
  void onUpdatePNType();
  void setAsPtrAdd(CGNode &Other, OffsetRange Off);
  bool hasPointerEdge() const;
  bool hasIncomingLoadOrStore() const;
  // Some sanity check
  bool isPNIAndEdgeMatch() const;
  bool isPNIPointer() const {
    return getPNIVar() != nullptr && getPNIVar()->isPointer();
  }
  unsigned getSize() const { return Size; }
  // after determinization, can get single outEdge target node by EdgeLabel
  CGNode *getLabelTarget(const EdgeLabel &L) const;
  CGNode *getLabelSource(const EdgeLabel &L) const;
  std::optional<int64_t> getSizeHint() const;

  using iteratorTy = std::list<CGNode>::iterator;
  iteratorTy getIterator() {
    size_t iterOffset = (size_t)&(*((iteratorTy) nullptr));
    iteratorTy iter;
    *(intptr_t *)&iter = (intptr_t)this - iterOffset;
    return iter;
  }
  void removeAllEdges();
  void removeInEdges();
  void removeOutEdges();
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
  std::shared_ptr<TRContext> Ctx;
  std::string Name;
  std::unique_ptr<PNIGraph> PG;
  std::list<CGNode> Nodes;
  std::map<CGNode *, CGNode *> RevVariance;
  // TODO split constraint generation and solving
  // std::vector<Constraint> AddConstraints;
  std::set<CGNode *> StartNodes;
  std::set<CGNode *> EndNodes;
  std::vector<std::tuple<CGNode *, CGNode *, PRExp>> PathSeq;
  std::map<CGNode *, std::set<std::pair<FieldLabel, CGNode *>>> ReachingSet;
  CGNode *Start = nullptr;
  CGNode *End = nullptr;
  CGNode *Memory = nullptr;
  CGNode *MemoryC = nullptr;
  bool isNotSymmetry = false;
  // If the graph has eliminated the forget-label edges for building sketches.
  // TODO: update set and check of the flag, or use a enum for state.
  bool isSketchSplit = false;
  // TODO replace with datalayout?
  long PointerSize = 0;

  ConstraintGraph(std::shared_ptr<TRContext> Ctx, long PointerSize,
                  std::string Name, bool disablePNI = false);

  enum SubtypeRelation {
    SR_Equal,
    SR_Sub,    // new node subtype to old node.
    SR_Parent, // old node subtype to new node.
    SR_None,
  };

  // isMergeClone: merge the graph to another graph.
  // TransformKey: 主要针对不冲突的情况，也转换NodeKey。
  // ConflictKeyRelation: 处理冲突，看看怎么连子类型边。
  static void
  clone(std::map<const CGNode *, CGNode *> &Old2New,
        const ConstraintGraph &From, ConstraintGraph &To,
        bool isMergeClone = false,
        std::function<SubtypeRelation(CGNode &, CGNode &)> ConflictKeyRelation =
            nullptr);
  // Node map is the core data of cloning.
  ConstraintGraph clone(std::map<const CGNode *, CGNode *> &Old2New) const;

  CGNode &getReverseVariant(CGNode &N) { return *RevVariance.at(&N); }

  CGNode &createNodeNoPNI(const NodeKey &N, unsigned Size);
  CGNode &createNode(const NodeKey &N, llvm::Type *LowType);
  CGNode &createNodeWithPNI(const NodeKey &N, PNINode *PNI);
  CGNode &createNodeClonePNI(const NodeKey &N, PNINode *ON);

  std::pair<CGNode &, CGNode &>
  createNodePair(NodeKey K, llvm::Type *LowType);
  std::pair<CGNode &, CGNode &>
  createNodePairWithPNI(NodeKey K, PNINode *PNI);

  CGNode &cloneNode(const CGNode &Other);
  void removeNode(CGNode &N);

  std::string getName() const { return Name; }
  using iterator = decltype(Nodes)::iterator;
  iterator begin() { return Nodes.begin(); }
  iterator end() { return Nodes.end(); }
  bool empty() { return Nodes.empty(); }

  // Main interface for constraint simplification
  std::vector<SubTypeConstraint>
  simplifiedExpr(std::set<std::string> &InterestingVars) const;
  void linkPrimitives();
  void linkVars(std::set<std::string> &InterestingVars,
                bool LinkLoadStores = true);
  void linkEndVars(std::set<std::string> &InterestingVars);
  ConstraintGraph simplify(std::optional<std::string> DebugDir = std::nullopt);
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

  // void solveSketchQueries(
  //     const std::vector<std::pair<
  //         TypeVariable,
  //         std::function<void(std::shared_ptr<Sketch>)>>> &Queries)
  //         const;
  // std::shared_ptr<Sketch> solveSketch(CGNode &N) const;

  // internal steps
  void applyPNIPolicy();
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

  std::set<std::pair<CGNode *, OffsetRange>>
  getNodeReachableOffset(CGNode &Start);

  void linkLoadStore();
  void markVariance(std::map<CGNode *, Variance> *Initial = nullptr);
  /// Focus on the recall subgraph, remove all forget label edges.
  void sketchSplit();
  void ensureNoForgetLabel();
  std::vector<SubTypeConstraint> solve_constraints_between();
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

  static ConstraintGraph fromConstraints(std::shared_ptr<TRContext> Ctx,
                                         std::string FuncName,
                                         const ConstraintSummary &Summary);

  bool hasEdge(const CGNode &From, const CGNode &To, EdgeLabel Label) const {
    assert(&From.Parent == this && &To.Parent == this);
    return From.outEdges.count(
        CGEdge(const_cast<CGNode &>(From), const_cast<CGNode &>(To), Label));
  }

  std::map<const CGEdge *, const CGEdge *> mergeNodeTo(CGNode &From, CGNode &To,
                                                       bool NoSelfLoop = false);

  const CGEdge *addRecallEdge(CGNode &From, CGNode &To, FieldLabel Label) {
    assert(!isNotSymmetry && "addEdgeDualVariance: not symmetry!");
    if (Label.isStore() || Label.isIn()) {
      assert(From.getVariance() != To.getVariance());
    } else {
      assert(From.getVariance() == To.getVariance());
    }
    auto Ret = addEdgeDualVariance(From, To, {RecallLabel{.label = Label}});
    addEdgeDualVariance(To, From, {ForgetLabel{.label = Label}});
    return Ret;
  }

  const CGEdge *addEdgeDualVariance(CGNode &From, CGNode &To, EdgeLabel Label);

  const CGEdge *addEdge(CGNode &From, CGNode &To, EdgeLabel Label);

  void removeEdgeDual(CGNode &From, CGNode &To, EdgeLabel Label) {
    assert(&From.Parent == this && &To.Parent == this);
    if (&From == &To) {
      removeEdge(From, To, Label);
      auto &CN = getReverseVariant(From);
      removeEdge(CN, CN, Label);
      return;
    }
    removeEdge(From, To, Label);
    if (Label.isOne()) {
      removeEdge(getReverseVariant(To), getReverseVariant(From), Label);
    } else {
      removeEdge(getReverseVariant(From), getReverseVariant(To), Label);
    }
  }

  void removeEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    assert(&From.Parent == this && &To.Parent == this);
    auto it = From.outEdges.find(CGEdge(From, To, Label));
    assert(it != From.outEdges.end());
    To.inEdges.erase(const_cast<CGEdge *>(&*it));
    From.outEdges.erase(it);
  }

  const CGEdge *onlyAddEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    assert(&From.Parent == this && "onlyAddEdge: NodeFrom is not in the graph");
    assert(&To.Parent == this && "onlyAddEdge: NodeTo is not in the graph");
    if (TraceIds.count(From.getId()) || TraceIds.count(To.getId())) {
      std::cerr << "Add edge: " << From.str() << "(ID=" << From.getId()
                << ") -> " << To.str() << "(ID=" << From.getId()
                << ") Label: " << toString(Label) << "\n";
    }
    auto it = From.outEdges.emplace(From, To, Label);
    if (it.second) {
      To.inEdges.insert(const_cast<CGEdge *>(&*it.first));
    }
    return &*it.first;
  }

protected:
  CGNode &emplace(const NodeKey &N, llvm::Type *LowTy);

  friend struct CGNode;
  template <typename GraphTy, typename NodeTy> friend struct NFADeterminizer;

public:
  // void reTagBaseTV();
  void setPointer(CGNode &Node) {
    Node.getPNIVar()->setPtrOrNum(Pointer);
  }
};

struct ConstraintSummaryInstance {
  ConstraintGraph &CG;
  const ConstraintSummary &Summary;
  std::map<NodeKey, CGNode *> NodeMap;
  std::map<size_t, PNINode *> ID2PNI;
  std::map<size_t, std::string> ID2PNIStr;

  CGNode &getNode(NodeKey Key) { return *NodeMap.at(Key); }

  CGNode &getOrInsertNodeWithPNI(NodeKey Key, PNINode *PN) {
    auto It = NodeMap.find(Key);
    if (It != NodeMap.end()) {
      return *It->second;
    } else {
      auto [N, NC] = CG.createNodePairWithPNI(Key, PN);
      NodeMap.emplace(N.key, &N);
      NodeMap.emplace(NC.key, &NC);
      return N;
    }
  }

  void addRecalls(CGNode &N);
  void addForgets(CGNode &N);
  void addConstraint(CGNode &From, CGNode &To);

  void instantiateConstraints();
};

std::vector<SubTypeConstraint>
expToConstraints(std::shared_ptr<TRContext> Ctx, PRExp E);
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
  if (auto Rec = E.Label.getAs<RecallLabel>()) {
    return Rec->label.isOffset();
  } else if (auto Rec = E.Label.getAs<ForgetLabel>()) {
    return Rec->label.isOffset();
  } else if (E.Label.isOne()) {
    return true;
  }
  return false;
}

bool hasOffsetEdge(CGNode &Start);
const CGEdge *getOnlyLoadOrStoreEdge(CGNode &N);
unsigned countLoadOrStoreEdge(CGNode &N);

} // namespace notdec::mlsub

#endif
