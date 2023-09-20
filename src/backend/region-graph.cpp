


#include "backend/region-graph.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/CFG.h>
namespace notdec::backend {

static inline int predCount(const llvm::BasicBlock* bb) {
    llvm::const_pred_iterator PI = llvm::pred_begin(bb), E = llvm::pred_end(bb);
    unsigned int predecessorCount = 0;
    for (;PI != E; ++PI) {
        predecessorCount++;
    }
    return predecessorCount;
}

static inline bool hasPred(const llvm::BasicBlock* bb) {
    return bb->getSinglePredecessor() != nullptr;
}

void RegionGraph::build() {
    for(llvm::BasicBlock& bb: func) {
        auto reg = createRegion(bb);
        btor.emplace(&bb, &reg);
        addNode(reg);
    }
}



}
