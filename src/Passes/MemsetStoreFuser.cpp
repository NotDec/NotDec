// MemsetStoreFuser: fold the scalar stores that surround an IDA-recognised
// memset into that memset.  Rationale and evidence: microsub2/logs 66.

#include "binarysub/Env.h"
#include "Passes/MemOpMatcher.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Local.h>

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <set>
#include <string>

#include "Utils/Utils.h"

using namespace llvm;

namespace notdec {

namespace {

void appendFuserPassBanner(llvm::StringRef FuncName) {
  std::string Message;
  llvm::raw_string_ostream OS(Message);
  OS << " ============== MemsetStoreFuser";
  if (!FuncName.empty()) {
    OS << " [" << FuncName << "]";
  }
  OS << " ===============\n";
  notdec::appendRecoveryPassLog(OS.str());
}

// Fuse the constant stores surrounding MS into MS.  Returns true when anything
// was absorbed (the instruction stream then changed).
bool fuseStoresIntoMemset(Function &F, CallInst &MS) {
  auto *Fill = dyn_cast<ConstantInt>(MS.getArgOperand(1));
  auto *Size = dyn_cast<ConstantInt>(MS.getArgOperand(2));
  if (Fill == nullptr || Size == nullptr) {
    return false;
  }
  const uint64_t FillByte = Fill->getZExtValue() & 0xFF;
  const int64_t MemsetSize = Size->getSExtValue();
  if (MemsetSize <= 0) {
    return false;
  }
  const auto &DL = F.getParent()->getDataLayout();
  auto GetStoreSize = [&](Type *Ty) -> int64_t {
    if (Ty->isPointerTy()) {
      return static_cast<int64_t>(DL.getPointerSize());
    }
    return static_cast<int64_t>(DL.getTypeStoreSize(Ty).getKnownMinValue());
  };

  std::set<Instruction *> MemsetVisited;
  MemsetVisited.insert(&MS);
  auto MemsetBase = matchOffsetNormalized(MS.getArgOperand(0), MemsetVisited);
  if (!MemsetBase) {
    return false;
  }
  Value *Base = MemsetBase->first;
  const int64_t OriginalBegin = MemsetBase->second;
  const int64_t OriginalEnd = OriginalBegin + MemsetSize;
  int64_t Begin = OriginalBegin;
  int64_t End = OriginalEnd;
  bool IsVolatile = MS.isVolatile();
  SmallVector<StoreInst *, 8> Absorbed;

  auto TryAbsorb = [&](StoreInst *SI) {
    auto *Stored = dyn_cast<ConstantInt>(SI->getValueOperand());
    // O2 vectorization produces stores of constants wider than 64 bits; such a
    // store cannot be modelled with the uint64 fill value below, so skip it
    // instead of tripping APInt::getZExtValue()'s assertion (reproduced on
    // coreutils O2/stat).
    if (Stored == nullptr || Stored->getBitWidth() > 64) {
      return false;
    }
    const std::uint64_t StoredValue = Stored->getZExtValue();
    if (!isCyclicConstantValue(StoredValue)) {
      return false;
    }
    if ((StoredValue & 0xFF) != FillByte) {
      return false;
    }
    std::set<Instruction *> StoreVisited;
    StoreVisited.insert(SI);
    auto StoreBase =
        matchOffsetNormalized(SI->getPointerOperand(), StoreVisited);
    if (!StoreBase || StoreBase->first != Base) {
      return false;
    }
    const int64_t StoreBegin = StoreBase->second;
    const int64_t StoreEnd =
        StoreBegin + GetStoreSize(SI->getValueOperand()->getType());
    // Only adjacent or overlapping stores are part of the same bulk write.  A
    // store fully inside the current range is a redundant write and is dropped
    // without changing the range.
    if (StoreEnd < Begin || StoreBegin > End) {
      return false;
    }
    Begin = std::min(Begin, StoreBegin);
    End = std::max(End, StoreEnd);
    IsVolatile |= SI->isVolatile();
    Absorbed.push_back(SI);
    return true;
  };

  BasicBlock &BB = *MS.getParent();
  for (auto It = MS.getIterator(); It != BB.begin();) {
    --It;
    if (auto *SI = dyn_cast<StoreInst>(&*It)) {
      if (TryAbsorb(SI)) {
        continue;
      }
      break;
    }
    if (isTransparentBetweenStoreAndMemset(*It)) {
      continue;
    }
    break;
  }
  for (auto It = std::next(MS.getIterator()); It != BB.end(); ++It) {
    if (auto *SI = dyn_cast<StoreInst>(&*It)) {
      if (TryAbsorb(SI)) {
        continue;
      }
      break;
    }
    if (isTransparentBetweenStoreAndMemset(*It)) {
      continue;
    }
    break;
  }
  if (Absorbed.empty()) {
    return false;
  }

  const bool Grew = Begin != OriginalBegin || End != OriginalEnd;
  if (Grew) {
    IRBuilder<> Builder(&MS);
    auto *NewBase =
        addOffset(Builder, F.getParent()->getDataLayout(), Base, Begin);
    auto *NewFill = ConstantInt::get(Fill->getType(), FillByte);
    auto *NewSize = ConstantInt::get(Size->getType(), End - Begin);
    Builder.CreateMemSet(NewBase, NewFill, NewSize, MaybeAlign(), IsVolatile);
  }

  std::string Message;
  llvm::raw_string_ostream OS(Message);
  OS << "Fusing " << Absorbed.size() << " store(s) into memset in func "
     << F.getName() << ": [" << Begin << "," << End << ") was [" << OriginalBegin
     << "," << OriginalEnd << ")"
     << (Grew ? " rebuilt" : " redundant stores dropped") << "\n";
  notdec::appendRecoveryPassLog(OS.str());

  for (auto *SI : Absorbed) {
    Value *Address = SI->getPointerOperand();
    SI->eraseFromParent();
    // The address computation of a dropped store has to go away as well:
    // otherwise its pointer-add is still recorded as field evidence by the type
    // recovery, which is exactly the bogus member this pass removes.
    RecursivelyDeleteTriviallyDeadInstructions(Address);
  }
  if (Grew) {
    MS.eraseFromParent();
  }
  return true;
}

} // namespace

PreservedAnalyses MemsetStoreFuser::run(Function &F,
                                        FunctionAnalysisManager &) {
  static const bool Enabled =
      !binarysub::env_flag("NOTDEC_DISABLE_MEMSET_STORE_FUSER");
  if (!Enabled) {
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  bool Again = true;
  bool LoggedBanner = false;
  while (Again) {
    Again = false;
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *CB = dyn_cast<CallInst>(&I);
        if (CB == nullptr || CB->getIntrinsicID() != Intrinsic::memset) {
          continue;
        }
        if (!LoggedBanner) {
          appendFuserPassBanner(F.getName());
          LoggedBanner = true;
        }
        if (fuseStoresIntoMemset(F, *CB)) {
          Changed = true;
          Again = true;
          break;
        }
      }
      if (Again) {
        break;
      }
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec
