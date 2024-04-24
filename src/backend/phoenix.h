#ifndef _NOTDEC_BACKEND_PHOENIX_H_
#define _NOTDEC_BACKEND_PHOENIX_H_

#include "backend/CFG.h"
#include "backend/Dominators.h"
#include "backend/structural-analysis.h"
#include <utility>
#include <vector>

namespace notdec::backend {

class Phoenix : IStructuralAnalysis {
protected:
  bool isCanceled = false;
  CFGDomTree Dom;
  std::vector<CFGBlock *> unresolvedSwitches;
  std::vector<std::pair<CFGBlock *, std::vector<CFGBlock *>>> unresolvedCycles;
  bool ReduceAcyclic(CFGBlock *Block);
  bool IsCyclic(CFGBlock *Block);
  bool ReduceCyclic(CFGBlock *Block);
  bool ProcessUnresolvedRegions();
  bool refineLoop(CFGBlock *head, std::vector<CFGBlock *> loopNodes);
  void refineIncSwitch(CFGBlock *switchHead);
  bool virtualizeReturn(CFGBlock *B);
  bool CoalesceTailRegion(CFGBlock *B,
                          llvm::iterator_range<CFG::iterator> range);
  bool LastResort(CFGBlock *B);

public:
  Phoenix(SAFuncContext &Ctx) : IStructuralAnalysis(Ctx) {}

  void execute() override;
};

} // namespace notdec::backend

#endif
