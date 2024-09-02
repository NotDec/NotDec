
#include "TypeRecovery/NFAMinimize.h"
#include "TypeRecovery/Schema.h"
#include "optimizers/ConstraintGenerator.h"
#include <cassert>
#include <llvm/ADT/iterator_range.h>
#include <queue>
#include <set>

namespace notdec::retypd {

ConstraintGraph determinize(const ConstraintGraph *G) {
  ConstraintGraph NewG(nullptr, G->getName(), true);
  NFADeterminizer D(G, &NewG);
  D.run();
  return NewG;
}

ConstraintGraph minimize(const ConstraintGraph *G) {
  ConstraintGraph NewG(nullptr, G->getName(), true);
  NFAInvDeterminizer D(G, &NewG);
  D.run();
  ConstraintGraph NewG2(nullptr, G->getName(), true);
  NFAInvDeterminizer D2(&NewG, &NewG2);
  D2.run();
  return NewG2;
}
} // namespace notdec::retypd
