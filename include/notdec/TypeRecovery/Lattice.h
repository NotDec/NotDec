#ifndef _NOTDEC_RETYPD_LATTICE_H
#define _NOTDEC_RETYPD_LATTICE_H

#include "TypeRecovery/LowTy.h"
#include "notdec-llvm2c/Interface/HType.h"
#include <cassert>
#include <clang/AST/Type.h>
#include <cstddef>
#include <optional>
namespace notdec::retypd {

using notdec::ast::HType;
using notdec::ast::HTypeContext;

class LatticeTy {
public:
  enum LatticeTyKind {
    LK_Int,
    LK_Float,
    LK_Pointer,
  };

protected:
  LatticeTyKind kind;
  unsigned Size = 0;

public:
  // const LowTy &getLowTy() const;
  bool join(const LatticeTy &other);
  bool meet(const LatticeTy &other);
  HType * buildType(HTypeContext &ctx) const;

  LatticeTy(LatticeTyKind Kind, unsigned Size) : kind(Kind), Size(Size){};
  LatticeTyKind getKind() const { return kind; }
  unsigned getSize() const { return Size; }

  static LatticeTy create(LowTy LTy, std::string TyName);
};

enum Signedness { SI_UNKNOW, SI_SIGNED, SI_UNSIGNED, SI_CONFLICT };

class IntLattice : public LatticeTy {
  Signedness Sign = SI_UNKNOW;

public:
  IntLattice(unsigned Size) : LatticeTy(LK_Int, Size) {}
  static bool classof(const LatticeTy *S) { return S->getKind() == LK_Int; }
  static LatticeTy create(LowTy LTy, std::string TyName);
  bool join(const IntLattice &Other);
  bool meet(const IntLattice &Other);
  HType * buildType(HTypeContext &ctx) const;

  auto getSign() const { return Sign; }
  bool setSign(Signedness NewSign) {
    if (Sign == NewSign) {
      return false;
    }
    Sign = NewSign;
    return true;
  }
};

class FloatLattice : public LatticeTy {
public:
  FloatLattice(unsigned Size) : LatticeTy(LK_Float, Size) {}
  static bool classof(const LatticeTy *S) { return S->getKind() == LK_Float; }
  static LatticeTy create(LowTy LTy, std::string TyName) {
    return FloatLattice(LTy.getSize());
  }
  bool join(const FloatLattice &Other) { return false; }
  bool meet(const FloatLattice &Other) { return false; }
};

class PointerLattice : public LatticeTy {
public:
  PointerLattice(unsigned Size) : LatticeTy(LK_Pointer, Size) {}
  static bool classof(const LatticeTy *S) { return S->getKind() == LK_Pointer; }
  static LatticeTy create(LowTy LTy, std::string TyName) {
    return PointerLattice(LTy.getSize());
  }
  bool join(const PointerLattice &Other) { return false; }
  bool meet(const PointerLattice &Other) { return false; }
};

std::optional<LatticeTy> createLatticeTy(LowTy LTy, std::string Name);
bool join(std::optional<LatticeTy> &L1, const std::optional<LatticeTy> &L2);
bool meet(std::optional<LatticeTy> &L1, const std::optional<LatticeTy> &L2);

} // namespace notdec::retypd

#endif
