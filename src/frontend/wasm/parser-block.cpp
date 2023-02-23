
#include "frontend/wasm/parser-block.h"

namespace notdec::frontend::wasm {

void BlockContext::visitBlock(wabt::LabelType lty, llvm::BasicBlock* entry, llvm::BasicBlock* exit, wabt::BlockDeclaration& decl, wabt::ExprList& exprs) {
    // create Phi for block type
    using namespace wabt;
    std::deque<llvm::PHINode*> phis;
    llvm::BasicBlock* breakTo;
    bool ret2phi = false;
    switch (lty) {
        case LabelType::Block:
        case LabelType::If:
        case LabelType::Else:
        case LabelType::Func:
            // 为基本块返回值创建Phi
            irBuilder.SetInsertPoint(exit, exit->begin());
            for (wabt::Index i = 0; i < decl.GetNumResults(); i++) {
                llvm::PHINode* phi = irBuilder.CreatePHI(convertType(llvmContext, decl.GetResultType(i)), 0, entry->getName() + "_" + std::to_string(i));
                phis.push_back(phi);
            }
            ret2phi = true;
            // 有跳出的直接跳到exit
            breakTo = exit;

            irBuilder.SetInsertPoint(entry, entry->begin());
            break;
        case LabelType::Loop:
            // 为基本块参数创建Phi，放到那边BreakoutTarget里。
            // 为参数创建Phi
            irBuilder.SetInsertPoint(entry, entry->begin());
            // https://stackoverflow.com/questions/5458204/unsigned-int-reverse-iteration-with-for-loops
            for (wabt::Index i = decl.GetNumParams(); i-- > 0 ;) {
                llvm::PHINode* phi = irBuilder.CreatePHI(convertType(llvmContext, decl.GetParamType(i)), 0, entry->getName() + "_" + std::to_string(i));
                phis.push_front(phi);
                // pop stack to Phi
                assert(stack.size() > 0);
                phi->addIncoming(stack.back(), entry); stack.pop_back();
            }
            // 把栈上参数转换为Phi
            for (auto phi: phis) {
                stack.push_back(phi);
            }
            // 有跳出的直接跳到entry
            breakTo = entry;
            break;
        case LabelType::Try:
        case LabelType::InitExpr:
        case LabelType::Catch:
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: unexpected LabelType: " << labelTypeToString(lty) << std::endl;
            std::abort();
    }
    // 标记当前栈的位置，结束的时候先读取返回值然后再弹出到这个位置。
    std::size_t stack_pos = stack.size() - decl.GetNumParams();
    blockStack.emplace_back(*breakTo, stack_pos, decl, lty);

    // 依次遍历每个指令，同时处理栈的变化。先处理控制流相关的，简单的单独用一个函数
    visitControlInsts(entry, exit, exprs);

    // 如果是block这种类型，把返回值转换为Phi。
    if (ret2phi) {
        // 从栈上pop值出来，准备放到Phi里。
        for (auto it = phis.rbegin(); it != phis.rend(); ++it) {
            assert(stack.size() > 0);
            (*it)->addIncoming(stack.back(), entry); stack.pop_back();
        }
        for (auto phi: phis) {
            stack.push_back(phi);
        }
    }
    // TODO 结束后隐含跳转到exit
}

void BlockContext::visitControlInsts(llvm::BasicBlock* entry, llvm::BasicBlock* exit, wabt::ExprList& exprs) {
    using namespace wabt;
    // ref: wabt\src\wat-writer.cc
    for (Expr& expr : exprs) {
        switch (expr.type()) {
            case ExprType::Binary:
                visitBinaryInst(cast<BinaryExpr>(&expr));
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
            default:
                std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported expr type: " << GetExprTypeName(expr) << std::endl;
                // really abort?
                // std::abort();
        }
    }
}

void BlockContext::visitLocalGet(wabt::LocalGetExpr* expr) {
    using namespace llvm;
    // ctx.module->GetGlobalIndex(expr->var);
    // assert(expr->var.is_index()); // 冗余
    Value* target = locals.at(expr->var.index());
    Value* loaded = irBuilder.CreateLoad(target->getType()->getPointerElementType(), target);
    stack.push_back(loaded);
}

void BlockContext::visitGlobalGet(wabt::GlobalGetExpr* expr) {
    using namespace llvm;
    Value* target = locals.at(ctx.module->GetGlobalIndex(expr->var));
    Value* loaded = irBuilder.CreateLoad(target->getType()->getPointerElementType(), target);
    stack.push_back(loaded);
}


void BlockContext::visitStoreInst(wabt::StoreExpr* expr) {
    using namespace llvm;
    Value* addr = convertStackAddr(expr->offset);
    assert(stack.size() > 0);
    Value* val = stack.back(); stack.pop_back();
    // 看是否要cast
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
    case wabt::Opcode::I32Load8S:
    case wabt::Opcode::I32Load8U:
    case wabt::Opcode::I64Load8S:
    case wabt::Opcode::I64Load8U:
        targetType = Type::getInt8Ty(llvmContext);
    case wabt::Opcode::I32Load16S:
    case wabt::Opcode::I32Load16U:
    case wabt::Opcode::I64Load16S:
    case wabt::Opcode::I64Load16U:
        targetType = Type::getInt16Ty(llvmContext);
    case wabt::Opcode::I64Load32S:
    case wabt::Opcode::I64Load32U:
        targetType = Type::getInt32Ty(llvmContext);
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

void BlockContext::visitBinaryInst(wabt::BinaryExpr* expr) {
    using namespace llvm;
    Value *ret = nullptr, *p1, *p2;
    switch (expr->opcode)
    {
    case wabt::Opcode::I32Add:
    case wabt::Opcode::I64Add:
    case wabt::Opcode::F32Add:
    case wabt::Opcode::F64Add:
        assert(stack.size() >= 2);
        p1 = stack.back(); stack.pop_back();
        p2 = stack.back(); stack.pop_back();
        ret = irBuilder.CreateAdd(p1, p2);
        /* code */
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