#ifndef _NOTDEC_RETYPD_SCHEMA_H_
#define _NOTDEC_RETYPD_SCHEMA_H_

#include <deque>
#include <llvm/ADT/Optional.h>
#include <string>
#include <variant>

namespace notdec::retypd {

using Variance = bool;
const Variance Covariant = true;
const Variance Contravariant = false;

inline const char *toString(Variance v) { return v ? "⊕" : "⊖"; }
inline Variance invert(Variance v) { return !v; }
inline Variance combine(Variance a, Variance b) { return a == b; }

// Field labels

struct None {};
struct NullTerm {};
struct NoBound {};
struct Fixed {
  uint32_t bound;
};
using Bound = std::variant<None, NullTerm, NoBound, Fixed>;

std::string toString(Bound b);

struct InLabel {
  std::string name;
};
struct OutLabel {
  std::string name;
};
struct DerefLabel {
  uint32_t size;
  int32_t offset;
  Bound bound;
};
struct LoadLabel {};
struct StoreLabel {};
using FieldLabel =
    std::variant<InLabel, OutLabel, DerefLabel, LoadLabel, StoreLabel>;

std::string toString(FieldLabel f);

Variance getVariance(FieldLabel &f);

struct DerivedTypeVariable {
  std::string name;
  std::deque<FieldLabel> labels;
  unsigned instanceId = 0;

  DerivedTypeVariable getSubDtv(size_t index) const {
    DerivedTypeVariable sub_dtv;
    sub_dtv.name = this->name;
    sub_dtv.labels.insert(sub_dtv.labels.end(), this->labels.begin(),
                          this->labels.begin() + index);
    return sub_dtv;
  }

  Variance pathVariance(DerivedTypeVariable dtv) const {
    Variance v = Covariant;
    for (auto &label : dtv.labels) {
      v = combine(v, getVariance(label));
    }
    return v;
  }

  bool isPrimitive() const { return name.at(0) == '#'; }
};

std::string toString(DerivedTypeVariable dtv);

struct SubTypeConstraint {
  DerivedTypeVariable sub;
  DerivedTypeVariable sup;
};

std::string toString(SubTypeConstraint c);

struct AddConstraint {
  DerivedTypeVariable left;
  DerivedTypeVariable right;
  DerivedTypeVariable result;
};
struct SubConstraint {
  DerivedTypeVariable left;
  DerivedTypeVariable right;
  DerivedTypeVariable result;
};
std::string toString(AddConstraint c);
std::string toString(SubConstraint c);

using Constraint =
    std::variant<SubTypeConstraint, AddConstraint, SubConstraint>;
std::string toString(Constraint c);
inline bool isSubtypeConstraint(Constraint c) {
  return std::holds_alternative<SubTypeConstraint>(c);
}

} // namespace notdec::retypd

#endif
