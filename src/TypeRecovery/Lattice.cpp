
#include <cassert>
#include <clang/AST/ASTContext.h>
#include <llvm/Support/Casting.h>
#include <memory>

#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/LowTy.h"
#include "TypeRecovery/Schema.h"
#include "notdec-llvm2c/Interface/HType.h"

namespace notdec::retypd {

HType *LatticeTy::buildType(HTypeContext &ctx) const {
  switch (kind) {
  case LK_Int:
    return llvm::cast<IntLattice>(this)->buildType(ctx);
  case LK_Float:
    return llvm::cast<FloatLattice>(this)->buildType(ctx);
  case LK_Pointer:
    return llvm::cast<PointerLattice>(this)->buildType(ctx);
  }
  assert(false && "LatticeTy::buildType: Unhandled kind");
}

bool join(std::optional<std::shared_ptr<LatticeTy>> &L1, const std::optional<std::shared_ptr<LatticeTy>> &L2) {
  if (!L1.has_value()) {
    L1 = L2;
    return L2.has_value();
  }
  if (!L2.has_value()) {
    return false;
  }
  return (*L1)->join(**L2);
}

bool meet(std::optional<std::shared_ptr<LatticeTy>> &L1, const std::optional<std::shared_ptr<LatticeTy>> &L2) {
  if (!L1.has_value()) {
    L1 = L2;
    return L2.has_value();
  }
  if (!L2.has_value()) {
    return true;
  }
  return (*L1)->meet(**L2);
}

std::optional<std::shared_ptr<LatticeTy>> createLatticeTy(LowTy LTy, Variance V,
                                         std::string Name) {
  if (LTy.isUnknown() || LTy.isNull()) {
    return std::nullopt;
  }
  // TODO implement other types
  if (LTy.isNumber()) {
    return LatticeTy::create(LTy, V, Name);
  }
  return std::nullopt;
}

std::shared_ptr<LatticeTy> LatticeTy::create(LowTy LTy, Variance V, std::string Name) {
  assert(!LTy.isUnknown());
  if (LTy.isNumber()) {
    return IntLattice::create(LTy, V, Name);
  } else if (LTy.isPointer()) {
    return PointerLattice::create(LTy, V, Name);
  } else if (LTy.isNotPN()) {
    if (Name == "float") {
      return FloatLattice::create(LTy, V, Name);
    } else if (Name == "double") {
      return FloatLattice::create(LTy, V, Name);
    }
  }
  assert(false && "LatticeTy::create: Unhandled LowTy");
}

bool LatticeTy::join(const LatticeTy &other) {
  assert(kind == other.kind);
  if (kind == LK_Int) {
    return llvm::cast<IntLattice>(this)->join(llvm::cast<IntLattice>(other));
  } else if (kind == LK_Float) {
    return llvm::cast<FloatLattice>(this)->join(
        llvm::cast<FloatLattice>(other));
  } else if (kind == LK_Pointer) {
    return llvm::cast<PointerLattice>(this)->join(
        llvm::cast<PointerLattice>(other));
  }
  assert(false && "LatticeTy::join: Unhandled kind");
}

bool LatticeTy::meet(const LatticeTy &other) {
  assert(kind == other.kind);
  if (kind == LK_Int) {
    return llvm::cast<IntLattice>(this)->meet(llvm::cast<IntLattice>(other));
  } else if (kind == LK_Float) {
    return llvm::cast<FloatLattice>(this)->meet(
        llvm::cast<FloatLattice>(other));
  } else if (kind == LK_Pointer) {
    return llvm::cast<PointerLattice>(this)->meet(
        llvm::cast<PointerLattice>(other));
  }
  assert(false && "LatticeTy::meet: Unhandled kind");
}

std::shared_ptr<IntLattice> IntLattice::create(LowTy LTy, Variance V, std::string TyName) {
  assert(LTy.isNumber());
  std::shared_ptr<IntLattice> IL = std::make_shared<IntLattice>(LTy.getSize(), V);
  if (TyName == "") {
    return IL;
  } else if (TyName == "char") {
    IL->Sign = SI_SIGNED;
  } else if (TyName == "sint" || TyName == "slonglong") {
    IL->Sign = SI_SIGNED;
  } else if (TyName == "uint" || TyName == "ulonglong") {
    IL->Sign = SI_UNSIGNED;
  } else if (TyName == "int" || TyName == "longlong") {
    // do nothing
  } else {
    assert(false && "IntLattice::create: Unhandled type");
  }
  return IL;
}

bool IntLattice::join(const IntLattice &Other) {
  // least upper bound
  if (Sign == SI_UNKNOW || Other.Sign == SI_UNKNOW) {
    return setSign(SI_UNKNOW);
  } else if (Other.Sign == SI_CONFLICT) {
    // do nothing
  } else if (Sign == SI_CONFLICT) {
    return setSign(Other.Sign);
  }
  assert(Sign != SI_CONFLICT && Sign != SI_UNKNOW);
  assert(Other.Sign != SI_CONFLICT && Other.Sign != SI_UNKNOW);
  if (Sign != Other.Sign) {
    return setSign(SI_UNKNOW);
  }
  return false;
}

bool IntLattice::meet(const IntLattice &Other) {
  // greatest lower bound
  if (Sign == SI_CONFLICT || Other.Sign == SI_CONFLICT) {
    return setSign(SI_CONFLICT);
  } else if (Other.Sign == SI_UNKNOW) {
    // do nothing
  } else if (Sign == SI_UNKNOW) {
    return setSign(Other.Sign);
  }
  assert(Sign != SI_CONFLICT && Sign != SI_UNKNOW);
  assert(Other.Sign != SI_CONFLICT && Other.Sign != SI_UNKNOW);
  if (Sign != Other.Sign) {
    return setSign(SI_CONFLICT);
  }
  return false;
}

HType *IntLattice::buildType(HTypeContext &Ctx) const {
  assert(getSize() != 0);
  if (getSize() == 8) {
    return Ctx.getChar();
  }
  if (Sign == SI_SIGNED) {
    return Ctx.getIntegerType(false, getSize(), false);
  } else if (Sign == SI_UNSIGNED) {
    return Ctx.getIntegerType(false, getSize(), true);
  }
  // Default to signed? TODO
  return Ctx.getIntegerType(false, getSize(), true);
}

} // namespace notdec::retypd
