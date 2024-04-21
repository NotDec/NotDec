// Phoenix: Native x86 Decompilation Using Semantics-Preserving Structural
// Analysis and Iterative Control-Flow Structuring

#include <iostream>
#include <llvm/Support/raw_ostream.h>

#include "backend/PostOrderCFGView.h"
#include "backend/phoenix.h"

namespace notdec::backend {

void Phoenix::execute() {
  CFG &CFG = FCtx.getCFG();
  int iterations = 0;
  do {
    if (isCanceled) {
      break;
    }
    ++iterations;
    if (iterations > 1000) {
      llvm::errs() << "Structure analysis stopped making progress, quitting."
                   << " Func: " << FCtx.getFunction().getName();
      break;
    }

    int oldCount = CFG.size();
    auto postView = PostOrderCFGView::create(&CFG);

    for (auto Block : *postView) {
      // Probe();
      bool Changed = false;
      // do {
      //   Changed = ReduceAcyclic(Block);
      //   if (!Changed && IsCyclic(Block)) {
      //     Changed = ReduceCyclic(Block);
      //   }
      // } while (Changed);

      // if (CFG.size() == oldCount && CFG.size() > 1) {
      //   // Didn't make any progress this round,
      //   // try refining unstructured regions
      //   ProcessUnresolvedRegions();
      // }
    }

  } while (CFG.size() > 1);
}

} // namespace notdec::backend
