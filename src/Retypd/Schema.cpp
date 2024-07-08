#include "Retypd/Schema.h"

namespace notdec::retypd {

std::string toString(Bound b) {
  if (std::holds_alternative<None>(b)) {
    return "";
  } else if (std::holds_alternative<NoBound>(b)) {
    return "nobound";
  } else if (std::holds_alternative<NullTerm>(b)) {
    return "nullterm";
  } else if (std::holds_alternative<Fixed>(b)) {
    return "*[" + std::to_string(std::get<Fixed>(b).bound) + "]";
  } else {
    assert(false && "unknown Bound");
  }
}

std::string toString(FieldLabel f) {
  if (std::holds_alternative<InLabel>(f)) {
    return "in_" + std::get<InLabel>(f).name.str();
  } else if (std::holds_alternative<OutLabel>(f)) {
    return "out_" + std::get<OutLabel>(f).name.str();
  } else if (std::holds_alternative<DerefLabel>(f)) {
    auto d = std::get<DerefLabel>(f);
    return "σ" + std::to_string(d.base) + "@" + std::to_string(d.offset) +
           toString(d.bound);
  } else if (std::holds_alternative<LoadLabel>(f)) {
    return "load";
  } else if (std::holds_alternative<StoreLabel>(f)) {
    return "store";
  } else {
    assert(false && "unknown FieldLabel");
  }
}

Variance getVariance(FieldLabel &f) {
  if (std::holds_alternative<InLabel>(f)) {
    return Contravariant;
  } else if (std::holds_alternative<OutLabel>(f)) {
    return Covariant;
  } else if (std::holds_alternative<DerefLabel>(f)) {
    return Covariant;
  } else if (std::holds_alternative<LoadLabel>(f)) {
    return Covariant;
  } else if (std::holds_alternative<StoreLabel>(f)) {
    return Contravariant;
  } else {
    assert(false && "unknown FieldLabel");
  }
}

std::string toString(DerivedTypeVariable dtv) {
  std::string s = dtv.name.str();
  for (auto &label : dtv.labels) {
    s += "." + toString(label);
  }
  return s;
}

std::string toString(Constraint c) {
  return toString(c.left) + " <= " + toString(c.right);
}

} // namespace notdec::retypd
