#ifndef _NOTDEC_BACKEND_GOTO_H_
#define _NOTDEC_BACKEND_GOTO_H_

#include "backend/structural-analysis.h"

namespace notdec::backend {

class Goto : IStructuralAnalysis {

public:
  Goto(SAFuncContext &ctx) : IStructuralAnalysis(ctx) {}

  void execute() override;
};

} // namespace notdec::backend

#endif
