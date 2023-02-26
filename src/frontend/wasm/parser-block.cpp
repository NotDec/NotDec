
#include "frontend/wasm/parser-block.h"

namespace notdec::frontend::wasm {

void BlockContext::visitBlock(wabt::LabelType lty, llvm::BasicBlock* entry, llvm::BasicBlock* exit, wabt::BlockDeclaration& decl, wabt::ExprList& exprs) {
    // create Phi for block type
    using namespace wabt;
    std::deque<llvm::PHINode*> phis;
    llvm::BasicBlock* breakTo;

    if (log_level >= level_debug) {
        std::cerr << "Debug: Visiting Block with label type " << labelTypeToString(lty) << std::endl;
    }

    // ensure not empty
    assert(entry->getFirstNonPHI() == nullptr);

    auto entryInst = entry->getFirstNonPHIOrDbgOrLifetime();
    llvm::BasicBlock::iterator entryInsertTo;
    if (entryInst != nullptr) {
        entryInsertTo = entryInst->getIterator();
    } else {
        entryInsertTo = entry->begin();
    }

    // true for block, false for loop
    bool isBlock = true;
    switch (lty) {
        case LabelType::Block:
        case LabelType::Func:
            // 把参数留在栈上
            // 为基本块返回值创建Phi
            irBuilder.SetInsertPoint(exit, exit->begin());
            // 因为是push_back，所以是顺序遍历的。
            for (wabt::Index i = 0; i < decl.GetNumResults(); i++) {
                llvm::PHINode* phi = irBuilder.CreatePHI(convertType(llvmContext, decl.GetResultType(i)), 0, entry->getName() + "_" + std::to_string(i));
                phis.push_back(phi);
            }
            isBlock = true;
            // 有跳出的直接跳到exit
            breakTo = exit;

            irBuilder.SetInsertPoint(entry, entryInsertTo);
            break;
        case LabelType::Loop:
            // 为基本块参数创建Phi，放到那边BreakoutTarget里。
            // 为参数创建Phi
            irBuilder.SetInsertPoint(entry, entry->begin());
            // https://stackoverflow.com/questions/5458204/unsigned-int-reverse-iteration-with-for-loops
            for (wabt::Index i = decl.GetNumParams(); i-- > 0 ;) {
                llvm::PHINode* phi = irBuilder.CreatePHI(convertType(llvmContext, decl.GetParamType(i)), 0, entry->getName() + "_" + std::to_string(i));
                phis.push_front(phi);
                // 给phi赋值，然后用Phi替换栈上值
                assert(stack.size() > 0);
                phi->addIncoming(stack.back(), entry); stack.pop_back();
            }
            // 把栈上参数转换为Phi
            for (auto phi: phis) {
                stack.push_back(phi);
            }
            isBlock = false;
            // 有跳出的直接跳到entry
            breakTo = entry;
            break;
        
        case LabelType::If:
        case LabelType::Else:
        case LabelType::Try:
        case LabelType::InitExpr:
        case LabelType::Catch:
        default:
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: unexpected LabelType: " << labelTypeToString(lty) << std::endl;
            std::abort();
    }
    // 标记当前栈的位置，结束的时候先读取返回值然后再弹出到这个位置。
    std::size_t stack_pos = stack.size() - decl.GetNumParams();
    blockStack.emplace_back(*breakTo, phis, stack_pos, decl, lty);

    // 依次遍历每个指令，同时处理栈的变化。
    visitControlInsts(entry, exit, exprs);
    // 如果没有terminator，增加隐含的return。
    if (irBuilder.GetInsertBlock()->getTerminator() == nullptr) {
        assert(stack.size() >= phis.size());
        // 栈上值和Phi的转换，需要和创建的br一起。
        if (isBlock) {
            for (auto it = phis.rbegin(); it != phis.rend(); ++it) {
                (*it)->addIncoming(stack.back(), irBuilder.GetInsertBlock()); stack.pop_back();
            }
            for (auto phi: phis) {
                stack.push_back(phi);
            }
        }
        irBuilder.CreateBr(exit);
    } else { // 主动跳转了的话
        for (auto phi: phis) {
            stack.push_back(phi);
        }
    }
    
}

void BlockContext::visitControlInsts(llvm::BasicBlock* entry, llvm::BasicBlock* exit, wabt::ExprList& exprs) {
    using namespace llvm;
    // ref: wabt\src\wat-writer.cc
    for (wabt::Expr& expr : exprs) {
        if (log_level >= level_debug) {
            std::cerr << "Debug: Visiting expr " << wabt::GetExprTypeName(expr);
            if (expr.loc.line != 0) {
                std::cerr << ", at line " << expr.loc.line;
            }
            std::cerr << std::endl;
        }
        if (expr.type() == wabt::ExprType::Block) {
            // 创建新的基本块作为entry，exit
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "TODO handle nested control flow." << std::endl;
            std::abort();
            // BasicBlock* entryBasicBlock = llvm::BasicBlock::Create(llvmContext, "entry", ctx.);
            // BasicBlock* returnBlock = llvm::BasicBlock::Create(llvmContext, "return", function);
            // irBuilder.CreateBr(entryBasicBlock);

        } else {
            dispatchExprs(expr);
        }

    }
}

void BlockContext::dispatchExprs(wabt::Expr& expr) {
    using namespace wabt;
    // 看每个expr type有哪些指令：wabt\src\lexer-keywords.txt
    switch (expr.type()) {
    case ExprType::Return:
        visitReturn(cast<ReturnExpr>(&expr));
        break;
    case ExprType::Unary:
        visitUnaryInst(cast<UnaryExpr>(&expr));
        break;
    case ExprType::Binary:
        visitBinaryInst(cast<BinaryExpr>(&expr));
        break;
    case ExprType::Compare:
        visitCompareExpr(cast<CompareExpr>(&expr));
        break;
    case ExprType::Const:
        visitConstInst(cast<ConstExpr>(&expr));
        break;
    case ExprType::Call:
        visitCallInst(cast<CallExpr>(&expr));
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
    case ExprType::GlobalGet:
        visitGlobalGet(cast<GlobalGetExpr>(&expr));
        break;
    case ExprType::GlobalSet:
        visitGlobalSet(cast<GlobalSetExpr>(&expr));
        break;
    case ExprType::Unreachable:
        irBuilder.CreateUnreachable();
        break;
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported expr type: " << GetExprTypeName(expr) << std::endl;
        // really abort?
        // std::abort();
    }
}

void BlockContext::visitReturn(wabt::ReturnExpr* expr) {
    // 等价于直接跳转出最外面的函数体block
    BreakoutTarget& bt = blockStack.at(0);
    assert(bt.lty == wabt::LabelType::Func);
    // irBuilder.CreateRet(stack.back()); stack.pop_back();
    // 返回值放到Phi里
    assert(stack.size() >= bt.phis.size());
    for (auto it = bt.phis.rbegin(); it != bt.phis.rend(); ++it) {
        (*it)->addIncoming(stack.back(), irBuilder.GetInsertBlock()); stack.pop_back();
    }
    // 跳转出去
    irBuilder.CreateBr(&bt.target);
}

void BlockContext::visitLocalSet(wabt::LocalSetExpr* expr) {
    using namespace llvm;
    assert(stack.size() > 0);
    Value* val = stack.back(); stack.pop_back();
    Value* target = locals.at(expr->var.index());
    irBuilder.CreateStore(val, target);
}

void BlockContext::visitGlobalSet(wabt::GlobalSetExpr* expr) {
    using namespace llvm;
    assert(stack.size() > 0);
    Value* val = stack.back(); stack.pop_back();
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
    assert(stack.size() > 0);
    Value* val = stack.back(); stack.pop_back();
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

// 从栈上读一个整数地址，加上offset，取默认的mem，然后返回指针。
llvm::Value* BlockContext::convertStackAddr(uint64_t offset) {
    using namespace llvm;
    GlobalVariable* mem = this->ctx.mems.at(0);
    assert(stack.size() > 0);
    Value* base = stack.back(); stack.pop_back();

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

void BlockContext::visitUnaryInst(wabt::UnaryExpr* expr) {
    using namespace llvm;
    Value *ret = nullptr, *p1;
    Function* f;
    assert(stack.size() >= 2);
    p1 = stack.back(); stack.pop_back();
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
    assert(stack.size() >= 2);
    p1 = stack.back(); stack.pop_back();
    p2 = stack.back(); stack.pop_back();
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
    assert(stack.size() >= 2);
    p1 = stack.back(); stack.pop_back();
    p2 = stack.back(); stack.pop_back();
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
        // using Intrinsics fshl https://github.com/llvm-mirror/llvm/blob/2c4ca6832fa6b306ee6a7010bfb80a3f2596f824/test/CodeGen/X86/funnel-shift-rot.ll
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::fshl, {p1->getType(),p1->getType(),p1->getType()});
        ret = irBuilder.CreateCall(f, {p2, p2, p1});
        break;
    case wabt::Opcode::I32Rotr:
    case wabt::Opcode::I64Rotr:
        f = Intrinsic::getDeclaration(&ctx.llvmModule, Intrinsic::fshr, {p1->getType(),p1->getType(),p1->getType()});
        ret = irBuilder.CreateCall(f, {p2, p2, p1});
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
        assert(stack.size() > 0);
        callArgsAlloca[i] = stack.back(); stack.pop_back();
    }
    // TODO MultiValue
    stack.push_back(irBuilder.CreateCall(target->getFunctionType(), target, callArgs));
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

int64_t unwrapIntConstant(llvm::Constant* c) {
    using namespace llvm;
    assert(isa<ConstantInt>(c));
    return cast<ConstantInt>(c)->getSExtValue();
}

const char* labelTypeToString(wabt::LabelType lty) {
    using namespace wabt;
    switch (lty) {
        case LabelType::Func:
            return "LabelType::Func";
        case LabelType::InitExpr:
            return "LabelType::InitExpr";
        case LabelType::Block:
            return "LabelType::Block";
        case LabelType::Loop:
            return "LabelType::Loop";
        case LabelType::If:
            return "LabelType::If";
        case LabelType::Else:
            return "LabelType::Else";
        case LabelType::Try:
            return "LabelType::Try";
        case LabelType::Catch:
            return "LabelType::Catch";
        default:
            return "<unknown LabelType>";
    }
}

}