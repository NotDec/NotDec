#ifndef _NOTDEC_FRONTEND_CONTEXT_H_
#define _NOTDEC_FRONTEND_CONTEXT_H_

#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

namespace notdec::frontend {

// default to zero or false
struct options {
    bool recompile;
    bool compat_mode;
    int log_level = 5; // default log level = level_notice
};

struct BaseContext {
    options opt;
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    llvm::Module mod;
    std::map<std::string, llvm::Value *> namedValues;
    BaseContext(std::string name, options opt)
        : opt(opt), context(), builder(context), mod(name, context) {}
};

}

#endif

