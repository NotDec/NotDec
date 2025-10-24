
#include "TypeRecovery/NFAMinimize.h"
#include "Passes/ConstraintGenerator.h"
#include "TypeRecovery/Schema.h"
#include <cassert>
#include <llvm/ADT/iterator_range.h>
#include <queue>
#include <set>
#include <utility>

namespace notdec::retypd {

ConstraintGraph determinize(const ConstraintGraph *G) {
  ConstraintGraph NewG(G->Ctx, G->PointerSize, G->getName(), true);
  NFADeterminizer D(G, &NewG);
  D.run();
  return NewG;
}

std::map<std::set<CGNode *>, CGNode *> combineDFAMap(
    const std::map<std::set<llvm::InverseVal<CGNode *>>, CGNode *> &M1,
    const std::map<std::set<llvm::InverseVal<CGNode *>>, CGNode *> &M2,
    ConstraintGraph *G2) {
  std::map<std::set<CGNode *>, CGNode *> Result;

  // build a reverse map of M1
  std::map<CGNode *, std::set<CGNode *>> M1Rev;
  for (auto &Ent : M1) {
    for (auto &Node : Ent.first) {
      M1Rev[Ent.second].insert(Node.Graph);
    }
  }

  // M1 is a map from Original state to a G1 New state, M2 is a map from G1 New
  // states to a G2 New state. build a map of Original states to a G2 New state.
  for (auto &Ent : M2) {
    std::set<CGNode *> OriginalNodeSet;
    for (auto G1Node : Ent.first) {
      for (auto OriNode : M1Rev[G1Node.Graph]) {
        OriginalNodeSet.insert(OriNode);
      }
    }
    assert(&Ent.second->Parent == G2);
    Result[OriginalNodeSet] = Ent.second;
  }

  return Result;
}

ConstraintGraph minimize(const ConstraintGraph *G) {
  ConstraintGraph NewG(G->Ctx, G->PointerSize, G->getName(), G->PG == nullptr);
  NFAInvDeterminizer D(G, &NewG);
  D.run();
  ConstraintGraph NewG2(G->Ctx, G->PointerSize, G->getName(), G->PG == nullptr);
  NFAInvDeterminizer D2(&NewG, &NewG2);
  D2.run();
  return NewG2;
}

void minimizeTo(const ConstraintGraph *G, ConstraintGraph *To,
                std::map<std::set<CGNode *>, CGNode *> *NodeMap) {
  ConstraintGraph NewG(G->Ctx, G->PointerSize, G->getName(), G->PG == nullptr);
  NFAInvDeterminizer D(G, &NewG);
  D.run();
  // ConstraintGraph NewG2(G->Ctx, G->PointerSize, G->getName(), G->PG ==
  // nullptr);
  assert(To->Ctx == G->Ctx && To->PointerSize == G->PointerSize &&
         (bool)To->PG == (bool)G->PG);
  NFAInvDeterminizer D2(&NewG, To);
  D2.run();

  if (NodeMap) {
    // Combine two map
    assert(NodeMap->empty());
    *NodeMap = combineDFAMap(D.DTrans, D2.DTrans, To);
  }
}

ConstraintGraph
determinizeWithMap(const ConstraintGraph *G,
                   std::map<std::set<CGNode *>, CGNode *> &NodeMap) {
  ConstraintGraph NewG(G->Ctx, G->PointerSize, G->getName(), G->PG == nullptr);
  NFADeterminizer D(G, &NewG);
  D.run();
  NodeMap = D.DTrans;
  return NewG;
}

// When encountered a primitive node, ignore its edges.
std::set<CGNode *> countClosureNoPrimitiveEdges(const std::set<CGNode *> &N) {
  auto isNotPrimitive = [](const CGNode *Node) {
    return !Node->key.Base.isPrimitive();
  };
  return retypd::NFADeterminizer<>::countClosure(N, isNotPrimitive);
}

} // namespace notdec::retypd
