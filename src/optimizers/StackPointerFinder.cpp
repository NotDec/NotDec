#include "optimizers/StackPointerFinder.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

namespace notdec {

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
  Instruction *add;

  auto pat_alloc = StackPointerMatcher(sp_val, size_val, load, add);

  bool matched = false;
  for (Instruction &I : entryBlock) {
    if (PatternMatch::match(&I, pat_alloc)) {
      sp = dyn_cast<GlobalVariable>(sp_val);
      matched = true;
      break;
    }
  }
  if (!matched) {
    return nullptr;
  }
  // 0 for negative, 1 for positive.
  bool direction = 0;
  if (auto constant = dyn_cast<ConstantInt>(size_val)) {
    if (constant->getSExtValue() > 0) {
      direction = 1;
    } else {
      direction = 0;
    }
    direction_count[direction]++;
  }
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

const char *StackPointerFinderAnalysis::StackPointerNames[] = {
    "__stack_pointer", "env.__stack_pointer"};

static inline bool isStackPointerName(const StringRef &Name) {
  for (const char *Str : StackPointerFinderAnalysis::StackPointerNames) {
    if (Name == Str) {
      return true;
    }
  }
  return false;
}

StackPointerFinderAnalysis::Result
StackPointerFinderAnalysis::run(llvm::Module &mod) {
  GlobalVariable *sp = nullptr;
  for (GlobalVariable &gv : mod.getGlobalList()) {
    if (isStackPointerName(gv.getName())) {
      sp = &gv;
    }
  }
  GlobalVariable *max_sp = nullptr;
  for (Function &f : mod) {
    if (GlobalVariable *gv = find_stack_ptr(f)) {
      sp_count[gv]++;
    }
  }
  size_t max = 0;
  std::cerr << "Try to guess stack pointer:" << std::endl;
  for (auto pair : sp_count) {
    llvm::errs() << *pair.first << "(score: " << pair.second << ")\n";
    if (pair.second > max) {
      max = pair.second;
      max_sp = pair.first;
    }
  }
  if (max_sp != nullptr) {
    llvm::errs() << "Selected stack pointer: " << *max_sp << "\n";
  }
  if (sp != nullptr) {
    errs() << "Select stack pointer because of its NAME: " << *sp << "\n";
    if (sp != max_sp && max_sp != nullptr) {
      errs() << "WARNING: Stack pointer mismatch! (Name vs Analysis)\n";
    }
  } else {
    // find the most voted stack pointer.
    sp = max_sp;
  }

  Result ret;
  ret.result = sp;
  ret.direction = direction_count[0] >= direction_count[1] ? 0 : 1;
  std::cerr << "stack direction: "
            << (ret.direction == 0 ? "negative" : "positive") << " ("
            << direction_count[ret.direction] << ")" << std::endl;
  if (direction_count[ret.direction] == 0) {
    errs()
        << "WARNING: Stack direction is not determined! Default to negative.\n";
  }
  return ret;
}

} // namespace notdec
