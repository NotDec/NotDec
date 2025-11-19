#ifndef _NOTDEC_RETYPD_LATTICE_H
#define _NOTDEC_RETYPD_LATTICE_H

#include "TypeRecovery/LowTy.h"
#include "TypeRecovery/retypd/Schema.h"
#include "notdec-llvm2c/Interface/HType.h"
#include <cassert>
#include <clang/AST/Type.h>
#include <cstddef>
#include <optional>
namespace notdec::retypd {

using notdec::ast::HType;
using notdec::ast::HTypeContext;

std::optional<unsigned> strToUl(std::string s);
std::optional<unsigned> decodeSi(std::string s);
std::optional<unsigned> decodeUi(std::string s);

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
  HType *buildType(HTypeContext &ctx) const;

  LatticeTy(LatticeTyKind Kind, unsigned Size) : kind(Kind), Size(Size){};
  LatticeTy(const LatticeTy &Other) = delete;
  LatticeTyKind getKind() const { return kind; }
  unsigned getSize() const { return Size; }

  static std::shared_ptr<LatticeTy> create(PNTy LTy, Variance V, std::string TyName);
};

enum Signedness { SI_UNKNOW, SI_SIGNED, SI_UNSIGNED, SI_CONFLICT };

class IntLattice : public LatticeTy {
  Signedness Sign = SI_UNKNOW;

public:
  IntLattice(unsigned Size, Variance V)
      : LatticeTy(LK_Int, Size),
        Sign(V == Covariant ? SI_UNKNOW : SI_CONFLICT) {}
  static bool classof(const LatticeTy *S) { return S->getKind() == LK_Int; }
  static std::shared_ptr<IntLattice> create(PNTy LTy, Variance V, std::string TyName);
  bool join(const IntLattice &Other);
  bool meet(const IntLattice &Other);
  HType *buildType(HTypeContext &ctx) const;

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
  static std::shared_ptr<FloatLattice> create(PNTy LTy, Variance V, std::string TyName) {
    return std::make_shared<FloatLattice>(LTy.getSize());
  }
  bool join(const FloatLattice &Other) { return false; }
  bool meet(const FloatLattice &Other) { return false; }
};

class PointerLattice : public LatticeTy {
public:
  PointerLattice(unsigned Size) : LatticeTy(LK_Pointer, Size) {}
  static bool classof(const LatticeTy *S) { return S->getKind() == LK_Pointer; }
  static std::shared_ptr<PointerLattice> create(PNTy LTy, Variance V, std::string TyName) {
    return std::make_shared<PointerLattice>(LTy.getSize());
  }
  bool join(const PointerLattice &Other) { return false; }
  bool meet(const PointerLattice &Other) { return false; }
};

std::optional<std::shared_ptr<LatticeTy>> createLatticeTy(PNTy LTy, Variance V,
                                         std::string Name);
bool join(std::optional<std::shared_ptr<LatticeTy>> &L1, const std::optional<std::shared_ptr<LatticeTy>> &L2);
bool meet(std::optional<std::shared_ptr<LatticeTy>> &L1, const std::optional<std::shared_ptr<LatticeTy>> &L2);

std::string getNameForInt(std::string Name, llvm::Type* Ty);
// std::string getNameForInt(std::string Name, unsigned Size);

} // namespace notdec::retypd

#endif
