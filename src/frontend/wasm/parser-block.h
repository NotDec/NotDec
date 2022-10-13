#ifndef _NOTDEC_FRONTEND_WASM_PARSER_BLOCK_H_
#define _NOTDEC_FRONTEND_WASM_PARSER_BLOCK_H_

#include <vector>

#include "llvm/IR/IRBuilder.h"

namespace notdec::frontend::wasm {

struct BreakoutTarget {
    llvm::BasicBlock& target;
};

struct BlockContext
{
    llvm::Function& function;
    llvm::IRBuilder<>& irBuilder;
    std::vector<BreakoutTarget> blockStack;
    std::vector<llvm::Value*> locals;

    void visitBlock();
};


}

#endif