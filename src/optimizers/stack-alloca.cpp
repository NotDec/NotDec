#include "optimizers/stack-alloca.h"
#include "optimizers/stack-pointer-finder.h"
#include <cassert>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PatternMatch.h>

namespace notdec::optimizers {
PreservedAnalyses LinearAllocationRecovery::run(Module &M,
                                                ModuleAnalysisManager &MAM) {
  errs() << "(llvm-tutor) Hello Module: " << M.getName() << "\n";
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
  Value *prev_sp = nullptr;
  Value *space;
  // rely on canonical form of LLVM Instructions, So run instcombine first.
  // see:
  // https://www.npopov.com/2023/04/10/LLVM-Canonicalization-and-target-independence.html

  auto pat_alloc1 =
      m_Store(m_Add(m_Load(m_Specific(sp)), m_Value(space)), m_Specific(sp));
  auto pat_alloc2 =
      m_Store(m_Add(m_Value(space), m_Load(m_Specific(sp))), m_Specific(sp));
  auto pat_alloc_sub =
      m_Store(m_Sub(m_Load(m_Specific(sp)), m_Value(space)), m_Specific(sp));
  auto pat_restore = m_Store(m_Deferred(prev_sp), m_Specific(sp));

  // 1. find the stack allocation in entry
  // 2. remove stack pointer restoration.
  // 3. replace allocation with alloca
  // 3.1 according to the grow direction
  for (auto *F : worklist) {
    Instruction *prologue_store = nullptr;
    Instruction *prologue_add = nullptr;

    auto *entry = &F->getEntryBlock();
    // use llvm PatternMatch to match in the entry block.
    for (auto &I : *entry) {
      if (PatternMatch::match(&I, pat_alloc1) ||
          PatternMatch::match(&I, pat_alloc_sub)) {
        prologue_store = &I;
        prologue_add = cast<Instruction>(I.getOperand(0));
        // get the original sp
        prev_sp = prologue_add->getOperand(0);
        // llvm::errs() << "stack alloc: " << *space << "\n";
        break;
      } else if (PatternMatch::match(&I, pat_alloc2)) {
        prologue_store = &I;
        prologue_add = cast<Instruction>(I.getOperand(0));
        // get the original sp
        prev_sp = prologue_add->getOperand(1);
        // llvm::errs() << "stack alloc: " << *space << "\n";
        break;
      }
    }
    if (prev_sp == nullptr) {
      llvm::errs() << "ERROR: cannot find stack allocation for " << F->getName()
                   << "\n";
      continue;
    }
    // find epilogue in exit blocks, and remove it.
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
      llvm::errs() << "ERROR: Cannot find sp restore?: " << F->getName()
                   << "\n";
      continue;
    }
  }
  // perform the transformation:

  return PreservedAnalyses::none();
}
} // namespace notdec::optimizers
