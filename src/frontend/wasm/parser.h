#ifndef _FRONTEND_WASM_PARSER_H_
#define _FRONTEND_WASM_PARSER_H_

#include <iostream>

// wabt header
#include "src/binary-reader.h"
#include "src/binary-reader-ir.h"
#include "src/ir.h"
#include "src/stream.h"
#include "src/validator.h"
#include "src/cast.h"

#include "frontend/context.h"

namespace notdec::frontend::wasm {

struct Context {
    BaseContext& baseCtx;
    llvm::LLVMContext& llvmContext;
    llvm::Module& llvmModule;
    wabt::Module module;
    // mapping from global index to llvm thing
    std::vector<llvm::GlobalVariable*> globs;
    std::vector<llvm::Function*> funcs;

    Context(BaseContext& baseCtx)
        : baseCtx(baseCtx), llvmContext(baseCtx.context), llvmModule(baseCtx.mod) {}

    void visitModule();
    void visitGlobal(wabt::Global* gl, wabt::Index index);
    void visitImportFunc(wabt::Func* func);
    llvm::Type* convertType(wabt::Type& ty);
    llvm::FunctionType* convertFuncType(const wabt::FuncSignature& decl);


private:
    wabt::Index _func_index = 0;
};

std::unique_ptr<Context> 
parse_wasm(BaseContext& llvmCtx, const char *file_name);

}
#endif