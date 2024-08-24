
#ifndef _NOTDEC_RETYPD_DFA_MIN_H_
#define _NOTDEC_RETYPD_DFA_MIN_H_

#include <map>

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Schema.h"

namespace notdec::retypd {

/// Minimize a DFA
/// only single #Start and #End node because of the property of the
/// ConstraintGraph.
/// TODO allow minimize only recall half of the graph. view forget as epsilon to
/// #End.
struct DFAMinimizer {
  ConstraintGraph &OldG;
  ConstraintGraph &NewG;
  std::map<std::set<CGNode *>, CGNode *> DTrans;
  using EntryTy = std::map<std::set<CGNode *>, CGNode *>::iterator;
  DFAMinimizer(ConstraintGraph &Old, ConstraintGraph &New)
      : OldG(Old), NewG(New) {}
  void run();
  std::set<CGNode *> move(const std::set<CGNode *> &N, EdgeLabel L);
  std::set<CGNode *> countClosure(const std::set<CGNode *> &N);
  EntryTy getOrSetNewNode(const std::set<CGNode *> &N);
  std::set<EdgeLabel> allOutLabels(const std::set<CGNode *> &N);
};

} // namespace notdec::retypd

#endif
