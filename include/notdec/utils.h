#ifndef _NOTDEC_UTILS_H_
#define _NOTDEC_UTILS_H_

#include <cstdint>
#include <cstring>
#include <iostream>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <sstream>

enum log_level {
  level_emergent = 0,
  level_alert = 1,
  level_critical = 2,
  level_error = 3,
  level_warning = 4,
  level_notice = 5,
  level_info = 6,
  level_debug = 7
};

extern llvm::cl::opt<log_level> logLevel;

namespace notdec {

bool inline is_size_t(llvm::Type *ty, llvm::Module &M) {
  return ty->isIntegerTy(M.getDataLayout().getPointerSizeInBits());
}

bool inline is_size_t(llvm::Value *val, llvm::Module &M) {
  return is_size_t(val->getType(), M);
}

void inline assert_size_t(llvm::Type *ty, llvm::Module &M) {
  assert(is_size_t(ty, M));
}

void inline assert_size_t(llvm::Value *val, llvm::Module &M) {
  assert(is_size_t(val, M));
}

void printModule(llvm::Module &M, const char *path);

} // namespace notdec

#endif
