#include "TypeRecovery/Lattice.h"
#include "utils.h"
#include <cassert>
#include <iostream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

namespace notdec::retypd {

// https://en.wikipedia.org/wiki/Join_and_meet

static bool isFloat(std::string a) { return a == "float" || a == "double"; }
static bool isInt1(std::string a) {
  return startswith(a, "int") || startswith(a, "sint") || startswith(a, "uint");
}

bool isLowTyString(std::string a) {
  return startswith(a, "int") || a == "float" || a == "double" || a == "ptr";
}

llvm::Type *ToLLVMType(llvm::LLVMContext &Ctx, std::string a) {
  return nullptr;
}

std::string fromLLVMType(llvm::Type *T) {
  if (T == nullptr) {
    return "top";
  }
  if (T->isIntegerTy()) {
    return "int" + std::to_string(T->getIntegerBitWidth());
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
  assert(false && "TODO: unhandled LLVM type");
}

std::string join(std::string a, std::string b) {
  // std::cerr << "joining " << a << " with " << b << "\n";
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

  return a;
}

std::string meet(std::string a, std::string b) {
  if (a == b) {
    return a;
  }
  if (a == "top") {
    return b;
  }
  if (b == "top") {
    return a;
  }

  // prefer float
  if (isFloat(a) && isInt1(b)) {
    return a;
  }
  if (isInt1(a) && isFloat(b)) {
    return b;
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

  return a;
}

} // namespace notdec::retypd
