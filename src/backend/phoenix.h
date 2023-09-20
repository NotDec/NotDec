#ifndef _NOTDEC_BACKEND_PHOENIX_H_
#define _NOTDEC_BACKEND_PHOENIX_H_

#include "backend/region-graph.h"
#include "backend/structural-analysis.h"


namespace notdec::backend {

class Phoenix: IStructuralAnalysis
{
    RegionGraph graph;
    bool isCanceled = false;

public:
    Phoenix(llvm::Module& mod, llvm::Function& func): IStructuralAnalysis(mod, func), graph(func) {}

    virtual Region execute();
};


}

#endif
