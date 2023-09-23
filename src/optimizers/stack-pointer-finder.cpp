#include "optimizers/stack-pointer-finder.h"
#include <llvm/IR/GlobalVariable.h>

namespace notdec::optimizers {

using namespace llvm;

/* 寻找栈指针
1.加载栈指针存在局部变量读出再加减的情况 所以在去除局部变量后进行
2.wasm中不一定存在global.set 全局栈指针
*/
GlobalVariable *find_stack_ptr(BasicBlock &entryBlock) {
  // assert(enrtyBlock.isEntryBlock());
  GlobalVariable *sp = nullptr;
  Instruction *load = nullptr;
  for (Instruction &I : entryBlock) {
    if (StoreInst *store = dyn_cast<StoreInst>(&I)) {
      return nullptr;
    }
    // 第一个内存访问的load指令
    if ((load = dyn_cast<LoadInst>(&I))) {
      if ((sp = dyn_cast<GlobalVariable>(load->getOperand(0)))) {
        // bool maybeStackPtr = true;
        // for(Value* next : load->users()){  //遍历后继指令
        //   Instruction *nextInst = dyn_cast<Instruction>(next);
        //   if (!(nextInst->getOpcode() == Instruction::Add ||
        //         nextInst->getOpcode() == Instruction::Sub ||
        //         nextInst->getOpcode() == Instruction::Store)) {
        //     maybeStackPtr = false;
        //     break;
        //   }
        // }
        // if(!maybeStackPtr)
        //   sp = nullptr;
      }
      break; // 只判断第一条load指令
    }
  }
  if (sp == nullptr) {
    return nullptr;
  }
  // 判断是否有分配栈空间的行为。
  // 1. 有add/sub行为
  Instruction *sub = nullptr;
  for (Value *next : load->users()) { // 遍历后继指令
    if (Instruction *nextInst = dyn_cast<Instruction>(next)) {
      if (nextInst->getParent() != &entryBlock) {
        continue;
      }
      if (nextInst->getOpcode() == Instruction::Add ||
          nextInst->getOpcode() == Instruction::Sub) {
        sub = nextInst;
        break;
      }
    }
  }
  if (sub == nullptr) {
    return nullptr;
  }
  // 2. 有store行为
  Instruction *store = nullptr;
  for (Value *next : load->users()) { // 遍历后继指令
    if (Instruction *nextInst = dyn_cast<Instruction>(next)) {
      if (nextInst->getParent() != &entryBlock) {
        continue;
      }
      if (nextInst->getOpcode() == Instruction::Store) {
        store = nextInst;
      }
    }
  }
  if (store == nullptr) {
    return nullptr;
  }
  if (store->getOperand(0) != sub || store->getOperand(1) != sp) {
    return nullptr;
  }
  return sp;
}

GlobalVariable *find_stack_ptr(Function &f) {
  if (!f.empty())
    return find_stack_ptr(f.getEntryBlock());
  return nullptr;
}

GlobalVariable *find_stack_ptr(Module &mod) {
  GlobalVariable *sp = nullptr;
  for (GlobalVariable &gv : mod.getGlobalList()) {
    if (gv.getName().equals("__stack_pointer")) {
      sp = &gv;
    }
  }
  if (sp == nullptr) {
    std::map<GlobalVariable *, size_t> sp_count;
    for (Function &f : mod) {
      if (GlobalVariable *gv = find_stack_ptr(f)) {
        sp_count[gv]++;
      }
    }
    std::cerr << "Try to guess stack pointer:" << std::endl;
    size_t max = 0;
    GlobalVariable *max_sp = nullptr;
    for (auto pair : sp_count) {
      llvm::errs() << *pair.first << ": " << pair.second << "\n";
      if (pair.second > max) {
        max = pair.second;
        max_sp = pair.first;
      }
    }
    llvm::errs() << "Selected stack pointer: " << *max_sp << "\n";
    sp = max_sp;
  }
  return sp;
}

} // namespace notdec::optimizers
