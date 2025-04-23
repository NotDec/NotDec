
#include "Passes/MemOpMatcher.h"
#include "Passes/ConstraintGenerator.h"
#include <cstdint>
#include <functional>
#include <llvm/ADT/Optional.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <utility>

namespace notdec {

// Helper to decompose a value into base + offset
Optional<std::pair<Value *, int64_t>>
matchOffset(Value *V, std::set<Instruction *> &Visited) {
  if (llvm::ConstantExpr *CE = llvm::dyn_cast<llvm::ConstantExpr>(V)) {
    if (CE->getOpcode() == llvm::Instruction::IntToPtr) {
      return matchOffset(CE->getOperand(0), Visited);
    }
  }
  if (auto *CI = dyn_cast<ConstantInt>(V))
    return {{nullptr, CI->getSExtValue()}};

  if (auto *I = dyn_cast<Instruction>(V)) {
    Visited.insert(I);

    if (auto *Cast = dyn_cast<CastInst>(I)) {
      return matchOffset(Cast->getOperand(0), Visited);
    }
    if (auto *ZE = dyn_cast<ZExtInst>(I))
      return matchOffset(ZE->getOperand(0), Visited);
    if (auto *SE = dyn_cast<SExtInst>(I))
      return matchOffset(SE->getOperand(0), Visited);
    if (auto *Trunc = dyn_cast<TruncInst>(I))
      return matchOffset(Trunc->getOperand(0), Visited);
    if (auto *Add = dyn_cast<BinaryOperator>(I)) {
      if (Add->getOpcode() == Instruction::Add) {
        auto LHS = matchOffset(Add->getOperand(0), Visited);
        auto RHS = matchOffset(Add->getOperand(1), Visited);
        if (LHS && RHS) {
          if (!LHS->first && RHS->first)
            return {{RHS->first, RHS->second + LHS->second}};
          if (LHS->first && !RHS->first)
            return {{LHS->first, LHS->second + RHS->second}};
        }
      }
    }
  }

  // Default case: base is V with offset 0
  return {{V, 0}};
};

llvm::Value *addOffset(llvm::IRBuilder<> &Builder, const llvm::DataLayout &DL,
                       llvm::Value *Base, int64_t Offset) {
  if (Base == nullptr) {
    Base = Builder.CreateIntToPtr(
        Builder.getIntN(DL.getPointerSizeInBits(), Offset),
        Type::getInt8PtrTy(Builder.getContext()));
  } else if (Offset == 0) {
    // do nothing
  } else {
    if (Base->getType()->isPointerTy()) {
      // how to add the offset?
      // 1 gep
      // 2 cast to int and add?
      Base = Builder.CreatePtrToInt(
          Base,
          Type::getIntNTy(Builder.getContext(), DL.getPointerSizeInBits()));
    }
    Base = Builder.CreateAdd(
        Base, Builder.getIntN(Base->getType()->getIntegerBitWidth(), Offset));
    Base =
        Builder.CreateIntToPtr(Base, Type::getInt8PtrTy(Builder.getContext()));
  }
  return Base;
}

PreservedAnalyses MemsetMatcher::run(Function &F, FunctionAnalysisManager &) {
  auto PointerSizeInBytes = F.getParent()->getDataLayout().getPointerSize();

  auto isCyclicConstant = [](int64_t Val) {
    auto Low = Val & 0xFF;
    if ((Val & 0xFF00) == (Low << 8)) {
      if ((Val & 0xFF0000) == (Low << 16)) {
        if ((Val & 0xFF000000) == (Low << 24)) {
          return true;
        }
      }
    }
    return false;
  };

  for (auto BB = F.begin(), E = F.end(); BB != E; ++BB) {
    // reverse iterate over the basic block
    for (auto I = BB->rbegin(), E = BB->rend(); I != E; ++I) {
      if (auto *SI = dyn_cast<StoreInst>(&*I)) {
        // memset must set a cyclic constant
        if (!isa<ConstantInt>(SI->getValueOperand())) {
          continue;
        }
        auto *SetVal = dyn_cast<ConstantInt>(SI->getValueOperand());
        if (!isCyclicConstant(SetVal->getZExtValue())) {
          continue;
        }

        std::set<Instruction *> Visited;

        Optional<std::pair<Value *, int64_t>> BeginOffset = None;
        Optional<std::pair<Value *, int64_t>> EndOffset = None;

        // init with match
        Visited.insert(SI);
        BeginOffset = matchOffset(SI->getPointerOperand(), Visited);
        if (!BeginOffset) {
          continue;
        }
        auto AccessSize =
            SI->getValueOperand()->getType()->getPrimitiveSizeInBits() / 8;
        EndOffset = BeginOffset;
        EndOffset->second += AccessSize;
        // check for the same store backwards.
        // int64_t TotalStoreCount = 1;
        std::set<Instruction *> Stores;
        Stores.insert(SI);
        for (auto II = std::next(I); II != E; ++II) {
          if (Visited.count(&*II)) {
            continue;
          }
          auto *SI2 = dyn_cast<StoreInst>(&*II);
          if (!SI2) {
            break;
          }
          Visited.insert(SI2);

          // memset must set a cyclic constant
          if (!isa<ConstantInt>(SI2->getValueOperand())) {
            break;
          }
          auto *SetVal2 = dyn_cast<ConstantInt>(SI2->getValueOperand());
          if (SetVal2->getZExtValue() != SetVal->getZExtValue()) {
            break;
          }

          Optional<std::pair<Value *, int64_t>> NextBegin =
              matchOffset(SI2->getPointerOperand(), Visited);
          // failed to match: stores must be in sequence
          if (!NextBegin) {
            break;
          }
          // has same base value
          if (NextBegin->first != EndOffset->first) {
            break;
          }
          int64_t NextBeginOff = NextBegin->second;
          int64_t NextEndOff =
              NextBeginOff +
              SI2->getValueOperand()->getType()->getPrimitiveSizeInBits() / 8;
          // Current range: [BeginOffset, EndOffset]
          // Next range: [NextBeginOff, NextEndOff]
          // case 1: equal: BeginOffset < EndOffset == NextBeginOff <
          // NextEndOff case 2: overlap: BeginOffset < NextBeginOff <
          // EndOffset < NextEndOff
          if (NextBeginOff <= EndOffset->second &&
              EndOffset->second < NextEndOff) {
            EndOffset->second = NextEndOff;
            Stores.insert(SI2);
            continue;
            // Extend the begin of offset
            // case 3: equal: NextBeginOff < NextEndOff == BeginOffset <
            // EndOffset case 4: overlap: NextBeginOff < BeginOffset <
            // NextEndOff < EndOffset
          } else if (NextEndOff >= BeginOffset->second &&
                     NextBeginOff < BeginOffset->second) {
            BeginOffset->second = NextBeginOff;
            Stores.insert(SI2);
            continue;
          }
        }

        auto Size = EndOffset->second - BeginOffset->second;
        if (Size > PointerSizeInBytes) {
          // found a memset
          // create a IRBuilder
          IRBuilder<> Builder(SI);
          Builder.SetInsertPoint(SI);
          auto *Base = addOffset(Builder, F.getParent()->getDataLayout(),
                                 BeginOffset->first, BeginOffset->second);
          llvm::errs() << "Merging " << std::to_string(Stores.size())
                       << " stores in func " << F.getName()
                       << " into memset at " << *Base << ": " << *SI << "\n";
          auto *SetValByte =
              ConstantInt::get(IntegerType::get(F.getContext(), 8),
                               SetVal->getZExtValue() & 0xFF);

          assert(Size > 0);
          Builder.CreateMemSet(Base, SetValByte, Size, MaybeAlign());
          // remove all visited insts
          for (auto *I1 : Stores) {
            I1->eraseFromParent();
          }
          // revisit the basic block
          --BB;
          break;
        }
      }
    }
  }

  return PreservedAnalyses::none();
}

PreservedAnalyses MemcpyMatcher::run(Function &F, FunctionAnalysisManager &) {
  auto PointerSizeInBytes = F.getParent()->getDataLayout().getPointerSize();

  for (auto BB = F.begin(), E = F.end(); BB != E; ++BB) {
    // Reverse iteration to find store sequences
    for (auto I = BB->rbegin(), E = BB->rend(); I != E; ++I) {
      if (auto *SI = dyn_cast<StoreInst>(&*I)) {
        // Must store loaded value.
        auto *LI = dyn_cast<LoadInst>(SI->getValueOperand());
        if (!LI)
          continue;

        std::set<Instruction *> Visited;
        Visited.insert(SI);
        Visited.insert(LI);

        // Match source address (load operand)
        auto SrcAddr = matchOffset(LI->getPointerOperand(), Visited);
        if (!SrcAddr)
          continue;
        auto [SrcBase, SrcStart] = *SrcAddr;

        // Match destination address (store operand)
        auto DestAddr = matchOffset(SI->getPointerOperand(), Visited);
        if (!DestAddr)
          continue;
        auto [DestBase, DestStart] = *DestAddr;

        // Calculate access size
        uint64_t AccessSize = LI->getType()->getPrimitiveSizeInBits() / 8;

        // Collect continuous accesses
        int64_t DestEnd = DestStart + AccessSize;
        int64_t SrcEnd = SrcStart + AccessSize;
        // int64_t ExpectedNextSrc = SrcAddr->second - AccessSize;
        // int64_t ExpectedNextDest = DestAddr->second - AccessSize;
        std::vector<StoreInst *> Cluster = {SI};
        // Value *BasePtr = DestAddr->first;

        for (auto II = std::next(I); II != E; ++II) {
          if (Visited.count(&*II))
            continue;

          if (auto *NextSI = dyn_cast<StoreInst>(&*II)) {
            // 1. 检查是否还是Store和Load的组合
            auto *NextLI = dyn_cast<LoadInst>(NextSI->getValueOperand());
            if (!NextLI)
              break;

            std::set<Instruction *> NewVisited = Visited;
            NewVisited.insert(NextSI);
            NewVisited.insert(NextLI);

            // 2. 检查是否Load和Store两边的Base相同。
            // Verify source address
            auto NextSrc = matchOffset(NextLI->getPointerOperand(), NewVisited);
            if (!NextSrc || NextSrc->first != SrcBase)
              break;
            auto [NextSrcBase, NextSrcStart] = *NextSrc;

            // Verify destination address
            auto NextDest =
                matchOffset(NextSI->getPointerOperand(), NewVisited);
            if (!NextDest || NextDest->first != DestBase)
              break;
            auto [NextDestBase, NextDestStart] = *NextDest;

            // 3. 检查是否和之前的范围贴着
            // TODO 是否要改成支持overlap的？
            int64_t NextAccessSize =
                NextLI->getType()->getPrimitiveSizeInBits() / 8;
            auto NextSrcEnd = NextSrcStart + NextAccessSize;
            auto NextDestEnd = NextDestStart + NextAccessSize;
            // Update expected addresses if OK
            // 1. extend end
            if (NextSrcStart == SrcEnd && NextDestStart == DestEnd) {
              SrcEnd += NextAccessSize;
              DestEnd += NextAccessSize;
            } else if (NextSrcEnd == SrcStart &&
                       NextDestEnd == DestStart) { // extend start
              SrcStart -= NextAccessSize;
              DestStart -= NextAccessSize;
            } else {
              // failed
              break;
            }

            Cluster.push_back(NextSI);
            Visited = NewVisited;
          } else {
            break;
          }
        }

        auto Size = SrcEnd - SrcStart;
        assert(Size == (DestEnd - DestStart));

        if (Size > PointerSizeInBytes) {
          IRBuilder<> Builder(SI);
          // Create source pointer
          Value *SrcPtr = addOffset(Builder, F.getParent()->getDataLayout(),
                                    SrcBase, SrcStart);

          // Create destination pointer
          Value *DestPtr = addOffset(Builder, F.getParent()->getDataLayout(),
                                     DestBase, DestStart);

          // Create memcpy
          auto MS = Builder.CreateMemCpy(DestPtr, MaybeAlign(), SrcPtr,
                                         MaybeAlign(), Size);
          llvm::errs() << "Merging " << std::to_string(Cluster.size())
                       << " stores in func " << F.getName()
                       << " into memset: " << *MS << "\n";

          // Remove original instructions
          for (auto *SI : Cluster) {
            auto *LI = dyn_cast<LoadInst>(SI->getValueOperand());
            SI->eraseFromParent();
            if (LI->getNumUses() == 0) {
              LI->eraseFromParent();
            }
          }

          // Restart analysis after modification
          --BB;
          break;
        }
      }
    }
  }

  return PreservedAnalyses::none();
}

} // namespace notdec
