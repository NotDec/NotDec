
#include "frontend/wasm/parser-block.h"
#include "wabt/ir.h"
#include <cassert>
#include <cstdlib>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>

namespace notdec::frontend::wasm {

void BlockContext::visitBlock(wabt::LabelType lty, llvm::BasicBlock* entry, llvm::BasicBlock* exit, wabt::BlockDeclaration& decl, wabt::ExprList& exprs) {
    // create Phi for block type
    using namespace wabt;
    std::deque<llvm::PHINode*> phis;
    llvm::BasicBlock* breakTo;

    if (log_level >= level_debug) {
        std::cerr << "Debug: Visiting Block with label type " << labelTypeToString(lty) << std::endl;
    }

    // true for block, false for loop
    bool isBlockLike = true;
    switch (lty) {
        case LabelType::Block:
        case LabelType::Func:
        case LabelType::If:
            assert(exit->getFirstNonPHI() == nullptr);
            // 把参数留在栈上
            // 为基本块返回值创建Phi
            irBuilder.SetInsertPoint(exit, exit->begin());
            // 因为是push_back，所以是顺序遍历的。
            for (wabt::Index i = 0; i < decl.GetNumResults(); i++) {
                llvm::PHINode* phi = irBuilder.CreatePHI(convertType(llvmContext, decl.GetResultType(i)), 0, exit->getName() + "_" + std::to_string(i));
                phis.push_back(phi);
            }
        case LabelType::Else:
            isBlockLike = true;
            // 有跳出的直接跳到exit
            breakTo = exit;

            irBuilder.SetInsertPoint(entry);
            break;
        case LabelType::Loop:
            assert(entry->getFirstNonPHI() == nullptr);
            // 为基本块参数创建Phi，放到那边BreakoutTarget里。
            // 为参数创建Phi
            irBuilder.SetInsertPoint(entry, entry->begin());
            // https://stackoverflow.com/questions/5458204/unsigned-int-reverse-iteration-with-for-loops
            for (wabt::Index i = decl.GetNumParams(); i-- > 0 ;) {
                llvm::PHINode* phi = irBuilder.CreatePHI(convertType(llvmContext, decl.GetParamType(i)), 0, entry->getName() + "_" + std::to_string(i));
                phis.push_front(phi);
                // 给phi赋值，然后用Phi替换栈上值
                phi->addIncoming(popStack(), entry);
            }
            // 把栈上参数转换为Phi
            for (auto phi: phis) {
                stack.push_back(phi);
            }
            isBlockLike = false;
            // 有跳出的直接跳到entry
            breakTo = entry;
            break;
        case LabelType::Try:
        case LabelType::InitExpr:
        case LabelType::Catch:
        default:
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: unexpected LabelType: " << labelTypeToString(lty) << std::endl;
            std::abort();
    }
    // 标记当前栈的位置，结束的时候先读取返回值然后再弹出到这个位置。
    std::size_t stack_pos = lty == LabelType::Func ? stack.size() : stack.size() - decl.GetNumParams();
    if (lty != LabelType::Else) {
        blockStack.emplace_back(*breakTo, phis, stack_pos, decl, lty);
    } else { // restore previous phis
        stack_pos = blockStack.back().pos;
        phis = blockStack.back().phis;
    }
    // 依次遍历每个指令，同时处理栈的变化。
    visitControlInsts(entry, exit, exprs);
    if (lty != LabelType::If) {
        assert(blockStack.size() >= 0); blockStack.pop_back();
    }
    if (irBuilder.GetInsertBlock() != nullptr) { // not unreachable, handle implicit return
        if (stack.size() != (stack_pos + decl.GetNumResults())) {
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Not enough value on stack" << std::endl;
            std::cerr << decl.GetNumResults() << " expected, but got " << (stack.size() - stack_pos) << std::endl;
            std::abort();
        }
        assert(stack.size() >= phis.size());
        // 栈上值和Phi的转换，需要和创建的br一起。
        if (isBlockLike) {
            for (auto it = phis.rbegin(); it != phis.rend(); ++it) {
                (*it)->addIncoming(stack.back(), irBuilder.GetInsertBlock()); stack.pop_back();
            }
            if (lty != LabelType::If) {
                for (auto phi: phis) {
                    stack.push_back(phi);
                }
            }
            irBuilder.CreateBr(exit); // only for block, not for Loop
        }
    } else if (isBlockLike) { // terminate unreachable state if block
        irBuilder.SetInsertPoint(exit, getFirstNonPHIOrDbgOrLifetime(exit));
        // 把栈unwind到原本的位置
        if (stack.size() > stack_pos) {
            if (log_level >= level_info) {
                std::cerr << "Info: Discard " << stack.size() - stack_pos << " unreachable values." << std::endl;
            }
            while(stack.size() > stack_pos) {
                stack.pop_back();
            }
        }
        if (stack.size() < stack_pos) {
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Not enough values!" << std::endl;
            std::abort();
        }
        // push 返回值
        for (auto phi: phis) {
            stack.push_back(phi);
        }
    } // keep unreachable state if loop
}

void BlockContext::unwindStackTo(std::size_t stack_pos) {
    if (stack.size() > stack_pos) {
        if (log_level >= level_debug) {
            std::cerr << "Debug: Unwind " << stack.size() - stack_pos << " values." << std::endl;
        }
        while(stack.size() > stack_pos) {
            stack.pop_back();
        }
    }
    if (stack.size() < stack_pos) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Not enough values!" << std::endl;
        std::abort();
    }
}

llvm::BasicBlock::iterator getFirstNonPHIOrDbgOrLifetime(llvm::BasicBlock* bb) {
    auto entryInst = bb->getFirstNonPHIOrDbgOrLifetime();
    llvm::BasicBlock::iterator it;
    if (entryInst != nullptr) {
        it = entryInst->getIterator();
    } else {
        it = bb->begin();
    }
    return it;
}

void BlockContext::visitControlInsts(llvm::BasicBlock* entry, llvm::BasicBlock* exit, wabt::ExprList& exprs) {
    for (wabt::Expr& expr : exprs) { // 遍历每个指令的主要循环
        // llvm::StringRef entName = entry->getName();
        // llvm::StringRef exitName = exit->getName();
        // llvm::BasicBlock* bb = irBuilder.GetInsertBlock();
        if (log_level >= level_debug) {
            std::cerr << "Debug: Visiting expr " << wabt::GetExprTypeName(expr);
            if (isContainBlock(expr)) {
                wabt::Block& b = getBlock(expr);
                if (!b.label.empty()) {
                    std::cerr << " " << b.label;
                }
            }
            if (expr.loc.line != 0) {
                std::cerr << ", at "<< expr.loc.filename << ":" << expr.loc.line;
            }
            std::cerr << std::endl;
        }
        if (irBuilder.GetInsertBlock() == nullptr) { // unreachable place after br
            if (log_level >= level_warning) {
                std::cerr << "Warning: Skipping unreachable expr " << wabt::GetExprTypeName(expr);
                if (expr.loc.line != 0) {
                    std::cerr << ", at line " << expr.loc.line;
                }
                std::cerr << std::endl;
            }
            continue;
        }
        
        switch (expr.type()) {
        case wabt::ExprType::Block: {
            using namespace llvm;
            auto e = wabt::cast<wabt::BlockExpr>(&expr);
            // 创建新的基本块作为entry，exit
            // BasicBlock* entryBlock = llvm::BasicBlock::Create(llvmContext, "blkentry", &function);
            BasicBlock* exitBlock = llvm::BasicBlock::Create(llvmContext, "blk_exit", &function);
            visitBlock(wabt::LabelType::Block, entry, exitBlock, e->block.decl, e->block.exprs);
            entry = exitBlock; irBuilder.SetInsertPoint(entry);
            break;
        }
        case wabt::ExprType::Loop: {
            using namespace llvm;
            auto e = wabt::cast<wabt::LoopExpr>(&expr);
            BasicBlock* entryBlock = llvm::BasicBlock::Create(llvmContext, "loop_entry", &function);
            irBuilder.CreateBr(entryBlock);
            entry = entryBlock; irBuilder.SetInsertPoint(entry);
            visitBlock(wabt::LabelType::Loop, entryBlock, exit, e->block.decl, e->block.exprs);
            entry = irBuilder.GetInsertBlock();
            break;
        }
        case wabt::ExprType::If: {
            using namespace llvm;
            auto e = wabt::cast<wabt::IfExpr>(&expr);
            std::vector<llvm::Value*> params;
            // boolean arg
            assert(stack.size() >= 1);
            Value* p1 = stack.back(); stack.pop_back();
            // save other args
            assert(stack.size() >= e->true_.decl.GetNumParams());
            std::size_t start_ind = stack.size() - e->true_.decl.GetNumParams();
            for (std::size_t i=start_ind;i < stack.size(); i++) {
                params.push_back(stack.at(i));
            }
            // convert to i1
            p1 = irBuilder.CreateICmpNE(p1, ConstantInt::getNullValue(p1->getType()), "brif_val");
            BasicBlock* ifBlock = BasicBlock::Create(llvmContext, "ifblk", &function);
            BasicBlock* elseBlock = BasicBlock::Create(llvmContext, "elseblk", &function);
            BasicBlock* exitBlock = BasicBlock::Create(llvmContext, "ifblk_exit", &function);
            irBuilder.CreateCondBr(p1, ifBlock, elseBlock);

            entry = ifBlock; irBuilder.SetInsertPoint(entry);
            visitBlock(wabt::LabelType::If, entry, exitBlock, e->true_.decl, e->true_.exprs);
            // duplicate parameter
            for (std::size_t i=0;i<params.size();i++) {
                stack.push_back(params.at(i));
            }
            entry = elseBlock; irBuilder.SetInsertPoint(entry);
            visitBlock(wabt::LabelType::Else, entry, exitBlock, e->true_.decl, e->false_);
            entry = exitBlock; irBuilder.SetInsertPoint(entry);
            break;
        }
        case wabt::ExprType::BrIf: {
            using namespace llvm;
            auto e = wabt::cast<wabt::BrIfExpr>(&expr);
            std::size_t brind = (e->var.index() + 1);
            // boolean arg
            Value* p1 = popStack();
            // convert to i1
            p1 = irBuilder.CreateICmpNE(p1, ConstantInt::getNullValue(p1->getType()), "brif_val");
            BasicBlock* nextBlock = llvm::BasicBlock::Create(llvmContext, "brif_next", &function);
            assert(blockStack.size() >= brind);
            visitBr(e, blockStack.size() - brind, p1, nextBlock);
            entry = nextBlock; irBuilder.SetInsertPoint(entry);
            break;
        }
        case wabt::ExprType::Br: {
            // index里面有assert。
            // add 1 for negative index
            std::size_t brind = (wabt::cast<wabt::BrExpr>(&expr)->var.index() + 1);
            assert(blockStack.size() >= brind); // 防止下溢
            visitBr(&expr, blockStack.size() - brind, nullptr, nullptr);
            break;
        }
        case wabt::ExprType::BrTable: {
            using namespace llvm;
            wabt::BrTableExpr* brt = wabt::cast<wabt::BrTableExpr>(&expr);
            // 默认target
            std::size_t defInd = (brt->default_target.index() + 1);
            Value* p1 = popStack();
            SwitchInst* si = cast<SwitchInst>(visitBr(brt, blockStack.size() - defInd, p1, nullptr));
            // 其他target
            for (wabt::Index i=0;i<brt->targets.size();i++) {
                std::size_t ind = (brt->targets.at(i).index() + 1);
                BreakoutTarget& bt = blockStack.at(blockStack.size() - ind);
                si->addCase(ConstantInt::get(Type::getInt32Ty(llvmContext), i), &bt.target);
                auto stackIt = stack.rbegin();
                for (auto it = bt.phis.rbegin(); it != bt.phis.rend(); ++it, ++stackIt) {
                    if (stackIt == stack.rend()){
                        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Not enough value for br_table ind " << i << " level " << brt->targets.at(i).index() << std::endl;
                        std::cerr << "Note: Jump target lable " << labelTypeToString(bt.lty) << " param num " << bt.sig.GetNumParams() << " result num " << bt.sig.GetNumResults() << std::endl;
                        std::abort();
                    }
                    (*it)->addIncoming((*stackIt), irBuilder.GetInsertBlock());
                }
            }
            break;
        }
        case wabt::ExprType::Return:
            visitBr(&expr, 0, nullptr, nullptr);
            break;
        default:
            dispatchExprs(expr);
            break;
        }
    }
}

llvm::Instruction* BlockContext::visitBr(wabt::Expr* expr, std::size_t ind, llvm::Value* cond, llvm::BasicBlock* nextBlock) {
    llvm::Instruction* ret;
    BreakoutTarget& bt = blockStack.at(ind);
    if (ind == 0) {
        // 等价于直接跳转出最外面的函数体block
        assert(bt.lty == wabt::LabelType::Func);
    }
    // 返回值放到Phi里
    assert(stack.size() >= bt.phis.size());
    auto stackIt = stack.rbegin();
    for (auto it = bt.phis.rbegin(); it != bt.phis.rend(); ++it, ++stackIt) {
        (*it)->addIncoming((*stackIt), irBuilder.GetInsertBlock());
    }
    if (cond == nullptr && nextBlock == nullptr) {
        assert(expr->type() == wabt::ExprType::Br || expr->type() == wabt::ExprType::Return);
        ret = irBuilder.CreateBr(&bt.target);
        irBuilder.ClearInsertionPoint(); // mark unreachable
    } else if (cond != nullptr && nextBlock != nullptr) {
        assert(expr->type() == wabt::ExprType::BrIf);
        ret = irBuilder.CreateCondBr(cond, &bt.target, nextBlock);
    } else if (cond != nullptr && nextBlock == nullptr) {
        assert(expr->type() == wabt::ExprType::BrTable);
        ret = irBuilder.CreateSwitch(cond, &bt.target);
        irBuilder.ClearInsertionPoint(); // mark unreachable
    } else {
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unexpected call to BlockContext::visitBr: " << std::endl;
        std::abort();
    }
    return ret;
}

bool isContainBlock(wabt::Expr& expr) {
    switch (expr.type()) {
    case wabt::ExprType::Block:
    case wabt::ExprType::Loop:
    case wabt::ExprType::If:
        return true;
    default:
        return false;
    }
}

wabt::Block& getBlock(wabt::Expr& expr) {
    switch (expr.type()) {
    case wabt::ExprType::Block:
        return wabt::cast<wabt::BlockExpr>(&expr)->block;
    case wabt::ExprType::Loop:
        return wabt::cast<wabt::LoopExpr>(&expr)->block;
    case wabt::ExprType::If:
        return wabt::cast<wabt::IfExpr>(&expr)->true_;
    default:
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Unsupported expr type for `getBlock`: " << GetExprTypeName(expr) << std::endl;
        std::abort();
    }
}

bool checkBlockLike(wabt::LabelType lty) {
    using namespace wabt;
    switch (lty) {
        case LabelType::Block:
        case LabelType::Func:
            return true;
        case LabelType::Loop:
            return false;
        case LabelType::If:
        case LabelType::Else:
            return true;
        case LabelType::Try:
        case LabelType::InitExpr:
        case LabelType::Catch:
        default:
            std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: unexpected LabelType: " << labelTypeToString(lty) << std::endl;
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