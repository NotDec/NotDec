#include "optimizers/stack-alloca.h"
#include "optimizers/stack-pointer-finder.h"
#include <cassert>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <type_traits>
#include <vector>

namespace notdec {

/// Scan for stack pointer subtraction and convert to alloca.
/// Uses ptrtoint to ensure type correctness.
///
/// Typical case:
/// entry:
/// %0 = load i32, i32* @__stack_pointer, align 4
/// %1 = add i32 %0, -16
/// store i32 %1, i32* @__stack_pointer, align 4 ; update the stack pointer
/// ... ; real function body
/// return_block:
/// store i32 %0, i32* @__stack_pointer, align 4 ; restore the stack pointer
/// ...; probably other instructions
/// ret i32 0
///
/// For tail functions, the update and the restore step can be omitted. and they
/// just address the stack directly.
/// TODO handle tail function with no stack space. two pattern will not match.
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
  Value *sp1;
  Instruction *load_sp;
  Value *space;
  Instruction *add_load_sp;
  ConstantInt *offset;
  // rely on canonical form of LLVM Instructions, So run instcombine first.
  // see:
  // https://www.npopov.com/2023/04/10/LLVM-Canonicalization-and-target-independence.html

  auto pat_alloc = StackPointerMatcher(sp1, space, load_sp, add_load_sp, sp);
  auto pat_alloc_offset = m_Add(m_Load(m_Specific(sp)), m_ConstantInt(offset));

  // 1. match the stack allocation in entry
  // 2. remove stack pointer restoration.
  // 3. replace allocation with alloca
  // 3.1 according to the grow direction
  IRBuilder builder(M.getContext());
  for (auto *F : worklist) {
    sp1 = nullptr;
    load_sp = nullptr;
    space = nullptr;
    add_load_sp = nullptr;
    Instruction *prologue_store = nullptr;
    // level = 2 normal stack allocation.
    // level = 1 tail function stack allocation.
    // level = 0 cannot match stack allocation.
    int match_level = 0;
    auto *entry = &F->getEntryBlock();
    // try to Match normal stack allocation.
    // use llvm PatternMatch to match in the entry block.
    for (auto &I : *entry) {
      if (PatternMatch::match(&I, pat_alloc)) {
        prologue_store = &I;
        match_level = 2;
        // llvm::errs() << "stack alloc: " << *space << "\n";
        break;
      }
    }
    // try to match tail function stack allocation if not matched.
    // there is at least one Add(Load(sp), offset).
    if (match_level == 0) {
      bool has_load_sp = false;
      for (auto &BB : *F) {
        for (auto &I : BB) {
          if (PatternMatch::match(&I, pat_alloc_offset)) {
            add_load_sp = &I;
            load_sp = cast<Instruction>(add_load_sp->getOperand(0));
            match_level = 1;
            // llvm::errs() << "stack alloc: " << *space << "\n";
            break;
          }
          if (PatternMatch::match(&I, m_Load(m_Specific(sp)))) {
            has_load_sp = true;
          }
        }
      }
      if (match_level == 0 && has_load_sp) {
        llvm::errs()
            << "ERROR: No pattern matched but the stack pointer is accessed!\n";
      }
    }
    // failed to match any stack allocation.
    if (match_level == 0) {
      llvm::errs() << "ERROR: cannot find stack allocation in func: "
                   << F->getName() << "\n";
      continue;
    }
    if (match_level == 2) {
      assert(add_load_sp != nullptr && space != nullptr);
      assert(add_load_sp->getParent() == load_sp->getParent());
      // find epilogue in exit blocks, and remove it.
      auto pat_restore = m_Store(m_Deferred(load_sp), m_Specific(sp));
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
    }

    // replace the stack allocation with alloca.
    bool grow_negative = sp_result.direction == 0;
    // TODO handle positive grow direction.
    assert(grow_negative == true);

    RangeSpliting rs;
    std::map<long, std::vector<llvm::Instruction *>> offset2insts;
    // get the total size of the stack allocation, only for match_level == 2.
    if (match_level == 2) {
      if (auto C = dyn_cast<llvm::ConstantInt>(space)) {
        rs.splitAt(C->getSExtValue());
        // offset2insts[0].push_back(load_sp);
      } else {
        llvm::errs() << "ERROR: stack allocation size is not a constant! func: "
                     << F->getName() << "\n";
      }
    }

    // match all add instruction.
    for (auto &BB : *F) {
      for (auto &I : make_early_inc_range(BB)) {
        if (PatternMatch::match(&I, pat_alloc_offset)) {
          rs.splitAt(offset->getSExtValue());
          offset2insts[offset->getSExtValue()].push_back(&I);
        }
      }
    }

    if (entry->empty()) {
      builder.SetInsertPoint(entry);
    } else {
      builder.SetInsertPoint(&entry->front());
    }
    auto pty = PointerType::get(IntegerType::get(M.getContext(), 8), 0);

    Value *zero = nullptr;
    assert(load_sp != nullptr);
    // create alloca for each range
    for (auto it = rs.ranges.rbegin(); it != rs.ranges.rend(); ++it) {
      auto &r = *it;
      Value *alloc = builder.CreateAlloca(
          pty->getPointerElementType(),
          ConstantInt::get(M.getDataLayout().getIntPtrType(M.getContext()),
                           r.size),
          "stack_" + offsetStr(r.start) + "_" + offsetStr(r.start + r.size) +
              "_");
      alloc = builder.CreatePtrToInt(alloc, load_sp->getType());
      if (r.start == 0) {
        zero = alloc;
      }

      for (auto inst : offset2insts[r.start]) {
        inst->replaceAllUsesWith(alloc);
        inst->eraseFromParent();
      }
    }
    if (rs.last() != 0 && !offset2insts[rs.last()].empty()) {
      llvm::errs()
          << "ERROR: Cannot find the last stack allocation instruction\n";
      std::abort();
    }

    if (zero != nullptr) {
      load_sp->replaceAllUsesWith(zero);
      load_sp->eraseFromParent();
    }
    if (match_level == 2) {
      prologue_store->eraseFromParent();
    }
  }
  // perform the transformation:

  return PreservedAnalyses::none();
}
} // namespace notdec
