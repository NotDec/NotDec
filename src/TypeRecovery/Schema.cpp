#include "TypeRecovery/Schema.h"

namespace notdec::retypd {

std::string toString(const Bound &b) {
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

std::string toString(const FieldLabel &f) {
  if (std::holds_alternative<InLabel>(f)) {
    return "in_" + std::get<InLabel>(f).name;
  } else if (std::holds_alternative<OutLabel>(f)) {
    return "out_" + std::get<OutLabel>(f).name;
  } else if (std::holds_alternative<OffsetLabel>(f)) {
    auto d = std::get<OffsetLabel>(f);
    return toString(d.range);
  } else if (std::holds_alternative<LoadLabel>(f)) {
    return "load" + std::to_string(std::get<LoadLabel>(f).Size);
  } else if (std::holds_alternative<StoreLabel>(f)) {
    return "store" + std::to_string(std::get<StoreLabel>(f).Size);
  } else {
    assert(false && "unknown FieldLabel");
  }
}

Variance getVariance(const FieldLabel &f) {
  if (std::holds_alternative<InLabel>(f)) {
    return Contravariant;
  } else if (std::holds_alternative<OutLabel>(f)) {
    return Covariant;
  } else if (std::holds_alternative<OffsetLabel>(f)) {
    return Covariant;
  } else if (std::holds_alternative<LoadLabel>(f)) {
    return Covariant;
  } else if (std::holds_alternative<StoreLabel>(f)) {
    return Contravariant;
  } else {
    assert(false && "unknown FieldLabel");
  }
}

std::string toString(const DerivedTypeVariable &dtv) {
  std::string s = dtv.Name;
  for (auto &label : dtv.Labels) {
    s += "." + toString(label);
  }
  if (dtv.instanceId != 0) {
    s += "#" + std::to_string(dtv.instanceId);
  }
  return s;
}

std::string toString(const IntConstantVar &V) {
  return "#Int#" + toString(V.Val) + "#" + toString(V.InstanceId);
}

std::string toString(const PrimitiveTypeVariable &dtv) {
  return "#" + dtv.name;
}

std::string toString(const TypeVariable &dtv) {
  if (std::holds_alternative<DerivedTypeVariable>(dtv.Inner)) {
    return toString(std::get<DerivedTypeVariable>(dtv.Inner));
  } else if (std::holds_alternative<IntConstantVar>(dtv.Inner)) {
    return toString(std::get<IntConstantVar>(dtv.Inner));
  } else if (std::holds_alternative<PrimitiveTypeVariable>(dtv.Inner)) {
    return toString(std::get<PrimitiveTypeVariable>(dtv.Inner));
  } else {
    assert(false && "unknown TypeVariable");
  }
}

std::string toStringImpl(const SubTypeConstraint &c) {
  return toString(c.sub) + " <= " + toString(c.sup);
}

std::string toStringImpl(const AddConstraint &c) {
  return "Add(" + toString(c.left) + " + " + toString(c.right) + "=" +
         toString(c.result) + ")";
}
std::string toStringImpl(const SubConstraint &c) {
  return "Sub(" + toString(c.left) + " - " + toString(c.right) + "=" +
         toString(c.result) + ")";
}

std::string toStringImpl(const CmpConstraint &c) {
  return "Cmp(" + toString(c.left) + " == " + toString(c.right) + ")";
}

std::string toString(const Constraint &c) {
  if (std::holds_alternative<SubTypeConstraint>(c)) {
    return toStringImpl(std::get<SubTypeConstraint>(c));
  } else if (std::holds_alternative<CmpConstraint>(c)) {
    return toStringImpl(std::get<CmpConstraint>(c));
  } else if (std::holds_alternative<AddConstraint>(c)) {
    return toStringImpl(std::get<AddConstraint>(c));
  } else if (std::holds_alternative<SubConstraint>(c)) {
    return toStringImpl(std::get<SubConstraint>(c));
  } else {
    assert(false && "unknown Constraint");
  }
}

} // namespace notdec::retypd
