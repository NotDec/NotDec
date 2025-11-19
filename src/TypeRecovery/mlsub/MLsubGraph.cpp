#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Type.h>
#include <memory>
#include <string>
#include <utility>

#include <llvm/ADT/SCCIterator.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

#include "TypeRecovery/RExp.h"
#include "TypeRecovery/mlsub/MLsubGraph.h"
#include "TypeRecovery/mlsub/PNDiff.h"
#include "TypeRecovery/retypd/Schema.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"

#define DEBUG_TYPE "mlsub_graph"

using namespace llvm;

namespace notdec::mlsub {

using retypd::Contravariant;
using retypd::Covariant;
using retypd::rexp::PRExp;

CGNode::CGNode(ConstraintGraph &Parent, Variance V, llvm::Type *LowTy)
    : Parent(Parent), Id(ValueNamer::getId()), Vari(V),
      Size(Parent.PG ? ::notdec::getSize(LowTy, Parent.PointerSize) : 0) {
  if (Parent.PG) {
    auto N = Parent.PG->createPNINode(LowTy);
    PNIGraph::addPNINodeTarget(*this, *N);
  }
}

CGNode::CGNode(ConstraintGraph &Parent, Variance V, PNINode *N)
    : Parent(Parent), Id(ValueNamer::getId()), Vari(V),
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

ConstraintGraph::ConstraintGraph(long PointerSize, std::string Name,
                                 bool disablePNI)
    : Name(Name), PointerSize(PointerSize) {
  if (!disablePNI) {
    PG = std::make_unique<PNIGraph>(*this, Name, PointerSize);
  }
}

CGNode &ConstraintGraph::createNodeNoPNI(Variance V, unsigned Size) {
  return Nodes.emplace_back(*this, V, Size);
}

CGNode &ConstraintGraph::createNode(Variance V, llvm::Type *LowType) {
  return emplace(V, LowType);
}

CGNode &ConstraintGraph::createNodeWithPNI(Variance V, PNINode *PNI) {
  return Nodes.emplace_back(*this, V, PNI);
}

std::pair<CGNode &, CGNode &>
ConstraintGraph::createNodePairWithPNI(PNINode *PNI) {
  auto &N = createNodeWithPNI(Covariant, PNI);
  auto &NC = createNodeWithPNI(Contravariant, N.getPNIVar());
  addFlowEdge(NC, N);
  return {N, NC};
}

std::pair<CGNode &, CGNode &>
ConstraintGraph::createNodePair(llvm::Type *LowType) {
  auto &N = createNode(Covariant, LowType);
  auto &NC = createNodeWithPNI(Contravariant, N.getPNIVar());
  addFlowEdge(NC, N);
  return {N, NC};
}

void ConstraintGraph::removeNode(CGNode &Node) {
  assert(&Node.Parent == this && "removeNode: node not found");
  assert(Node.outEdges.empty() && "removeNode: node has out edges");
  assert(Node.inEdges.empty() && "removeNode: node has in edges");
  assert(Node.flowEdges.empty() && "removeNode: node has flow edges");
  PG->markRemoved(Node);
  Nodes.erase(Node.getIterator());
}

CGNode &ConstraintGraph::emplace(Variance V, llvm::Type *LowType) {
  return Nodes.emplace_back(*this, V, LowType);
}

} // namespace notdec::mlsub
