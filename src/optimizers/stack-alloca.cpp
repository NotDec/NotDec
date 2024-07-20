#include "optimizers/stack-alloca.h"
#include "optimizers/stack-pointer-finder.h"
#include <cassert>
#include <cstdint>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Metadata.h>
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
/// rely on canonical form of LLVM Instructions, So run instcombine first.
/// see:
/// https://www.npopov.com/2023/04/10/LLVM-Canonicalization-and-target-independence.html
///
/// Currently we only replace the stack pointer load and restore with alloca. No add is converted to gep currently.
/// If the stack grows negative, the offsets will also be negative. But in the alloca, the size is still positive.
PreservedAnalyses LinearAllocationRecovery::run(Module &M, ModuleAnalysisManager &MAM) {
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
  Instruction *LoadSP;
  Value *space;
  Instruction *add_load_sp;
  ConstantInt *offset;

  auto pat_alloc = StackPointerMatcher(sp1, space, LoadSP, add_load_sp, sp);
  auto pat_alloc_offset = m_Add(m_Load(m_Specific(sp)), m_ConstantInt(offset));

  // ======== 1. Matching patterns. ===========

  IRBuilder Builder(M.getContext());
  for (auto *F : worklist) {
    sp1 = nullptr;
    LoadSP = nullptr;
    space = nullptr;
    add_load_sp = nullptr;
    Instruction *prologue_store = nullptr;
    // 1. Match for stack allocation level.
    // level = 2 normal stack allocation.
    // level = 1 tail function stack allocation.
    // level = 0 cannot match stack allocation.
    int match_level = 0;
    auto *entry = &F->getEntryBlock();
    // 1.1 Match normal stack allocation in the entry block.
    for (auto &I : *entry) {
      if (PatternMatch::match(&I, pat_alloc)) {
        prologue_store = &I;
        match_level = 2;
        // llvm::errs() << "stack alloc: " << *space << "\n";
        break;
      }
    }
    // 1.2 Try to match tail function stack allocation if not matched.
    // There should be at least one Add(Load(sp), offset).
    if (match_level == 0) {
      bool has_load_sp = false;
      for (auto &BB : *F) {
        for (auto &I : BB) {
          if (PatternMatch::match(&I, pat_alloc_offset)) {
            LoadSP = cast<Instruction>(I.getOperand(0));
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
        llvm::errs() << "ERROR: No pattern matched but the stack pointer is accessed in func: " << F->getName() << "!\n";
      }
    }
    // 1.3 Failed to match any stack allocation.
    if (match_level == 0) {
      llvm::errs() << "ERROR: cannot find stack allocation in func: " << F->getName() << "\n";
      continue;
    }
    // For normal stack allocation (level = 2): Remove epilogue that restore the
    // stack pointer
    if (match_level == 2) {
      assert(add_load_sp != nullptr && space != nullptr);
      assert(add_load_sp->getParent() == LoadSP->getParent());
      // find epilogue in exit blocks, and remove it.
      auto pat_restore = m_Store(m_Specific(LoadSP), m_Specific(sp));
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
        llvm::errs() << "ERROR: Cannot find sp restore? func: " << F->getName() << "\n";
        continue;
      }
    }

    // replace the stack allocation with alloca.
    bool grow_negative = sp_result.direction == 0;
    auto pty = PointerType::get(IntegerType::get(M.getContext(), 8), 0);
    auto SizeTy = IntegerType::get(M.getContext(), M.getDataLayout().getPointerSizeInBits());
    // TODO handle positive grow direction.
    assert(grow_negative == true);
    // When match_level == 1, Only LoadSP is not null.
    if (match_level == 1) {
      space = ConstantInt::getNullValue(SizeTy);
    }
    //  else if (grow_negative) {
    //   space = Builder.CreateNeg(space);
    // }
    Builder.SetInsertPoint(LoadSP);
    Value *alloc =
        Builder.CreateAlloca(pty->getPointerElementType(), grow_negative ? Builder.CreateNeg(space) : space, "stack_addr");
    cast<Instruction>(alloc)->setMetadata("notdec.stack_direction",
                                          MDNode::get(M.getContext(), MDString::get(M.getContext(), "negative")));
    alloc = Builder.CreatePtrToInt(alloc, LoadSP->getType(), "stack");
    Value *alloc_end = Builder.CreateAdd(alloc, space, "stack_end");

    // The name is incorrect. high_addr does not mean it will must
    Instruction *high_addr = add_load_sp;
    Instruction *low_addr = LoadSP;
    // if (grow_negative) {
    //   std::swap(high_addr, low_addr);
    // }
    // replace all uses of LoadSP with alloc_end.
    low_addr->replaceAllUsesWith(alloc);
    low_addr->eraseFromParent();
    if (high_addr != nullptr) {
      assert(match_level == 2);
      high_addr->replaceAllUsesWith(alloc_end);
      high_addr->eraseFromParent();
    }

    if (match_level == 2) {
      prologue_store->eraseFromParent();
    }
  }
  // perform the transformation:

  return PreservedAnalyses::none();
}
} // namespace notdec
