#ifndef _NOTDEC_RETYPD_SCHEMA_H_
#define _NOTDEC_RETYPD_SCHEMA_H_

#include "Utils/Range.h"
#include "utils.h"
#include <cassert>
#include <cstdint>
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

struct OffsetLabel {
  OffsetRange range;
  bool operator<(const OffsetLabel &rhs) const { return range < rhs.range; }
  bool operator==(const OffsetLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct LoadLabel {
  uint32_t Size;
  bool operator<(const LoadLabel &rhs) const { return Size < rhs.Size; }
  bool operator==(const LoadLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct StoreLabel {
  uint32_t Size;
  bool operator<(const StoreLabel &rhs) const { return Size < rhs.Size; }
  bool operator==(const StoreLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

using FieldLabel =
    std::variant<InLabel, OutLabel, OffsetLabel, LoadLabel, StoreLabel>;

std::string toString(const FieldLabel &f);

Variance getVariance(const FieldLabel &f);

struct BaseConstant {
  OffsetRange Val = {0};
  void *User = nullptr;
  bool operator<(const BaseConstant &rhs) const {
    return std::tie(Val, User) < std::tie(rhs.Val, rhs.User);
  }
  bool operator==(const BaseConstant &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  std::string str() const {
    return "#Int#" + toString(Val) + "_" + int_to_hex((unsigned long)User);
  }
};

struct DerivedTypeVariable {
  using BaseTy = std::variant<std::string, BaseConstant>;
  BaseTy Base;
  std::deque<FieldLabel> Labels;
  // differentiate Nodes from summary instantiation
  size_t instanceId = 0;

  size_t getInstanceId() { return instanceId; }
  void setInstanceId(size_t id) { instanceId = id; }

  bool hasBaseName() const { return std::holds_alternative<std::string>(Base); }

  std::string getBaseName() const {
    if (auto *s = std::get_if<std::string>(&Base)) {
      return *s;
    } else {
      assert(false && "DerivedTypeVariable::getBaseName: Base.Base is not a "
                      "string, but a IntConstant");
    }
  }

  bool isIntConstant() const {
    return std::holds_alternative<BaseConstant>(Base);
  }

  OffsetRange getIntConstant() const {
    if (auto *r = std::get_if<BaseConstant>(&Base)) {
      return r->Val;
    } else {
      assert(false && "DerivedTypeVariable::getIntConstant: Base.Base is not a "
                      "IntConstant");
    }
  }

  DerivedTypeVariable getSubDtv(size_t index) const {
    assert(index <= this->Labels.size());
    DerivedTypeVariable sub_dtv;
    sub_dtv.Base = this->Base;
    sub_dtv.Labels.insert(sub_dtv.Labels.end(), this->Labels.begin(),
                          this->Labels.begin() + index);
    return sub_dtv;
  }

  Variance pathVariance() const {
    Variance v = Covariant;
    for (auto &label : Labels) {
      v = combine(v, getVariance(label));
    }
    return v;
  }

  // Comparator for stored in a std::map
  // https://stackoverflow.com/questions/26918912/efficient-operator-with-multiple-members
  bool operator<(const DerivedTypeVariable &rhs) const {
    return std::tie(Base, Labels) < std::tie(rhs.Base, rhs.Labels);
  }
  bool operator==(const DerivedTypeVariable &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  std::string str() const {
    std::string Ret;
    if (auto *s = std::get_if<std::string>(&Base)) {
      Ret = *s;
    } else {
      Ret = std::get<BaseConstant>(Base).str();
    }
    if (instanceId > 0) {
      Ret += "#" + std::to_string(instanceId);
    }
    for (auto &label : Labels) {
      Ret += "." + toString(label);
    }
    return Ret;
  }
};

struct PrimitiveTypeVariable {
  std::string name;
  bool operator<(const PrimitiveTypeVariable &rhs) const {
    return std::tie(name) < std::tie(rhs.name);
  }
  bool operator==(const PrimitiveTypeVariable &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct TypeVariable {
  using InnerTy = std::variant<DerivedTypeVariable, PrimitiveTypeVariable>;
  InnerTy Inner;

  static TypeVariable CreatePrimitive(std::string name) {
    return TypeVariable{PrimitiveTypeVariable{name}};
  }

  static TypeVariable CreateDtv(std::string name) {
    return TypeVariable{DerivedTypeVariable{.Base = name}};
  }

  static TypeVariable CreateIntConstant(OffsetRange val, void *User) {
    return TypeVariable{
        DerivedTypeVariable{.Base = BaseConstant{.Val = val, .User = User}}};
  }

  Variance pathVariance() const {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      return dtv->pathVariance();
    } else {
      return Covariant;
    }
  }

  TypeVariable toBase() const {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      return {DerivedTypeVariable{.Base = dtv->Base,
                                  .instanceId = dtv->instanceId}};
    } else {
      return *this;
    }
  }

  bool hasLabel() const {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      return dtv->Labels.size() > 0;
    } else {
      return false;
    }
  }

  std::deque<FieldLabel> &getLabels() {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      return dtv->Labels;
    } else {
      assert(false && "getLabels: Not a DerivedTypeVariable");
    }
  }

  size_t getInstanceId() {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      return dtv->getInstanceId();
    } else {
      assert(false && "getInstanceId: No instanceId.");
    }
  }

  void setInstanceId(size_t id) {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      dtv->setInstanceId(id);
    } else {
      assert(false && "setInstanceId: Not a DerivedTypeVariable");
    }
  }

  std::string getPrimitiveName() const {
    if (auto *ptv = std::get_if<PrimitiveTypeVariable>(&Inner)) {
      return ptv->name;
    } else {
      assert(false && "getPrimitiveName: Not a PrimitiveTypeVariable");
    }
  }

  bool hasBaseName() const {
    return std::holds_alternative<PrimitiveTypeVariable>(Inner) ||
           (std::holds_alternative<DerivedTypeVariable>(Inner) &&
            std::get<DerivedTypeVariable>(Inner).hasBaseName());
  }

  std::string getBaseName() const {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      return dtv->getBaseName();
    } else if (auto *ptv = std::get_if<PrimitiveTypeVariable>(&Inner)) {
      return ptv->name;
    } else {
      assert(false && "getBaseName: No name.");
    }
  }

  DerivedTypeVariable::BaseTy getBase() const {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      return dtv->Base;
    } else {
      assert(false && "getBase: Not a DerivedTypeVariable");
    }
  }

  void setBase(DerivedTypeVariable::BaseTy Base) {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      dtv->Base = Base;
    } else {
      assert(false && "TypeVariable::setName: Not a DerivedTypeVariable!");
    }
  }

  bool hasInstanceId() const {
    return std::holds_alternative<DerivedTypeVariable>(Inner);
  }

  bool isPrimitive() const {
    return std::holds_alternative<PrimitiveTypeVariable>(Inner);
  }

  bool isIntConstant() const {
    return std::holds_alternative<DerivedTypeVariable>(Inner) &&
           std::get<DerivedTypeVariable>(Inner).isIntConstant();
  }
  OffsetRange getIntConstant() const {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      return dtv->getIntConstant();
    } else {
      assert(false && "getIntConstant: Not a DerivedTypeVariable");
    }
  }

  // Comparator for stored in a std::map
  // https://stackoverflow.com/questions/26918912/efficient-operator-with-multiple-members
  bool operator<(const TypeVariable &rhs) const {
    return std::tie(Inner) < std::tie(rhs.Inner);
  }

  bool operator==(const TypeVariable &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  bool operator!=(const TypeVariable &rhs) const {
    return (*this < rhs) || (rhs < *this);
  }
};

std::string toString(const TypeVariable &dtv);

struct SubTypeConstraint {
  TypeVariable sub;
  TypeVariable sup;
};

struct AddConstraint {
  TypeVariable left;
  TypeVariable right;
  TypeVariable result;
};
struct SubConstraint {
  TypeVariable left;
  TypeVariable right;
  TypeVariable result;
};
struct CmpConstraint {
  TypeVariable left;
  TypeVariable right;
};

using Constraint = std::variant<SubTypeConstraint, AddConstraint, SubConstraint,
                                CmpConstraint>;
std::string toString(const Constraint &c);

struct One {
  bool operator<(const One &rhs) const { return false; }
  bool operator==(const One &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};
struct ForgetLabel {
  FieldLabel label;
  bool operator<(const ForgetLabel &rhs) const { return label < rhs.label; }
  bool operator==(const ForgetLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};
struct ForgetBase {
  TypeVariable base;
  Variance V;
  bool operator<(const ForgetBase &rhs) const {
    return std::tie(base, V) < std::tie(rhs.base, rhs.V);
  }
  bool operator==(const ForgetBase &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};
struct RecallLabel {
  FieldLabel label;
  bool operator<(const RecallLabel &rhs) const { return label < rhs.label; }
  bool operator==(const RecallLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};
struct RecallBase {
  TypeVariable base;
  Variance V;
  bool operator<(const RecallBase &rhs) const {
    return std::tie(base, V) < std::tie(rhs.base, rhs.V);
  }
  bool operator==(const RecallBase &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};
using EdgeLabel =
    std::variant<One, ForgetLabel, ForgetBase, RecallLabel, RecallBase>;

std::string toString(const EdgeLabel &label);
inline bool isBase(EdgeLabel label) {
  return std::holds_alternative<ForgetBase>(label) ||
         std::holds_alternative<RecallBase>(label);
}
inline TypeVariable getBase(EdgeLabel label) {
  if (auto *fb = std::get_if<ForgetBase>(&label)) {
    return fb->base;
  } else if (auto *rb = std::get_if<RecallBase>(&label)) {
    return rb->base;
  } else {
    assert(false && "getBase: Not a ForgetBase or RecallBase");
  }
}

inline FieldLabel getLabel(EdgeLabel label) {
  if (auto *fl = std::get_if<ForgetLabel>(&label)) {
    return fl->label;
  } else if (auto *rl = std::get_if<RecallLabel>(&label)) {
    return rl->label;
  } else {
    assert(false && "getLabel: Not a ForgetLabel or RecallLabel");
  }
}

inline bool isLabel(EdgeLabel label) {
  return std::holds_alternative<ForgetLabel>(label) ||
         std::holds_alternative<RecallLabel>(label);
}

inline bool isForget(EdgeLabel label) {
  return std::holds_alternative<ForgetLabel>(label) ||
         std::holds_alternative<ForgetBase>(label);
}

inline bool isRecall(EdgeLabel label) {
  return std::holds_alternative<RecallLabel>(label) ||
         std::holds_alternative<RecallBase>(label);
}

inline bool hasSameBaseOrLabel(EdgeLabel a, EdgeLabel b) {
  if (isBase(a) && isBase(b)) {
    return getBase(a) == getBase(b);
  } else if (isLabel(a) && isLabel(b)) {
    return getLabel(a) == getLabel(b);
  } else {
    return false;
  }
}

inline bool operator!=(const EdgeLabel &N1, const EdgeLabel &N2) {
  return !(N1 == N2);
}

} // namespace notdec::retypd

#endif
