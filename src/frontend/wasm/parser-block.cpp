
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
            default:
                std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported expr type: " << GetExprTypeName(expr) << std::endl;
                // really abort?
                // std::abort();
        }
    }
}

void BlockContext::visitBinaryInst(wabt::BinaryExpr* expr) {
    switch (expr->opcode)
    {
    case wabt::Opcode::I32Add:
        /* code */
        break;
    
    default:
        break;
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