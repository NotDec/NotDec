
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include "Passes/StackPointerFinder.h"
#include "Utils/Utils.h"
#include "notdec-llvm2c/Utils.h"

namespace notdec {

using namespace llvm;

void addSPMetadata(GlobalVariable *GV) {
  LLVMContext &Context = GV->getContext();
  MDString *SizeMD = MDString::get(Context, "true");
  MDNode *Node = MDNode::get(Context, {SizeMD});
  GV->setMetadata(llvm2c::KIND_STACK_POINTER, Node);
}

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
  notdec::appendRecoveryPassLog(
      " ============== StackPointerFinderAnalysis ===============\n");
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
  notdec::appendRecoveryPassLog("Try to guess stack pointer:\n");
  for (auto pair : sp_count) {
    std::string Message;
    llvm::raw_string_ostream OS(Message);
    OS << *pair.first << "(score: " << pair.second << ")\n";
    notdec::appendRecoveryPassLog(OS.str());
    if (pair.second > max) {
      max = pair.second;
      max_sp = pair.first;
    }
  }
  if (max_sp != nullptr) {
    std::string Message;
    llvm::raw_string_ostream OS(Message);
    OS << "Selected stack pointer: " << *max_sp << "\n";
    notdec::appendRecoveryPassLog(OS.str());
  }
  if (sp != nullptr) {
    std::string Message;
    llvm::raw_string_ostream OS(Message);
    OS << "Select stack pointer because of its NAME: " << *sp << "\n";
    notdec::appendRecoveryPassLog(OS.str());
    if (sp != max_sp && max_sp != nullptr) {
      notdec::appendRecoveryPassLog(
          "WARNING: Stack pointer mismatch! (Name vs Analysis)\n");
    }
  } else {
    // find the most voted stack pointer.
    sp = max_sp;
  }

  if (sp != nullptr) {
    addSPMetadata(sp);
  }

  Result ret;
  ret.result = sp;
  ret.direction = direction_count[0] >= direction_count[1] ? 0 : 1;
  std::string DirectionMessage =
      "stack direction: " +
      std::string(ret.direction == 0 ? "negative" : "positive") + " (" +
      std::to_string(direction_count[ret.direction]) + ")\n";
  notdec::appendRecoveryPassLog(DirectionMessage);
  if (direction_count[ret.direction] == 0) {
    notdec::appendRecoveryPassLog(
        "WARNING: Stack direction is not determined! Default to negative.\n");
  }
  return ret;
}

} // namespace notdec
