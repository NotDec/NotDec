#ifndef _NOTDEC_RETYPD_LOWTY_H
#define _NOTDEC_RETYPD_LOWTY_H

#include <cassert>
#include <iostream>
#include <string>

namespace llvm {
class LLVMContext;
class Type;
} // namespace llvm

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
struct PNTy {
  unsigned Size;
  PtrOrNum Ty = Null;
  std::string Elem;
  bool hasConflict = false;

  PNTy() = default;
  PNTy(PtrOrNum Ty, unsigned Size, std::string Elem = {});
  PNTy(llvm::Type *Ty, unsigned PointerSize);
  PNTy(std::string Str, unsigned Size);
  PNTy(std::string Serialized);

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
  bool merge(PNTy Other, bool joinOrMeet = false);

  std::string latticeStr() const {
    if (Ty == NotPN) {
      assert(!Elem.empty());
      return Elem;
    }
    return toString(Ty);
  }
  std::string str() const { return latticeStr() + " " + std::to_string(Size); }
  bool operator==(const PNTy &rhs) const {
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
  bool operator!=(const PNTy &rhs) const { return !(*this == rhs); }
};

} // namespace notdec::retypd
#endif
