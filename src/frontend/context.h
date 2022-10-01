#ifndef _FRONTEND_CONTEXT_H_
#define _FRONTEND_CONTEXT_H_

#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

namespace notdec::frontend {

struct BaseContext {
    LLVMContextRef context;
    LLVMBuilderRef builder;
    LLVMModuleRef mod;
    std::map<std::string, llvm::Value *> namedValues;
};

}

#endif

