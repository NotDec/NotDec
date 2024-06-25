#include "optimizers/stack-alloca.h"
#include "optimizers/stack-pointer-finder.h"
#include <cassert>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PatternMatch.h>
#include <type_traits>

namespace notdec {

/// Uses ptrtoint to ensure type correctness.
PreservedAnalyses LinearAllocationRecovery::run(Module &M,
                                                ModuleAnalysisManager &MAM) {
  errs() << " ============== LinearAllocationRecovery  ===============\n";
  auto sp_result = MAM.getResult<StackPointerFinderAnalysis>(M);
  auto sp = sp_result.result;
  if (sp == nullptr) {
    std::cerr << "ERROR: Stack pointer is not found!!";
    std::cerr << "LinearAllocationRecovery cannot proceed!\n";
    return PreservedAnalyses::all();
  }
  // iterate each use of sp, collect a list of functions to process.
  std::set<Function *> worklist;
  for (auto U : sp->users()) {
    if (Instruction *I = dyn_cast<Instruction>(U)) {
      auto *F = I->getFunction();
      worklist.emplace(F);
    }
  }

  if (worklist.empty()) {
    std::cerr << "ERROR: The stack pointer is not used in any function?";
    std::cerr << "LinearAllocationRecovery cannot proceed!\n";
    return PreservedAnalyses::all();
  }

  using namespace llvm::PatternMatch;
  Value *sp1 = nullptr;
  Instruction *prev_sp = nullptr;
  Value *space;
  Instruction *prologue_add = nullptr;
  // rely on canonical form of LLVM Instructions, So run instcombine first.
  // see:
  // https://www.npopov.com/2023/04/10/LLVM-Canonicalization-and-target-independence.html

  auto pat_alloc = StackPointerMatcher(sp1, space, prev_sp, prologue_add, sp);

  // 1. find the stack allocation in entry
  // 2. remove stack pointer restoration.
  // 3. replace allocation with alloca
  // 3.1 according to the grow direction
  Instruction *prologue_store = nullptr;
  IRBuilder builder(M.getContext());
  for (auto *F : worklist) {
    bool matched = false;
    auto *entry = &F->getEntryBlock();
    // use llvm PatternMatch to match in the entry block.
    for (auto &I : *entry) {
      if (PatternMatch::match(&I, pat_alloc)) {
        prologue_store = &I;
        matched = true;
        // llvm::errs() << "stack alloc: " << *space << "\n";
        break;
      }
    }
    if (!matched) {
      llvm::errs()
          << "ERROR: cannot find stack allocation in entry block! func: "
          << F->getName() << "\n";
      continue;
    }
    assert(prologue_add != nullptr && space != nullptr);
    // find epilogue in exit blocks, and remove it.
    auto pat_restore = m_Store(m_Deferred(prev_sp), m_Specific(sp));
    bool removed = false;
    for (auto &BB : *F) {
      if (BB.getTerminator()->getNumSuccessors() == 0) {
        for (auto &I : make_early_inc_range(BB)) {
          if (PatternMatch::match(&I, pat_restore)) {
            // llvm::errs() << "recover sp: " << I << "\n";
            I.eraseFromParent();
            removed = true;
            break;
          }
        }
      }
    }
    if (!removed) {
      llvm::errs() << "ERROR: Cannot find sp restore? func: " << F->getName()
                   << "\n";
      continue;
    }

    // replace the stack allocation with alloca.
    bool grow_negative = sp_result.direction == 0;
    auto pty = PointerType::get(IntegerType::get(M.getContext(), 8), 0);
    assert(prologue_add->getParent() == prev_sp->getParent());

    if (grow_negative) {
      space = builder.CreateNeg(space);
    }
    builder.SetInsertPoint(prologue_add);
    Value *alloc =
        builder.CreateAlloca(pty->getPointerElementType(), space, "stack");
    Value *alloc_end = builder.CreateGEP(pty->getPointerElementType(), alloc,
                                         space, "stack_end");
    alloc = builder.CreatePtrToInt(alloc, prev_sp->getType());
    alloc_end = builder.CreatePtrToInt(alloc_end, prev_sp->getType());

    Instruction *high_addr = prologue_add;
    Instruction *low_addr = prev_sp;
    if (grow_negative) {
      std::swap(high_addr, low_addr);
    }
    // replace all uses of prev_sp with alloc_end.
    low_addr->replaceAllUsesWith(alloc);
    low_addr->eraseFromParent();
    high_addr->replaceAllUsesWith(alloc_end);
    high_addr->eraseFromParent();
    prologue_store->eraseFromParent();
  }
  // perform the transformation:

  return PreservedAnalyses::none();
}
} // namespace notdec
