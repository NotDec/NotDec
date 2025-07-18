#ifndef _NOTDEC_CONTEXT_H_
#define _NOTDEC_CONTEXT_H_

#include <map>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace notdec {

// sync with cmdline default value.
struct Options {
  int trLevel = 3;
  /// the stack recovery algorithm to use: retdec or notdec. default: notdec
  std::string stackRec = "notdec";
  int log_level = 5; // default log level = level_notice
};

struct DecompilerContext {
  Options opt;
  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  std::unique_ptr<llvm::Module> mod;
  std::map<std::string, llvm::Value *> namedValues;
  DecompilerContext(std::string name, Options opt)
      : opt(opt), context(), builder(context),
        mod(new llvm::Module(name, context)) {}

  llvm::Module &getModule() { return *mod; }
  bool hasModule() { return mod != nullptr; }
  void setModule(std::unique_ptr<llvm::Module> m) { mod = std::move(m); }
};

} // namespace notdec

#endif
