#ifndef _NOTDEC_FRONTEND_WASM_PARSER_BLOCK_H_
#define _NOTDEC_FRONTEND_WASM_PARSER_BLOCK_H_

#include <vector>
#include <deque>
#include <iostream>

// wabt header
#include "wabt/ir.h"
#include "wabt/cast.h"

#include "llvm/IR/IRBuilder.h"

#include "parser.h"

namespace notdec::frontend::wasm {

struct BreakoutTarget {
    llvm::BasicBlock& target;
    std::deque<llvm::PHINode*> phis;
    // value stack location
    std::size_t pos;
    wabt::BlockDeclaration& sig;
    wabt::LabelType lty;

    BreakoutTarget(llvm::BasicBlock& target, std::deque<llvm::PHINode*> phis, std::size_t pos, wabt::BlockDeclaration& sig, wabt::LabelType lty)
        : target(target), phis(phis), pos(pos), sig(sig), lty(lty) {}
};
struct extendType
{
	llvm::Type* i8Type;
	llvm::Type* i16Type;
	llvm::Type* i32Type;
	llvm::Type* i64Type;
	llvm::Type* f32Type;
	llvm::Type* f64Type;
	llvm::PointerType* i8PtrType;
	
	llvm::FixedVectorType* i8x8Type;
	llvm::FixedVectorType* i16x4Type;
	llvm::FixedVectorType* i32x2Type;
	llvm::FixedVectorType* i64x1Type;
	llvm::FixedVectorType* f32x2Type;
	llvm::FixedVectorType* f64x1Type;
	llvm::FixedVectorType* i8x16Type;
	llvm::FixedVectorType* i16x8Type;
	llvm::FixedVectorType* i32x4Type;
	llvm::FixedVectorType* i64x2Type;
	llvm::FixedVectorType* f32x4Type;
	llvm::FixedVectorType* f64x2Type;
	llvm::FixedVectorType* i8x32Type;
	llvm::FixedVectorType* i16x16Type;
	llvm::FixedVectorType* i32x8Type;
	llvm::FixedVectorType* i64x4Type;
	llvm::FixedVectorType* i8x48Type;
	llvm::FixedVectorType* i16x24Type;
	llvm::FixedVectorType* i32x12Type;
	llvm::FixedVectorType* i64x6Type;
	llvm::FixedVectorType* i8x64Type;
	llvm::FixedVectorType* i16x32Type;
	llvm::FixedVectorType* i32x16Type;
	llvm::FixedVectorType* i64x8Type;
	extendType(llvm::LLVMContext& llvmContext);
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
    int log_level;
    extendType type;

    // rvalue reference here, use it by std::move(locals): BlockContext bctx(*function, irBuilder, std::move(locals));
    BlockContext(Context& ctx, llvm::Function& f, llvm::IRBuilder<>& b, std::vector<llvm::Value*>&& locals)
        : ctx(ctx), llvmContext(ctx.llvmContext), function(f), irBuilder(b), locals(locals), log_level(ctx.log_level), type(ctx.llvmContext) {}

    void visitBlock(wabt::LabelType lty, llvm::BasicBlock* entry, llvm::BasicBlock* exit, wabt::BlockDeclaration& decl, wabt::ExprList& exprs);
    void visitControlInsts(llvm::BasicBlock* entry, llvm::BasicBlock* exit, wabt::ExprList& exprs);
    void dispatchExprs(wabt::Expr& expr);
    void unwindStackTo(size_t pos);

    llvm::Instruction* visitBr(wabt::Expr* expr, std::size_t ind, llvm::Value* cond, llvm::BasicBlock* nextBlock);
    void visitUnaryInst(wabt::UnaryExpr* expr);
    void visitBinaryInst(wabt::BinaryExpr* expr);
    void visitTernaryInst(wabt::TernaryExpr* expr);
    void visitCompareExpr(wabt::CompareExpr* expr);
    void visitConvertExpr(wabt::ConvertExpr* expr);
    void visitConstInst(wabt::ConstExpr* expr);
    void visitCallInst(wabt::CallExpr* expr);
    void visitCallIndirectInst(wabt::CallIndirectExpr* expr);
    void visitSelectExpr(wabt::SelectExpr* expr);

    void visitLoadInst(wabt::LoadExpr* expr);
    void visitStoreInst(wabt::StoreExpr* expr);
    llvm::Value* convertStackAddr(uint64_t offset);

    void visitLocalGet(wabt::LocalGetExpr* expr);
    void visitLocalSet(wabt::LocalSetExpr* expr);
    void visitLocalTee(wabt::LocalTeeExpr* expr);
    void visitGlobalGet(wabt::GlobalGetExpr* expr);
    void visitGlobalSet(wabt::GlobalSetExpr* expr);

    void visitSimdLaneOp(wabt::SimdLaneOpExpr* expr);
    void visitSimdLoadLane(wabt::SimdLoadLaneExpr* expr);
    void visitSimdStoreLane(wabt::SimdStoreLaneExpr* expr);

    llvm::Value* createLoadLane(llvm::Value* vector, llvm::Value* addr, llvm::Type* ty, uint64_t imm);
    llvm::Value* createStoreLane(llvm::Value* vector, llvm::Value* addr, llvm::Type* ty, uint64_t imm);
    llvm::Value* createExtractLane(llvm::Value* vector, llvm::Type* ty, uint64_t imm , bool sign);
    llvm::Value* createReplaceLane(llvm::Value* vector, llvm::Value* scalar, llvm::Type* ty, uint64_t imm);
    llvm::Value* createAllTrue(llvm::Value* vector, llvm::FixedVectorType* vectorType);
    llvm::Value* createSIMDExtend(llvm::Value* vector, llvm::FixedVectorType* vectorType, llvm::FixedVectorType* destType, unsigned int startIdx, bool sign);
    llvm::Value* createSIMDTrunc(llvm::Value* vector, llvm::FixedVectorType* vectorType, llvm::FixedVectorType* destType, llvm::Value* floatMin,  bool sign);
    llvm::Value* popStack() {
        assert(stack.size() >= 1);
        llvm::Value* p1 = stack.back(); stack.pop_back();
        return p1;
    }
};

llvm::Constant* visitConst(llvm::LLVMContext &llvmContext, const wabt::Const& const_);
const char* labelTypeToString(wabt::LabelType lty);
int64_t unwrapIntConstant(llvm::Constant* c);
llvm::BasicBlock::iterator getFirstNonPHIOrDbgOrLifetime(llvm::BasicBlock* bb);
bool checkBlockLike(wabt::LabelType lty);
bool isContainBlock(wabt::Expr& expr);
wabt::Block& getBlock(wabt::Expr& expr);

}

#endif