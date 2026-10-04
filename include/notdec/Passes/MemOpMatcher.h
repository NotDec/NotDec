#ifndef _NOTDEC_PASSES_MEMOP_MATCHER_H_
#define _NOTDEC_PASSES_MEMOP_MATCHER_H_

#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <optional>
#include <set>
#include <utility>

namespace notdec {

struct MemsetMatcher : llvm::PassInfoMixin<MemsetMatcher> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);
};

// Fold constant stores that are adjacent to, or fully contained in, an
// existing llvm.memset with the same fill byte into that memset.  The IDA
// microcode already recognises bulk zeroing (rep stos) as a memset, but the
// scalar head/tail stores of the same source-level call survive next to it;
// they are still recorded as field evidence by the type recovery, which turns
// them into bogus struct members.  See microsub2/logs 66.
struct MemsetStoreFuser : llvm::PassInfoMixin<MemsetStoreFuser> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);
};

struct MemcpyMatcher : llvm::PassInfoMixin<MemcpyMatcher> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &);
};

// ---------------------------------------------------------------------------
// Helpers shared by the memory-op matchers.
// ---------------------------------------------------------------------------

// A store value that repeats one byte can be expanded into a memset.
inline bool isCyclicConstantValue(int64_t Val) {
  auto Low = Val & 0xFF;
  if ((Val & 0xFF00) == (Low << 8)) {
    if ((Val & 0xFF0000) == (Low << 16)) {
      if ((Val & 0xFF000000) == (Low << 24)) {
        return true;
      }
    }
  }
  return false;
}

// "base + offset" of an address, looking through casts, adds and alignment
// masks.  The alignment mask ("and x, -A", A a power of two) appears because
// the compiler rounds an already aligned base down before a bulk store, so
// "(p & -8) + 120" and "p + 120" are the same address; without this the scalar
// store and the neighbouring memset would look like different bases and could
// not be fused.  See microsub2/logs 66.
inline std::optional<std::pair<llvm::Value *, int64_t>>
matchOffsetNormalized(llvm::Value *V, std::set<llvm::Instruction *> &Visited) {
  if (llvm::ConstantExpr *CE = llvm::dyn_cast<llvm::ConstantExpr>(V)) {
    if (CE->getOpcode() == llvm::Instruction::IntToPtr) {
      return matchOffsetNormalized(CE->getOperand(0), Visited);
    }
  }
  if (auto *CI = llvm::dyn_cast<llvm::ConstantInt>(V))
    return {{nullptr, CI->getSExtValue()}};

  if (auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    Visited.insert(I);

    if (auto *Cast = llvm::dyn_cast<llvm::CastInst>(I)) {
      return matchOffsetNormalized(Cast->getOperand(0), Visited);
    }
    if (auto *ZE = llvm::dyn_cast<llvm::ZExtInst>(I))
      return matchOffsetNormalized(ZE->getOperand(0), Visited);
    if (auto *SE = llvm::dyn_cast<llvm::SExtInst>(I))
      return matchOffsetNormalized(SE->getOperand(0), Visited);
    if (auto *Trunc = llvm::dyn_cast<llvm::TruncInst>(I))
      return matchOffsetNormalized(Trunc->getOperand(0), Visited);
    if (auto *BO = llvm::dyn_cast<llvm::BinaryOperator>(I)) {
      if (BO->getOpcode() == llvm::Instruction::Add) {
        auto LHS = matchOffsetNormalized(BO->getOperand(0), Visited);
        auto RHS = matchOffsetNormalized(BO->getOperand(1), Visited);
        if (LHS && RHS) {
          if (!LHS->first && RHS->first)
            return {{RHS->first, RHS->second + LHS->second}};
          if (LHS->first && !RHS->first)
            return {{LHS->first, LHS->second + RHS->second}};
        }
      }
      if (BO->getOpcode() == llvm::Instruction::And) {
        auto IsAlignMask = [](llvm::Value *C) {
          auto *K = llvm::dyn_cast<llvm::ConstantInt>(C);
          if (K == nullptr) {
            return false;
          }
          int64_t Mask = K->getSExtValue();
          return Mask < 0 && (Mask & 0x7) == 0;
        };
        if (IsAlignMask(BO->getOperand(1))) {
          return matchOffsetNormalized(BO->getOperand(0), Visited);
        }
        if (IsAlignMask(BO->getOperand(0))) {
          return matchOffsetNormalized(BO->getOperand(1), Visited);
        }
      }
    }
  }

  // Default case: base is V with offset 0
  return {{V, 0}};
}

// Materialise "Base + Offset" as a pointer value.
inline llvm::Value *addOffset(llvm::IRBuilder<> &Builder,
                              const llvm::DataLayout &DL, llvm::Value *Base,
                              int64_t Offset) {
  if (Base == nullptr) {
    Base = Builder.CreateIntToPtr(
        Builder.getIntN(DL.getPointerSizeInBits(), Offset),
        llvm::PointerType::get(Builder.getContext(), 0));
    return Base;
  } else if (Offset == 0) {
    // do nothing
  } else {
    if (Base->getType()->isPointerTy()) {
      // how to add the offset?
      // 1 gep
      // 2 cast to int and add?
      Base = Builder.CreatePtrToInt(
          Base,
          llvm::Type::getIntNTy(Builder.getContext(),
                                DL.getPointerSizeInBits()));
    }
    Base = Builder.CreateAdd(
        Base,
        Builder.getIntN(Base->getType()->getIntegerBitWidth(), Offset));
  }
  if (Base->getType()->isIntegerTy()) {
    Base = Builder.CreateIntToPtr(Base,
                                  llvm::PointerType::get(Builder.getContext(), 0));
  }
  return Base;
}

// Instructions that can sit between a fused store and its memset without
// making the fusion observable: pure, non-memory operations (typically the
// address computation of the store itself).
inline bool isTransparentBetweenStoreAndMemset(const llvm::Instruction &I) {
  if (I.isTerminator() || llvm::isa<llvm::PHINode>(I)) {
    return false;
  }
  return !I.mayReadOrWriteMemory();
}

} // namespace notdec

#endif
