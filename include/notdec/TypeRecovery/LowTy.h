#ifndef _NOTDEC_RETYPD_LOWTY_H
#define _NOTDEC_RETYPD_LOWTY_H

#include <cassert>
#include <iostream>
#include <string>

#include "binarysub/LowTy.h"

namespace llvm {
class LLVMContext;
class Type;
} // namespace llvm

namespace notdec::retypd {

unsigned getSize(llvm::Type *Ty, unsigned PointerSize);

llvm::Type *Elem2LLVMType(llvm::LLVMContext &Ctx, std::string a,
                          unsigned Size);
std::string llvmType2Elem(llvm::Type *T);
PtrOrNum fromLLVMTy(llvm::Type *LowTy, long PointerSize);
bool isPtrOrNum(llvm::Type *LowTy, long PointerSize);
PNTy makePNTyFromLLVMType(llvm::Type *Ty, unsigned PointerSize);

} // namespace notdec::retypd
#endif
