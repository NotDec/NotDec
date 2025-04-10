
#include "Passes/MemOpMatcher.h"
#include "Passes/ConstraintGenerator.h"
#include <cstdint>
#include <functional>
#include <llvm/ADT/Optional.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <utility>

namespace notdec {
PreservedAnalyses MemsetMatcher::run(Function &F, FunctionAnalysisManager &) {
  // try to match the operand of store, return the base value and offset 
  std::function<Optional<std::pair<Value *, int64_t>>(
      Value *, std::set<Instruction *> &)>
      MatchOffset = [&](Value *StoreOp, std::set<Instruction *> &Visited)
      -> Optional<std::pair<Value *, int64_t>> {
    if (auto *CI = dyn_cast<ConstantInt>(StoreOp)) {
      return std::make_pair((Value *)nullptr, CI->getSExtValue());
    } else if (auto *Cast = dyn_cast<CastInst>(StoreOp)) {
      Visited.insert(Cast);
      return MatchOffset(Cast->getOperand(0), Visited);
    } else if (auto *Add = dyn_cast<BinaryOperator>(StoreOp)) {
      if (Add->getOpcode() == Instruction::Add) {
        auto *Left = Add->getOperand(0);
        auto *Right = Add->getOperand(1);
        ensureSequence(Left, Right);
        if (auto *CI = dyn_cast<ConstantInt>(Right)) {
          Visited.insert(Add);
          return std::make_pair(Left, CI->getSExtValue());
        }
      }
    }
    return std::make_pair((Value *)StoreOp, 0l);
  };

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
        BeginOffset = MatchOffset(SI->getPointerOperand(), Visited);
        if (!BeginOffset) {
          continue;
        }
        EndOffset = BeginOffset;
        EndOffset->second +=
            SI->getValueOperand()->getType()->getPrimitiveSizeInBits() / 8;
        // check for the same store backwards.
        // long TotalStoreCount = 1;
        std::set<Instruction *> Stores;
        Stores.insert(SI);
        for (auto II = std::next(I); II != E; ++II) {
          if (Visited.count(&*II)) {
            continue;
          }
          if (auto *SI2 = dyn_cast<StoreInst>(&*II)) {
            Visited.insert(SI2);

            // memset must set a cyclic constant
            if (!isa<ConstantInt>(SI2->getValueOperand())) {
              continue;
            }
            auto *SetVal2 = dyn_cast<ConstantInt>(SI2->getValueOperand());
            if (SetVal2->getZExtValue() != SetVal->getZExtValue()) {
              continue;
            }

            Optional<std::pair<Value *, int64_t>> NextBegin =
                MatchOffset(SI2->getPointerOperand(), Visited);
            // failed to match: stores must be in sequence
            if (!NextBegin) {
              break;
            }
            // has same base value
            if (NextBegin->first == EndOffset->first) {
              int64_t NextBeginOff = NextBegin->second;
              int64_t NextEndOff =
                  NextBeginOff +
                  SI2->getValueOperand()->getType()->getPrimitiveSizeInBits() /
                      8;
              // Current range: [BeginOffset, EndOffset]
              // Next range: [NextBeginOff, NextEndOff]
              // case 1: equal: BeginOffset < EndOffset == NextBeginOff < NextEndOff
              // case 2: overlap: BeginOffset < NextBeginOff < EndOffset < NextEndOff
              if (NextBeginOff <= EndOffset->second && EndOffset->second < NextEndOff) {
                EndOffset->second = NextEndOff;
                Stores.insert(SI2);
                continue;
                // Extend the begin of offset
                // case 3: equal: NextBeginOff < NextEndOff == BeginOffset < EndOffset
                // case 4: overlap: NextBeginOff < BeginOffset < NextEndOff < EndOffset
              } else if (NextEndOff >= BeginOffset->second && NextBeginOff < BeginOffset->second ) {
                BeginOffset->second = NextBeginOff;
                Stores.insert(SI2);
                continue;
              }
            }
          }
          // failed to match
          break;
        }
        if (Stores.size() > 1) {
          // found a memset
          // create a IRBuilder
          IRBuilder<> Builder(SI);
          Builder.SetInsertPoint(SI);
          auto *Base = BeginOffset->first;
          if (BeginOffset->first != nullptr && BeginOffset->second != 0) {
            if (Base->getType()->isPointerTy()) {
              assert(false && "TODO");
              // The offset is byte offset or element index?
            } else {
              Base = Builder.CreateAdd(
                  Base, Builder.getIntN(Base->getType()->getIntegerBitWidth(),
                                        BeginOffset->second));
              Base = Builder.CreateIntToPtr(Base,
                                            Type::getInt8PtrTy(F.getContext()));
            }
          }
          llvm::errs() << "Merging " << std::to_string(Stores.size())
                       << " stores in func " << F.getName()
                       << " into memset at " << *Base << ": " << *SI << "\n";
          auto *SetValByte =
              ConstantInt::get(IntegerType::get(F.getContext(), 8),
                               SetVal->getZExtValue() & 0xFF);
          Builder.CreateMemSet(Base, SetValByte,
                               EndOffset->second - BeginOffset->second,
                               MaybeAlign());
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

} // namespace notdec
