#ifndef _NOTDEC_FRONTEND_WASM_PARSER_H_
#define _NOTDEC_FRONTEND_WASM_PARSER_H_

#include <iostream>

// wabt header
#include "src/binary-reader.h"
#include "src/binary-reader-ir.h"
#include "src/ir.h"
#include "src/stream.h"
#include "src/validator.h"
#include "src/cast.h"

#include "frontend/context.h"
#include "utils.h"

namespace notdec::frontend::wasm {

struct Context {
    BaseContext& baseCtx;
    llvm::LLVMContext& llvmContext;
    llvm::Module& llvmModule;
    wabt::Module module;
    // mapping from global index to llvm thing
    std::vector<llvm::GlobalVariable*> globs;
    std::vector<llvm::Function*> funcs;
    std::vector<llvm::GlobalVariable*> mems;

    Context(BaseContext& baseCtx)
        : baseCtx(baseCtx), llvmContext(baseCtx.context), llvmModule(baseCtx.mod) {}

    void visitModule();
    void visitGlobal(wabt::Global& gl, bool isExternal);
    void visitFunc(wabt::Func& func);
    llvm::Constant* visitInitExpr(wabt::ExprList& expr);

    llvm::Function* declareFunc(wabt::Func& func, bool isExternal);
    llvm::GlobalVariable* declareMemory(wabt::Memory& mem, bool isExternal);
    llvm::Type* convertType(const wabt::Type& ty);
    llvm::FunctionType* convertFuncType(const wabt::FuncSignature& decl);
    void setFuncArgName(llvm::Function& function, const wabt::FuncSignature& decl);

private:
    wabt::Index _func_index = 0;
    wabt::Index _glob_index = 0;
    wabt::Index _mem_index = 0;
};

std::unique_ptr<Context> 
parse_wasm(BaseContext& llvmCtx, const char *file_name);

}
#endif