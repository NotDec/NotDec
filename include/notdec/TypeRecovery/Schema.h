#ifndef _NOTDEC_RETYPD_SCHEMA_H_
#define _NOTDEC_RETYPD_SCHEMA_H_

#include "Utils/Utils.h"
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/Utils.h"
#include <cassert>
#include <cstdint>
#include <deque>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SmallVector.h>
#include <string>
#include <variant>

namespace notdec::retypd {

struct TRContext;

using Variance = bool;
const Variance Covariant = false;
const Variance Contravariant = true;

inline const char *toString(Variance v) { return v ? "⊖" : "⊕"; }
inline Variance invert(Variance v) { return !v; }
inline Variance combine(Variance a, Variance b) { return a != b; }

bool getPreferredVariance(ExtValuePtr &Val);

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

// using FieldLabel =
//     std::variant<InLabel, OutLabel, OffsetLabel, LoadLabel, StoreLabel>;
struct FieldLabel {
  std::variant<InLabel, OutLabel, OffsetLabel, LoadLabel, StoreLabel> L;
  bool operator<(const FieldLabel &rhs) const {
    return std::tie(L) < std::tie(rhs.L);
  }
  bool operator==(const FieldLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }

  bool isIn() const { return std::holds_alternative<InLabel>(L); }
  bool isOut() const { return std::holds_alternative<OutLabel>(L); }
  bool isOffset() const { return std::holds_alternative<OffsetLabel>(L); }
  bool isLoad() const { return std::holds_alternative<LoadLabel>(L); }
  bool isStore() const { return std::holds_alternative<StoreLabel>(L); }

  template <typename T> T *getAs() { return std::get_if<T>(&L); }
  template <typename T> const T *getAs() const { return std::get_if<T>(&L); }
};

std::string toString(const FieldLabel &f);

inline StoreLabel toStore(LoadLabel L) { return StoreLabel{.Size = L.Size}; }

Variance getVariance(const FieldLabel &f);

struct BaseConstant {
  OffsetRange Val = {0};
  void *User = nullptr;
  long OpInd = -1;
  bool operator<(const BaseConstant &rhs) const {
    return std::tie(Val, User, OpInd) < std::tie(rhs.Val, rhs.User, rhs.OpInd);
  }
  bool operator==(const BaseConstant &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  std::string str() const {
    return "#Int#" + toString(Val) + "_" + int_to_hex((unsigned long)User) +
           "_" + std::to_string(OpInd);
  }
};

struct DerivedTypeVariable {
  using BaseTy = std::variant<std::string, BaseConstant>;
  BaseTy Base;
  std::deque<FieldLabel> Labels;

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
    for (auto &label : Labels) {
      Ret += "." + toString(label);
    }
    return Ret;
  }
};

std::string toString(const DerivedTypeVariable &dtv);

struct PrimitiveTypeVariable {
  std::string name;
  bool operator<(const PrimitiveTypeVariable &rhs) const {
    return std::tie(name) < std::tie(rhs.name);
  }
  bool operator==(const PrimitiveTypeVariable &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

std::string toString(const PrimitiveTypeVariable &dtv);

struct PooledTypeVariable {
  TRContext *Ctx;
  using InnerTy = std::variant<DerivedTypeVariable, PrimitiveTypeVariable>;
  InnerTy Inner;

  static const PooledTypeVariable *intern(TRContext &Ctx,
                                          const PooledTypeVariable &TV);
  static const PooledTypeVariable *CreatePrimitive(TRContext &Ctx,
                                                   std::string name);

  static const PooledTypeVariable *CreateDtv(TRContext &Ctx, std::string name);

  static const PooledTypeVariable *CreateDtv(TRContext &Ctx,
                                             DerivedTypeVariable dtv);

  static const PooledTypeVariable *
  CreateIntConstant(TRContext &Ctx, OffsetRange val, void *User, long OpInd);

  const PooledTypeVariable *pushLabel(FieldLabel label) const;
  const PooledTypeVariable *popLabel() const;

  const PooledTypeVariable *toBase() const;

  Variance pathVariance() const {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      return dtv->pathVariance();
    } else {
      return Covariant;
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

  const std::deque<FieldLabel> &getLabels() const {
    if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
      return dtv->Labels;
    } else {
      assert(false && "getLabels: Not a DerivedTypeVariable");
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

  bool isPrimitive() const {
    return std::holds_alternative<PrimitiveTypeVariable>(Inner);
  }

  bool isDtv() const {
    return std::holds_alternative<DerivedTypeVariable>(Inner);
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
  bool operator<(const PooledTypeVariable &rhs) const {
    assert(Ctx != nullptr && rhs.Ctx != nullptr);
    return std::tie(Ctx, Inner) < std::tie(rhs.Ctx, rhs.Inner);
  }

  bool operator==(const PooledTypeVariable &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  bool operator!=(const PooledTypeVariable &rhs) const {
    return (*this < rhs) || (rhs < *this);
  }

  std::string str() const {
    if (std::holds_alternative<DerivedTypeVariable>(Inner)) {
      std::string Ret = toString(std::get<DerivedTypeVariable>(Inner));
      return Ret;
    } else if (std::holds_alternative<PrimitiveTypeVariable>(Inner)) {
      return toString(std::get<PrimitiveTypeVariable>(Inner));
    } else {
      assert(false && "unknown TypeVariable");
    }
  }

  // TODO how to make this private while allow emplace_back.
  // protected:
  friend struct ExprToConstraintsContext;
  PooledTypeVariable &operator=(const PooledTypeVariable &) = default;
  PooledTypeVariable(const PooledTypeVariable &) = default;
  PooledTypeVariable() = default;
};

std::string toString(const PooledTypeVariable *dtv);

struct TypeVariable {
  using ContextIdTy = uintptr_t;
  using ContextIdListTy = llvm::SmallVector<ContextIdTy, 1>;
  TRContext *Ctx;
  const PooledTypeVariable *Var;
  // differentiate Nodes from summary instantiation
  ContextIdListTy ContextId = {};
  // differentiate formal and actual parameter/return values
  // Used in summary instantiation
  bool IsActual = false;

  bool hasContextId() const {
    return std::holds_alternative<DerivedTypeVariable>(Var->Inner);
  }

  const ContextIdListTy &getContextId() const {
    if (hasContextId()) {
      return ContextId;
    } else {
      assert(ContextId.size() == 0);
      assert(false && "getContextId: No ContextId.");
    }
  }

  void pushContextId(ContextIdTy Id) { ContextId.push_back(Id); }

  // void setContextId(size_t id) {
  //   if (hasContextId()) {
  //     ContextId = id;
  //   } else {
  //     assert(ContextId == 0);
  //     assert(false && "setContextId: Not a DerivedTypeVariable");
  //   }
  // }

  static TypeVariable CreatePrimitive(TRContext &Ctx, std::string name) {
    return {&Ctx, PooledTypeVariable::CreatePrimitive(Ctx, name)};
  }

  static TypeVariable CreateDtv(TRContext &Ctx, std::string name,
                                bool IsActual = false) {
    return {&Ctx, PooledTypeVariable::CreateDtv(Ctx, name), {}, IsActual};
  }

  static TypeVariable CreateDtv(TRContext &Ctx, std::string name,
                                size_t ContextId, bool IsActual = false) {
    return {
        &Ctx, PooledTypeVariable::CreateDtv(Ctx, name), {ContextId}, IsActual};
  }

  static TypeVariable CreateDtv(TRContext &Ctx, DerivedTypeVariable dtv) {
    return {&Ctx, PooledTypeVariable::CreateDtv(Ctx, dtv)};
  }

  static TypeVariable CreateIntConstant(TRContext &Ctx, OffsetRange val,
                                        void *User, long OpInd) {
    return {&Ctx, PooledTypeVariable::CreateIntConstant(Ctx, val, User, OpInd)};
  }

  TypeVariable pushLabel(FieldLabel label) const {
    TypeVariable Ret = *this;
    Ret.Var = Var->pushLabel(label);
    return Ret;
  }
  TypeVariable popLabel() const {
    TypeVariable Ret = *this;
    Ret.Var = Var->popLabel();
    return Ret;
  }
  TypeVariable markActual() const {
    // assert(!IsActual);
    TypeVariable Ret = *this;
    Ret.IsActual = true;
    return Ret;
  }

  Variance pathVariance() const { return Var->pathVariance(); }

  TypeVariable toBase() const { return {Ctx, Var->toBase(), ContextId}; }

  bool hasLabel() const { return Var->hasLabel(); }

  const std::deque<FieldLabel> &getLabels() const { return Var->getLabels(); }

  std::string getPrimitiveName() const { return Var->getPrimitiveName(); }

  bool hasBaseName() const { return Var->hasBaseName(); }

  std::string getBaseName() const { return Var->getBaseName(); }

  bool isTop() const {
    return Var->isPrimitive() && Var->getPrimitiveName() == "top";
  }

  // TODO prevent name collision
  static const char *Memory;
  bool isMemory() const {
    return Var->hasBaseName() && Var->getBaseName() == Memory;
  }

  DerivedTypeVariable::BaseTy getBase() const { return Var->getBase(); }

  bool isPrimitive() const { return Var->isPrimitive(); }

  bool isIntConstant() const { return Var->isIntConstant(); }
  OffsetRange getIntConstant() const { return Var->getIntConstant(); }

  bool isEnd() const {
    return Var->isPrimitive() && Var->getPrimitiveName() == "#End";
  }

  bool isActual() const { return IsActual; }

  std::string str() const {
    std::string Ret = Var->str();
    if (std::holds_alternative<DerivedTypeVariable>(Var->Inner)) {
      for (auto CId : getContextId()) {
        Ret += "#" + int_to_hex<decltype(CId)>(CId);
      }
    }
    if (IsActual) {
      Ret += "#Actual";
    }
    return Ret;
  }

  // Comparator for stored in a std::map
  // https://stackoverflow.com/questions/26918912/efficient-operator-with-multiple-members
  bool operator<(const TypeVariable &rhs) const {
    if (!std::holds_alternative<DerivedTypeVariable>(Var->Inner)) {
      assert(ContextId.size() == 0);
    }
    return std::tie(Var, ContextId, IsActual) <
           std::tie(rhs.Var, rhs.ContextId, rhs.IsActual);
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
  bool operator<(const SubTypeConstraint &rhs) const {
    return std::tie(sub, sup) < std::tie(rhs.sub, rhs.sup);
  }
  bool operator==(const SubTypeConstraint &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
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
  TypeVariable Base;
  Variance V = Covariant;
  bool operator<(const ForgetBase &rhs) const {
    return std::tie(Base, V) < std::tie(rhs.Base, rhs.V);
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
  TypeVariable Base;
  Variance V = Covariant;
  bool operator<(const RecallBase &rhs) const {
    return std::tie(Base, V) < std::tie(rhs.Base, rhs.V);
  }
  bool operator==(const RecallBase &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  bool operator!=(const RecallBase &rhs) const { return !(*this == rhs); }
};

struct RecallNode {
  CGNode *Base;
  bool operator<(const RecallNode &rhs) const {
    return std::tie(Base) < std::tie(rhs.Base);
  }
  bool operator==(const RecallNode &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  bool operator!=(const RecallNode &rhs) const { return !(*this == rhs); }
};

struct ForgetNode {
  CGNode *Base;
  bool operator<(const ForgetNode &rhs) const {
    return std::tie(Base) < std::tie(rhs.Base);
  }
  bool operator==(const ForgetNode &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  bool operator!=(const ForgetNode &rhs) const { return !(*this == rhs); }
};

struct RecallString {
  std::string Base;
  bool operator<(const RecallString &rhs) const {
    return std::tie(Base) < std::tie(rhs.Base);
  }
  bool operator==(const RecallString &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  bool operator!=(const RecallString &rhs) const { return !(*this == rhs); }
};

struct ForgetString {
  std::string Base;
  bool operator<(const ForgetString &rhs) const {
    return std::tie(Base) < std::tie(rhs.Base);
  }
  bool operator==(const ForgetString &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  bool operator!=(const ForgetString &rhs) const { return !(*this == rhs); }
};

struct ForgetSize {
  int64_t Base;
  bool operator<(const ForgetSize &rhs) const {
    return std::tie(Base) < std::tie(rhs.Base);
  }
  bool operator==(const ForgetSize &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  bool operator!=(const ForgetSize &rhs) const { return !(*this == rhs); }
};

// using EdgeLabel = std::variant<One, ForgetLabel, ForgetBase, RecallLabel,
// RecallBase>;
struct EdgeLabel {
  std::variant<One, ForgetLabel, ForgetBase, RecallLabel, RecallBase,
               RecallNode, ForgetNode, RecallString, ForgetString, ForgetSize>
      L;

  bool operator<(const EdgeLabel &rhs) const {
    return std::tie(L) < std::tie(rhs.L);
  }
  bool operator==(const EdgeLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }

  bool isOne() const { return std::holds_alternative<One>(L); }
  bool isForgetBase() const { return std::holds_alternative<ForgetBase>(L); }
  bool isRecallBase() const { return std::holds_alternative<RecallBase>(L); }
  bool isForgetLabel() const { return std::holds_alternative<ForgetLabel>(L); }
  bool isRecallLabel() const { return std::holds_alternative<RecallLabel>(L); }
  bool isForgetNode() const { return std::holds_alternative<ForgetNode>(L); }
  bool isRecallNode() const { return std::holds_alternative<RecallNode>(L); }
  bool isForgetString() const { return std::holds_alternative<ForgetString>(L); }
  bool isRecallString() const { return std::holds_alternative<RecallString>(L); }
  bool isForgetSize() const { return std::holds_alternative<ForgetSize>(L); }

  template <typename T> T *getAs() { return std::get_if<T>(&L); }
  template <typename T> const T *getAs() const { return std::get_if<T>(&L); }
};

[[nodiscard]] std::string toString(const EdgeLabel &label);
inline bool isBase(EdgeLabel label) {
  return label.isForgetBase() || label.isRecallBase();
}

inline bool isNodeLabel(EdgeLabel label) {
  return label.isForgetNode() || label.isRecallNode();
}

inline TypeVariable getBase(EdgeLabel label) {
  if (auto *fb = label.getAs<ForgetBase>()) {
    return fb->Base;
  } else if (auto *rb = label.getAs<RecallBase>()) {
    return rb->Base;
  } else {
    assert(false && "getBase: Not a ForgetBase or RecallBase");
  }
}

inline CGNode *getNode(EdgeLabel L) {
  if (auto *fb = L.getAs<ForgetNode>()) {
    return fb->Base;
  } else if (auto *rb = L.getAs<RecallNode>()) {
    return rb->Base;
  } else {
    assert(false && "getNode: Not a isForgetNode or isRecallNode");
  }
}

inline std::string getString(EdgeLabel L) {
  if (auto *fb = L.getAs<ForgetString>()) {
    return fb->Base;
  } else if (auto *rb = L.getAs<RecallString>()) {
    return rb->Base;
  } else {
    assert(false && "getString: Not a ForgetString or RecallString");
  }
}

inline bool isStore(const EdgeLabel &label) {
  if (auto *RL = label.getAs<RecallLabel>()) {
    return RL->label.isStore();
  } else if (auto *FL = label.getAs<ForgetLabel>()) {
    return FL->label.isStore();
  } else {
    return false;
  }
}

inline bool isLoadOrStore(const EdgeLabel &label) {
  if (auto *RL = label.getAs<RecallLabel>()) {
    return RL->label.isLoad() || RL->label.isStore();
  } else if (auto *FL = label.getAs<ForgetLabel>()) {
    return FL->label.isLoad() || FL->label.isStore();
  } else {
    return false;
  }
}

inline uint32_t getLoadOrStoreSize(const EdgeLabel &label) {
  if (auto *RL = label.getAs<RecallLabel>()) {
    if (auto *LL = RL->label.getAs<LoadLabel>()) {
      return LL->Size;
    } else if (auto *SL = RL->label.getAs<StoreLabel>()) {
      return SL->Size;
    } else {
      assert(false && "getLoadOrStoreSize: Not a LoadLabel or StoreLabel");
    }
  } else if (auto *FL = label.getAs<ForgetLabel>()) {
    if (auto *LL = FL->label.getAs<LoadLabel>()) {
      return LL->Size;
    } else if (auto *SL = FL->label.getAs<StoreLabel>()) {
      return SL->Size;
    } else {
      assert(false && "getLoadOrStoreSize: Not a LoadLabel or StoreLabel");
    }
  } else {
    assert(false && "getLoadOrStoreSize: Not a ForgetLabel or RecallLabel");
  }
}

inline FieldLabel getLabel(EdgeLabel label) {
  if (auto *fl = label.getAs<ForgetLabel>()) {
    return fl->label;
  } else if (auto *rl = label.getAs<RecallLabel>()) {
    return rl->label;
  } else {
    assert(false && "getLabel: Not a ForgetLabel or RecallLabel");
  }
}

inline bool isLabel(const EdgeLabel &label) {
  return label.isForgetLabel() || label.isRecallLabel();
}

inline bool isForget(const EdgeLabel &label) {
  return label.isForgetLabel() || label.isForgetBase() || label.isForgetNode() || label.isForgetString();
}

inline bool isRecall(const EdgeLabel &label) {
  return label.isRecallLabel() || label.isRecallBase() || label.isRecallNode() || label.isRecallString();
}

inline bool isRecallOffset(const EdgeLabel &label) {
  if (auto *RL = label.getAs<RecallLabel>()) {
    return RL->label.isOffset();
  } else {
    return false;
  }
}

inline ForgetLabel toForget(RecallLabel L) {
  return ForgetLabel{.label = L.label};
}

inline RecallLabel toRecall(ForgetLabel L) {
  return RecallLabel{.label = L.label};
}

inline ForgetBase toForget(RecallBase L) {
  return ForgetBase{.Base = L.Base, .V = L.V};
}

inline RecallBase toRecall(ForgetBase L) {
  return RecallBase{.Base = L.Base, .V = L.V};
}

inline const notdec::retypd::OffsetLabel *
getOffsetLabel(const EdgeLabel &label) {
  if (auto *RL = label.getAs<RecallLabel>()) {
    return RL->label.getAs<OffsetLabel>();
  } else if (auto *FL = label.getAs<ForgetLabel>()) {
    return FL->label.getAs<OffsetLabel>();
  } else {
    return nullptr;
  }
}

inline OffsetRange&
getLabelOffsetRef(EdgeLabel &label) {
  if (auto *RL = label.getAs<RecallLabel>()) {
    return RL->label.getAs<OffsetLabel>()->range;
  } else if (auto *FL = label.getAs<ForgetLabel>()) {
    return FL->label.getAs<OffsetLabel>()->range;
  }
  assert(false);
}

inline bool hasSameBaseOrLabel(EdgeLabel a, EdgeLabel b) {
  if (isBase(a) && isBase(b)) {
    return getBase(a) == getBase(b);
  } else if (isLabel(a) && isLabel(b)) {
    return getLabel(a) == getLabel(b);
  } else if (isNodeLabel(a) && isNodeLabel(b)) {
    return getNode(a) == getNode(b);
  } else {
    return false;
  }
}

inline bool operator!=(const EdgeLabel &N1, const EdgeLabel &N2) {
  return !(N1 == N2);
}

} // namespace notdec::retypd

#endif
