
#ifndef _NOTDEC_BACKEND_REGION_GRAPH_H_
#define _NOTDEC_BACKEND_REGION_GRAPH_H_

#include "backend/statements.h"

#include "llvm/ADT/DirectedGraph.h"
#include <llvm/IR/BasicBlock.h>
#include <map>
#include <vector>

namespace notdec::backend {

// following the usage of LLVM DirectedGraph
// See: llvm-project-llvmorg-14.0.6/llvm/include/llvm/Analysis/DDG.h
// the graph does not own the nodes and edges, but only hold the pointer.
// The nodes and edges may be allocated on the heap, but we use a vector in
// graph and node to take the ownership.
// To insert a node or edge, first create one in the vector, then take the
// pointer to add to the graph.
class Region;
class RegionEdge;
using RegionBase = llvm::DGNode<Region, RegionEdge>;
using RegionEdgeBase = llvm::DGEdge<Region, RegionEdge>;
using RegionGraphBase = llvm::DirectedGraph<Region, RegionEdge>;

// Region is a group of basic block, to represent a collapsed node during the
// algorithm.
class Region : public RegionBase {
  friend class SAFuncContext;

public:
  enum RegionType {
    Linear,
    Condition,
    Switch,
    Tail,
  };

protected:
  llvm::BasicBlock *head;
  Statements stmts;

  RegionType ty;
  // take the ownership of RegionEdge
  std::vector<RegionEdge> edges;

public:
  Region(RegionType ty, llvm::BasicBlock *head) : head(head), ty(ty) {}
  void addStmt(Statement &stmt) { stmts.push_back(stmt); }
  void setType(RegionType ty) { this->ty = ty; }
  llvm::BasicBlock *getHead() { return head; }

  bool isEqualTo(const Region &N) const { return this == &N; }
};

class RegionEdge : public RegionEdgeBase {
public:
  RegionEdge(Region &t) : RegionEdgeBase(t) {}
  bool isEqualTo(const RegionEdge &E) const {
    return &getTargetNode() == &E.getTargetNode();
  }
};

class RegionGraph : public RegionGraphBase {
  friend class SAFuncContext;

  // take the ownership of Region
  std::vector<Region> regions;

  // the block belong to which region
  std::map<llvm::BasicBlock *, Region *> btor;
};

} // namespace notdec::backend

#endif
