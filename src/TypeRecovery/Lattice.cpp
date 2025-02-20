#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/LowTy.h"
#include <cassert>
#include <clang/AST/ASTContext.h>
#include <llvm/Support/Casting.h>

namespace notdec::retypd {

clang::QualType LatticeTy::buildType(clang::ASTContext &ctx) const {
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

bool join(std::optional<LatticeTy> &L1, const std::optional<LatticeTy> &L2) {
  if (!L1.has_value()) {
    L1 = L2;
    return L2.has_value();
  }
  if (!L2.has_value()) {
    return false;
  }
  return L1->join(*L2);
}

bool meet(std::optional<LatticeTy> &L1, const std::optional<LatticeTy> &L2) {
  if (!L1.has_value()) {
    return false;
  }
  if (!L2.has_value()) {
    L1.reset();
    return true;
  }
  return L1->meet(*L2);
}

std::optional<LatticeTy> createLatticeTy(LowTy LTy, std::string Name) {
  if (LTy.isUnknown() || LTy.isNull()) {
    return std::nullopt;
  }
  // TODO implement other types
  if (LTy.isNumber()) {
    return LatticeTy::create(LTy, Name);
  }
  return std::nullopt;
}

LatticeTy LatticeTy::create(LowTy LTy, std::string Name) {
  assert(!LTy.isUnknown());
  if (LTy.isNumber()) {
    return IntLattice::create(LTy, Name);
  } else if (LTy.isPointer()) {
    return PointerLattice::create(LTy, Name);
  } else if (LTy.isNotPN()) {
    if (Name == "float") {
      return FloatLattice::create(LTy, Name);
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

LatticeTy IntLattice::create(LowTy LTy, std::string TyName) {
  assert(LTy.isNumber());
  IntLattice IL(LTy.getSize());
  if (TyName == "") {
    return IL;
  } else if (TyName == "sint" || TyName == "char") {
    IL.Sign = SI_SIGNED;
  } else if (TyName == "uint") {
    IL.Sign = SI_UNSIGNED;
  } else if (TyName == "int") {
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

clang::QualType IntLattice::buildType(clang::ASTContext &ctx) const {
  assert(getSize() != 0);
  if (getSize() == 8) {
    return ctx.CharTy;
  }
  if (Sign == SI_SIGNED) {
    return ctx.getIntTypeForBitwidth(getSize(), true);
  } else if (Sign == SI_UNSIGNED) {
    return ctx.getIntTypeForBitwidth(getSize(), false);
  }
  // Default to signed? TODO
  return ctx.getIntTypeForBitwidth(getSize(), true);
}

} // namespace notdec::retypd
