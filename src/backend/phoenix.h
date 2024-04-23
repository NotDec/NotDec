#ifndef _NOTDEC_BACKEND_PHOENIX_H_
#define _NOTDEC_BACKEND_PHOENIX_H_

#include "backend/Dominators.h"
#include "backend/structural-analysis.h"

namespace notdec::backend {

class Phoenix : IStructuralAnalysis {
protected:
  bool isCanceled = false;
  CFGDomTree Dom;
  bool ReduceAcyclic(CFGBlock *Block);
  bool IsCyclic(CFGBlock *Block);
  bool ReduceCyclic(CFGBlock *Block);
  void ProcessUnresolvedRegions();

public:
  Phoenix(SAFuncContext &Ctx) : IStructuralAnalysis(Ctx) {}

  void execute() override;
};

} // namespace notdec::backend

#endif
