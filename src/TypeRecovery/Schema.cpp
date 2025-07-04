#include "notdec-llvm2c/Interface/Range.h"
#include <string>
#include <variant>

#include "TypeRecovery/TRContext.h"

#include "TypeRecovery/Schema.h"

namespace notdec::retypd {

const char *TypeVariable::Memory = "MEMORY";

// TODO: two insertion and one allocation.
// Optimize to one insertion and no allocation using several separate map.
// Map String base -> TV
// Map TV,label -> TV

const PooledTypeVariable *
PooledTypeVariable::intern(TRContext &Ctx, const PooledTypeVariable &TV) {
  auto IT = Ctx.TypeVars.find(const_cast<PooledTypeVariable *>(&TV));
  if (IT != Ctx.TypeVars.end()) {
    return *IT;
  } else {
    auto *TVPtr = new PooledTypeVariable(TV);
    Ctx.TypeVars.insert(TVPtr);
    return TVPtr;
  }
}

const PooledTypeVariable *PooledTypeVariable::popLabel() const {
  PooledTypeVariable TV = *this;
  TV.getLabels().pop_back();
  return intern(*Ctx, TV);
}

const PooledTypeVariable *
PooledTypeVariable::pushLabel(FieldLabel label) const {
  PooledTypeVariable TV = *this;
  TV.getLabels().push_back(label);
  return intern(*Ctx, TV);
}

const PooledTypeVariable *PooledTypeVariable::toBase() const {
  if (auto *dtv = std::get_if<DerivedTypeVariable>(&Inner)) {
    PooledTypeVariable TV = PooledTypeVariable{.Ctx = Ctx,
                                               .Inner = DerivedTypeVariable{
                                                   .Base = dtv->Base,
                                               }};
    return intern(*Ctx, TV);
  } else {
    return this;
  }
}

const PooledTypeVariable *
PooledTypeVariable::CreatePrimitive(TRContext &Ctx, std::string name) {
  PooledTypeVariable TV = {&Ctx, PrimitiveTypeVariable{name}};
  return intern(Ctx, TV);
}

const PooledTypeVariable *
PooledTypeVariable::CreateDtv(TRContext &Ctx, DerivedTypeVariable dtv) {
  PooledTypeVariable TV = {&Ctx, dtv};
  return intern(Ctx, TV);
}

const PooledTypeVariable *PooledTypeVariable::CreateDtv(TRContext &Ctx,
                                                        std::string name) {
  PooledTypeVariable TV = {&Ctx, DerivedTypeVariable{.Base = name}};
  return intern(Ctx, TV);
}

const PooledTypeVariable *PooledTypeVariable::CreateIntConstant(TRContext &Ctx,
                                                                OffsetRange val,
                                                                void *User,
                                                                long OpInd) {
  PooledTypeVariable TV = PooledTypeVariable{
      &Ctx, DerivedTypeVariable{.Base = BaseConstant{
                                    .Val = val, .User = User, .OpInd = OpInd}}};
  return intern(Ctx, TV);
}

std::string toString(const EdgeLabel &label) {
  if (label.isOne()) {
    return "_1_";
  } else if (auto FL = label.getAs<ForgetLabel>()) {
    return "forget " + toString(FL->label);
  } else if (auto FB = label.getAs<ForgetBase>()) {
    return "forget " + toString(FB->Base) +
           toString(FB->V);
  } else if (auto RL = label.getAs<RecallLabel>()) {
    return "recall " + toString(RL->label);
  } else if (auto RB = label.getAs<RecallBase>()) {
    return "recall " + toString(RB->Base) +
           toString(RB->V);
  }
  assert(false && "Unknown FieldLabel!");
}

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
  if (auto *In = f.getAs<InLabel>()) {
    return "in_" + In->name;
  } else if (auto *Out = f.getAs<OutLabel>()) {
    return Out->name.empty()
               ? "out"
               : "out_" + Out->name;
  } else if (auto *O = f.getAs<OffsetLabel>()) {
    return toString(O->range);
  } else if (auto *L = f.getAs<LoadLabel>()) {
    return "load" + std::to_string(L->Size);
  } else if (auto *S = f.getAs<StoreLabel>()) {
    return "store" + std::to_string(S->Size);
  } else {
    assert(false && "unknown FieldLabel");
  }
}

Variance getVariance(const FieldLabel &f) {
  if (f.isIn()) {
    return Contravariant;
  } else if (f.isOut()) {
    return Covariant;
  } else if (f.isOffset()) {
    return Covariant;
  } else if (f.isLoad()) {
    return Covariant;
  } else if (f.isStore()) {
    return Contravariant;
  } else {
    assert(false && "unknown FieldLabel");
  }
}

std::string toString(const DerivedTypeVariable &dtv) { return dtv.str(); }

std::string toString(const PrimitiveTypeVariable &dtv) {
  return "#" + dtv.name;
}

std::string toString(const PooledTypeVariable *dtv) { return dtv->str(); }

std::string toString(const TypeVariable &dtv) { return dtv.str(); }

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
