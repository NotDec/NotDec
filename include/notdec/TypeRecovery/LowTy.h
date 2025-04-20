#ifndef _NOTDEC_RETYPD_LOWTY_H
#define _NOTDEC_RETYPD_LOWTY_H

#include <cassert>
#include <iostream>
#include <llvm/IR/Type.h>
#include <string>

namespace notdec::retypd {

unsigned getSize(llvm::Type *Ty, unsigned PointerSize);

std::string join(std::string a, std::string b);
std::string meet(std::string a, std::string b);
llvm::Type *Elem2LLVMType(llvm::LLVMContext &Ctx, std::string a);
std::string llvmType2Elem(llvm::Type *T);

// #region PtrOrNum

enum PtrOrNum {
  Unknown = 0,
  Number = 1,
  Pointer = 2,
  Null = 3,
  NotPN = 4,
};

inline PtrOrNum inverse(PtrOrNum Ty) {
  if (Ty == Number) {
    return Pointer;
  } else if (Ty == Pointer) {
    return Number;
  } else {
    assert(false && "inverse: Unknown type");
  }
}

// mergeMap[left index][right index] = isPreserveLeft
// column -> right index, row -> left index
const unsigned char UniTyMergeMap[][3] = {
    {2, 0, 0},
    {1, 2, 0},
    {1, 1, 2},
};

PtrOrNum unify(const PtrOrNum &Left, const PtrOrNum &Right);
PtrOrNum fromIPChar(char C);
PtrOrNum fromLLVMTy(llvm::Type *LowTy, long PointerSize);
bool isPtrOrNum(llvm::Type *LowTy, long PointerSize);
std::string toString(PtrOrNum Ty);
PtrOrNum str2PtrOrNum(std::string Str);

// #endregion PtrOrNum

// Only when Ty is NotPN, Elem is meaningful.
struct LowTy {
  unsigned Size;
  PtrOrNum Ty = Null;
  std::string Elem;
  bool hasConflict = false;

  LowTy() = default;
  LowTy(llvm::Type *Ty, unsigned PointerSize)
      : Size(::notdec::retypd::getSize(Ty, PointerSize)),
        Ty(fromLLVMTy(Ty, PointerSize)) {
    if (isNotPN()) {
      Elem = llvmType2Elem(Ty);
      assert(Elem != "int");
    }
  }

  LowTy(std::string Str, unsigned Size)
      : Size(Size), Ty(str2PtrOrNum(Str)) {
    if (isNotPN()) {
      Elem = Str;
      assert(Elem != "int");
    }
  }

  LowTy(std::string Serialized)
      : LowTy(Serialized.substr(0, Serialized.find(" ")),
                  std::stoi(Serialized.substr(Serialized.find(" ") + 1))) {
    assert(Serialized.find(" ") != std::string::npos);
    assert(this->str() == Serialized);
  }

  unsigned getSize() const { return Size; }
  std::string getElem() const { return Elem; }
  PtrOrNum getPtrOrNum() const { return Ty; }
  bool isNumber() const { return getPtrOrNum() == Number; }
  bool isPointer() const { return getPtrOrNum() == Pointer; }
  bool isUnknown() const { return getPtrOrNum() == Unknown; }
  bool isNull() const { return getPtrOrNum() == Null; }
  bool isNotPN() const { return getPtrOrNum() == NotPN; }
  bool isPNRelated() const {
    return getPtrOrNum() != NotPN && getPtrOrNum() != Null;
  }
  char getPNChar() const {
    assert(isPNRelated());
    if (isNull()) {
      return 'n';
    } else if (isUnknown()) {
      return 'u';
    } else if (isNumber()) {
      return 'i';
    } else if (isPointer()) {
      return 'p';
    } else if (isNotPN()) {
      return 'o';
    }
    assert(false && "CGNode::getPNChar: unhandled type");
  }
  bool isConflict() const { return hasConflict; }
  void setConflict() { hasConflict = true; }

  // return true if updated.
  bool setPtrOrNum(PtrOrNum NewTy);

  // return if updated
  bool merge(LowTy Other, bool joinOrMeet = false);

  std::string latticeStr() const {
    if (Ty == NotPN) {
      assert(!Elem.empty());
      return Elem;
    }
    return toString(Ty);
  }
  std::string str() const { return latticeStr() + " " + std::to_string(Size); }
  bool operator==(const LowTy &rhs) const {
    if (rhs.Size != Size) {
      return false;
    }
    if (rhs.Ty != Ty) {
      return false;
    }
    if (Ty == NotPN) {
      return rhs.Elem == Elem;
    }
    return true;
  }
  bool operator!=(const LowTy &rhs) const { return !(*this == rhs); }
};

} // namespace notdec::retypd
#endif
