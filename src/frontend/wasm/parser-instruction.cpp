#include "frontend/wasm/parser-block.h"
#include <llvm-13/llvm/Support/Alignment.h>

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

    //TODO: support 2.0
    // case ExprType::MemoryGrow:
    //     stack.pop_back();
    //     stack.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext), 0));
    //     break;
    // case ExprType::MemorySize:
    //     stack.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext), 0));
    //     break;
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
    base = irBuilder.CreateAdd(base, llvm::ConstantInt::get(base->getType(), offset, false), "calcOffset");
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
        targetType = Type::getInt8Ty(llvmContext);
        break;
    case wabt::Opcode::I32Load16S:
    case wabt::Opcode::I32Load16U:
    case wabt::Opcode::I64Load16S:
    case wabt::Opcode::I64Load16U:
        targetType = Type::getInt16Ty(llvmContext);
        break;
    case wabt::Opcode::I64Load32S:
    case wabt::Opcode::I64Load32U:
        targetType = Type::getInt32Ty(llvmContext);
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
    default:
        break;
    }
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
        ret = irBuilder.CreateZExt(ret, p1->getType());
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

void BlockContext::visitUnaryInst(wabt::UnaryExpr* expr) {
    using namespace llvm;
    Value *ret = nullptr, *p1;
    Function* f;
    p1 = popStack();
    switch (expr->opcode)
    {
    case wabt::Opcode::F32Neg:
    case wabt::Opcode::F64Neg:
        ret = irBuilder.CreateFNeg(p1);
        break;
    case wabt::Opcode::F32Abs:
    case wabt::Opcode::F64Abs:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::fabs, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::I32Popcnt:
    case wabt::Opcode::I64Popcnt:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::ctpop, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::I32Clz:
    case wabt::Opcode::I64Clz:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::ctlz, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::I32Ctz:
    case wabt::Opcode::I64Ctz:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::cttz, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::F32Ceil:
    case wabt::Opcode::F64Ceil:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::ceil, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::F32Floor:
    case wabt::Opcode::F64Floor:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::floor, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::F32Nearest:
    case wabt::Opcode::F64Nearest:
        // https://www.w3.org/TR/wasm-core-1/#-hrefop-fnearestmathrmfnearest_n-z
        // https://llvm.org/docs/LangRef.html#llvm-round-intrinsic
        // https://developer.mozilla.org/en-US/docs/WebAssembly/Reference/Numeric/Nearest
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::roundeven, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::F32Trunc:
    case wabt::Opcode::F64Trunc:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::trunc, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
        break;
    case wabt::Opcode::F32Sqrt:
    case wabt::Opcode::F64Sqrt:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::sqrt, p1->getType());
        ret = irBuilder.CreateCall(f, p1);
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
    p1 = popStack();
    p2 = popStack();
    switch (expr->opcode)
    {
    case wabt::Opcode::I32Eq:
    case wabt::Opcode::I64Eq:
        ret = irBuilder.CreateICmpEQ(p2, p1);
        break;
    case wabt::Opcode::I32Ne:
    case wabt::Opcode::I64Ne:
        ret = irBuilder.CreateICmpNE(p2, p1);
        break;
    case wabt::Opcode::I32GeS:
    case wabt::Opcode::I64GeS:
        ret = irBuilder.CreateICmpSGE(p2, p1);
        break;
    case wabt::Opcode::I32GeU:
    case wabt::Opcode::I64GeU:
        ret = irBuilder.CreateICmpUGE(p2, p1);
        break;
    case wabt::Opcode::I32GtU:
    case wabt::Opcode::I64GtU:
        ret = irBuilder.CreateICmpUGT(p2, p1);
        break;
    case wabt::Opcode::I32GtS:
    case wabt::Opcode::I64GtS:
        ret = irBuilder.CreateICmpSGT(p2, p1);
        break;
    case wabt::Opcode::I32LeS:
    case wabt::Opcode::I64LeS:
        ret = irBuilder.CreateICmpSLE(p2, p1);
        break;
    case wabt::Opcode::I32LeU:
    case wabt::Opcode::I64LeU:
        ret = irBuilder.CreateICmpULE(p2, p1);
        break;
    case wabt::Opcode::I32LtS:
    case wabt::Opcode::I64LtS:
        ret = irBuilder.CreateICmpSLT(p2, p1);
        break;
    case wabt::Opcode::I32LtU:
    case wabt::Opcode::I64LtU:
        ret = irBuilder.CreateICmpULT(p2, p1);
        break;
    case wabt::Opcode::F32Eq:
    case wabt::Opcode::F64Eq:
        ret = irBuilder.CreateFCmpOEQ(p2, p1);
        break;
    case wabt::Opcode::F32Ne:
    case wabt::Opcode::F64Ne:
        ret = irBuilder.CreateFCmpUNE(p2, p1); // 出现NAN时要返回true
        break;
    case wabt::Opcode::F32Ge:
    case wabt::Opcode::F64Ge:
        ret = irBuilder.CreateFCmpOGE(p2, p1);
        break;
    case wabt::Opcode::F32Gt:
    case wabt::Opcode::F64Gt:
        ret = irBuilder.CreateFCmpOGT(p2, p1);
        break;
    case wabt::Opcode::F32Le:
    case wabt::Opcode::F64Le:
        ret = irBuilder.CreateFCmpOLE(p2, p1);
        break;
    case wabt::Opcode::F32Lt:
    case wabt::Opcode::F64Lt:
        ret = irBuilder.CreateFCmpOLT(p2, p1);
        break;
    
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported expr type: " << expr->opcode.GetName() << std::endl;
        break;
    }
    if (ret != nullptr) {
        ret = irBuilder.CreateZExt(ret, convertType(llvmContext, expr->opcode.GetResultType()));
        stack.push_back(ret);
    }
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
        /* code */
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
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported expr type: " << expr->opcode.GetName() << std::endl;
        break;
    }
    if (ret != nullptr) {
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
    auto callArgsAlloca = (llvm::Value**)alloca(sizeof(llvm::Value*) * paramCount);
    llvm::ArrayRef<llvm::Value*> callArgs = llvm::ArrayRef<llvm::Value*>(callArgsAlloca, paramCount);
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

void BlockContext::visitCallIndirectInst(wabt::CallIndirectExpr* expr) {
    using namespace llvm;
    // 创建函数指针数组，全局变量
    

    Function* target = ctx.findFunc(expr->);
    // get wabt func
    wabt::Func* wfunc = ctx.module->GetFunc(expr->decl);
    wabt::Index paramCount = wfunc->GetNumParams();
    assert (target->getFunctionType()->getNumParams() == paramCount);
    auto callArgsAlloca = (llvm::Value**)alloca(sizeof(llvm::Value*) * paramCount);
    llvm::ArrayRef<llvm::Value*> callArgs = llvm::ArrayRef<llvm::Value*>(callArgsAlloca, paramCount);
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

}