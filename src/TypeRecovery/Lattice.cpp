
#include <cassert>
#include <clang/AST/ASTContext.h>
#include <cstdlib>
#include <llvm/Support/Casting.h>
#include <memory>
#include <optional>
#include <string>

#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/LowTy.h"
#include "TypeRecovery/Schema.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/Utils.h"

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

bool join(std::optional<std::shared_ptr<LatticeTy>> &L1,
          const std::optional<std::shared_ptr<LatticeTy>> &L2) {
  if (!L1.has_value()) {
    L1 = L2;
    return L2.has_value();
  }
  if (!L2.has_value()) {
    return false;
  }
  return (*L1)->join(**L2);
}

bool meet(std::optional<std::shared_ptr<LatticeTy>> &L1,
          const std::optional<std::shared_ptr<LatticeTy>> &L2) {
  if (!L1.has_value()) {
    L1 = L2;
    return L2.has_value();
  }
  if (!L2.has_value()) {
    return true;
  }
  return (*L1)->meet(**L2);
}

std::optional<std::shared_ptr<LatticeTy>> createLatticeTy(PNTy LTy, Variance V,
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

std::shared_ptr<LatticeTy> LatticeTy::create(PNTy LTy, Variance V,
                                             std::string Name) {
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

std::optional<unsigned> strToUl(std::string s) {
  char *pEnd = nullptr;
  unsigned long num = strtoul(s.c_str(), &pEnd, 10);
  if (pEnd == (s.c_str() + s.size())) {
    return num;
  }
  return std::nullopt;
}

std::optional<unsigned> decodeSi(std::string s) {
  if (s.front() == 'i') {
    return strToUl(s.substr(1));
  }
  return std::nullopt;
}

std::optional<unsigned> decodeUi(std::string s) {
  if (s.front() == 'u') {
    return strToUl(s.substr(1));
  }
  return std::nullopt;
}

std::shared_ptr<IntLattice> IntLattice::create(PNTy LTy, Variance V,
                                               std::string TyName) {
  assert(LTy.isNumber());
  std::shared_ptr<IntLattice> IL =
      std::make_shared<IntLattice>(LTy.getSize(), V);
  if (TyName == "") {
    return IL;

  } else if (startswith(TyName, "char")) {
    IL->Sign = SI_SIGNED;
  } else if (startswith(TyName, "sint") || decodeSi(TyName) || startswith(TyName, "slonglong")) {
    IL->Sign = SI_SIGNED;
  } else if (startswith(TyName, "uint") || decodeUi(TyName) || startswith(TyName, "ulonglong")) {
    IL->Sign = SI_UNSIGNED;
  } else if (startswith(TyName, "int") || startswith(TyName, "longlong") || startswith(TyName, "long")) {
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
  return Ctx.getIntegerType(false, getSize(), false);
}

std::string getNameForInt(std::string Name, llvm::Type *Ty) {
  if (startswith(Name, "int") || startswith(Name, "uint") ||
      startswith(Name, "sint")) {
    assert(Ty->isIntegerTy());
    // default to signed
    bool Signed = true;
    if (startswith(Name, "uint")) {
      Signed = false;
    }
    if (Ty->isIntegerTy(1)) {
      return "bool";
    } else if (Ty->isIntegerTy(8)) {
      return "char";
    } else if (Ty->isIntegerTy(16)) {
      if (Signed) {
        return "short";
      } else {
        return "ushort";
      }
    } else {
      if (Signed) {
        return "u" + std::to_string(Ty->getPrimitiveSizeInBits());
      } else {
        return "i" + std::to_string(Ty->getPrimitiveSizeInBits());
      }
    }
  }
  return Name;
}
// std::string getNameForInt(std::string Name, unsigned Size) {}

} // namespace notdec::retypd
