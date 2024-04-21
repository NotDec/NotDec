#ifndef _NOTDEC_BACKEND_PHOENIX_H_
#define _NOTDEC_BACKEND_PHOENIX_H_

#include "backend/structural-analysis.h"

namespace notdec::backend {

class Phoenix : IStructuralAnalysis {
protected:
  bool isCanceled = false;

public:
  Phoenix(SAFuncContext &Ctx) : IStructuralAnalysis(Ctx) {}

  void execute() override;
};

} // namespace notdec::backend

#endif
