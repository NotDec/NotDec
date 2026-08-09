#include "TypeRecovery/LowTy.h"
#include <cassert>
#include <iostream>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <string>
#include <utility>

namespace notdec::retypd {

unsigned getSize(llvm::Type *Ty, unsigned PointerSize) {
  auto Size = Ty->getPrimitiveSizeInBits();
  if (Size != 0) {
    return Size;
  }
  if (Ty->isPointerTy() || Ty->isFunctionTy()) {
    return PointerSize;
  }
  if (auto *StructTy = llvm::dyn_cast<llvm::StructType>(Ty)) {
    unsigned Size = 0;
    for (llvm::Type *ElemTy : StructTy->elements()) {
      Size += getSize(ElemTy, PointerSize);
    }
    return Size;
  }
  if (auto *ArrayTy = llvm::dyn_cast<llvm::ArrayType>(Ty)) {
    return ArrayTy->getNumElements() *
           getSize(ArrayTy->getElementType(), PointerSize);
  }
  assert(false && "TODO: unhandled type");
}

PtrOrNum fromLLVMTy(llvm::Type *LowTy, long PointerSize) {
  assert(PointerSize == 32 || PointerSize == 64 || PointerSize == 256);
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

llvm::Type *Elem2LLVMType(llvm::LLVMContext &Ctx, std::string a,
                          unsigned Size) {
  // if (a == "top") {
  //   return nullptr;
  // }
  if (llvm::StringRef(a).starts_with("int")) {
    return llvm::Type::getIntNTy(Ctx, Size);
  }
  if (a == "float") {
    return llvm::Type::getFloatTy(Ctx);
  }
  if (a == "double") {
    return llvm::Type::getDoubleTy(Ctx);
  }
  if (a == "ptr") {
    return llvm::PointerType::get(Ctx, 0);
  }
  assert(false && "TODO: unhandled type");
}

/*
整数这一块规划：
- int就表示普通int，不用加size后缀。sint就表示signed int。uint就表示unsigned int，都不加后缀，而是根据ModuleLayout来。
- 部分特殊整数类型也不加后缀，比如short，char，long，longlong。
- 加后缀的以i或者u为后缀。i8 i16 i32 u32。
*/

std::string llvmType2Elem(llvm::Type *T) {
  if (T == nullptr) {
    return "top";
  }
  if (T->isIntegerTy(1)) {
    return "bool";
  }
  if (T->isIntegerTy(8)) {
    return "char";
  }
  if (T->isIntegerTy(16)) {
    return "short";
  }
  if (T->isIntegerTy(64)) {
    return "longlong";
  }
  if (T->isIntegerTy()) {
    return "i" + std::to_string(T->getIntegerBitWidth());
  }
  if (T->isFloatTy()) {
    return "float";
  }
  if (T->isDoubleTy()) {
    return "double";
  }
  // C 的 long double 在 x86-64 上是 x86_fp80（80 位扩展精度）；wrk 等
  // 项目含此类型，缺分支会直接 assert 崩溃。
  if (T->isX86_FP80Ty()) {
    return "longdouble";
  }
  if (T->isFP128Ty()) {
    return "fp128";
  }
  if (T->isPPC_FP128Ty()) {
    return "ppcfp128";
  }
  if (T->isHalfTy()) {
    return "half";
  }
  if (T->isBFloatTy()) {
    return "bfloat";
  }
  if (T->isPointerTy()) {
    return "ptr";
  }
  if (T->isFunctionTy()) {
    return "func";
  }
  if (T->isAggregateType()) {
    return "aggregate";
  }
  if (T->isVectorTy()) {
    return "vector";
  }
  assert(false && "TODO: unhandled LLVM type");
}

PNTy makePNTyFromLLVMType(llvm::Type *Ty, unsigned PointerSize) {
  auto PN = fromLLVMTy(Ty, PointerSize);
  std::string Elem;
  if (PN == NotPN) {
    Elem = llvmType2Elem(Ty);
    assert(Elem != "int");
  }
  return PNTy(PN, getSize(Ty, PointerSize), std::move(Elem));
}

} // namespace notdec::retypd
