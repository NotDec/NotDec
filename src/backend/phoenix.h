#ifndef _NOTDEC_BACKEND_PHOENIX_H_
#define _NOTDEC_BACKEND_PHOENIX_H_

#include "backend/region-graph.h"
#include "backend/structural-analysis.h"

namespace notdec::backend {

class Phoenix : IStructuralAnalysis {
  bool isCanceled = false;

public:
  Phoenix(SAFuncContext &ctx) : IStructuralAnalysis(ctx) {}

  virtual Region execute();
};

} // namespace notdec::backend

#endif
