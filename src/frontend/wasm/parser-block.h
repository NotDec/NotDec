#ifndef _NOTDEC_FRONTEND_WASM_PARSER_BLOCK_H_
#define _NOTDEC_FRONTEND_WASM_PARSER_BLOCK_H_

#include <vector>
#include <deque>
#include <iostream>

// wabt header
#include "src/ir.h"
#include "src/cast.h"

#include "llvm/IR/IRBuilder.h"

#include "parser.h"

namespace notdec::frontend::wasm {

struct BreakoutTarget {
    llvm::BasicBlock& target;
    // value stack location
    std::size_t pos;
    wabt::BlockDeclaration& sig;
    wabt::LabelType lty;

    BreakoutTarget(llvm::BasicBlock& target, std::size_t pos, wabt::BlockDeclaration& sig, wabt::LabelType lty)
        : target(target), pos(pos), sig(sig), lty(lty) {}
};

struct BlockContext
{
    Context& ctx;
    llvm::LLVMContext& llvmContext;
    llvm::Function& function;
    llvm::IRBuilder<>& irBuilder;
    std::vector<BreakoutTarget> blockStack;
    std::vector<llvm::Value*> locals;
    // wasm stack
    std::vector<llvm::Value*> stack;
    std::vector<wabt::Type> type_stack;

    // rvalue reference here, use it by std::move(locals): BlockContext bctx(*function, irBuilder, std::move(locals));
    BlockContext(Context& ctx, llvm::Function& f, llvm::IRBuilder<>& b, std::vector<llvm::Value*>&& locals)
        : ctx(ctx), llvmContext(ctx.llvmContext), function(f), irBuilder(b), locals(locals) {}

    void visitBlock(wabt::LabelType lty, llvm::BasicBlock* entry, llvm::BasicBlock* exit, wabt::BlockDeclaration& decl, wabt::ExprList& exprs);
    void visitControlInsts(llvm::BasicBlock* entry, llvm::BasicBlock* exit, wabt::ExprList& exprs);
    void visitBinaryInst(wabt::BinaryExpr* expr);
    void visitConstInst(wabt::ConstExpr* expr);
    void visitCallInst(wabt::CallExpr* expr);

    void visitLoadInst(wabt::LoadExpr* expr);
    void visitStoreInst(wabt::StoreExpr* expr);
    llvm::Value* convertStackAddr(uint64_t offset);

    void visitLocalGet(wabt::LocalGetExpr* expr);
    void visitGlobalGet(wabt::GlobalGetExpr* expr);
};

llvm::Constant* visitConst(llvm::LLVMContext &llvmContext, const wabt::Const& const_);
const char* labelTypeToString(wabt::LabelType lty);
int64_t unwrapIntConstant(llvm::Constant* c);

}

#endif