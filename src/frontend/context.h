#ifndef _NOTDEC_FRONTEND_CONTEXT_H_
#define _NOTDEC_FRONTEND_CONTEXT_H_

#include <map>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace notdec::frontend {

// sync with cmdline default value.
struct options {
  bool from_wasm = false;
  bool recompile = false;
  bool compat_mode = true;
  bool expandMem = true;
  std::string stackRec = "notdec";
  int log_level = 5; // default log level = level_notice
};

struct BaseContext {
  options opt;
  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  std::unique_ptr<llvm::Module> mod;
  std::map<std::string, llvm::Value *> namedValues;
  BaseContext(std::string name, options opt)
      : opt(opt), context(), builder(context),
        mod(new llvm::Module(name, context)) {}

  llvm::Module &getModule() { return *mod; }
  bool hasModule() { return mod != nullptr; }
  void setModule(std::unique_ptr<llvm::Module> m) { mod = std::move(m); }
};

} // namespace notdec::frontend

#endif
