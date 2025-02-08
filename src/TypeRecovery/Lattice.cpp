#include "TypeRecovery/Lattice.h"
#include "utils.h"
#include <cassert>
#include <iostream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

namespace notdec::retypd {

unsigned getSize(llvm::Type *Ty) {
  auto Size = Ty->getPrimitiveSizeInBits();
  if (Size != 0) {
    return Size;
  }
  assert(false && "TODO: unhandled type");
}

// #region PtrOrNum

// Like a operator
PtrOrNum unify(const PtrOrNum &Left, const PtrOrNum &Right) {
  // this type enum is determined when creating the node by inspecting the LLVM
  // Type
  if (Left == NotPN) {
    assert(Right == NotPN);
  }
  if (Right == NotPN) {
    assert(Left == NotPN);
  }

  if (Left == Right) { // same inner type
    return Left;
  } else {
    unsigned char Val = UniTyMergeMap[Left][Right];
    assert(Val != 2);
    if (Val != 0) {
      return Left;
    } else {
      return Right;
    }
  }
}

PtrOrNum fromIPChar(char C) {
  switch (C) {
  case 'I':
    return Number;
  case 'P':
    return Pointer;
  default:
    assert(false && "fromIPChar: unhandled char");
  }
}

PtrOrNum fromLLVMTy(llvm::Type *LowTy, long PointerSize) {
  assert(PointerSize == 32 || PointerSize == 64);
  if (LowTy == nullptr) {
    return Null;
  } else if (LowTy->isPointerTy()) {
    return Pointer;
  } else if (LowTy->isIntegerTy(PointerSize)) {
    return Unknown;
  } else {
    return NotPN;
  }
}

bool isPtrOrNum(llvm::Type *LowTy, long PointerSize) {
  PtrOrNum PN = fromLLVMTy(LowTy, PointerSize);
  return PN != Null && PN != NotPN;
}

std::string toString(PtrOrNum Ty) {
  switch (Ty) {
  case Unknown:
    return "unk";
  case Number:
    return "num";
  case Pointer:
    return "ptr";
  case Null:
    return "null";
  case NotPN:
    return "notPN";
  default:
    assert(false && "toString: Unknown type");
  }
}

// #endregion PtrOrNum

bool LatticeTy::setPtrOrNum(PtrOrNum NewTy) {
  assert(NewTy != Null);
  if (Ty == Null) {
    Ty = NewTy;
    return true;
  }

  if (Ty == NotPN) {
    assert(NewTy == NotPN);
    return false;
  }
  assert(NewTy != NotPN);

  assert(isPNRelated() && NewTy != NotPN && NewTy != Null);
  if (Ty == NewTy) {
    return false;
  }
  if ((Ty == Number && NewTy == Pointer) ||
      (Ty == Pointer && NewTy == Number)) {
    std::cerr << "Warning: PNINode::setPtrOrNum: Pointer and NonPtr merge\n";
    hasConflict = true;
    if (Ty == Number && NewTy == Pointer) {
      Ty = Pointer;
      return true;
    } else {
      return false;
    }
  }
  assert(Ty == Unknown);
  Ty = NewTy;
  return true;
}

bool LatticeTy::merge(LatticeTy Other, bool joinOrMeet) {
  bool Updated = setPtrOrNum(Other.Ty);
  if (isNotPN() && !Other.Elem.empty()) {
    std::string NewElem;
    if (joinOrMeet) {
      NewElem = join(Other.Elem, Elem);
    } else {
      NewElem = meet(Other.Elem, Elem);
    }
    Updated |= NewElem != Elem;
    Elem = NewElem;
  }
  return Updated;
}

// https://en.wikipedia.org/wiki/Join_and_meet

static bool isFloat(std::string a) { return a == "float" || a == "double"; }
static bool isInt1(std::string a) {
  return startswith(a, "int") || startswith(a, "sint") || startswith(a, "uint");
}

llvm::Type *ToLLVMType(llvm::LLVMContext &Ctx, std::string a, unsigned Size) {
  // if (a == "top") {
  //   return nullptr;
  // }
  if (startswith(a, "int")) {
    return llvm::Type::getIntNTy(Ctx, Size);
  }
  if (a == "float") {
    return llvm::Type::getFloatTy(Ctx);
  }
  if (a == "double") {
    return llvm::Type::getDoubleTy(Ctx);
  }
  if (a == "ptr") {
    return llvm::Type::getInt8PtrTy(Ctx);
  }
  assert(false && "TODO: unhandled type");
}

std::string llvmType2Elem(llvm::Type *T) {
  if (T == nullptr) {
    return "top";
  }
  if (T->isIntegerTy()) {
    return "int";
  }
  if (T->isFloatTy()) {
    return "float";
  }
  if (T->isDoubleTy()) {
    return "double";
  }
  if (T->isPointerTy()) {
    return "ptr";
  }
  if (T->isFunctionTy()) {
    return "func";
  }
  assert(false && "TODO: unhandled LLVM type");
}

std::string join(std::string a, std::string b) {
  // std::cerr << "joining " << a << " with " << b << "\n";
  if (a.empty()) {
    return b;
  }
  if (b.empty()) {
    return a;
  }
  if (a == b) {
    return a;
  }
  if (a == "top") {
    return a;
  }
  if (b == "top") {
    return b;
  }

  std::cerr << "unable to handle join: " << a << " with " << b << "\n";
  std::abort();
  return a;
}

std::string meet(std::string a, std::string b) {
  if (a.empty()) {
    return b;
  }
  if (b.empty()) {
    return a;
  }
  if (a == b) {
    return a;
  }
  if (a == "top") {
    return b;
  }
  if (b == "top") {
    return a;
  }

  // conflict, return top
  if (isFloat(a) && isInt1(b)) {
    return "top";
  }
  if (isInt1(a) && isFloat(b)) {
    return "top";
  }

  // prefer specific int
  if (isInt1(a) && isInt1(b)) {
    if (a == "int") {
      return b;
    }
    if (b == "int") {
      return a;
    }
  }

  std::cerr << "unable to handle meet: " << a << " with " << b << "\n";
  std::abort();
  return a;
}

} // namespace notdec::retypd
