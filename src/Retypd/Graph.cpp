#include "Retypd/Graph.h"

namespace notdec::retypd {

std::string toString(EdgeLabel label) {
  if (std::holds_alternative<One>(label)) {
    return "_1_";
  } else if (std::holds_alternative<ForgetLabel>(label)) {
    return "forget " + toString(std::get<ForgetLabel>(label).label);
  } else if (std::holds_alternative<ForgetBase>(label)) {
    return "forget " + std::get<ForgetBase>(label).base.str();
  } else if (std::holds_alternative<RecallLabel>(label)) {
    return "recall " + toString(std::get<RecallLabel>(label).label);
  } else if (std::holds_alternative<RecallBase>(label)) {
    return "recall " + std::get<RecallBase>(label).base.str();
  }
  return "unknown";
}
} // namespace notdec::retypd
