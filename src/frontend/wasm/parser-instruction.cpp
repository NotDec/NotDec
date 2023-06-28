#include "frontend/wasm/parser-block.h"
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

namespace notdec::frontend::wasm {

 // 处理非控制流指令的分发函数
void BlockContext::dispatchExprs(wabt::Expr& expr) {
    using namespace wabt;
    // ref: wabt\src\wat-writer.cc
    // 看每个expr type有哪些指令：wabt\src\lexer-keywords.txt
    switch (expr.type()) {
    case ExprType::Unary:
        visitUnaryInst(cast<UnaryExpr>(&expr));
        break;
    case ExprType::Binary:
        visitBinaryInst(cast<BinaryExpr>(&expr));
        break;
    case ExprType::Compare:
        visitCompareExpr(cast<CompareExpr>(&expr));
        break;
    case ExprType::Convert:
        visitConvertExpr(cast<ConvertExpr>(&expr));
        break;
    case ExprType::Const:
        visitConstInst(cast<ConstExpr>(&expr));
        break;
    case ExprType::Call:
        visitCallInst(cast<CallExpr>(&expr));
        break;
    case ExprType::CallIndirect:
        visitCallIndirectInst(cast<CallIndirectExpr>(&expr));
        break;
    case ExprType::Load:
        visitLoadInst(cast<LoadExpr>(&expr));
        break;
    case ExprType::Store:
        visitStoreInst(cast<StoreExpr>(&expr));
        break;
    case ExprType::LocalGet:
        visitLocalGet(cast<LocalGetExpr>(&expr));
        break;
    case ExprType::LocalSet:
        visitLocalSet(cast<LocalSetExpr>(&expr));
        break;
    case ExprType::LocalTee:
        visitLocalTee(cast<LocalTeeExpr>(&expr));
        break;
    case ExprType::GlobalGet:
        visitGlobalGet(cast<GlobalGetExpr>(&expr));
        break;
    case ExprType::GlobalSet:
        visitGlobalSet(cast<GlobalSetExpr>(&expr));
        break;
    case ExprType::Unreachable:
        irBuilder.CreateUnreachable();
        irBuilder.ClearInsertionPoint();
        break;
    case ExprType::Nop:
        break;
    case ExprType::Select:
        visitSelectExpr(cast<SelectExpr>(&expr));
        break;
    // TODO: support 2.0
    case ExprType::MemoryGrow:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Warning: dummy implementation for " << GetExprTypeName(expr) << std::endl;
        stack.pop_back();
        stack.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext), 0));
        break;
    case ExprType::MemorySize:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Warning: dummy implementation for " << GetExprTypeName(expr) << std::endl;
        stack.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext), 0));
        break;
    case ExprType::MemoryCopy:
        {
            llvm::Value* num = popStack();
            llvm::Value* src = convertStackAddr(0);
            llvm::Value* dest = convertStackAddr(0);
            irBuilder.CreateMemCpy(dest, llvm::MaybeAlign(0), src, llvm::MaybeAlign(0), num);
        }
        break;
    case ExprType::MemoryFill:
        {
            llvm::Value* num = popStack();
            llvm::Value* byte = popStack();
            byte = irBuilder.CreateTrunc(byte, llvm::Type::getInt8Ty(llvmContext));
            llvm::Value* dest = convertStackAddr(0);
            irBuilder.CreateMemSet(dest, byte, num, llvm::MaybeAlign(0));
        }
        break;
    case ExprType::Drop:
        assert(stack.size() > 0); stack.pop_back();
        break;

    case ExprType::SimdLaneOp:
        visitSimdLaneOp(cast<SimdLaneOpExpr>(&expr));
        break;
    case ExprType::SimdLoadLane:
        visitSimdLoadLane(cast<SimdLoadLaneExpr>(&expr));
        break;
    case ExprType::SimdStoreLane:
        visitSimdStoreLane(cast<SimdStoreLaneExpr>(&expr));
        break;
    case ExprType::SimdShuffleOp:
        {   //https://discourse.llvm.org/t/createshufflevector/67947/2
            SimdShuffleOpExpr *shuffle = cast<SimdShuffleOpExpr>(&expr);
            if(shuffle->opcode == Opcode::I8X16Shuffle){
                v128 imm = shuffle->val;
                llvm::Value *b = irBuilder.CreateBitCast(popStack(),type.i8x16Type);
                llvm::Value *a = irBuilder.CreateBitCast(popStack(),type.i8x16Type);
                int mask[16] = {0};
                for(unsigned int i = 0 ; i < 16; i++)
                    mask[i] = imm.u8(i);
                stack.push_back(irBuilder.CreateShuffleVector(a,b,llvm::ArrayRef<int>(mask,16)));
            }
            else
                std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported Opcode: " << shuffle->opcode.GetName() << std::endl;
        }
        break;

    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported expr type: " << GetExprTypeName(expr) << std::endl;
        // really abort?
        std::abort();
    }
}

void BlockContext::visitLocalSet(wabt::LocalSetExpr* expr) {
    using namespace llvm;
    Value* val = popStack();
    Value* target = locals.at(expr->var.index());
    irBuilder.CreateStore(val, target);
}

void BlockContext::visitLocalTee(wabt::LocalTeeExpr* expr) {
    using namespace llvm;
    assert(stack.size() > 0);
    Value* val = stack.back(); /* stack.pop_back(); */
    Value* target = locals.at(expr->var.index());
    irBuilder.CreateStore(val, target);
}

void BlockContext::visitGlobalSet(wabt::GlobalSetExpr* expr) {
    using namespace llvm;
    Value* val = popStack();
    Value* target = ctx.globs.at(ctx.module->GetGlobalIndex(expr->var));
    irBuilder.CreateStore(val, target);
}

void BlockContext::visitLocalGet(wabt::LocalGetExpr* expr) {
    using namespace llvm;
    // assert(expr->var.is_index()); // 冗余
    Value* target = locals.at(expr->var.index());
    Value* loaded = irBuilder.CreateLoad(target->getType()->getPointerElementType(), target);
    stack.push_back(loaded);
}

void BlockContext::visitGlobalGet(wabt::GlobalGetExpr* expr) {
    using namespace llvm;
    Value* target = ctx.globs.at(ctx.module->GetGlobalIndex(expr->var));
    Value* loaded = irBuilder.CreateLoad(target->getType()->getPointerElementType(), target);
    stack.push_back(loaded);
}

void BlockContext::visitStoreInst(wabt::StoreExpr* expr) {
    using namespace llvm;
    Value* val = popStack();
    Value* addr = convertStackAddr(expr->offset);
    // 看是否要cast
    Type* targetType = nullptr;
    switch (expr->opcode)
    {
    case wabt::Opcode::I32Store:
    case wabt::Opcode::I64Store:
    case wabt::Opcode::F32Store:
    case wabt::Opcode::F64Store:
        targetType = convertType(llvmContext, expr->opcode.GetParamType2());
        break;
    case wabt::Opcode::I32Store8:
    case wabt::Opcode::I64Store8:
        targetType = Type::getInt8Ty(llvmContext);
        val = irBuilder.CreateTrunc(val, targetType);
        break;
    case wabt::Opcode::I32Store16:
    case wabt::Opcode::I64Store16:
        targetType = Type::getInt16Ty(llvmContext);
        val = irBuilder.CreateTrunc(val, targetType);
        break;
    case wabt::Opcode::I64Store32:
        targetType = Type::getInt32Ty(llvmContext);
        val = irBuilder.CreateTrunc(val, targetType);
        break;
    case wabt::Opcode::V128Store:
        targetType = type.i64x2Type;
    default:
        break;
    }
    assert(targetType != nullptr);
    addr = irBuilder.CreateBitCast(addr, PointerType::getUnqual(targetType));
    irBuilder.CreateStore(val, addr);
}

void BlockContext::visitSelectExpr(wabt::SelectExpr* expr) {
    using namespace llvm;
    Value* cond = popStack();
    // convert to i1
    cond = irBuilder.CreateICmpNE(cond, ConstantInt::getNullValue(cond->getType()), "select_cond");
    Value* val2 = popStack();
    Value* val1 = popStack();
    Value* res = irBuilder.CreateSelect(cond, val1, val2);
    stack.push_back(res);
}

// 从栈上读一个整数地址，加上offset，取默认的mem，然后返回指针。
llvm::Value* BlockContext::convertStackAddr(uint64_t offset) {
    using namespace llvm;
    GlobalVariable* mem = this->ctx.mems.at(0);
    Value* base = popStack();

    // 会被IRBuilder处理，所以不用搞自己的特判优化。
    // if (expr->offset != 0) {
    base = irBuilder.CreateAdd(base, ConstantInt::get(base->getType(), offset, false), "calcOffset");
    // }
    // if ea+N/8 is larger than the length of mem, then trap?
    Value* arr[2] = { ConstantInt::getNullValue(base->getType()) , base};
    return irBuilder.CreateGEP(mem->getValueType(), mem, makeArrayRef(arr, 2)); // Type::getInt8PtrTy(llvmContext)
}

// 1. addr = mem + stack op
// 2. addr += offset
// 3. bit cast to expected ptr type
// 4. load
void BlockContext::visitLoadInst(wabt::LoadExpr* expr) {
    using namespace llvm;
    Value* addr = convertStackAddr(expr->offset);

    Type* targetType = nullptr;
    switch (expr->opcode)
    {
    case wabt::Opcode::I32Load:
    case wabt::Opcode::I64Load:
    case wabt::Opcode::F32Load:
    case wabt::Opcode::F64Load:
        targetType = convertType(llvmContext, expr->opcode.GetResultType());
        break;
    case wabt::Opcode::I32Load8S:
    case wabt::Opcode::I32Load8U:
    case wabt::Opcode::I64Load8S:
    case wabt::Opcode::I64Load8U:
    case wabt::Opcode::V128Load8Splat:
        targetType = Type::getInt8Ty(llvmContext);
        break;
    case wabt::Opcode::I32Load16S:
    case wabt::Opcode::I32Load16U:
    case wabt::Opcode::I64Load16S:
    case wabt::Opcode::I64Load16U:
    case wabt::Opcode::V128Load16Splat:
        targetType = Type::getInt16Ty(llvmContext);
        break;
    case wabt::Opcode::I64Load32S:
    case wabt::Opcode::I64Load32U:
    case wabt::Opcode::V128Load32Splat:
    case wabt::Opcode::V128Load32Zero:
        targetType = Type::getInt32Ty(llvmContext);
        break;
    case wabt::Opcode::V128Load64Splat:
    case wabt::Opcode::V128Load64Zero:
        targetType = Type::getInt64Ty(llvmContext);
        break;
    case wabt::Opcode::V128Load:
        targetType = type.i64x2Type;
        break;
    case wabt::Opcode::V128Load8X8S:
    case wabt::Opcode::V128Load8X8U:    
        targetType = type.i8x8Type;
        break;
    case wabt::Opcode::V128Load16X4S:
    case wabt::Opcode::V128Load16X4U:
        targetType = type.i16x4Type;
        break;
    case wabt::Opcode::V128Load32X2S:
    case wabt::Opcode::V128Load32X2U:
        targetType = type.i32x2Type;
        break;

    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported Opcode: " << expr->opcode.GetName() << std::endl;
        break;
    }
    assert(targetType != nullptr);

    addr = irBuilder.CreateBitCast(addr, PointerType::getUnqual(targetType));
    Value* result = irBuilder.CreateLoad(targetType, addr, "loadResult");
    // possible extension
    switch (expr->opcode)
    {
    case wabt::Opcode::I32Load8S:
    case wabt::Opcode::I64Load8S:
    case wabt::Opcode::I32Load16S:
    case wabt::Opcode::I64Load16S:
    case wabt::Opcode::I64Load32S:
        result = irBuilder.CreateSExt(result, convertType(llvmContext, expr->opcode.GetResultType()));
        break;
    case wabt::Opcode::I32Load16U:
    case wabt::Opcode::I64Load16U:
    case wabt::Opcode::I32Load8U:
    case wabt::Opcode::I64Load8U:
    case wabt::Opcode::I64Load32U:
        result = irBuilder.CreateZExt(result, convertType(llvmContext, expr->opcode.GetResultType()));
        break;
    case wabt::Opcode::V128Load8Splat:
        result = irBuilder.CreateVectorSplat(16, result);
        break;
    case wabt::Opcode::V128Load16Splat:
        result = irBuilder.CreateVectorSplat(8, result);
        break;
    case wabt::Opcode::V128Load32Splat:
        result = irBuilder.CreateVectorSplat(4, result);
        break;
    case wabt::Opcode::V128Load64Splat:
        result = irBuilder.CreateVectorSplat(2, result);
        break;
    case wabt::Opcode::V128Load32Zero:
        result = irBuilder.CreateInsertElement(ConstantVector::getNullValue(type.i32x4Type), result, (uint64_t)(0));
        break;
    case wabt::Opcode::V128Load64Zero:
        result = irBuilder.CreateInsertElement(ConstantVector::getNullValue(type.i64x2Type), result, (uint64_t)(0));
        break;
    case wabt::Opcode::V128Load8X8S:
        result = irBuilder.CreateSExt(result, type.i16x8Type);
        break;
    case wabt::Opcode::V128Load8X8U:
        result = irBuilder.CreateZExt(result, type.i16x8Type);
        break;
    case wabt::Opcode::V128Load16X4S:
        result = irBuilder.CreateSExt(result, type.i32x4Type);
        break;
    case wabt::Opcode::V128Load16X4U:
        result = irBuilder.CreateZExt(result, type.i32x4Type);
        break;
    case wabt::Opcode::V128Load32X2S:
        result = irBuilder.CreateSExt(result, type.i64x2Type);
        break;
    case wabt::Opcode::V128Load32X2U:
        result = irBuilder.CreateZExt(result, type.i64x2Type);
        break;
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported Opcode: " << expr->opcode.GetName() << std::endl;
        break;
    }
    if(result != nullptr)
        stack.push_back(result);
}

void BlockContext::visitConvertExpr(wabt::ConvertExpr* expr) {
    using namespace llvm;
    Value *ret = nullptr, *p1;
    p1 = popStack();
    switch (expr->opcode)
    {
    case wabt::Opcode::I32Eqz:
    case wabt::Opcode::I64Eqz:
        ret = irBuilder.CreateICmpEQ(p1, ConstantInt::get(p1->getType(), 0));
        ret = irBuilder.CreateZExt(ret, Type::getInt32Ty(llvmContext));
        break;
    case wabt::Opcode::I64ExtendI32U:
        ret = irBuilder.CreateZExt(p1,Type::getInt64Ty(llvmContext));
        break;
    case wabt::Opcode::I64ExtendI32S:
        ret = irBuilder.CreateSExt(p1,Type::getInt64Ty(llvmContext));
        break;
    case wabt::Opcode::I32WrapI64:
        ret = irBuilder.CreateTrunc(p1,Type::getInt32Ty(llvmContext));
        break;
    case wabt::Opcode::I32TruncF32S:
    case wabt::Opcode::I32TruncF64S:
        ret = irBuilder.CreateFPToSI(p1,Type::getInt32Ty(llvmContext));
        break;
    case wabt::Opcode::I32TruncF32U:
    case wabt::Opcode::I32TruncF64U:
        ret = irBuilder.CreateFPToUI(p1,Type::getInt32Ty(llvmContext));
        break;
    case wabt::Opcode::I64TruncF32S:
    case wabt::Opcode::I64TruncF64S:
        ret = irBuilder.CreateFPToSI(p1,Type::getInt64Ty(llvmContext));
        break;
    case wabt::Opcode::I64TruncF32U:
    case wabt::Opcode::I64TruncF64U:
        ret = irBuilder.CreateFPToUI(p1,Type::getInt64Ty(llvmContext));
        break;

    case wabt::Opcode::F32ConvertI32S:
    case wabt::Opcode::F32ConvertI64S:
        ret = irBuilder.CreateSIToFP(p1,Type::getFloatTy(llvmContext));
        break;
    case wabt::Opcode::F32ConvertI32U:
    case wabt::Opcode::F32ConvertI64U:
        ret = irBuilder.CreateUIToFP(p1,Type::getFloatTy(llvmContext));
        break;
    case wabt::Opcode::F64ConvertI32S:
    case wabt::Opcode::F64ConvertI64S:
        ret = irBuilder.CreateSIToFP(p1,Type::getDoubleTy(llvmContext));
        break;
    case wabt::Opcode::F64ConvertI32U:
    case wabt::Opcode::F64ConvertI64U:
        ret = irBuilder.CreateUIToFP(p1,Type::getDoubleTy(llvmContext));
        break;
    
    case wabt::Opcode::F32DemoteF64:
        ret = irBuilder.CreateFPTrunc(p1,Type::getFloatTy(llvmContext));
        break;
    case wabt::Opcode::F64PromoteF32:
        ret = irBuilder.CreateFPExt(p1,Type::getDoubleTy(llvmContext));
        break;

    case wabt::Opcode::I32ReinterpretF32:
        ret = irBuilder.CreateBitCast(p1,Type::getInt32Ty(llvmContext));
        break;
    case wabt::Opcode::I64ReinterpretF64:
        ret = irBuilder.CreateBitCast(p1,Type::getInt64Ty(llvmContext));
        break;
    case wabt::Opcode::F32ReinterpretI32:
        ret = irBuilder.CreateBitCast(p1,Type::getFloatTy(llvmContext));
        break;
    case wabt::Opcode::F64ReinterpretI64:
        ret = irBuilder.CreateBitCast(p1,Type::getDoubleTy(llvmContext));
        break;

    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported Opcode: " << expr->opcode.GetName() << std::endl;
        break;
    }
    if (ret != nullptr) {
        stack.push_back(ret);
    }
}

#define EMIT_SIMD_UNARY_OP(llvmType, emitCode)                  \
{                                                               \
	llvm::Value* v = irBuilder.CreateBitCast(p1, llvmType);     \
	ret = emitCode;                                             \
}         

void BlockContext::visitUnaryInst(wabt::UnaryExpr* expr) {
    using namespace llvm;
    Value *ret = nullptr, *p1;
    Function* f;
    p1 = popStack();
    switch (expr->opcode)
    {
    case wabt::Opcode::I8X16Neg:
        EMIT_SIMD_UNARY_OP(type.i8x16Type, irBuilder.CreateNeg(v));
        break;
    case wabt::Opcode::I16X8Neg:
        EMIT_SIMD_UNARY_OP(type.i16x8Type, irBuilder.CreateNeg(v));
        break;
    case wabt::Opcode::I32X4Neg:
        EMIT_SIMD_UNARY_OP(type.i32x4Type, irBuilder.CreateNeg(v));
        break;
    case wabt::Opcode::I64X2Neg:
        EMIT_SIMD_UNARY_OP(type.i64x2Type, irBuilder.CreateNeg(v));
        break;
    case wabt::Opcode::F32Neg:
    case wabt::Opcode::F64Neg:
        ret = irBuilder.CreateFNeg(p1);
        break;
    case wabt::Opcode::F32X4Neg:
        EMIT_SIMD_UNARY_OP(type.f32x4Type, irBuilder.CreateFNeg(v));
        break;
    case wabt::Opcode::F64X2Neg:
        EMIT_SIMD_UNARY_OP(type.f64x2Type, irBuilder.CreateFNeg(v));
        break;

    case wabt::Opcode::I8X16Abs:
    {
        Value* v = irBuilder.CreateBitCast(p1, type.i8x16Type);
        ret = irBuilder.CreateSelect(
            irBuilder.CreateICmpSLT(v, Constant::getNullValue(type.i8x16Type)),
            irBuilder.CreateNeg(v),
            v);
    }
        break;
    case wabt::Opcode::I16X8Abs:
    {
        Value* v = irBuilder.CreateBitCast(p1, type.i16x8Type);
        ret = irBuilder.CreateSelect(                                                               
			irBuilder.CreateICmpSLT(v, Constant::getNullValue(type.i16x8Type)),         
			irBuilder.CreateNeg(v),
            v);
    }
        break;
    case wabt::Opcode::I32X4Abs:
    {
        Value* v = irBuilder.CreateBitCast(p1, type.i32x4Type);
        ret = irBuilder.CreateSelect(
            irBuilder.CreateICmpSLT(v, Constant::getNullValue(type.i32x4Type)),
            irBuilder.CreateNeg(v),
            v);
    }
        break;
    case wabt::Opcode::I64X2Abs:
    {
        Value* v = irBuilder.CreateBitCast(p1, type.i64x2Type);
        ret = irBuilder.CreateSelect(
            irBuilder.CreateICmpSLT(v, Constant::getNullValue(type.i64x2Type)),
            irBuilder.CreateNeg(v),
            v);
    }
        break;
    case wabt::Opcode::F32Abs:
    case wabt::Opcode::F64Abs:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::fabs, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::F32X4Abs:
        EMIT_SIMD_UNARY_OP(type.f32x4Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::fabs, type.f32x4Type), v));
        break;
    case wabt::Opcode::F64X2Abs:
        EMIT_SIMD_UNARY_OP(type.f64x2Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::fabs, type.f64x2Type), v));
        break;
    case wabt::Opcode::I32Popcnt:
    case wabt::Opcode::I64Popcnt:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::ctpop, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::I8X16Popcnt:
        EMIT_SIMD_UNARY_OP(type.i8x16Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::ctpop, type.i8x16Type), v));
        break;       
    case wabt::Opcode::I32Clz:
    case wabt::Opcode::I64Clz:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::ctlz, {p1->getType(),Type::getInt1Ty(llvmContext)});
        // is_zero_poison = false
        ret = irBuilder.CreateCall(f, {p1, ConstantInt::get(Type::getInt1Ty(llvmContext),false)});
        break;
    case wabt::Opcode::I32Ctz:
    case wabt::Opcode::I64Ctz:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::cttz, {p1->getType(),Type::getInt1Ty(llvmContext)});
        // is_zero_poison = false
        ret = irBuilder.CreateCall(f, {p1, ConstantInt::get(Type::getInt1Ty(llvmContext),false)});
        break;
    
    case wabt::Opcode::F32Ceil:
    case wabt::Opcode::F64Ceil:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::ceil, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::F32X4Ceil:
        EMIT_SIMD_UNARY_OP(type.f32x4Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::ceil, type.f32x4Type), v));
        break;
    case wabt::Opcode::F64X2Ceil:
        EMIT_SIMD_UNARY_OP(type.f64x2Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::ceil, type.f64x2Type), v));
        break;
    
    case wabt::Opcode::F32Floor:
    case wabt::Opcode::F64Floor:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::floor, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::F32X4Floor:
        EMIT_SIMD_UNARY_OP(type.f32x4Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::floor, type.f32x4Type), v));
        break;
    case wabt::Opcode::F64X2Floor:
        EMIT_SIMD_UNARY_OP(type.f64x2Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::floor, type.f64x2Type), v));
        break;
    

    // https://www.w3.org/TR/wasm-core-1/#-hrefop-fnearestmathrmfnearest_n-z
    // https://llvm.org/docs/LangRef.html#llvm-round-intrinsic
    // https://developer.mozilla.org/en-US/docs/WebAssembly/Reference/Numeric/Nearest
    case wabt::Opcode::F32Nearest:
    case wabt::Opcode::F64Nearest:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::roundeven, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::F32X4Nearest:
        EMIT_SIMD_UNARY_OP(type.f32x4Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::roundeven,type.f32x4Type), v));
        break;
    case wabt::Opcode::F64X2Nearest:
        EMIT_SIMD_UNARY_OP(type.f64x2Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::roundeven,type.f64x2Type), v));
        break;    

    case wabt::Opcode::F32Trunc:
    case wabt::Opcode::F64Trunc:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::trunc, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::F32X4Trunc:
        EMIT_SIMD_UNARY_OP(type.f32x4Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::trunc,type.f32x4Type), v));
        break;
    case wabt::Opcode::F64X2Trunc:
        EMIT_SIMD_UNARY_OP(type.f64x2Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::trunc,type.f64x2Type), v));
        break;
    
    case wabt::Opcode::F32Sqrt:
    case wabt::Opcode::F64Sqrt:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::sqrt, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::F32X4Sqrt:
        EMIT_SIMD_UNARY_OP(type.f32x4Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::sqrt,type.f32x4Type), v));
        break;
    case wabt::Opcode::F64X2Sqrt:
        EMIT_SIMD_UNARY_OP(type.f64x2Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::sqrt,type.f64x2Type), v));
        break;    

    case wabt::Opcode::I8X16AllTrue:
        EMIT_SIMD_UNARY_OP(type.i8x16Type, createAllTrue(v, type.i8x16Type));
        break;
    case wabt::Opcode::I16X8AllTrue:
        EMIT_SIMD_UNARY_OP(type.i16x8Type, createAllTrue(v, type.i16x8Type));
        break;
    case wabt::Opcode::I32X4AllTrue:
        EMIT_SIMD_UNARY_OP(type.i32x4Type, createAllTrue(v, type.i32x4Type));
        break;
    case wabt::Opcode::I64X2AllTrue:
        EMIT_SIMD_UNARY_OP(type.i64x2Type, createAllTrue(v, type.i64x2Type));
        break;

    case wabt::Opcode::I8X16Bitmask:    
    {
    	Value* i8x16Operand = irBuilder.CreateBitCast(p1, type.i8x16Type);
	    Value* i1x16Mask = irBuilder.CreateICmpSLT(i8x16Operand, ConstantVector::getNullValue(type.i8x16Type));
		ret = irBuilder.CreateZExt(irBuilder.CreateBitCast(i1x16Mask, type.i16Type), type.i32Type);
    }
        break;  
    case wabt::Opcode::I16X8Bitmask: 
    {	
        Value* i8x16Operand = irBuilder.CreateBitCast(p1, type.i16x8Type);
	    Value* i1x8Mask = irBuilder.CreateICmpSLT(i8x16Operand, ConstantVector::getNullValue(type.i16x8Type));
	    ret = irBuilder.CreateZExt(irBuilder.CreateBitCast(i1x8Mask, type.i8Type), type.i32Type);
    }
        break;
    case wabt::Opcode::I32X4Bitmask: 
    {	
        Value* i32x4Operand = irBuilder.CreateBitCast(p1, type.i32x4Type);
	    Value* i1x4Mask = irBuilder.CreateICmpSLT(i32x4Operand, ConstantVector::getNullValue(type.i32x4Type));
	    ret = irBuilder.CreateZExt(irBuilder.CreateBitCast(i1x4Mask, IntegerType::get(llvmContext, 4)), type.i32Type);
    }
        break;
    case wabt::Opcode::I64X2Bitmask: 
    {	
        Value* i64x2Operand = irBuilder.CreateBitCast(p1, type.i64x2Type);
	    Value* i1x2Mask = irBuilder.CreateICmpSLT(i64x2Operand, ConstantVector::getNullValue(type.i64x2Type));
	    ret = irBuilder.CreateZExt(irBuilder.CreateBitCast(i1x2Mask, IntegerType::get(llvmContext, 2)), type.i32Type);
    }
        break;
    case wabt::Opcode::V128Not:
        ret = irBuilder.CreateNot(p1);
        break;
    case wabt::Opcode::V128AnyTrue:{
	    ConstantInt* zero = ConstantInt::get(Type::getInt64Ty(irBuilder.getContext()), 0);
	    Value* boolResult = irBuilder.CreateOr(
	    	irBuilder.CreateICmpNE(irBuilder.CreateExtractElement(p1, uint64_t(0)), zero),
	    	irBuilder.CreateICmpNE(irBuilder.CreateExtractElement(p1, uint64_t(1)), zero));
	    ret = irBuilder.CreateZExt(boolResult, Type::getInt32Ty(irBuilder.getContext()));
        break;
    }
    // SIMD Splat
    case wabt::Opcode::I8X16Splat:
        ret = irBuilder.CreateVectorSplat(16,irBuilder.CreateTrunc(p1, Type::getInt8Ty(llvmContext)));
        break;
    case wabt::Opcode::I16X8Splat:
        ret = irBuilder.CreateVectorSplat(8,irBuilder.CreateTrunc(p1, Type::getInt16Ty(llvmContext)));
        break;
    case wabt::Opcode::I32X4Splat:
        ret = irBuilder.CreateVectorSplat(4, p1);
        break;    
    case wabt::Opcode::I64X2Splat:
        ret = irBuilder.CreateVectorSplat(2, p1);
        break;
    case wabt::Opcode::F32X4Splat:
        ret = irBuilder.CreateVectorSplat(4, p1);
        break;
    case wabt::Opcode::F64X2Splat:
        ret = irBuilder.CreateVectorSplat(2, p1);
        break;
    case wabt::Opcode::F32X4DemoteF64X2Zero:
    {   
        Value* vec = irBuilder.CreateBitCast(p1, type.f64x2Type);
        vec = irBuilder.CreateFPTrunc(vec, type.f32x2Type);
        //two higher lanes of the result are initialized to zero
        ret = Constant::getNullValue(type.f32x4Type);
        ret = irBuilder.CreateInsertElement(ret, irBuilder.CreateExtractElement(vec, uint64_t(0)), uint64_t(0));
        ret = irBuilder.CreateInsertElement(ret, irBuilder.CreateExtractElement(vec, uint64_t(1)), uint64_t(1));
    }
        break;
    case wabt::Opcode::F64X2PromoteLowF32X4:
    {   
        Value* vec = irBuilder.CreateBitCast(p1, type.f32x4Type);
        vec = irBuilder.CreateShuffleVector(vec,
										UndefValue::get(type.f32x4Type),
										(ArrayRef<int>){0, 1});
        ret = irBuilder.CreateFPExt(vec, type.f64x2Type);
    }
        break;
    // SIMD Extadd
    case wabt::Opcode::I16X8ExtaddPairwiseI8X16S:
    {
        Value* vec = irBuilder.CreateBitCast(p1, type.i8x16Type);
        Value* undef = UndefValue::get(type.i8x16Type);
        Value* vec1 = irBuilder.CreateShuffleVector(vec,
                                        undef,
                                        (ArrayRef<int>){0, 2, 4, 6, 8, 10, 12, 14});
        Value* vec2 = irBuilder.CreateShuffleVector(vec,
                                        undef,
                                        (ArrayRef<int>){1, 3, 5, 7, 9, 11, 13, 15});
        vec1 = irBuilder.CreateSExt(vec1, type.i16x8Type);
        vec2 = irBuilder.CreateSExt(vec2, type.i16x8Type);
        ret = irBuilder.CreateAdd(vec1, vec2);                                
    }
        break;
    case wabt::Opcode::I16X8ExtaddPairwiseI8X16U:
    {
        Value* vec = irBuilder.CreateBitCast(p1, type.i8x16Type);
        Value* undef = UndefValue::get(type.i8x16Type);
        Value* vec1 = irBuilder.CreateShuffleVector(vec,
                                        undef,
                                        (ArrayRef<int>){0, 2, 4, 6, 8, 10, 12, 14});
        Value* vec2 = irBuilder.CreateShuffleVector(vec,
                                        undef,
                                        (ArrayRef<int>){1, 3, 5, 7, 9, 11, 13, 15});
        vec1 = irBuilder.CreateZExt(vec1, type.i16x8Type);
        vec2 = irBuilder.CreateZExt(vec2, type.i16x8Type);
        ret = irBuilder.CreateAdd(vec1, vec2);                                
    }
    case wabt::Opcode::I32X4ExtaddPairwiseI16X8S:
    {
        Value* vec = irBuilder.CreateBitCast(p1, type.i16x8Type);
        Value* undef = UndefValue::get(type.i16x8Type);
        Value* vec1 = irBuilder.CreateShuffleVector(vec,
                                        undef,
                                        (ArrayRef<int>){0, 2, 4, 6});
        Value* vec2 = irBuilder.CreateShuffleVector(vec,
                                        undef,
                                        (ArrayRef<int>){1, 3, 5, 7});
        vec1 = irBuilder.CreateSExt(vec1, type.i32x4Type);
        vec2 = irBuilder.CreateSExt(vec2, type.i32x4Type);
        ret = irBuilder.CreateAdd(vec1, vec2);                                
    }
        break;
    case wabt::Opcode::I32X4ExtaddPairwiseI16X8U:
    {
        Value* vec = irBuilder.CreateBitCast(p1, type.i16x8Type);
        Value* undef = UndefValue::get(type.i16x8Type);
        Value* vec1 = irBuilder.CreateShuffleVector(vec,
                                        undef,
                                        (ArrayRef<int>){0, 2, 4, 6});
        Value* vec2 = irBuilder.CreateShuffleVector(vec,
                                        undef,
                                        (ArrayRef<int>){1, 3, 5, 7});
        vec1 = irBuilder.CreateZExt(vec1, type.i32x4Type);
        vec2 = irBuilder.CreateZExt(vec2, type.i32x4Type);
        ret = irBuilder.CreateAdd(vec1, vec2);                                
    }
        break;
    // SIMD Extend
    case wabt::Opcode::I16X8ExtendLowI8X16S:
        ret = createSIMDExtend(p1, type.i8x16Type, type.i16x8Type, 0, true);
        break;
    case wabt::Opcode::I16X8ExtendHighI8X16S:
        ret = createSIMDExtend(p1, type.i8x16Type, type.i16x8Type, 8, true);
        break;
    case wabt::Opcode::I16X8ExtendLowI8X16U:
        ret = createSIMDExtend(p1, type.i8x16Type, type.i16x8Type, 0, false);
        break;
    case wabt::Opcode::I16X8ExtendHighI8X16U:
        ret = createSIMDExtend(p1, type.i8x16Type, type.i16x8Type, 8, false);
        break;    
    case wabt::Opcode::I32X4ExtendLowI16X8S:
        ret = createSIMDExtend(p1, type.i16x8Type, type.i32x4Type, 0, true);
        break;
    case wabt::Opcode::I32X4ExtendHighI16X8S:
        ret = createSIMDExtend(p1, type.i16x8Type, type.i32x4Type, 4, true);
        break;
    case wabt::Opcode::I32X4ExtendLowI16X8U:
        ret = createSIMDExtend(p1, type.i16x8Type, type.i32x4Type, 0, false);
        break;
    case wabt::Opcode::I32X4ExtendHighI16X8U:
        ret = createSIMDExtend(p1, type.i16x8Type, type.i32x4Type, 4, false);
        break;
    case wabt::Opcode::I64X2ExtendLowI32X4S:
        ret = createSIMDExtend(p1, type.i32x4Type, type.i64x2Type, 0, true);
        break;
    case wabt::Opcode::I64X2ExtendHighI32X4S:
        ret = createSIMDExtend(p1, type.i32x4Type, type.i64x2Type, 2, true);
        break;
    case wabt::Opcode::I64X2ExtendLowI32X4U:
        ret = createSIMDExtend(p1, type.i32x4Type, type.i64x2Type, 0, false);
        break;
    case wabt::Opcode::I64X2ExtendHighI32X4U:
        ret = createSIMDExtend(p1, type.i32x4Type, type.i64x2Type, 2, false);
        break;
    // SIMD Convert
    case wabt::Opcode::F32X4ConvertI32X4S:
        EMIT_SIMD_UNARY_OP(type.f32x4Type,irBuilder.CreateSIToFP(v, type.f32x4Type));
    case wabt::Opcode::F32X4ConvertI32X4U:
        EMIT_SIMD_UNARY_OP(type.f32x4Type,irBuilder.CreateUIToFP(v, type.f32x4Type));
    case wabt::Opcode::F64X2ConvertLowI32X4S:
    {
        Value* vec = irBuilder.CreateBitCast(p1, type.i32x4Type);
        ret = Constant::getNullValue(type.i32x2Type);
        for(uint32_t i = 0; i < 2; i++){
            ret = irBuilder.CreateInsertElement(ret, irBuilder.CreateExtractElement(vec, i), i);
        }
        ret = irBuilder.CreateSIToFP(ret, type.f64x2Type);
    }
        break;
    case wabt::Opcode::F64X2ConvertLowI32X4U:
    {
        Value* vec = irBuilder.CreateBitCast(p1, type.i32x4Type);
        ret = Constant::getNullValue(type.i32x2Type);
        for(uint32_t i = 0; i < 2; i++){
            ret = irBuilder.CreateInsertElement(ret, irBuilder.CreateExtractElement(vec, i), i);
        }
        ret = irBuilder.CreateUIToFP(ret, type.f64x2Type);
    }
        break;
    // SIMD Trunc
    // TODO IR指令语义与Wasm指令不一致：上下界问题
    case wabt::Opcode::I32X4TruncSatF32X4S:
        ret = irBuilder.CreateBitCast(p1, type.f32x4Type);
        ret = irBuilder.CreateFPToSI(ret, type.i32x4Type);
        break;
    case wabt::Opcode::I32X4TruncSatF32X4U:
        ret = irBuilder.CreateBitCast(p1, type.f32x4Type);
        ret = irBuilder.CreateFPToUI(ret, type.i32x4Type);
        break;
    case wabt::Opcode::I32X4TruncSatF64X2SZero:
    {
        Value* vec = irBuilder.CreateBitCast(p1, type.f64x2Type);
        vec = irBuilder.CreateFPToSI(ret, type.i32x2Type);
        ret = Constant::getNullValue(type.i32x4Type);
        for(uint32_t i = 0; i < 2; i++){
            ret = irBuilder.CreateInsertElement(ret, irBuilder.CreateExtractElement(vec, i), i);
        }
    }
        break;
    case wabt::Opcode::I32X4TruncSatF64X2UZero:
    {
        Value* vec = irBuilder.CreateBitCast(p1, type.f64x2Type);
        vec = irBuilder.CreateFPToUI(ret, type.i32x2Type);
        ret = Constant::getNullValue(type.i32x4Type);
        for(uint32_t i = 0; i < 2; i++){
            ret = irBuilder.CreateInsertElement(ret, irBuilder.CreateExtractElement(vec, i), i);
        }
    }
        break;
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported expr type: " << expr->opcode.GetName() << std::endl;
        break;
    }
    if (ret != nullptr) {
        stack.push_back(ret);
    }
}

llvm::Value* BlockContext::createSIMDExtend(llvm::Value* vector, llvm::FixedVectorType* vectorType, llvm::FixedVectorType* destType, unsigned int startIdx, bool sign){
    using namespace llvm;
    vector = irBuilder.CreateBitCast(vector, vectorType);
    FixedVectorType* halfVectorType = FixedVectorType::getHalfElementsVectorType(vectorType);
    Value* halfVector = UndefValue::get(halfVectorType);
    int elementCount = halfVectorType->getNumElements();
    int mask[elementCount];
    for(int i = startIdx; i < elementCount; i++)
        mask[i] = i;
    halfVector = irBuilder.CreateShuffleVector(vector,
                                  halfVector,
                                  makeArrayRef<int>(mask, elementCount));
    if(sign)
        return irBuilder.CreateSExt(halfVector, destType);
    else
        return irBuilder.CreateZExt(halfVector, destType);
}

void BlockContext::visitBinaryInst(wabt::BinaryExpr* expr) {
    using namespace llvm;
    Value *ret = nullptr, *p1, *p2;
    Function *f; // for intrinsic
    p1 = popStack();
    p2 = popStack();
    switch (expr->opcode)
    {
    case wabt::Opcode::I32Add:
    case wabt::Opcode::I64Add:
        ret = irBuilder.CreateAdd(p2, p1);
        break;
    case wabt::Opcode::F32Add:
    case wabt::Opcode::F64Add:
        ret = irBuilder.CreateFAdd(p2, p1);
        break;

    case wabt::Opcode::I32Sub:
    case wabt::Opcode::I64Sub:
        ret = irBuilder.CreateSub(p2, p1);
        break;
    case wabt::Opcode::F32Sub:
    case wabt::Opcode::F64Sub:
        ret = irBuilder.CreateFSub(p2, p1);
        break;
    case wabt::Opcode::I32Mul:
    case wabt::Opcode::I64Mul:
        ret = irBuilder.CreateMul(p2, p1);
        break;
    case wabt::Opcode::F32Mul:
    case wabt::Opcode::F64Mul:
        ret = irBuilder.CreateFMul(p2, p1);
        break;

    case wabt::Opcode::I32DivS:
    case wabt::Opcode::I64DivS:
        ret = irBuilder.CreateSDiv(p2, p1);
        break;
    case wabt::Opcode::I32DivU:
    case wabt::Opcode::I64DivU:
        ret = irBuilder.CreateUDiv(p2, p1);
        break;
    case wabt::Opcode::F32Div:
    case wabt::Opcode::F64Div:
        ret = irBuilder.CreateFDiv(p2, p1);
        break;

    case wabt::Opcode::I32RemS:
    case wabt::Opcode::I64RemS:
        ret = irBuilder.CreateSRem(p2, p1);
        break;
    case wabt::Opcode::I32RemU:
    case wabt::Opcode::I64RemU:
        ret = irBuilder.CreateURem(p2, p1);
        break;
    case wabt::Opcode::I32And:
    case wabt::Opcode::I64And:
        ret = irBuilder.CreateAnd(p2, p1);
        break;
    case wabt::Opcode::I32Or:   
    case wabt::Opcode::I64Or:
        ret = irBuilder.CreateOr(p2, p1);
        break;
    case wabt::Opcode::I32Xor:
    case wabt::Opcode::I64Xor:
        ret = irBuilder.CreateXor(p2, p1);
        break;
    case wabt::Opcode::I32Shl:
    case wabt::Opcode::I64Shl:
        ret = irBuilder.CreateShl(p2, p1);
        break;
    case wabt::Opcode::I32ShrS:
    case wabt::Opcode::I64ShrS:
        ret = irBuilder.CreateAShr(p2, p1);
        break;
    case wabt::Opcode::I32ShrU:
    case wabt::Opcode::I64ShrU:
        ret = irBuilder.CreateLShr(p2, p1);
        break;
    
    case wabt::Opcode::I32Rotl:
    case wabt::Opcode::I64Rotl:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::fshl, {p1->getType(),p1->getType(),p1->getType()});
        ret = irBuilder.CreateCall(f, {p2, p2, p1});
        break;
    case wabt::Opcode::I32Rotr:
    case wabt::Opcode::I64Rotr:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::fshr, {p1->getType(),p1->getType(),p1->getType()});
        ret = irBuilder.CreateCall(f, {p2, p2, p1});
        break;

    // floor type only
    case wabt::Opcode::F32Min:
    case wabt::Opcode::F64Min:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::minnum, {p1->getType(),p1->getType()});
        ret = irBuilder.CreateCall(f, {p2, p1});
        break;
    case wabt::Opcode::F32Max:
    case wabt::Opcode::F64Max:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::maxnum, {p1->getType(),p1->getType()});
        ret = irBuilder.CreateCall(f, {p2, p1});
        break;
    case wabt::Opcode::F32Copysign:
    case wabt::Opcode::F64Copysign:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::copysign, {p1->getType(),p1->getType()});
        ret = irBuilder.CreateCall(f, {p2, p1});
        break;

    /* SIMD */
    case wabt::Opcode::V128Andnot:
        ret = irBuilder.CreateAnd(p2, irBuilder.CreateNot(p1));
        break;
    case wabt::Opcode::V128And:
        ret = irBuilder.CreateAnd(p2, p1);
        break;
    case wabt::Opcode::V128Or:
        ret = irBuilder.CreateOr(p2, p1);
        break;
    case wabt::Opcode::V128Xor:
        ret = irBuilder.CreateXor(p2, p1);
        break;
    // case wabt::Opcode::I8X16Swizzle:
    //     //TODO
    //     f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::aarch64_neon_tbl1, {p1->getType(),p1->getType()}); 
    //     ret = irBuilder.CreateCall(f, {p2, p1});
    //     break;

    #define EMIT_SIMD_BINARY_OP(llvmType, emitCode)                 \
    {                                                               \
		Value* right = irBuilder.CreateBitCast(p2, llvmType); \
		Value* left = irBuilder.CreateBitCast(p1, llvmType);  \
		ret = emitCode;                                             \
    }                         
	                                                                                       
    case wabt::Opcode::I8X16Add:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateAdd(left, right));
        break;
    case wabt::Opcode::I8X16AddSatS:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::sadd_sat, {p1->getType(),p1->getType()});
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateCall(f, {left, right}));
        break;
    case wabt::Opcode::I8X16AddSatU:
    {
        Value* right = irBuilder.CreateBitCast(p1, type.i8x16Type);
		Value* left = irBuilder.CreateBitCast(p2, type.i8x16Type);
		Value* add = irBuilder.CreateAdd(left, right);
        ret = irBuilder.CreateSelect(irBuilder.CreateICmpUGT(left, add), Constant::getAllOnesValue(left->getType()), add);
    }
        break;
    case wabt::Opcode::I16X8Add:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateAdd(left, right));
        break;
    case wabt::Opcode::I16X8AddSatS:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::sadd_sat, {p1->getType(),p1->getType()});
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateCall(f, {left, right}));
        break;
    case wabt::Opcode::I16X8AddSatU:
    {
        Value* right = irBuilder.CreateBitCast(p1, type.i16x8Type);
        Value* left = irBuilder.CreateBitCast(p2, type.i16x8Type);
        Value* add = irBuilder.CreateAdd(left, right);
        ret = irBuilder.CreateSelect(irBuilder.CreateICmpUGT(left, add), Constant::getAllOnesValue(left->getType()), add);
    }
        break;
    case wabt::Opcode::I32X4Add:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateAdd(left, right));
        break;
    case wabt::Opcode::I64X2Add:
        EMIT_SIMD_BINARY_OP(type.i64x2Type, irBuilder.CreateAdd(left, right));
        break;
    case wabt::Opcode::F32X4Add:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateFAdd(left, right));
        break;
    case wabt::Opcode::F64X2Add:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateFAdd(left, right));
        break;

    case wabt::Opcode::I8X16Sub:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateSub(left, right));
        break;
    case wabt::Opcode::I8X16SubSatS:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::ssub_sat, {p1->getType(),p1->getType()});
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateCall(f, {left, right}));
        break;
    case wabt::Opcode::I8X16SubSatU:
    {   
        Value* right = irBuilder.CreateBitCast(p1, type.i8x16Type);
        Value* left = irBuilder.CreateBitCast(p2, type.i8x16Type);
        Value* flag = irBuilder.CreateICmp(CmpInst::ICMP_UGT, left, right);
        ret = irBuilder.CreateSub(irBuilder.CreateSelect(flag, left, right), right);
    }
        break;   
    case wabt::Opcode::I16X8Sub:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateSub(left, right));
        break;
    case wabt::Opcode::I16X8SubSatS:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::ssub_sat, {p1->getType(),p1->getType()});
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateCall(f, {left, right}));
        break;
    case wabt::Opcode::I16X8SubSatU:
    {
        Value* right = irBuilder.CreateBitCast(p1, type.i16x8Type);
        Value* left = irBuilder.CreateBitCast(p2, type.i16x8Type);
        Value* flag = irBuilder.CreateICmp(CmpInst::ICMP_UGT, left, right);
        ret = irBuilder.CreateSub(irBuilder.CreateSelect(flag, left, right), right);
    }
        break;
    case wabt::Opcode::I32X4Sub:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateSub(left, right));
        break;
    case wabt::Opcode::I64X2Sub:
        EMIT_SIMD_BINARY_OP(type.i64x2Type, irBuilder.CreateSub(left, right));
        break;
    case wabt::Opcode::F32X4Sub:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateFSub(left, right));
        break;
    case wabt::Opcode::F64X2Sub:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateFSub(left, right));
        break;

    case wabt::Opcode::I16X8Mul:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateMul(left, right));
        break;
    case wabt::Opcode::I32X4Mul:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateMul(left, right));
        break;
    case wabt::Opcode::I64X2Mul:
        EMIT_SIMD_BINARY_OP(type.i64x2Type, irBuilder.CreateMul(left, right));
        break;
    case wabt::Opcode::F32X4Mul:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateFMul(left, right));
        break;
    case wabt::Opcode::F64X2Mul:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateFMul(left, right));
        break;

    case wabt::Opcode::F32X4Div:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateFDiv(left, right));
        break;
    case wabt::Opcode::F64X2Div:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateFDiv(left, right));
        break;

    #define EMIT_SIMD_SHIFT_OP(llvmType, emitCode)                                                              \
    {                                                                                                           \
    	FixedVectorType* vectorType = llvmType;                                                                 \
    	Type* scalarType = llvmType->getScalarType();                                                     \
        unsigned int numBits = scalarType->getIntegerBitWidth();                                                \
        llvm::Value* mask = irBuilder.CreateAnd(p1, llvm::ConstantInt::get(p1->getType(), numBits - 1));        \
    	llvm::Value* right = irBuilder.CreateVectorSplat(                                                       \
    		(unsigned int)vectorType->getNumElements(),                                                         \
    		irBuilder.CreateZExtOrTrunc(mask, scalarType));                                                     \
    	Value* left = irBuilder.CreateBitCast(p2, llvmType);                                              \
    	ret = emitCode;                                                                                         \
    }
    case wabt::Opcode::I8X16Shl:
        EMIT_SIMD_SHIFT_OP(type.i8x16Type,irBuilder.CreateShl(left, right));
        break;
    case wabt::Opcode::I16X8Shl:
        EMIT_SIMD_SHIFT_OP(type.i16x8Type,irBuilder.CreateShl(left, right));
        break;       
    case wabt::Opcode::I32X4Shl:
        EMIT_SIMD_SHIFT_OP(type.i32x4Type,irBuilder.CreateShl(left, right));
        break;           
    case wabt::Opcode::I64X2Shl:
        EMIT_SIMD_SHIFT_OP(type.i64x2Type,irBuilder.CreateShl(left, right));
        break;         
    
    case wabt::Opcode::I8X16ShrS:
        EMIT_SIMD_SHIFT_OP(type.i8x16Type,irBuilder.CreateAShr(left, right));
        break;
    case wabt::Opcode::I16X8ShrS:
        EMIT_SIMD_SHIFT_OP(type.i16x8Type,irBuilder.CreateAShr(left, right));
        break;       
    case wabt::Opcode::I32X4ShrS:
        EMIT_SIMD_SHIFT_OP(type.i32x4Type,irBuilder.CreateAShr(left, right));
        break;           
    case wabt::Opcode::I64X2ShrS:
        EMIT_SIMD_SHIFT_OP(type.i64x2Type,irBuilder.CreateAShr(left, right));
        break;        
    
    case wabt::Opcode::I8X16ShrU:
        EMIT_SIMD_SHIFT_OP(type.i8x16Type,irBuilder.CreateLShr(left, right));
        break;
    case wabt::Opcode::I16X8ShrU:
        EMIT_SIMD_SHIFT_OP(type.i16x8Type,irBuilder.CreateLShr(left, right));
        break;       
    case wabt::Opcode::I32X4ShrU:
        EMIT_SIMD_SHIFT_OP(type.i32x4Type,irBuilder.CreateLShr(left, right));
        break;           
    case wabt::Opcode::I64X2ShrU:
        EMIT_SIMD_SHIFT_OP(type.i64x2Type,irBuilder.CreateLShr(left, right));
        break; 

    case wabt::Opcode::I8X16AvgrU:
    {
		Value* right = irBuilder.CreateBitCast(p2, type.i8x16Type);                       
		Value* left = irBuilder.CreateBitCast(p1, type.i8x16Type);                        
		Value* rightZExt = irBuilder.CreateZExt(right, type.i16x16Type); 
		Value* leftZExt = irBuilder.CreateZExt(left, type.i16x16Type);   
		Value* oneZExt = ConstantVector::getSplat(                                   
			type.i8x16Type->getElementCount(),                          
			ConstantInt::get(type.i16x16Type->getElementType(), 1));
		ret = irBuilder.CreateTrunc(
			irBuilder.CreateLShr(
				irBuilder.CreateAdd(irBuilder.CreateAdd(leftZExt, rightZExt), oneZExt), oneZExt),
			type.i8x16Type); 
    }
        break;
    case wabt::Opcode::I16X8AvgrU:
    {
		Value* right = irBuilder.CreateBitCast(p2, type.i16x8Type);                       
		Value* left = irBuilder.CreateBitCast(p1, type.i16x8Type);                        
		Value* rightZExt = irBuilder.CreateZExt(right, type.i32x8Type); 
		Value* leftZExt = irBuilder.CreateZExt(left, type.i32x8Type);   
		Value* oneZExt = ConstantVector::getSplat(                                   
			type.i16x8Type->getElementCount(),                          
			ConstantInt::get(type.i32x8Type->getElementType(), 1));
		ret = irBuilder.CreateTrunc(
			irBuilder.CreateLShr(
				irBuilder.CreateAdd(irBuilder.CreateAdd(leftZExt, rightZExt), oneZExt), oneZExt),
			type.i16x8Type); 
    }
        break;    

    case wabt::Opcode::I8X16MinS:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateSelect(irBuilder.CreateICmpSLT(left, right), left, right));
        break;
    case wabt::Opcode::I8X16MinU:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateSelect(irBuilder.CreateICmpULT(left, right), left, right));
        break;
    case wabt::Opcode::I16X8MinS:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateSelect(irBuilder.CreateICmpSLT(left, right), left, right));
        break;
    case wabt::Opcode::I16X8MinU:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateSelect(irBuilder.CreateICmpULT(left, right), left, right));
        break;
    case wabt::Opcode::I32X4MinS:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateSelect(irBuilder.CreateICmpSLT(left, right), left, right));
        break;
    case wabt::Opcode::I32X4MinU:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateSelect(irBuilder.CreateICmpULT(left, right), left, right));
        break;
    case wabt::Opcode::F32X4Min:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::minnum, type.f32x4Type), {left,right}));
        break;
    case wabt::Opcode::F64X2Min:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::minnum, type.f64x2Type), {left,right}));
        break;        
    case wabt::Opcode::F32X4PMin:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateSelect(irBuilder.CreateFCmp(CmpInst::FCMP_OLT, right, left), right, left));
        break;        
    case wabt::Opcode::F64X2PMin:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateSelect(irBuilder.CreateFCmp(CmpInst::FCMP_OLT, right, left), right, left));
        break;                
    case wabt::Opcode::I8X16MaxS:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateSelect(irBuilder.CreateICmpSLT(left, right), right, left));
        break;
    case wabt::Opcode::I8X16MaxU:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateSelect(irBuilder.CreateICmpULT(left, right), right, left));
        break;
    case wabt::Opcode::I16X8MaxS:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateSelect(irBuilder.CreateICmpSLT(left, right), right, left));
        break;
    case wabt::Opcode::I16X8MaxU:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateSelect(irBuilder.CreateICmpULT(left, right), right, left));
        break;
    case wabt::Opcode::I32X4MaxS:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateSelect(irBuilder.CreateICmpSLT(left, right), right, left));
        break;
    case wabt::Opcode::I32X4MaxU:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateSelect(irBuilder.CreateICmpULT(left, right), right, left));
        break;
    case wabt::Opcode::F32X4Max:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::maxnum, type.f32x4Type), {left,right}));
        break;
    case wabt::Opcode::F64X2Max:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::maxnum, type.f64x2Type), {left,right}));
        break;        
    case wabt::Opcode::F32X4PMax:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateSelect(irBuilder.CreateFCmp(CmpInst::FCMP_OLT, left, right), right, left));
        break;        
    case wabt::Opcode::F64X2PMax:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateSelect(irBuilder.CreateFCmp(CmpInst::FCMP_OLT, left, right), right, left));
        break;         

    case wabt::Opcode::I16X8Q15mulrSatS:
    {
	    Value* left = irBuilder.CreateBitCast(p2, type.i16x8Type);
	    Value* right = irBuilder.CreateBitCast(p1, type.i16x8Type);

	    // Extend the inputs to 64-bit to avoid overflow.
	    left = irBuilder.CreateSExt(left, type.i64x8Type);
	    right = irBuilder.CreateSExt(right, type.i64x8Type);

	    // result = saturateS16((left * right + 0x4000) >> 15)
	    Value* product = irBuilder.CreateMul(left, right);
	    Value* sum = irBuilder.CreateAdd(
	    	product, irBuilder.CreateVectorSplat(8, ConstantInt::get(llvmContext, APInt(32, (uint64_t)0x4000, false))));
	    Value* shift = irBuilder.CreateAShr(sum, 15);
	    Value* minSplat
	    	= irBuilder.CreateVectorSplat(8,  ConstantInt::get(llvmContext, APInt(32, (int64_t)INT16_MIN, false)));
	    Value* maxSplat
	    	= irBuilder.CreateVectorSplat(8,  ConstantInt::get(llvmContext, APInt(32, (int64_t)INT16_MAX, false)));
	    Value* saturate = irBuilder.CreateSelect(
	    	irBuilder.CreateICmpSGT(shift, maxSplat),
	    	maxSplat,
	    	irBuilder.CreateSelect(irBuilder.CreateICmpSLT(shift, minSplat), minSplat, shift));
	    ret = irBuilder.CreateTrunc(saturate, type.i16x8Type);
    }
        break;
    // SIMD Narrow
    // case wabt::Opcode::I8X16NarrowI16X8S:
    //     EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::x86_sse2_packsswb_128), {left,right}));
    //     break;
    // case wabt::Opcode::I8X16NarrowI16X8U:
    //     EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::x86_sse2_packuswb_128), {left,right}));
    //     break;
    // case wabt::Opcode::I16X8NarrowI32X4S:
    //     EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::x86_sse2_packssdw_128), {left,right}));
    //     break;
    // case wabt::Opcode::I16X8NarrowI32X4U:
    //     EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateCall(Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::x86_sse41_packusdw), {left,right}));
    //     break; 

    // SIMD Extmul
    case wabt::Opcode::I16X8ExtmulLowI8X16S:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i8x16Type, type.i16x8Type, 0, true),
                                  createSIMDExtend(p1, type.i8x16Type, type.i16x8Type, 0, true));
        break;
    case wabt::Opcode::I16X8ExtmulHighI8X16S:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i8x16Type, type.i16x8Type, 8, true),
                                  createSIMDExtend(p1, type.i8x16Type, type.i16x8Type, 8, true));
        break;
    case wabt::Opcode::I16X8ExtmulLowI8X16U:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i8x16Type, type.i16x8Type, 0, false),
                                  createSIMDExtend(p1, type.i8x16Type, type.i16x8Type, 0, false));
        break;
    case wabt::Opcode::I16X8ExtmulHighI8X16U:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i8x16Type, type.i16x8Type, 8, false),
                                  createSIMDExtend(p1, type.i8x16Type, type.i16x8Type, 8, false));
        break;
    case wabt::Opcode::I32X4ExtmulLowI16X8S:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i16x8Type, type.i32x4Type, 0, true),
                                  createSIMDExtend(p1, type.i16x8Type, type.i32x4Type, 0, true));
        break;
    case wabt::Opcode::I32X4ExtmulHighI16X8S:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i16x8Type, type.i32x4Type, 4, true),
                                  createSIMDExtend(p1, type.i16x8Type, type.i32x4Type, 4, true));
        break;
    case wabt::Opcode::I32X4ExtmulLowI16X8U:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i16x8Type, type.i32x4Type, 0, false),
                                  createSIMDExtend(p1, type.i16x8Type, type.i32x4Type, 0, false));
        break;
    case wabt::Opcode::I32X4ExtmulHighI16X8U:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i16x8Type, type.i32x4Type, 4, false),
                                  createSIMDExtend(p1, type.i16x8Type, type.i32x4Type, 4, false));
        break;
    case wabt::Opcode::I64X2ExtmulLowI32X4S:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i32x4Type, type.i64x2Type, 0, true),
                                  createSIMDExtend(p1, type.i32x4Type, type.i64x2Type, 0, true));
        break;
    case wabt::Opcode::I64X2ExtmulHighI32X4S:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i32x4Type, type.i64x2Type, 2, true),
                                  createSIMDExtend(p1, type.i32x4Type, type.i64x2Type, 2, true));
        break;
    case wabt::Opcode::I64X2ExtmulLowI32X4U:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i32x4Type, type.i64x2Type, 0, false),
                                  createSIMDExtend(p1, type.i32x4Type, type.i64x2Type, 0, false));
        break;
    case wabt::Opcode::I64X2ExtmulHighI32X4U:
        ret = irBuilder.CreateMul(createSIMDExtend(p2, type.i32x4Type, type.i64x2Type, 2, false),
                                  createSIMDExtend(p1, type.i32x4Type, type.i64x2Type, 2, false));
        break;        
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported expr type: " << expr->opcode.GetName() << std::endl;
        break;
    }
    if (ret != nullptr) {
        stack.push_back(ret);
    }
}

void BlockContext::visitCompareExpr(wabt::CompareExpr* expr) {
    using namespace llvm;
    Value *ret = nullptr, *p1, *p2;
    Type *resultType = nullptr;
    p1 = popStack();
    p2 = popStack();
    switch (expr->opcode)
    {
    case wabt::Opcode::I32Eq:
    case wabt::Opcode::I64Eq:
        ret = irBuilder.CreateICmpEQ(p2, p1);
        break;        
    case wabt::Opcode::I8X16Eq:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateICmpEQ(left, right));
        resultType = type.i8x16Type;
        break;
    case wabt::Opcode::I16X8Eq:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateICmpEQ(left, right));
        resultType = type.i16x8Type;
        break;
    case wabt::Opcode::I32X4Eq:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateICmpEQ(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::I64X2Eq:
        EMIT_SIMD_BINARY_OP(type.i64x2Type, irBuilder.CreateICmpEQ(left, right));
        resultType = type.i64x2Type;
        break;
    case wabt::Opcode::I32Ne:
    case wabt::Opcode::I64Ne:
        ret = irBuilder.CreateICmpNE(p2, p1);
        break;
    case wabt::Opcode::I8X16Ne:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateICmpNE(left, right));
        resultType = type.i8x16Type;
        break;
    case wabt::Opcode::I16X8Ne:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateICmpNE(left, right));
        resultType = type.i16x8Type;
        break;
    case wabt::Opcode::I32X4Ne:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateICmpNE(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::I64X2Ne:
        EMIT_SIMD_BINARY_OP(type.i64x2Type, irBuilder.CreateICmpNE(left, right));
        resultType = type.i64x2Type;
        break;
    case wabt::Opcode::I32GeS:
    case wabt::Opcode::I64GeS:
        ret = irBuilder.CreateICmpSGE(p2, p1);
        break;
    case wabt::Opcode::I8X16GeS:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateICmpSGE(left, right));
        resultType = type.i8x16Type;
        break;
    case wabt::Opcode::I16X8GeS:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateICmpSGE(left, right));
        resultType = type.i16x8Type;
        break;
    case wabt::Opcode::I32X4GeS:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateICmpSGE(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::I64X2GeS:
        EMIT_SIMD_BINARY_OP(type.i64x2Type, irBuilder.CreateICmpSGE(left, right));
        resultType = type.i64x2Type;
        break;
    case wabt::Opcode::I32GeU:
    case wabt::Opcode::I64GeU:
        ret = irBuilder.CreateICmpUGE(p2, p1);
        break;
    case wabt::Opcode::I8X16GeU:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateICmpUGE(left, right));
        resultType = type.i8x16Type;
        break;
    case wabt::Opcode::I16X8GeU:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateICmpUGE(left, right));
        resultType = type.i16x8Type;
        break;
    case wabt::Opcode::I32X4GeU:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateICmpUGE(left, right));
        resultType = type.i32x4Type;
        break;
    /*case wabt::Opcode::I64X2GeU: */
    case wabt::Opcode::I32GtU:
    case wabt::Opcode::I64GtU:
        ret = irBuilder.CreateICmpUGT(p2, p1);
        break;
    case wabt::Opcode::I8X16GtU:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateICmpUGT(left, right));
        resultType = type.i8x16Type;
        break;
    case wabt::Opcode::I16X8GtU:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateICmpUGT(left, right));
        resultType = type.i16x8Type;
        break;
    case wabt::Opcode::I32X4GtU:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateICmpUGT(left, right));
        resultType = type.i32x4Type;
        break;
    /*case wabt::Opcode::I64X2GtU: */

    case wabt::Opcode::I32GtS:
    case wabt::Opcode::I64GtS:
        ret = irBuilder.CreateICmpSGT(p2, p1);
        break;
    case wabt::Opcode::I8X16GtS:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateICmpSGT(left, right));
        resultType = type.i8x16Type;
        break;
    case wabt::Opcode::I16X8GtS:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateICmpSGT(left, right));
        resultType = type.i16x8Type;
        break;
    case wabt::Opcode::I32X4GtS:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateICmpSGT(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::I64X2GtS:
        EMIT_SIMD_BINARY_OP(type.i64x2Type, irBuilder.CreateICmpSGT(left, right));
        resultType = type.i64x2Type;
        break;
    case wabt::Opcode::I32LeS:
    case wabt::Opcode::I64LeS:
        ret = irBuilder.CreateICmpSLE(p2, p1);
        break;
    case wabt::Opcode::I8X16LeS:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateICmpSLE(left, right));
        resultType = type.i8x16Type;
        break;
    case wabt::Opcode::I16X8LeS:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateICmpSLE(left, right));
        resultType = type.i16x8Type;
        break;
    case wabt::Opcode::I32X4LeS:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateICmpSLE(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::I64X2LeS:
        EMIT_SIMD_BINARY_OP(type.i64x2Type, irBuilder.CreateICmpSLE(left, right));
        resultType = type.i64x2Type;
        break;

    case wabt::Opcode::I32LeU:
    case wabt::Opcode::I64LeU:
        ret = irBuilder.CreateICmpULE(p2, p1);
        break;
    case wabt::Opcode::I8X16LeU:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateICmpULE(left, right));
        resultType = type.i8x16Type;
        break;
    case wabt::Opcode::I16X8LeU:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateICmpULE(left, right));
        resultType = type.i16x8Type;
        break;
    case wabt::Opcode::I32X4LeU:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateICmpULE(left, right));
        resultType = type.i32x4Type;
        break;
    /*case wabt::Opcode::I64X2LeU: */

    case wabt::Opcode::I32LtS:
    case wabt::Opcode::I64LtS:
        ret = irBuilder.CreateICmpSLT(p2, p1);
        break;    
    case wabt::Opcode::I8X16LtS:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateICmpSLT(left, right));
        resultType = type.i8x16Type;
        break;
    case wabt::Opcode::I16X8LtS:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateICmpSLT(left, right));
        resultType = type.i16x8Type;
        break;
    case wabt::Opcode::I32X4LtS:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateICmpSLT(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::I64X2LtS:
        EMIT_SIMD_BINARY_OP(type.i64x2Type, irBuilder.CreateICmpSLT(left, right));
        resultType = type.i64x2Type;
        break;
    case wabt::Opcode::I32LtU:
    case wabt::Opcode::I64LtU:
        ret = irBuilder.CreateICmpULT(p2, p1);
        break;
    case wabt::Opcode::I8X16LtU:
        EMIT_SIMD_BINARY_OP(type.i8x16Type, irBuilder.CreateICmpULT(left, right));
        resultType = type.i8x16Type;
        break;
    case wabt::Opcode::I16X8LtU:
        EMIT_SIMD_BINARY_OP(type.i16x8Type, irBuilder.CreateICmpULT(left, right));
        resultType = type.i16x8Type;
        break;
    case wabt::Opcode::I32X4LtU:
        EMIT_SIMD_BINARY_OP(type.i32x4Type, irBuilder.CreateICmpULT(left, right));
        resultType = type.i32x4Type;
        break;
    /*case wabt::Opcode::I64X2LtU: */


    case wabt::Opcode::F32Eq:
    case wabt::Opcode::F64Eq:
        ret = irBuilder.CreateFCmpOEQ(p2, p1);
        break;
    case wabt::Opcode::F32X4Eq:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateFCmpOEQ(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::F64X2Eq:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateFCmpOEQ(left, right));
        resultType = type.i64x2Type;
        break;

    case wabt::Opcode::F32Ne:
    case wabt::Opcode::F64Ne:
        ret = irBuilder.CreateFCmpOEQ(p2, p1);
        break;
    case wabt::Opcode::F32X4Ne:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateFCmpOEQ(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::F64X2Ne:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateFCmpOEQ(left, right));
        resultType = type.i64x2Type;
        break;

    case wabt::Opcode::F32Ge:
    case wabt::Opcode::F64Ge:
        ret = irBuilder.CreateFCmpOGE(p2, p1);
        break;
    case wabt::Opcode::F32X4Ge:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateFCmpOGE(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::F64X2Ge:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateFCmpOGE(left, right));
        resultType = type.i64x2Type;
        break;

    case wabt::Opcode::F32Gt:
    case wabt::Opcode::F64Gt:
        ret = irBuilder.CreateFCmpOGT(p2, p1);
        break;
    case wabt::Opcode::F32X4Gt:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateFCmpOGT(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::F64X2Gt:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateFCmpOGT(left, right));
        resultType = type.i64x2Type;
        break;

    case wabt::Opcode::F32Le:
    case wabt::Opcode::F64Le:
        ret = irBuilder.CreateFCmpOLE(p2, p1);
        break;
    case wabt::Opcode::F32X4Le:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateFCmpOLE(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::F64X2Le:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateFCmpOLE(left, right));
        resultType = type.i64x2Type;
        break;

    case wabt::Opcode::F32Lt:
    case wabt::Opcode::F64Lt:
        ret = irBuilder.CreateFCmpOLT(p2, p1);
        break;
    case wabt::Opcode::F32X4Lt:
        EMIT_SIMD_BINARY_OP(type.f32x4Type, irBuilder.CreateFCmpOLT(left, right));
        resultType = type.i32x4Type;
        break;
    case wabt::Opcode::F64X2Lt:
        EMIT_SIMD_BINARY_OP(type.f64x2Type, irBuilder.CreateFCmpOLT(left, right));
        resultType = type.i64x2Type;
        break;

    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported expr type: " << expr->opcode.GetName() << std::endl;
        break;
    }
    if (ret != nullptr) {
        ret = irBuilder.CreateZExt(ret, Type::getInt32Ty(ctx.llvmContext));
        if(resultType != nullptr)
            ret = irBuilder.CreateSExt(ret, resultType);
        stack.push_back(ret);
    }
}

void BlockContext::visitCallInst(wabt::CallExpr* expr) {
    using namespace llvm;
    Function* target = ctx.findFunc(expr->var);
    // get wabt func
    wabt::Func* wfunc = ctx.module->GetFunc(expr->var);
    wabt::Index paramCount = wfunc->GetNumParams();
    assert (target->getFunctionType()->getNumParams() == paramCount);
    auto callArgsAlloca = (Value**)alloca(sizeof(Value*) * paramCount);
    ArrayRef<Value*> callArgs = ArrayRef<Value*>(callArgsAlloca, paramCount);
    // https://stackoverflow.com/questions/5458204/unsigned-int-reverse-iteration-with-for-loops
    for (wabt::Index i = paramCount; i-- > 0;) {
        callArgsAlloca[i] = popStack();
    }
    // TODO MultiValue
    assert(wfunc->GetNumResults() <= 1);
    Value* ret = irBuilder.CreateCall(target->getFunctionType(), target, callArgs);
    if (wfunc->GetNumResults() != 0) {
        stack.push_back(ret);
    }
}

llvm::Value* BlockContext::createAllTrue(llvm::Value* vector, llvm::FixedVectorType* vectorType){
    using namespace llvm;
	vector = irBuilder.CreateBitCast(vector, vectorType);

	const uintptr_t numLanes = vectorType->getNumElements();
	Constant* zero
		= ConstantInt::get(vectorType->getScalarType(), APInt(vectorType->getScalarSizeInBits(), 0));

	Value* result = nullptr;
	for(uintptr_t laneIndex = 0; laneIndex < numLanes; ++laneIndex)
	{
		Value* scalar = irBuilder.CreateExtractElement(vector, laneIndex);
		Value* scalarBool = irBuilder.CreateICmpNE(scalar, zero);

		result = result ? irBuilder.CreateAnd(result, scalarBool) : scalarBool;
	}
	return irBuilder.CreateZExt(result, Type::getInt32Ty(irBuilder.getContext()));
}

void BlockContext::visitCallIndirectInst(wabt::CallIndirectExpr* expr) {
    using namespace llvm;

    // 1 找到table对应的函数指针数组（全局变量）
    GlobalVariable* table = ctx.tables.at(ctx.module->GetTableIndex(expr->table));
    // 2 获取index，保证index在范围内
    Value* index = popStack();
    // TODO 保证index在范围内
    // 3 获取正确的函数指针类型
    FunctionType* funcType = convertFuncType(llvmContext, expr->decl.sig);
    wabt::Index paramCount = expr->decl.sig.param_types.size();
    assert (funcType->getNumParams() == paramCount);
    // 4 取下标，调用函数
    Value* arr[2] = { ConstantInt::getNullValue(index->getType()) , index};
    Value* ptr = irBuilder.CreateGEP(table->getValueType(), table, makeArrayRef(arr, 2));
    
    // ctx.llvmModule.getDataLayout().getABITypeAlign(funcType);
    // outs() << *cast<GetElementPtrInst>(ptr);
    Value* funcPtr = irBuilder.CreateLoad(ptr->getType()->getPointerElementType(), ptr, "callind_funcptr");
    // Value* funcPtr = irBuilder.CreateExtractElement(table, index, "callind_funcptr");
    funcPtr = irBuilder.CreateBitOrPointerCast(funcPtr, PointerType::get(funcType, 0));
    auto callArgsAlloca = (Value**)calloc(sizeof(Value*), paramCount);
    // https://stackoverflow.com/questions/5458204/unsigned-int-reverse-iteration-with-for-loops
    for (wabt::Index i = paramCount; i-- > 0;) {
        callArgsAlloca[i] = popStack();
    }
    ArrayRef<Value*> callArgs = ArrayRef<Value*>(callArgsAlloca, paramCount);
    // TODO MultiValue
    assert(expr->decl.GetNumResults() <= 1);
    Value* ret = irBuilder.CreateCall(funcType, funcPtr, callArgs);
    if (expr->decl.GetNumResults() != 0) {
        stack.push_back(ret);
    }
}

void BlockContext::visitConstInst(wabt::ConstExpr* expr) {
    stack.push_back(visitConst(llvmContext, expr->const_));
}

llvm::Constant* visitConst(llvm::LLVMContext &llvmContext, const wabt::Const& const_) {
    using namespace wabt;
    uint64_t data[2];
    switch (const_.type()) {
    case Type::I32:
        return llvm::ConstantInt::get(convertType(llvmContext, const_.type()), const_.u32(), false);
    case Type::I64:
        return llvm::ConstantInt::get(convertType(llvmContext, const_.type()), const_.u64(), false);
    case Type::F32:
        return llvm::ConstantFP::get(convertType(llvmContext, const_.type()), llvm::APFloat(ieee_float(const_.f32_bits())));
    case Type::F64:
        return llvm::ConstantFP::get(convertType(llvmContext, const_.type()), llvm::APFloat(ieee_double(const_.f64_bits())));
    case Type::V128:
        data[0] = const_.vec128().u64(0);
        data[1] = const_.vec128().u64(1);
        return llvm::ConstantInt::get(convertType(llvmContext, const_.type()), llvm::APInt(128, llvm::ArrayRef<uint64_t>(data, 2)));
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: InitExpr type unknown: " << const_.type().GetName() << std::endl;
        std::abort();
    }
}


llvm::Value* BlockContext::createExtractLane(llvm::Value* vector,llvm::Type* ty,uint64_t imm ,bool sign){
    using namespace llvm;
    Value *ret = irBuilder.CreateExtractElement(irBuilder.CreateBitCast(vector, ty),imm);
    if(sign)
        ret = irBuilder.CreateSExt(ret, type.i32Type);
    else
        ret = irBuilder.CreateZExt(ret, type.i32Type);
    return ret;
}

void BlockContext::visitSimdLaneOp(wabt::SimdLaneOpExpr* expr){
    using namespace llvm;
    Value *ret = nullptr, *vec, *x;
    vec = popStack();
    uint64_t imm = expr->val;
    switch (expr->opcode){
    /* extract_lane */    
    case wabt::Opcode::I8X16ExtractLaneS:
        ret = createExtractLane(vec, type.i8x16Type, imm, 1);
        break;
    case wabt::Opcode::I8X16ExtractLaneU:
        ret = createExtractLane(vec, type.i8x16Type, imm, 0);
        break;
    case wabt::Opcode::I16X8ExtractLaneS:
        ret = createExtractLane(vec, type.i16x8Type, imm, 1);
        break;
    case wabt::Opcode::I16X8ExtractLaneU:
        ret = createExtractLane(vec, type.i16x8Type, imm, 0);
        break;
    case wabt::Opcode::I32X4ExtractLane:
        ret = createExtractLane(vec, type.i32x4Type, imm, -1);
        break;
    case wabt::Opcode::I64X2ExtractLane:
        ret = createExtractLane(vec, type.i64x2Type, imm, -1);
        break;
    case wabt::Opcode::F32X4ExtractLane:
        ret = createExtractLane(vec, type.f32x4Type, imm, -1);
        break;
    case wabt::Opcode::F64X2ExtractLane:
        ret = createExtractLane(vec, type.f64x2Type, imm, -1);
        break;
    /* replace_lane */
    case wabt::Opcode::I8X16ReplaceLane:
        x = vec;
        vec = popStack();
        ret = irBuilder.CreateBitCast(vec, type.i8x16Type);
        ret = irBuilder.CreateTrunc(x, type.i8Type);
        ret = irBuilder.CreateInsertElement(ret, x, imm);
        break;
    case wabt::Opcode::I16X8ReplaceLane:
        x = vec;
        vec = popStack();
        ret = irBuilder.CreateBitCast(vec, type.i16x8Type);
        ret = irBuilder.CreateTrunc(x, type.i16Type);
        ret = irBuilder.CreateInsertElement(ret, x, imm);
        break;
    case wabt::Opcode::I32X4ReplaceLane:
        x = vec;
        vec = popStack();
        ret = irBuilder.CreateBitCast(vec, type.i32x4Type);
        ret = irBuilder.CreateInsertElement(ret, x, imm);
        break;
    case wabt::Opcode::I64X2ReplaceLane:
        x = vec;
        vec = popStack();
        ret = irBuilder.CreateBitCast(vec, type.i64x2Type);
        ret = irBuilder.CreateInsertElement(ret, x, imm);
        break; 
    case wabt::Opcode::F32X4ReplaceLane:
        x = vec;
        vec = popStack();
        ret = irBuilder.CreateBitCast(vec, type.f32x4Type);
        ret = irBuilder.CreateInsertElement(ret, x, imm);
        break;
    case wabt::Opcode::F64X2ReplaceLane:
        x = vec;
        vec = popStack();
        ret = irBuilder.CreateBitCast(vec, type.f64x2Type);
        ret = irBuilder.CreateInsertElement(ret, x, imm);
        break;
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported Opcode: " << expr->opcode.GetName() << std::endl;
        break;
    }
    if (ret != nullptr) {
        stack.push_back(ret);
    }    
}

llvm::Value* BlockContext::createLoadLane(llvm::Value* vector, llvm::Value* addr, llvm::Type* ty, uint64_t imm){
    using namespace llvm;
    vector = irBuilder.CreateBitCast(vector, ty);
    Type* elementType = ty->getArrayElementType();
    addr = irBuilder.CreateBitCast(addr, PointerType::getUnqual(elementType));
    Value* result = irBuilder.CreateLoad(elementType, addr, "loadResult");
    return irBuilder.CreateInsertElement(vector, result, imm);
}

llvm::Value* BlockContext::createStoreLane(llvm::Value* vector, llvm::Value* addr, llvm::Type* ty, uint64_t imm){
    using namespace llvm;
    vector = irBuilder.CreateBitCast(vector, ty);
    Type* elementType = ty->getArrayElementType();
    addr = irBuilder.CreateBitCast(addr, PointerType::getUnqual(elementType));
    Value* result = irBuilder.CreateExtractElement(vector, imm);
    return irBuilder.CreateStore(result, addr);
}

void BlockContext::visitSimdLoadLane(wabt::SimdLoadLaneExpr* expr){
    using namespace llvm;
    Value *ret = nullptr, *vec, *addr;
    uint64_t imm = expr->val;
    vec = popStack();
    addr = popStack();
    switch (expr->opcode)
    {
    case wabt::Opcode::V128Load8Lane:
        ret = createLoadLane(vec, addr, type.i8x16Type, imm);
        break;
    case wabt::Opcode::V128Load16Lane:
        ret = createLoadLane(vec, addr, type.i16x8Type, imm);
        break;
    case wabt::Opcode::V128Load32Lane:
        ret = createLoadLane(vec, addr, type.i32x4Type, imm);
        break;
    case wabt::Opcode::V128Load64Lane:
        ret = createLoadLane(vec, addr, type.i64x2Type, imm);
        break;
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported Opcode: " << expr->opcode.GetName() << std::endl;
        break;
    }
    if (ret != nullptr) 
        stack.push_back(ret);
    
}

void BlockContext::visitSimdStoreLane(wabt::SimdStoreLaneExpr* expr){
    using namespace llvm;
    Value *ret = nullptr, *vec, *addr;
    uint64_t imm = expr->val;
    vec = popStack();
    addr = popStack();
    switch (expr->opcode)
    {
    case wabt::Opcode::V128Store8Lane:
        ret = createStoreLane(vec, addr, type.i8x16Type, imm);
        break;
    case wabt::Opcode::V128Store16Lane:
        ret = createStoreLane(vec, addr, type.i16x8Type, imm);
        break;
    case wabt::Opcode::V128Store32Lane:
        ret = createStoreLane(vec, addr, type.i32x4Type, imm);
        break;
    case wabt::Opcode::V128Store64Lane:
        ret = createStoreLane(vec, addr, type.i64x2Type, imm);
        break;
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported Opcode: " << expr->opcode.GetName() << std::endl;
        break;
    }
    if (ret != nullptr) 
        stack.push_back(ret);
    
}

void BlockContext::visitTernaryInst(wabt::TernaryExpr* expr){
    using namespace llvm;
    Value *ret = nullptr, *p1, *p2, *p3;
    p1 = popStack();
    p2 = popStack();
    p3 = popStack();
    switch (expr->opcode){
    case wabt::Opcode::V128BitSelect:
    {
        Value* mask = irBuilder.CreateBitCast(p1, type.i64x2Type);
        Value* falseValue = irBuilder.CreateBitCast(p2, type.i64x2Type);
        Value* trueValue = irBuilder.CreateBitCast(p3, type.i64x2Type);
	    ret = irBuilder.CreateOr(irBuilder.CreateAnd(trueValue, mask),
							  irBuilder.CreateAnd(falseValue, irBuilder.CreateNot(mask)));
    }
        break;
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported Opcode: " << expr->opcode.GetName() << std::endl;
        break;
    }
    if(ret != nullptr)
        stack.push_back(ret);
    
}


}