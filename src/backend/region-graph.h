
#ifndef _NOTDEC_BACKEND_REGION_GRAPH_H_
#define _NOTDEC_BACKEND_REGION_GRAPH_H_

#include "backend/statements.h"

#include "llvm/ADT/DirectedGraph.h"
#include <llvm/IR/BasicBlock.h>
#include <map>

namespace notdec::backend {

class Region;
class RegionEdge;
using RegionBase = llvm::DGNode<Region, RegionEdge>;
using RegionEdgeBase = llvm::DGEdge<Region, RegionEdge>;
using RegionGraphBase = llvm::DirectedGraph<Region, RegionEdge>;

// Region is a group of basic block, to represent a collapsed node during the algorithm.
class Region: RegionBase {
    llvm::BasicBlock* head;
    Statements stmts;

    enum type {
        Linear,
        Condition,
        Switch,
        Tail,
    };
    type ty;

};

class RegionEdge: RegionEdgeBase {

};

// 对一个函数构建
class RegionGraph: RegionGraphBase {
    llvm::Function &func;
    std::map<llvm::BasicBlock*, Region*> btor;

    // actual owner of Region

public:
    RegionGraph(llvm::Function &func): func(func) {}
    
    void build();
};

}

#endif
