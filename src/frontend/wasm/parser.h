#ifndef _FRONTEND_WASM_PARSER_H_
#define _FRONTEND_WASM_PARSER_H_

#include <iostream>

// wabt header
#include "src/binary-reader.h"
#include "src/binary-reader-ir.h"
#include "src/ir.h"
#include "src/stream.h"
#include "src/validator.h"

#include "frontend/context.h"

namespace notdec::frontend::wasm {

struct Context {
    BaseContext& llvmCtx;
    wabt::Module module;
    // mapping from global index to llvm thing
    std::vector<llvm::GlobalVariable*> globs;
    std::vector<llvm::Function*> funcs;

    Context(BaseContext& llvmCtx)
        : llvmCtx(llvmCtx) {}

    void visitModule();
    llvm::Type* convertType(wabt::Type& ty);
};

std::unique_ptr<Context> 
parse_wasm(BaseContext& llvmCtx, const char *file_name);

}
#endif