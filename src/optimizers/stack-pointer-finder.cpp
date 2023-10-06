#include "optimizers/stack-pointer-finder.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Support/raw_ostream.h>

namespace notdec::optimizers {

using namespace llvm;

// Commutable add matcher
template <typename LHS, typename RHS>
inline llvm::PatternMatch::BinaryOp_match<LHS, RHS, Instruction::Add, true>
m_Add_Comm(const LHS &L, const RHS &R) {
  return llvm::PatternMatch::BinaryOp_match<LHS, RHS, Instruction::Add, true>(
      L, R);
}

// store (add/sub (load sp) num) sp
/* 寻找栈指针
1.加载栈指针存在局部变量读出再加减的情况 所以在去除局部变量后进行
2.wasm中不一定存在global.set 全局栈指针
*/
GlobalVariable *
StackPointerFinderAnalysis::find_stack_ptr(BasicBlock &entryBlock) {
  // assert(enrtyBlock.isEntryBlock());
  GlobalVariable *sp = nullptr;
  Instruction *load = nullptr;
  using namespace llvm::PatternMatch;
  Value *sp_val;
  Value *size_val;
  auto pat_alloc_sub = m_Store(
      m_Sub(m_Load(m_Value(size_val)), m_Value(sp_val)), m_Deferred(sp_val));
  auto pat_alloc_add =
      m_Store(m_Add_Comm(m_Load(m_Value(size_val)), m_Value(sp_val)),
              m_Deferred(sp_val));

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
  // bool direction = sub.
  return sp;
}

GlobalVariable *StackPointerFinderAnalysis::find_stack_ptr(Function &f) {
  if (!f.empty())
    return find_stack_ptr(f.getEntryBlock());
  return nullptr;
}

GlobalVariable *StackPointerFinderAnalysis::find_stack_ptr(Module &mod) {
  StackPointerFinderAnalysis ana;
  return ana.run(mod).result;
}

StackPointerFinderAnalysis::Result
StackPointerFinderAnalysis::run(llvm::Module &mod) {
  GlobalVariable *sp = nullptr;
  for (GlobalVariable &gv : mod.getGlobalList()) {
    if (gv.getName().equals("__stack_pointer")) {
      errs() << "Select stack pointer because of its name: " << gv << "\n";
      sp = &gv;
    }
  }
  if (sp == nullptr) {
    for (Function &f : mod) {
      if (GlobalVariable *gv = find_stack_ptr(f)) {
        sp_count[gv]++;
      }
    }
    std::cerr << "Try to guess stack pointer:" << std::endl;
    // find the most voted stack pointer.
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

  Result ret;
  ret.result = sp;
  return ret;
}

} // namespace notdec::optimizers
