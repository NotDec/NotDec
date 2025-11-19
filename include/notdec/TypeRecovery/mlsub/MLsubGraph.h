#ifndef _BINARYSUB_MLSUBGRAPH_H_
#define _BINARYSUB_MLSUBGRAPH_H_

#include <cassert>
#include <list>
#include <llvm/ADT/Optional.h>
#include <llvm/IR/Type.h>
#include <memory>
#include <set>
#include <string>
#include <tuple>

#include <llvm/ADT/ilist.h>
#include <llvm/ADT/simple_ilist.h>
#include <llvm/IR/AssemblyAnnotationWriter.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Support/JSON.h>

#include "TypeRecovery/LowTy.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/mlsub/PNDiff.h"
#include "TypeRecovery/mlsub/Schema.h"
#include "TypeRecovery/retypd/Schema.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"

namespace notdec::mlsub {
struct ConstraintsGenerator;
}

namespace notdec::mlsub {

using retypd::toString;
using retypd::Contravariant;
using retypd::Covariant;
using retypd::Variance;

struct CGNode;
struct CGEdge;
struct ConstraintGraph;

struct CGNode {
  ConstraintGraph &Parent;
  unsigned long Id = 0;
  Variance Vari = Covariant;
  unsigned Size = 0;
  std::set<CGEdge *> inEdges;
  std::set<CGEdge> outEdges;
  // For Contravariant, it is outgoing edges.
  // For Covariant, it is incoming edges.
  std::set<CGNode *> flowEdges;

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
  bool hasNoPNI() const { return PNIVar == nullptr || PNIVar->isNull(); }
  bool isPNIPtr() const { return PNIVar != nullptr && PNIVar->isPointer(); }
  bool isPNIUnknown() const { return PNIVar != nullptr && PNIVar->isUnknown(); }

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
  // Create node with no PNINode, regardless of PG != nullptr.
  CGNode(ConstraintGraph &Parent, Variance V, unsigned Size)
      : Parent(Parent), Id(ValueNamer::getId()), Size(Size) {}
  CGNode(ConstraintGraph &Parent, Variance V, llvm::Type *LowTy);
  // Creating a new node with low type and set PNI accordingly.
  CGNode(ConstraintGraph &Parent, Variance V, PNINode *N);

  std::string str() const {
    return std::to_string(Id) + toString(Vari) +
           (PNIVar != nullptr ? "-" + PNIVar->serialize() : "");
  }

  Variance getVariance() const { return Vari; }
  bool isPNIPointer() const {
    return getPNIVar() != nullptr && getPNIVar()->isPointer();
  }
  unsigned getSize() const { return Size; }

  using iteratorTy = std::list<CGNode>::iterator;
  iteratorTy getIterator() {
    size_t iterOffset = (size_t)&(*((iteratorTy) nullptr));
    iteratorTy iter;
    *(intptr_t *)&iter = (intptr_t)this - iterOffset;
    return iter;
  }
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

struct ConstraintGraph {
  std::string Name;
  std::unique_ptr<PNIGraph> PG;
  std::list<CGNode> Nodes;
  CGNode *Start = nullptr;
  CGNode *End = nullptr;
  long PointerSize = 0;

  ConstraintGraph(long PointerSize, std::string Name, bool disablePNI = false);

  CGNode &createNodeNoPNI(Variance V, unsigned Size);
  CGNode &createNode(Variance V, llvm::Type *LowType);
  CGNode &createNodeWithPNI(Variance V, PNINode *PNI);

  std::pair<CGNode &, CGNode &> createNodePair(llvm::Type *LowType);
  std::pair<CGNode &, CGNode &> createNodePairWithPNI(PNINode *PNI);

  void removeNode(CGNode &N);

  std::string getName() const { return Name; }
  using iterator = decltype(Nodes)::iterator;
  iterator begin() { return Nodes.begin(); }
  iterator end() { return Nodes.end(); }
  bool empty() { return Nodes.empty(); }

public:
  // Lazy initialization of special nodes.
  CGNode *getStartNode();
  CGNode *getEndNode();
  CGNode *getMemoryNodeOrNull(Variance V);
  CGNode *getMemoryNode(Variance V /*= Covariant*/);

  void printGraph(const char *DotFile) const;

  bool hasEdge(const CGNode &From, const CGNode &To, EdgeLabel Label) const {
    assert(&From.Parent == this && &To.Parent == this);
    return From.outEdges.count(
        CGEdge(const_cast<CGNode &>(From), const_cast<CGNode &>(To), Label));
  }

  void removeEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    assert(&From.Parent == this && &To.Parent == this);
    auto it = From.outEdges.find(CGEdge(From, To, Label));
    assert(it != From.outEdges.end());
    To.inEdges.erase(const_cast<CGEdge *>(&*it));
    From.outEdges.erase(it);
  }

  const CGEdge *addEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    assert(&From.Parent == this && "addEdge: NodeFrom is not in the graph");
    assert(&To.Parent == this && "addEdge: NodeTo is not in the graph");
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

  bool addFlowEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    assert(&From.Parent == this && "addFlowEdge: NodeFrom is not in the graph");
    assert(&To.Parent == this && "addFlowEdge: NodeTo is not in the graph");
    assert(From.Vari == Contravariant);
    assert(To.Vari == Covariant);
    auto It = From.flowEdges.insert(&To);
    if (It.second) {
      To.flowEdges.insert(&From);
    } else {
      assert(To.flowEdges.count(&From) > 0);
    }
    return It.second;
  }

  const CGEdge *addFlowEdge(CGNode &From, CGNode &To);

protected:
  CGNode &emplace(Variance V, llvm::Type *LowTy);

public:
  // void reTagBaseTV();
  void setPointer(CGNode &Node) { Node.getPNIVar()->setPtrOrNum(Pointer); }
};

} // namespace notdec::mlsub

#endif
