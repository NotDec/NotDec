#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <set>
using namespace llvm;
// TOOD 1.如果offset是重新计算的，LLVM value不对应，但是运行时对应 （需要SAT？）
//      2.测试点55不通过:  回溯到参数，变成了一个过程间分析问题
namespace notdec::frontend::optimizers {

struct stack : PassInfoMixin<stack> {


    //判断一个Value是否与栈指针有关
    bool backtraceVar(Value* v, std::set<Value*>& realtedValue,std::set<Value*>& realtedVariable) {
        if(!isa<Instruction>(v)) //如果不是Inst 那就是参数
            return false;
        Instruction *i = dyn_cast<Instruction>(v);
        errs() << "backtrace " << *i << "\n";
        if (i->isBinaryOp()) {
            Value* op1 = i->getOperand(0);
            Value* op2 = i->getOperand(1);
            if (realtedValue.find(op1) != realtedValue.end() ||
                realtedValue.find(op2) != realtedValue.end()) {
                realtedValue.insert(i);
                return true;
            } else
                return (isa<Constant>(op1)?false:backtraceVar(op1,realtedValue,realtedVariable)) ||
                       (isa<Constant>(op2)?false:backtraceVar(op2,realtedValue,realtedVariable));
        } else if (i->isUnaryOp() || i->getOpcode() == Instruction::Load || i->getOpcode() == Instruction::BitCast) {
            Value* op = i->getOperand(0);
            if(isa<ConstantInt>(op))
                return false;
            if (realtedValue.find(op) != realtedValue.end()) {
                realtedValue.insert(i);
                return true;
            } else
                return backtraceVar(op, realtedValue,realtedVariable);
        } else if(i->getOpcode() == Instruction::GetElementPtr){ //只有两种可能 从栈中取或从mem取
            Value* op = i->getOperand(2);
            if(realtedVariable.find(op) != realtedVariable.end())
                return true;
        }
        else 
            errs() << "[-] unhandled instruction " << *i << "\n";
        return false;
    }


    PreservedAnalyses run(Function& F, FunctionAnalysisManager&) {
        // init utils
        std::set<Value*> realtedValue;      //LLVM IR中的Value
        std::set<Value*> realtedVariable;   //实际栈上的变量 ,存的是caloffset对应的Value
        BasicBlock* entry = &F.getEntryBlock();
        IRBuilder<> builder(F.getContext());
        builder.SetInsertPoint(entry->getFirstNonPHI());
        std::vector<Instruction*> removeList;

        // find sp Value and stacksize
        GlobalValue* gStackPointer =
            F.getParent()->getNamedValue("$__stack_pointer");
        Value *sp=nullptr, *stackSize=nullptr;
        for (User* u : gStackPointer->users()) {
            Instruction* inst = dyn_cast<Instruction>(u);
            if (inst->getFunction() == &F) {
                removeList.push_back(inst);
                for(User* u:inst->users()){
                    inst = dyn_cast<Instruction>(u);
                    if (inst->getOpcode() == Instruction::Sub) {
                        stackSize = inst->getOperand(1);
                        sp = inst;
                        break;
                    }
                }
            }
            
        }
        if (stackSize == nullptr) {
            errs() << "[-] sp not found in function " << F.getName() << "\n";
            return PreservedAnalyses::all();
        }
        else
            errs() << "[+] sp found in function " << F.getName()  <<"size: " << *stackSize << "\n";

        //create newsp
        ArrayType* stackType =  ArrayType::get(Type::getInt8Ty(F.getContext()),
                           dyn_cast<ConstantInt>(stackSize)->getZExtValue());
        Value* newStack = builder.CreateAlloca(stackType,nullptr,"newStack");
        Value *zeroSP = builder.CreateAlloca(Type::getInt32Ty(F.getContext()),nullptr,"newSP_buf");
        builder.CreateStore(ConstantInt::get(Type::getInt32Ty(F.getContext()),0),zeroSP);
        Value *newSP = builder.CreateLoad(Type::getInt32Ty(F.getContext()),zeroSP,"newSP");

        //replace sp
        sp->replaceAllUsesWith(newSP);
        realtedValue.insert(newSP);

        //recursively remove all user of  $__stack_pointer https://stackoverflow.com/questions/34715947/delete-replace-code-from-llvm-ir
        for(Instruction *toRemove : removeList){
            // for(User* u:toRemove->users())
            //     removeList.push_back(dyn_cast<Instruction>(u));
            toRemove->replaceAllUsesWith(UndefValue::get(toRemove->getType()));
            toRemove->eraseFromParent();
        }

        //look for stack access instruction and replace GEP
        for (BasicBlock& BB : F) {
            for (Instruction& inst : BB) {
                errs() << "INS " << inst << "\n";
                if (inst.getName().str().find("calcOffset") != std::string::npos &&
                    backtraceVar(dyn_cast<Value>(&inst),realtedValue,realtedVariable)) {
                        errs() << "[+] found stack access " << inst << "\n";
                        GetElementPtrInst* GEPInst = dyn_cast<GetElementPtrInst>(inst.getNextNode());
                        GEPInst->setOperand(0, newStack);      //mem -> stack
                        GEPInst->setSourceElementType(stackType);

                        //判断栈变量的指针相关性 ，即load的操作数 
                        Instruction *next = GEPInst->getNextNode();
                        if(isa<BitCastInst>(next)) 
                            next = next->getNextNode();
                        if(isa<StoreInst>(next)){
                            Value *op = next->getOperand(0);
                            if(!isa<Constant>(op) && backtraceVar(op,realtedValue,realtedVariable)) 
                                realtedVariable.insert(&inst);
                            else if(realtedVariable.find(&inst) != realtedVariable.end()) //store的不是栈指针相关的值 如果在集合里，进行移除
                                realtedVariable.erase(&inst);

                        }

                }
            }
        }

        return PreservedAnalyses::all();
    }
};
}  // namespace notdec::frontend::optimizers
