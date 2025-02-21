#include "notdec-llvm2c/Range.h"
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
  if (std::holds_alternative<One>(label)) {
    return "_1_";
  } else if (std::holds_alternative<ForgetLabel>(label)) {
    return "forget " + toString(std::get<ForgetLabel>(label).label);
  } else if (std::holds_alternative<ForgetBase>(label)) {
    return "forget " + toString(std::get<ForgetBase>(label).Base) +
           toString(std::get<ForgetBase>(label).V);
  } else if (std::holds_alternative<RecallLabel>(label)) {
    return "recall " + toString(std::get<RecallLabel>(label).label);
  } else if (std::holds_alternative<RecallBase>(label)) {
    return "recall " + toString(std::get<RecallBase>(label).Base) +
           toString(std::get<RecallBase>(label).V);
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
  if (std::holds_alternative<InLabel>(f)) {
    return "in_" + std::get<InLabel>(f).name;
  } else if (std::holds_alternative<OutLabel>(f)) {
    return std::get<OutLabel>(f).name.empty()
               ? "out"
               : "out_" + std::get<OutLabel>(f).name;
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
