#ifndef _NOTDEC_RETYPD_LATTICE_H
#define _NOTDEC_RETYPD_LATTICE_H

#include <llvm/IR/Type.h>
#include <string>

namespace notdec::retypd {

bool isLowTyString(std::string a);
llvm::Type *ToLLVMType(llvm::LLVMContext &Ctx, std::string a);
std::string fromLLVMType(llvm::Type *T);
std::string join(std::string a, std::string b);
std::string meet(std::string a, std::string b);

} // namespace notdec::retypd
#endif
