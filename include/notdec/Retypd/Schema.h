#ifndef _NOTDEC_RETYPD_SCHEMA_H_
#define _NOTDEC_RETYPD_SCHEMA_H_

#include <deque>
#include <llvm/ADT/Optional.h>
#include <string>
#include <variant>

namespace notdec::retypd {

using Variance = bool;
const Variance Covariant = false;
const Variance Contravariant = true;

inline const char *toString(Variance v) { return v ? "⊖" : "⊕"; }
inline Variance invert(Variance v) { return !v; }
inline Variance combine(Variance a, Variance b) { return a != b; }

// Field labels

struct None {
  bool operator<(const None &rhs) const { return false; }
};
struct NullTerm {
  bool operator<(const NullTerm &rhs) const { return false; }
};
struct NoBound {
  bool operator<(const NoBound &rhs) const { return false; }
};
struct Fixed {
  uint32_t bound;
  bool operator<(const Fixed &rhs) const { return bound < rhs.bound; }
};
using Bound = std::variant<None, NullTerm, NoBound, Fixed>;

std::string toString(const Bound &b);

struct InLabel {
  std::string name;
  // prevent variant to be default constructed.
  InLabel() = delete;
  bool operator<(const InLabel &rhs) const { return name < rhs.name; }
  bool operator==(const InLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct OutLabel {
  std::string name;
  bool operator<(const OutLabel &rhs) const { return name < rhs.name; }
  bool operator==(const OutLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct DerefLabel {
  uint32_t size;
  int32_t offset;
  Bound bound;
  bool operator<(const DerefLabel &rhs) const {
    return std::tie(size, offset, bound) <
           std::tie(rhs.size, rhs.offset, rhs.bound);
  }
  bool operator==(const DerefLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct LoadLabel {
  bool operator<(const LoadLabel &rhs) const { return false; }
  bool operator==(const LoadLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct StoreLabel {
  bool operator<(const StoreLabel &rhs) const { return false; }
  bool operator==(const StoreLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

using FieldLabel =
    std::variant<InLabel, OutLabel, DerefLabel, LoadLabel, StoreLabel>;

std::string toString(const FieldLabel &f);

Variance getVariance(const FieldLabel &f);

struct DerivedTypeVariable {
  std::string name;
  std::deque<FieldLabel> Labels;
  unsigned instanceId = 0;

  DerivedTypeVariable getSubDtv(size_t index) const {
    DerivedTypeVariable sub_dtv;
    sub_dtv.name = this->name;
    sub_dtv.Labels.insert(sub_dtv.Labels.end(), this->Labels.begin(),
                          this->Labels.begin() + index);
    return sub_dtv;
  }

  Variance pathVariance(DerivedTypeVariable dtv) const {
    Variance v = Covariant;
    for (auto &label : dtv.Labels) {
      v = combine(v, getVariance(label));
    }
    return v;
  }

  bool isPrimitive() const { return name.at(0) == '#'; }

  // Comparator for stored in a std::map
  // https://stackoverflow.com/questions/26918912/efficient-operator-with-multiple-members
  bool operator<(const DerivedTypeVariable &rhs) const {
    return std::tie(name, Labels, instanceId) <
           std::tie(rhs.name, rhs.Labels, rhs.instanceId);
  }

  bool operator!=(const DerivedTypeVariable &rhs) const {
    return std::tie(name, Labels, instanceId) !=
           std::tie(rhs.name, rhs.Labels, rhs.instanceId);
  }
};

std::string toString(const DerivedTypeVariable &dtv);

struct SubTypeConstraint {
  DerivedTypeVariable sub;
  DerivedTypeVariable sup;
};

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

using Constraint =
    std::variant<SubTypeConstraint, AddConstraint, SubConstraint>;
std::string toString(const Constraint &c);
inline bool isSubtypeConstraint(Constraint c) {
  return std::holds_alternative<SubTypeConstraint>(c);
}

struct One {
  bool operator<(const One &rhs) const { return false; }
};
struct ForgetLabel {
  FieldLabel label;
  bool operator<(const ForgetLabel &rhs) const { return label < rhs.label; }
};
struct ForgetBase {
  std::string base;
  bool operator<(const ForgetBase &rhs) const { return base < rhs.base; }
};
struct RecallLabel {
  FieldLabel label;
  bool operator<(const RecallLabel &rhs) const { return label < rhs.label; }
};
struct RecallBase {
  std::string base;
  bool operator<(const RecallBase &rhs) const { return base < rhs.base; }
};
using EdgeLabel =
    std::variant<One, ForgetLabel, ForgetBase, RecallLabel, RecallBase>;

std::string toString(const EdgeLabel &label);
inline bool isBase(EdgeLabel label) {
  return std::holds_alternative<ForgetBase>(label) ||
         std::holds_alternative<RecallBase>(label);
}
inline bool isLabel(EdgeLabel label) {
  return std::holds_alternative<ForgetLabel>(label) ||
         std::holds_alternative<RecallLabel>(label);
}

} // namespace notdec::retypd

#endif
