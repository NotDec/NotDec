#ifndef _NOTDEC_BACKEND_COMPOUNDCONDITIONBUILDER_H_
#define _NOTDEC_BACKEND_COMPOUNDCONDITIONBUILDER_H_

#include "backend/CFG.h"
#include "backend/structural-analysis.h"

namespace notdec::backend {

/// find and build logical and/or expressions
class CompoundConditionBuilder : IStructuralAnalysis {
public:
  CompoundConditionBuilder(SAFuncContext &Ctx) : IStructuralAnalysis(Ctx) {}

  bool maybeCoalesce(CFGBlock *block);
  void rebuildGraph(CFGBlock *head, CFGBlock *redundant, CFGBlock *body,
                    CFGBlock *succ);
  void execute() override;
};

} // namespace notdec::backend

#endif
