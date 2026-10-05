#include "notdec/TypeRecovery/mlsub/AllocatorBoundaryAnalysis.h"

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace notdec::mlsub {
namespace {

constexpr unsigned kMaxValueWalk = 256;

enum ReturnSource : std::uint32_t {
  ReturnNone = 0,
  ReturnCall = 1U << 0,
  ReturnArgument = 1U << 1,
  ReturnGlobal = 1U << 2,
  ReturnStack = 1U << 3,
  ReturnIntToPtr = 1U << 4,
  ReturnLoad = 1U << 5,
  ReturnOther = 1U << 6,
};

// The shape deliberately records coarse operations, not source types.  Lifted
// IR commonly erases pointee types, while offsets, access widths, data-flow and
// callee identity remain available.  Module-order callee IDs avoid accidentally
// turning a function name into a feature.
struct PointerUseShape {
  std::set<std::tuple<std::int64_t, std::uint64_t, char>> FixedAccesses;
  std::set<unsigned> DirectCallees;
  bool DynamicAccess = false;
  bool PassedToIndirectCall = false;
  bool Returned = false;
  bool StoredAsValue = false;
  bool Compared = false;
  bool PtrToInt = false;

  bool hasMemoryAccess() const {
    return DynamicAccess || !FixedAccesses.empty();
  }

  std::string fingerprint() const {
    std::string Result;
    llvm::raw_string_ostream OS(Result);
    for (const auto &[Offset, Width, Kind] : FixedAccesses) {
      OS << Kind << Offset << ':' << Width << ',';
    }
    OS << "dyn=" << DynamicAccess << ";ind=" << PassedToIndirectCall
       << ";ret=" << Returned << ";store=" << StoredAsValue
       << ";cmp=" << Compared << ";p2i=" << PtrToInt << ";callees=";
    for (unsigned Callee : DirectCallees) {
      OS << Callee << ',';
    }
    return OS.str();
  }

  // Allocator callers are clustered by the layout imposed on the returned
  // storage.  Callee IDs are intentionally excluded: passing the same fixed
  // object to different business helpers does not imply multiple layouts.
  std::string layoutFingerprint() const {
    std::string Result;
    llvm::raw_string_ostream OS(Result);
    for (const auto &[Offset, Width, Kind] : FixedAccesses) {
      OS << Kind << Offset << ':' << Width << ',';
    }
    OS << "dyn=" << DynamicAccess << ";ret=" << Returned
       << ";store=" << StoredAsValue << ";p2i=" << PtrToInt;
    return OS.str();
  }
};

struct PointerWalkItem {
  const llvm::Value *Value = nullptr;
  std::optional<std::int64_t> Offset = 0;
};

std::uint64_t getStoreSizeBits(const llvm::DataLayout &DL, llvm::Type *Ty) {
  if (Ty == nullptr || !Ty->isSized()) {
    return 0;
  }
  return DL.getTypeStoreSizeInBits(Ty).getKnownMinValue();
}

template <typename GEPType>
std::optional<std::int64_t> getConstantGEPOffset(const llvm::DataLayout &DL,
                                                 const GEPType &GEP) {
  llvm::APInt Offset(DL.getIndexTypeSizeInBits(GEP.getType()), 0, true);
  if (!GEP.accumulateConstantOffset(DL, Offset) ||
      Offset.getSignificantBits() > 64) {
    return std::nullopt;
  }
  return Offset.getSExtValue();
}

// Walk only SSA-preserving pointer operations.  The bounded traversal prevents
// a high-fanout global or cyclic PHI from making an audit itself expensive.
PointerUseShape analyzePointerUses(
    const llvm::Value *Root, const llvm::DataLayout &DL,
    const std::map<const llvm::Function *, unsigned> &FunctionIndices,
    const llvm::CallBase *ExcludedCall = nullptr) {
  PointerUseShape Shape;
  llvm::SmallVector<PointerWalkItem, 32> Worklist;
  std::set<std::pair<const llvm::Value *, std::optional<std::int64_t>>> Seen;
  Worklist.push_back({Root, 0});

  unsigned Walked = 0;
  while (!Worklist.empty() && Walked++ < kMaxValueWalk) {
    PointerWalkItem Item = Worklist.pop_back_val();
    if (!Seen.insert({Item.Value, Item.Offset}).second) {
      continue;
    }

    // Constants such as poison, undef and null may intentionally have no LLVM
    // use-list storage.  They still form a valid call-site origin, but provide
    // no local use-shape to traverse.
    if (!Item.Value->hasUseList()) {
      continue;
    }

    for (const llvm::User *User : Item.Value->users()) {
      auto *I = llvm::dyn_cast<llvm::Instruction>(User);
      if (I == nullptr) {
        continue;
      }
      if (auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(I)) {
        if (GEP->getPointerOperand() != Item.Value) {
          continue;
        }
        auto Step = getConstantGEPOffset(DL, *GEP);
        std::optional<std::int64_t> NextOffset;
        if (Item.Offset && Step) {
          NextOffset = *Item.Offset + *Step;
        }
        Worklist.push_back({GEP, NextOffset});
        continue;
      }
      // The ABI-erased lifted IR expresses field access as integer arithmetic
      // plus inttoptr (there are no typed GEPs): follow "add/sub const" and
      // "inttoptr" so the shapes below see the same offsets as a typed IR.
      if (auto *BO = llvm::dyn_cast<llvm::BinaryOperator>(I)) {
        if ((BO->getOpcode() == llvm::Instruction::Add ||
             BO->getOpcode() == llvm::Instruction::Sub) &&
            Item.Offset) {
          const llvm::Value *Other = nullptr;
          if (BO->getOperand(0) == Item.Value) {
            Other = BO->getOperand(1);
          } else if (BO->getOperand(1) == Item.Value) {
            Other = BO->getOperand(0);
          }
          if (auto *C = llvm::dyn_cast_or_null<llvm::ConstantInt>(Other)) {
            std::int64_t Delta = C->getSExtValue();
            std::int64_t Next = BO->getOpcode() == llvm::Instruction::Add
                                    ? *Item.Offset + Delta
                                    : *Item.Offset - Delta;
            Worklist.push_back({BO, Next});
            continue;
          }
        }
      }
      if (auto *ITP = llvm::dyn_cast<llvm::IntToPtrInst>(I)) {
        if (ITP->getOperand(0) == Item.Value) {
          Worklist.push_back({ITP, Item.Offset});
          continue;
        }
      }
      if (llvm::isa<llvm::BitCastInst, llvm::AddrSpaceCastInst,
                    llvm::FreezeInst>(I)) {
        Worklist.push_back({I, Item.Offset});
        continue;
      }
      if (llvm::isa<llvm::PHINode, llvm::SelectInst>(I)) {
        // A merge with another pointer loses a precise base offset, but its
        // downstream operations still carry useful byte-buffer evidence.
        Worklist.push_back({I, std::nullopt});
        continue;
      }
      if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(I)) {
        if (Load->getPointerOperand() == Item.Value) {
          if (Item.Offset) {
            Shape.FixedAccesses.insert(
                {*Item.Offset, getStoreSizeBits(DL, Load->getType()), 'r'});
          } else {
            Shape.DynamicAccess = true;
          }
        }
        continue;
      }
      if (auto *Store = llvm::dyn_cast<llvm::StoreInst>(I)) {
        if (Store->getPointerOperand() == Item.Value) {
          if (Item.Offset) {
            Shape.FixedAccesses.insert(
                {*Item.Offset,
                 getStoreSizeBits(DL, Store->getValueOperand()->getType()),
                 'w'});
          } else {
            Shape.DynamicAccess = true;
          }
        }
        if (Store->getValueOperand() == Item.Value) {
          Shape.StoredAsValue = true;
        }
        continue;
      }
      if (auto *CB = llvm::dyn_cast<llvm::CallBase>(I)) {
        bool IsPointerArgument = false;
        for (const llvm::Use &Arg : CB->args()) {
          IsPointerArgument |= Arg.get() == Item.Value;
        }
        if (!IsPointerArgument || CB == ExcludedCall) {
          continue;
        }
        if (auto *Callee = CB->getCalledFunction()) {
          auto It = FunctionIndices.find(Callee);
          if (It != FunctionIndices.end()) {
            Shape.DirectCallees.insert(It->second);
          }
        } else {
          Shape.PassedToIndirectCall = true;
        }
        continue;
      }
      if (llvm::isa<llvm::ReturnInst>(I)) {
        Shape.Returned = true;
      } else if (llvm::isa<llvm::ICmpInst>(I)) {
        Shape.Compared = true;
      } else if (llvm::isa<llvm::PtrToIntInst>(I)) {
        Shape.PtrToInt = true;
      }
    }
  }
  return Shape;
}

bool valueDependsOnImpl(const llvm::Value *Value, const llvm::Value *Root,
                        llvm::SmallPtrSetImpl<const llvm::Value *> &Seen,
                        unsigned &Budget) {
  if (Value == Root) {
    return true;
  }
  if (Value == nullptr || Budget-- == 0 || !Seen.insert(Value).second) {
    return false;
  }
  auto *I = llvm::dyn_cast<llvm::Instruction>(Value);
  if (I == nullptr || llvm::isa<llvm::LoadInst, llvm::CallBase>(I)) {
    return false;
  }
  for (const llvm::Use &Operand : I->operands()) {
    if (valueDependsOnImpl(Operand.get(), Root, Seen, Budget)) {
      return true;
    }
  }
  return false;
}

bool valueDependsOn(const llvm::Value *Value, const llvm::Value *Root) {
  llvm::SmallPtrSet<const llvm::Value *, 32> Seen;
  unsigned Budget = kMaxValueWalk;
  return valueDependsOnImpl(Value, Root, Seen, Budget);
}

bool valueTransitivelyDependsOnImpl(
    const llvm::Value *Value, const llvm::Value *Root,
    llvm::SmallPtrSetImpl<const llvm::Value *> &Seen, unsigned &Budget) {
  if (Value == Root) {
    return true;
  }
  if (Value == nullptr || Budget == 0 || !Seen.insert(Value).second) {
    return false;
  }
  --Budget;
  auto *I = llvm::dyn_cast<llvm::Instruction>(Value);
  if (I == nullptr || llvm::isa<llvm::LoadInst>(I)) {
    return false;
  }
  for (const llvm::Use &Operand : I->operands()) {
    if (valueTransitivelyDependsOnImpl(Operand.get(), Root, Seen, Budget)) {
      return true;
    }
  }
  return false;
}

bool valueTransitivelyDependsOn(const llvm::Value *Value,
                                const llvm::Value *Root) {
  llvm::SmallPtrSet<const llvm::Value *, 32> Seen;
  unsigned Budget = kMaxValueWalk;
  return valueTransitivelyDependsOnImpl(Value, Root, Seen, Budget);
}

std::string pointerOriginFingerprint(
    const llvm::Value *Value, const llvm::DataLayout &DL,
    const std::map<const llvm::Function *, unsigned> &FunctionIndices,
    unsigned Depth = 0) {
  if (Value == nullptr || Depth >= 8) {
    return "unknown";
  }
  if (auto *Arg = llvm::dyn_cast<llvm::Argument>(Value)) {
    auto It = FunctionIndices.find(Arg->getParent());
    return "arg:" +
           std::to_string(It == FunctionIndices.end() ? 0 : It->second) + ':' +
           std::to_string(Arg->getArgNo());
  }
  if (auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Value)) {
    return "alloca:" +
           std::to_string(getStoreSizeBits(DL, Alloca->getAllocatedType()));
  }
  if (llvm::isa<llvm::GlobalValue>(Value)) {
    return "global";
  }
  if (llvm::isa<llvm::ConstantPointerNull>(Value)) {
    return "null";
  }
  if (llvm::isa<llvm::PoisonValue>(Value)) {
    return "poison";
  }
  if (llvm::isa<llvm::UndefValue>(Value)) {
    return "undef";
  }
  if (auto *CB = llvm::dyn_cast<llvm::CallBase>(Value)) {
    auto It = FunctionIndices.find(CB->getCalledFunction());
    return It == FunctionIndices.end() ? "call:indirect"
                                       : "call:" + std::to_string(It->second);
  }
  if (auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Value)) {
    auto Offset = getConstantGEPOffset(DL, *GEP);
    return "gep:" + (Offset ? std::to_string(*Offset) : std::string("dyn")) +
           ':' +
           pointerOriginFingerprint(GEP->getPointerOperand(), DL,
                                    FunctionIndices, Depth + 1);
  }
  if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(Value)) {
    return "load:" + pointerOriginFingerprint(Load->getPointerOperand(), DL,
                                              FunctionIndices, Depth + 1);
  }
  if (auto *I = llvm::dyn_cast<llvm::Instruction>(Value)) {
    if (llvm::isa<llvm::BitCastInst, llvm::AddrSpaceCastInst, llvm::FreezeInst>(
            I)) {
      return pointerOriginFingerprint(I->getOperand(0), DL, FunctionIndices,
                                      Depth + 1);
    }
    if (llvm::isa<llvm::PHINode, llvm::SelectInst>(I)) {
      std::set<std::string> Origins;
      for (const llvm::Use &Operand : I->operands()) {
        if (Operand->getType()->isPointerTy()) {
          Origins.insert(pointerOriginFingerprint(Operand.get(), DL,
                                                  FunctionIndices, Depth + 1));
        }
      }
      std::string Result = "merge:";
      for (const std::string &Origin : Origins) {
        Result += Origin + ',';
      }
      return Result;
    }
  }
  return "other";
}

bool pointerDerivedFrom(const llvm::Value *Value, const llvm::Value *Root) {
  if (Value == Root) {
    return true;
  }
  llvm::SmallPtrSet<const llvm::Value *, 32> Seen;
  llvm::SmallVector<const llvm::Value *, 16> Worklist;
  Worklist.push_back(Value);
  unsigned Walked = 0;
  while (!Worklist.empty() && Walked++ < kMaxValueWalk) {
    const llvm::Value *Current = Worklist.pop_back_val();
    if (Current == Root) {
      return true;
    }
    if (!Seen.insert(Current).second) {
      continue;
    }
    auto *I = llvm::dyn_cast<llvm::Instruction>(Current);
    if (I == nullptr || !llvm::isa<llvm::GetElementPtrInst, llvm::BitCastInst,
                                   llvm::AddrSpaceCastInst, llvm::FreezeInst,
                                   llvm::PHINode, llvm::SelectInst>(I)) {
      continue;
    }
    for (const llvm::Use &Operand : I->operands()) {
      if (Operand->getType()->isPointerTy()) {
        Worklist.push_back(Operand.get());
      }
    }
  }
  return false;
}

bool pointerRootedAtGlobalImpl(
    const llvm::Value *Value,
    llvm::SmallPtrSetImpl<const llvm::Value *> &Seen) {
  if (llvm::isa<llvm::GlobalValue>(Value)) {
    return true;
  }
  if (Value == nullptr || !Seen.insert(Value).second) {
    return false;
  }
  auto *I = llvm::dyn_cast<llvm::Instruction>(Value);
  if (I == nullptr || !llvm::isa<llvm::GetElementPtrInst, llvm::BitCastInst,
                                 llvm::AddrSpaceCastInst, llvm::FreezeInst,
                                 llvm::PHINode, llvm::SelectInst>(I)) {
    return false;
  }
  for (const llvm::Use &Operand : I->operands()) {
    if (Operand->getType()->isPointerTy() &&
        pointerRootedAtGlobalImpl(Operand.get(), Seen)) {
      return true;
    }
  }
  return false;
}

bool pointerRootedAtGlobal(const llvm::Value *Value) {
  llvm::SmallPtrSet<const llvm::Value *, 32> Seen;
  return pointerRootedAtGlobalImpl(Value, Seen);
}

void collectReturnSources(const llvm::Value *Value, std::uint32_t &Sources,
                          std::set<const llvm::CallBase *> &Calls,
                          std::set<const llvm::LoadInst *> &Loads,
                          llvm::SmallPtrSetImpl<const llvm::Value *> &Seen) {
  if (Value == nullptr || llvm::isa<llvm::ConstantPointerNull>(Value) ||
      !Seen.insert(Value).second) {
    return;
  }
  if (auto *Arg = llvm::dyn_cast<llvm::Argument>(Value)) {
    (void)Arg;
    Sources |= ReturnArgument;
    return;
  }
  if (llvm::isa<llvm::GlobalValue>(Value)) {
    Sources |= ReturnGlobal;
    return;
  }
  if (llvm::isa<llvm::AllocaInst>(Value)) {
    Sources |= ReturnStack;
    return;
  }
  if (auto *CB = llvm::dyn_cast<llvm::CallBase>(Value)) {
    Sources |= ReturnCall;
    Calls.insert(CB);
    return;
  }
  if (llvm::isa<llvm::IntToPtrInst>(Value)) {
    Sources |= ReturnIntToPtr;
    return;
  }
  if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(Value)) {
    Sources |= ReturnLoad;
    Loads.insert(Load);
    return;
  }
  auto *I = llvm::dyn_cast<llvm::Instruction>(Value);
  if (I != nullptr && llvm::isa<llvm::GetElementPtrInst, llvm::BitCastInst,
                                llvm::AddrSpaceCastInst, llvm::FreezeInst,
                                llvm::PHINode, llvm::SelectInst>(I)) {
    for (const llvm::Use &Operand : I->operands()) {
      if (Operand->getType()->isPointerTy()) {
        collectReturnSources(Operand.get(), Sources, Calls, Loads, Seen);
      }
    }
    return;
  }
  Sources |= ReturnOther;
}

bool usesOnlyTransparentReturnFlowImpl(
    const llvm::Value *Value, llvm::SmallPtrSetImpl<const llvm::Value *> &Seen,
    unsigned &Budget) {
  if (Budget == 0) {
    return false;
  }
  --Budget;
  if (!Seen.insert(Value).second) {
    return true;
  }
  for (const llvm::User *User : Value->users()) {
    auto *I = llvm::dyn_cast<llvm::Instruction>(User);
    if (I == nullptr) {
      return false;
    }
    if (llvm::isa<llvm::ReturnInst, llvm::ICmpInst>(I)) {
      continue;
    }
    if (llvm::isa<llvm::BitCastInst, llvm::AddrSpaceCastInst, llvm::FreezeInst,
                  llvm::PHINode, llvm::SelectInst>(I)) {
      if (!usesOnlyTransparentReturnFlowImpl(I, Seen, Budget)) {
        return false;
      }
      continue;
    }
    return false;
  }
  return true;
}

bool usesOnlyTransparentReturnFlow(const llvm::Value *Value) {
  llvm::SmallPtrSet<const llvm::Value *, 32> Seen;
  unsigned Budget = kMaxValueWalk;
  return usesOnlyTransparentReturnFlowImpl(Value, Seen, Budget);
}

std::string confidenceForScore(int Score, int HighThreshold,
                               int MediumThreshold) {
  if (Score >= HighThreshold) {
    return "high";
  }
  if (Score >= MediumThreshold) {
    return "medium";
  }
  return "low";
}

bool hasPointerInterface(const llvm::Function &F) {
  if (F.getReturnType()->isPointerTy()) {
    return true;
  }
  return std::any_of(F.arg_begin(), F.arg_end(), [](const llvm::Argument &Arg) {
    return Arg.getType()->isPointerTy();
  });
}

RawBufferSlotEvidence analyzeRawBufferSlot(
    const llvm::Function &F, const llvm::Argument &PointerArg,
    llvm::ArrayRef<const llvm::CallBase *> Calls, const llvm::DataLayout &DL,
    const std::map<const llvm::Function *, unsigned> &FunctionIndices) {
  RawBufferSlotEvidence Result;
  Result.ArgIndex = PointerArg.getArgNo();

  std::vector<const llvm::Argument *> IntegerArgs;
  for (const llvm::Argument &Arg : F.args()) {
    if (Arg.getType()->isIntegerTy()) {
      IntegerArgs.push_back(&Arg);
    }
  }
  Result.HasIntegerPeer = !IntegerArgs.empty();
  if (PointerArg.getArgNo() + 1 < F.arg_size()) {
    Result.HasAdjacentIntegerPeer =
        F.getArg(PointerArg.getArgNo() + 1)->getType()->isIntegerTy();
  }

  PointerUseShape BodyShape =
      analyzePointerUses(&PointerArg, DL, FunctionIndices);
  for (const auto &[Offset, Width, Kind] : BodyShape.FixedAccesses) {
    (void)Kind;
    Result.HasByteAccess |= Width > 0 && Width <= 8;
    Result.HasFixedWideAccess |= Offset != 0 && Width > 8;
  }
  Result.HasDynamicOffsetAccess = BodyShape.DynamicAccess;

  // A forwarding wrapper is especially informative when the same nested call
  // receives both this pointer and an integer derived from a formal length.
  // This recognizes chains such as tty_add -> evbuffer_add without knowing
  // either symbol.
  for (const llvm::BasicBlock &BB : F) {
    for (const llvm::Instruction &I : BB) {
      auto *CB = llvm::dyn_cast<llvm::CallBase>(&I);
      if (CB == nullptr) {
        continue;
      }
      std::vector<unsigned> PointerPositions;
      std::vector<unsigned> LengthPositions;
      for (unsigned Index = 0; Index < CB->arg_size(); ++Index) {
        const llvm::Value *Actual = CB->getArgOperand(Index);
        if (pointerDerivedFrom(Actual, &PointerArg)) {
          PointerPositions.push_back(Index);
        }
        if (Actual->getType()->isIntegerTy()) {
          for (const llvm::Argument *IntegerArg : IntegerArgs) {
            if (valueDependsOn(Actual, IntegerArg)) {
              LengthPositions.push_back(Index);
              break;
            }
          }
        }
      }
      if (PointerPositions.empty() || LengthPositions.empty()) {
        continue;
      }
      ++Result.PairedForwardCalls;

      // A declaration is conservatively accepted as an unknown byte-oriented
      // leaf.  For a defined callee, require its corresponding formal slot to
      // exhibit byte/dynamic access; this rejects fixed-object forwarding such
      // as item_replace(old, new, hash_value).
      bool Validated = false;
      auto *Callee = CB->getCalledFunction();
      if (Callee != nullptr && Callee->isDeclaration()) {
        Validated = true;
      } else if (Callee != nullptr) {
        for (unsigned PointerPosition : PointerPositions) {
          if (PointerPosition >= Callee->arg_size()) {
            continue;
          }
          const llvm::Argument *CalleeArg = Callee->getArg(PointerPosition);
          if (!CalleeArg->getType()->isPointerTy()) {
            continue;
          }
          PointerUseShape CalleeShape =
              analyzePointerUses(CalleeArg, DL, FunctionIndices);
          bool HasByte = false;
          bool HasFixedWide = false;
          for (const auto &[Offset, Width, Kind] : CalleeShape.FixedAccesses) {
            (void)Kind;
            HasByte |= Width > 0 && Width <= 8;
            HasFixedWide |= Offset != 0 && Width > 8;
          }
          Validated |= (HasByte || CalleeShape.DynamicAccess) && !HasFixedWide;
        }
      }
      if (!Validated) {
        continue;
      }
      ++Result.ValidatedForwardCalls;
      for (unsigned PointerPosition : PointerPositions) {
        for (unsigned LengthPosition : LengthPositions) {
          Result.AdjacentLengthForwardCalls +=
              PointerPosition + 1 == LengthPosition;
        }
      }
    }
  }

  std::set<std::string> ActualShapes;
  for (const llvm::CallBase *CB : Calls) {
    if (PointerArg.getArgNo() >= CB->arg_size()) {
      continue;
    }
    const llvm::Value *Actual = CB->getArgOperand(PointerArg.getArgNo());
    if (!Actual->getType()->isPointerTy()) {
      continue;
    }
    ActualShapes.insert(
        pointerOriginFingerprint(Actual, DL, FunctionIndices) + '|' +
        analyzePointerUses(Actual, DL, FunctionIndices, CB).fingerprint());
  }
  Result.DistinctActualShapes = ActualShapes.size();

  if (Result.AdjacentLengthForwardCalls > 0) {
    Result.Score += 4;
  } else if (Result.ValidatedForwardCalls > 0) {
    Result.Score += 2;
  }
  if (F.isDeclaration() && Result.HasAdjacentIntegerPeer) {
    Result.Score += 3;
  }
  if (Result.HasByteAccess) {
    Result.Score += 3;
  }
  if (Result.HasDynamicOffsetAccess) {
    Result.Score += 2;
  }
  if (Calls.size() >= 2) {
    ++Result.Score;
  }
  if (Calls.size() >= 4) {
    ++Result.Score;
  }
  if (Calls.size() >= 8) {
    ++Result.Score;
  }
  if (Result.DistinctActualShapes >= 2) {
    Result.Score += 2;
  }
  if (Result.DistinctActualShapes >= 4) {
    ++Result.Score;
  }
  if (Result.HasFixedWideAccess) {
    Result.Score -= 5;
  }

  // A pointer with neither byte-wise behavior nor length-paired forwarding is
  // normally an owner/context object, even when it has many callers.
  if (Result.ValidatedForwardCalls == 0 && !Result.HasByteAccess &&
      !Result.HasDynamicOffsetAccess &&
      !(F.isDeclaration() && Result.HasAdjacentIntegerPeer)) {
    Result.Score = std::min(Result.Score, 2);
  }
  Result.Confidence = confidenceForScore(Result.Score, 9, 5);
  if (F.isDeclaration() && Result.Confidence == "high") {
    Result.Confidence = "medium";
  }
  return Result;
}

DeallocatorSlotEvidence analyzeDeallocatorSlot(
    const llvm::Function &F, const llvm::Argument &PointerArg,
    llvm::ArrayRef<const llvm::CallBase *> Calls, const llvm::DataLayout &DL,
    const std::map<const llvm::Function *, unsigned> &FunctionIndices) {
  DeallocatorSlotEvidence Result;
  Result.ArgIndex = PointerArg.getArgNo();

  PointerUseShape BodyShape =
      analyzePointerUses(&PointerArg, DL, FunctionIndices);
  Result.Compared = BodyShape.Compared;
  Result.HasMemoryAccess = BodyShape.hasMemoryAccess();
  for (const auto &[Offset, Width, Kind] : BodyShape.FixedAccesses) {
    (void)Kind;
    Result.HasFixedWideAccess |= Offset != 0 && Width > 8;
  }

  for (const llvm::BasicBlock &BB : F) {
    for (const llvm::Instruction &I : BB) {
      if (auto *Store = llvm::dyn_cast<llvm::StoreInst>(&I)) {
        Result.MutatesGlobalState |=
            pointerRootedAtGlobal(Store->getPointerOperand());
      } else if (auto *Atomic = llvm::dyn_cast<llvm::AtomicRMWInst>(&I)) {
        Result.MutatesGlobalState |=
            pointerRootedAtGlobal(Atomic->getPointerOperand());
      } else if (auto *CmpXchg = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(&I)) {
        Result.MutatesGlobalState |=
            pointerRootedAtGlobal(CmpXchg->getPointerOperand());
      }

      auto *CB = llvm::dyn_cast<llvm::CallBase>(&I);
      auto *Callee = CB == nullptr ? nullptr : CB->getCalledFunction();
      if (Callee == nullptr || !Callee->getReturnType()->isVoidTy() ||
          Callee->arg_size() != 1 || CB->arg_size() != 1 ||
          !Callee->getArg(0)->getType()->isPointerTy()) {
        continue;
      }
      Result.PointerOnlyVoidForwardCalls +=
          pointerDerivedFrom(CB->getArgOperand(0), &PointerArg);
    }
  }

  std::set<std::string> ActualOrigins;
  for (const llvm::CallBase *CB : Calls) {
    if (PointerArg.getArgNo() >= CB->arg_size()) {
      continue;
    }
    const llvm::Value *Actual = CB->getArgOperand(PointerArg.getArgNo());
    if (Actual->getType()->isPointerTy()) {
      ActualOrigins.insert(
          pointerOriginFingerprint(Actual, DL, FunctionIndices));
    }
  }
  Result.DistinctActualOrigins = ActualOrigins.size();

  if (F.isDeclaration() && F.getReturnType()->isVoidTy() && F.arg_size() == 1) {
    Result.Score += 3;
  }
  if (Result.PointerOnlyVoidForwardCalls > 0) {
    Result.Score += 4;
  }
  if (Result.Compared) {
    Result.Score += 2;
  }
  if (Result.MutatesGlobalState) {
    Result.Score += 2;
  }
  if (Calls.size() >= 3) {
    ++Result.Score;
  }
  if (Calls.size() >= 10) {
    ++Result.Score;
  }
  if (Result.DistinctActualOrigins >= 4) {
    ++Result.Score;
  }
  if (Result.HasMemoryAccess || Result.HasFixedWideAccess) {
    Result.Score -= 6;
  }
  if (!F.isDeclaration() && Result.PointerOnlyVoidForwardCalls == 0) {
    Result.Score = std::min(Result.Score, 2);
  }
  Result.Confidence = confidenceForScore(Result.Score, 9, 5);
  if (F.isDeclaration() && Result.Confidence == "high") {
    Result.Confidence = "medium";
  }
  return Result;
}

} // namespace

std::vector<AllocatorBoundaryEvidence>
analyzeAllocatorBoundaries(const llvm::Module &M) {
  const llvm::DataLayout &DL = M.getDataLayout();
  std::map<const llvm::Function *, unsigned> FunctionIndices;
  unsigned FunctionIndex = 0;
  for (const llvm::Function &F : M) {
    FunctionIndices.emplace(&F, FunctionIndex++);
  }

  std::map<const llvm::Function *, std::vector<const llvm::CallBase *>>
      DirectCalls;
  for (const llvm::Function &Caller : M) {
    for (const llvm::BasicBlock &BB : Caller) {
      for (const llvm::Instruction &I : BB) {
        auto *CB = llvm::dyn_cast<llvm::CallBase>(&I);
        if (CB != nullptr && CB->getCalledFunction() != nullptr) {
          DirectCalls[CB->getCalledFunction()].push_back(CB);
        }
      }
    }
  }

  std::vector<AllocatorBoundaryEvidence> Results;
  std::map<const llvm::Function *, std::set<const llvm::Function *>>
      ReturnedCallees;
  std::map<const llvm::Function *, std::set<std::string>> DirectResultShapes;
  for (const llvm::Function &F : M) {
    auto CallIt = DirectCalls.find(&F);
    if (!hasPointerInterface(F) || CallIt == DirectCalls.end() ||
        CallIt->second.empty()) {
      continue;
    }
    llvm::ArrayRef<const llvm::CallBase *> Calls(CallIt->second);

    AllocatorBoundaryEvidence Result;
    Result.Function = &F;
    Result.FunctionIndex = FunctionIndices.at(&F);
    Result.DirectCallSites = Calls.size();
    std::set<const llvm::Function *> Callers;
    for (const llvm::CallBase *CB : Calls) {
      Callers.insert(CB->getFunction());
    }
    Result.DistinctCallers = Callers.size();
    Result.ReturnsPointer = F.getReturnType()->isPointerTy();

    std::uint32_t Sources = ReturnNone;
    std::set<const llvm::CallBase *> ReturnedCalls;
    std::set<const llvm::LoadInst *> ReturnedLoads;
    if (Result.ReturnsPointer && !F.isDeclaration()) {
      llvm::SmallPtrSet<const llvm::Value *, 32> Seen;
      for (const llvm::BasicBlock &BB : F) {
        if (auto *RI = llvm::dyn_cast<llvm::ReturnInst>(BB.getTerminator())) {
          collectReturnSources(RI->getReturnValue(), Sources, ReturnedCalls,
                               ReturnedLoads, Seen);
        }
      }
    }
    Result.ReturnFromCall = (Sources & ReturnCall) != 0;
    Result.ReturnFromArgument = (Sources & ReturnArgument) != 0;
    Result.ReturnFromGlobal = (Sources & ReturnGlobal) != 0;
    Result.ReturnFromStack = (Sources & ReturnStack) != 0;
    Result.ReturnFromIntToPtr = (Sources & ReturnIntToPtr) != 0;
    Result.ReturnFromLoad = (Sources & ReturnLoad) != 0;

    for (const llvm::CallBase *ReturnedCall : ReturnedCalls) {
      if (auto *Callee = ReturnedCall->getCalledFunction()) {
        ReturnedCallees[&F].insert(Callee);
      }
      for (const llvm::Argument &Arg : F.args()) {
        for (const llvm::Use &Actual : ReturnedCall->args()) {
          if (!Actual->getType()->isIntegerTy()) {
            continue;
          }
          if (Arg.getType()->isIntegerTy()) {
            Result.IntegerArgumentFlowsToReturnedCall |=
                valueDependsOn(Actual.get(), &Arg);
          } else if (Arg.getType()->isPointerTy()) {
            Result.PointerArgumentFlowsToReturnedCall |=
                valueTransitivelyDependsOn(Actual.get(), &Arg);
          }
        }
      }
    }
    if (ReturnedCalls.size() == 1) {
      Result.PureReturnedCallForwarder =
          Sources == ReturnCall &&
          usesOnlyTransparentReturnFlow(*ReturnedCalls.begin());
    }
    for (const llvm::CallBase *Call : ReturnedCalls) {
      Result.ReturnedStorageAccessedInBody |=
          analyzePointerUses(Call, DL, FunctionIndices).hasMemoryAccess();
    }
    if (!ReturnedCalls.empty() && Sources == ReturnCall &&
        !Result.ReturnedStorageAccessedInBody) {
      // Calls such as malloc_usable_size may observe the allocation without
      // fixing its layout.  Multiple returned calls also cover reallocators
      // that select malloc or realloc on different CFG branches.
      Result.PureReturnedCallForwarder = std::none_of(
          ReturnedCalls.begin(), ReturnedCalls.end(),
          [&](const llvm::CallBase *Call) {
            return analyzePointerUses(Call, DL, FunctionIndices).StoredAsValue;
          });
    }

    bool ReturnedPointerLoadedFromGlobalState =
        std::any_of(ReturnedLoads.begin(), ReturnedLoads.end(),
                    [](const llvm::LoadInst *Load) {
                      return pointerRootedAtGlobal(Load->getPointerOperand());
                    });
    bool MutatesGlobalState = false;
    bool IntegerArgumentControlsGlobalAddress = false;
    for (const llvm::BasicBlock &BB : F) {
      for (const llvm::Instruction &I : BB) {
        if (auto *Store = llvm::dyn_cast<llvm::StoreInst>(&I)) {
          MutatesGlobalState |=
              pointerRootedAtGlobal(Store->getPointerOperand());
        }
        auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(&I);
        if (GEP == nullptr || !pointerRootedAtGlobal(GEP)) {
          continue;
        }
        for (const llvm::Argument &Arg : F.args()) {
          if (!Arg.getType()->isIntegerTy()) {
            continue;
          }
          for (const llvm::Use &Index : GEP->indices()) {
            IntegerArgumentControlsGlobalAddress |=
                valueDependsOn(Index.get(), &Arg);
          }
        }
      }
    }
    Result.CoreAllocatorStateEvidence = ReturnedPointerLoadedFromGlobalState &&
                                        MutatesGlobalState &&
                                        IntegerArgumentControlsGlobalAddress;

    std::set<std::string> ResultShapes;
    if (Result.ReturnsPointer) {
      for (const llvm::CallBase *CB : Calls) {
        PointerUseShape Shape = analyzePointerUses(CB, DL, FunctionIndices);
        Result.StructuredResultCallSites += Shape.hasMemoryAccess();
        ResultShapes.insert(Shape.layoutFingerprint());
      }
    }
    Result.DistinctResultShapes = ResultShapes.size();
    DirectResultShapes.emplace(&F, std::move(ResultShapes));

    if (Result.ReturnsPointer) {
      ++Result.AllocatorScore;
    }
    if (Result.PureReturnedCallForwarder) {
      Result.AllocatorScore += 5;
    } else if (Result.ReturnFromCall || Result.ReturnFromIntToPtr) {
      Result.AllocatorScore += 2;
    }
    if (Result.IntegerArgumentFlowsToReturnedCall) {
      Result.AllocatorScore += 2;
    }
    if (Result.PointerArgumentFlowsToReturnedCall) {
      Result.AllocatorScore += 2;
    }
    if (Result.CoreAllocatorStateEvidence) {
      Result.AllocatorScore += 8;
    }
    if (Result.DirectCallSites >= 3) {
      ++Result.AllocatorScore;
    }
    if (Result.DirectCallSites >= 10) {
      ++Result.AllocatorScore;
    }
    if (Result.DistinctCallers >= 2) {
      ++Result.AllocatorScore;
    }
    if (Result.DistinctResultShapes >= 2) {
      Result.AllocatorScore += 2;
    }
    if (Result.DistinctResultShapes >= 4) {
      ++Result.AllocatorScore;
    }
    if (Result.StructuredResultCallSites >= 2) {
      ++Result.AllocatorScore;
    }
    if (Result.ReturnFromArgument || Result.ReturnFromGlobal ||
        Result.ReturnFromStack) {
      Result.AllocatorScore -= 7;
    }
    if (Result.ReturnedStorageAccessedInBody) {
      Result.AllocatorScore -= Result.CoreAllocatorStateEvidence ? 1 : 6;
    }

    // A defined function needs an instruction-level fresh-storage hint.  A
    // declaration cannot provide one, so it remains at most medium confidence
    // and serves only as a boundary candidate for later interprocedural study.
    bool HasFreshStorageHint = Result.PureReturnedCallForwarder ||
                               Result.ReturnFromIntToPtr ||
                               Result.CoreAllocatorStateEvidence;
    if (!F.isDeclaration() && !HasFreshStorageHint) {
      Result.AllocatorScore = std::min(Result.AllocatorScore, 4);
    }
    if (F.isDeclaration()) {
      Result.AllocatorScore = std::min(Result.AllocatorScore, 7);
    }
    Result.AllocatorConfidence =
        confidenceForScore(Result.AllocatorScore, 9, 6);

    for (const llvm::Argument &Arg : F.args()) {
      if (Arg.getType()->isPointerTy()) {
        Result.RawBufferSlots.push_back(
            analyzeRawBufferSlot(F, Arg, Calls, DL, FunctionIndices));
        Result.DeallocatorSlots.push_back(
            analyzeDeallocatorSlot(F, Arg, Calls, DL, FunctionIndices));
      }
    }
    Results.push_back(std::move(Result));
  }

  // A pure wrapper is only as allocator-like as the storage origin it returns.
  // Propagating this fact down a forwarding chain rejects object lookup APIs
  // that happen to have allocator-like size arguments at their outer wrapper.
  std::map<const llvm::Function *, AllocatorBoundaryEvidence *> ByFunction;
  for (AllocatorBoundaryEvidence &Result : Results) {
    ByFunction.emplace(Result.Function, &Result);
  }

  // Imported allocators often have only a few local uses, all inside wrappers
  // that perform accounting.  Propagate the wrapper's downstream layout
  // diversity back through transparent return edges.  This remains structural:
  // neither the imported symbol nor source attributes are consulted.
  auto PropagatedResultShapes = DirectResultShapes;
  std::map<const llvm::Function *, unsigned> PropagatedStructuredSites;
  for (const AllocatorBoundaryEvidence &Result : Results) {
    PropagatedStructuredSites.emplace(Result.Function,
                                      Result.StructuredResultCallSites);
  }
  for (std::size_t Iteration = 0; Iteration < Results.size(); ++Iteration) {
    bool Changed = false;
    for (const AllocatorBoundaryEvidence &Wrapper : Results) {
      if (!Wrapper.PureReturnedCallForwarder ||
          (!Wrapper.IntegerArgumentFlowsToReturnedCall &&
           !Wrapper.PointerArgumentFlowsToReturnedCall)) {
        continue;
      }
      auto CalleesIt = ReturnedCallees.find(Wrapper.Function);
      if (CalleesIt == ReturnedCallees.end()) {
        continue;
      }
      for (const llvm::Function *Callee : CalleesIt->second) {
        auto &CalleeShapes = PropagatedResultShapes[Callee];
        std::size_t Before = CalleeShapes.size();
        const auto &WrapperShapes = PropagatedResultShapes[Wrapper.Function];
        CalleeShapes.insert(WrapperShapes.begin(), WrapperShapes.end());
        Changed |= CalleeShapes.size() != Before;

        unsigned &CalleeStructured = PropagatedStructuredSites[Callee];
        unsigned WrapperStructured =
            PropagatedStructuredSites[Wrapper.Function];
        if (WrapperStructured > CalleeStructured) {
          CalleeStructured = WrapperStructured;
          Changed = true;
        }
      }
    }
    if (!Changed) {
      break;
    }
  }
  for (AllocatorBoundaryEvidence &Result : Results) {
    Result.PropagatedDistinctResultShapes =
        PropagatedResultShapes[Result.Function].size();
    Result.PropagatedStructuredResultCallSites =
        PropagatedStructuredSites[Result.Function];
  }

  std::map<const llvm::Function *, bool> OriginMemo;
  std::set<const llvm::Function *> OriginStack;
  std::function<bool(const llvm::Function *)> HasAllocatorOrigin =
      [&](const llvm::Function *F) -> bool {
    if (F == nullptr || !F->getReturnType()->isPointerTy()) {
      return false;
    }
    if (auto It = OriginMemo.find(F); It != OriginMemo.end()) {
      return It->second;
    }
    if (!OriginStack.insert(F).second) {
      return false;
    }
    bool Result = false;
    if (F->isDeclaration()) {
      Result = std::any_of(F->arg_begin(), F->arg_end(),
                           [](const llvm::Argument &Arg) {
                             return Arg.getType()->isIntegerTy();
                           });
      auto EvidenceIt = ByFunction.find(F);
      Result = Result && EvidenceIt != ByFunction.end() &&
               EvidenceIt->second->DirectCallSites >= 3 &&
               EvidenceIt->second->PropagatedDistinctResultShapes >= 4 &&
               EvidenceIt->second->PropagatedStructuredResultCallSites >= 2;
    } else if (auto It = ByFunction.find(F); It != ByFunction.end()) {
      const AllocatorBoundaryEvidence &Evidence = *It->second;
      Result =
          Evidence.CoreAllocatorStateEvidence || Evidence.ReturnFromIntToPtr;
      if (!Result && Evidence.PureReturnedCallForwarder &&
          (Evidence.IntegerArgumentFlowsToReturnedCall ||
           Evidence.PointerArgumentFlowsToReturnedCall)) {
        auto CalleesIt = ReturnedCallees.find(F);
        Result = CalleesIt != ReturnedCallees.end() &&
                 !CalleesIt->second.empty() &&
                 std::all_of(CalleesIt->second.begin(), CalleesIt->second.end(),
                             HasAllocatorOrigin);
      }
    }
    OriginStack.erase(F);
    OriginMemo.emplace(F, Result);
    return Result;
  };

  for (AllocatorBoundaryEvidence &Result : Results) {
    if (!Result.PureReturnedCallForwarder) {
      continue;
    }
    auto CalleesIt = ReturnedCallees.find(Result.Function);
    Result.ReturnedCalleeHasAllocatorOrigin =
        CalleesIt != ReturnedCallees.end() && !CalleesIt->second.empty() &&
        std::all_of(CalleesIt->second.begin(), CalleesIt->second.end(),
                    HasAllocatorOrigin);
    bool HasSizeLikeInput = Result.IntegerArgumentFlowsToReturnedCall ||
                            Result.PointerArgumentFlowsToReturnedCall;
    if (!Result.ReturnedCalleeHasAllocatorOrigin || !HasSizeLikeInput) {
      Result.AllocatorScore = std::min(Result.AllocatorScore, 4);
    }
    Result.AllocatorConfidence =
        confidenceForScore(Result.AllocatorScore, 9, 6);
  }
  return Results;
}

void writeAllocatorBoundaryReport(
    const llvm::Module &M,
    const std::vector<AllocatorBoundaryEvidence> &Evidence,
    llvm::raw_ostream &Out) {
  (void)M;
  for (const AllocatorBoundaryEvidence &Entry : Evidence) {
    llvm::json::Object Object;
    Object["function_index"] = Entry.FunctionIndex;
    Object["audit_symbol"] = Entry.Function->hasName()
                                 ? Entry.Function->getName()
                                 : llvm::StringRef();
    Object["direct_call_sites"] = Entry.DirectCallSites;
    Object["distinct_callers"] = Entry.DistinctCallers;

    llvm::json::Object Allocator;
    Allocator["score"] = Entry.AllocatorScore;
    Allocator["confidence"] = Entry.AllocatorConfidence;
    Allocator["returns_pointer"] = Entry.ReturnsPointer;
    Allocator["pure_returned_call_forwarder"] = Entry.PureReturnedCallForwarder;
    Allocator["return_from_call"] = Entry.ReturnFromCall;
    Allocator["return_from_argument"] = Entry.ReturnFromArgument;
    Allocator["return_from_global"] = Entry.ReturnFromGlobal;
    Allocator["return_from_stack"] = Entry.ReturnFromStack;
    Allocator["return_from_inttoptr"] = Entry.ReturnFromIntToPtr;
    Allocator["return_from_load"] = Entry.ReturnFromLoad;
    Allocator["returned_storage_accessed_in_body"] =
        Entry.ReturnedStorageAccessedInBody;
    Allocator["integer_argument_flows_to_returned_call"] =
        Entry.IntegerArgumentFlowsToReturnedCall;
    Allocator["pointer_argument_flows_to_returned_call"] =
        Entry.PointerArgumentFlowsToReturnedCall;
    Allocator["core_allocator_state_evidence"] =
        Entry.CoreAllocatorStateEvidence;
    Allocator["returned_callee_has_allocator_origin"] =
        Entry.ReturnedCalleeHasAllocatorOrigin;
    Allocator["structured_result_call_sites"] = Entry.StructuredResultCallSites;
    Allocator["distinct_result_shapes"] = Entry.DistinctResultShapes;
    Allocator["propagated_structured_result_call_sites"] =
        Entry.PropagatedStructuredResultCallSites;
    Allocator["propagated_distinct_result_shapes"] =
        Entry.PropagatedDistinctResultShapes;
    Object["allocator"] = std::move(Allocator);

    llvm::json::Array Slots;
    for (const RawBufferSlotEvidence &Slot : Entry.RawBufferSlots) {
      llvm::json::Object SlotObject;
      SlotObject["arg_index"] = Slot.ArgIndex;
      SlotObject["score"] = Slot.Score;
      SlotObject["confidence"] = Slot.Confidence;
      SlotObject["has_integer_peer"] = Slot.HasIntegerPeer;
      SlotObject["has_adjacent_integer_peer"] = Slot.HasAdjacentIntegerPeer;
      SlotObject["has_byte_access"] = Slot.HasByteAccess;
      SlotObject["has_dynamic_offset_access"] = Slot.HasDynamicOffsetAccess;
      SlotObject["has_fixed_wide_access"] = Slot.HasFixedWideAccess;
      SlotObject["paired_forward_calls"] = Slot.PairedForwardCalls;
      SlotObject["validated_forward_calls"] = Slot.ValidatedForwardCalls;
      SlotObject["adjacent_length_forward_calls"] =
          Slot.AdjacentLengthForwardCalls;
      SlotObject["distinct_actual_shapes"] = Slot.DistinctActualShapes;
      Slots.push_back(std::move(SlotObject));
    }
    Object["raw_buffer_slots"] = std::move(Slots);

    llvm::json::Array DeallocatorSlots;
    for (const DeallocatorSlotEvidence &Slot :
         Entry.DeallocatorSlots) {
      llvm::json::Object SlotObject;
      SlotObject["arg_index"] = Slot.ArgIndex;
      SlotObject["score"] = Slot.Score;
      SlotObject["confidence"] = Slot.Confidence;
      SlotObject["pointer_only_void_forward_calls"] =
          Slot.PointerOnlyVoidForwardCalls;
      SlotObject["compared"] = Slot.Compared;
      SlotObject["mutates_global_state"] = Slot.MutatesGlobalState;
      SlotObject["has_memory_access"] = Slot.HasMemoryAccess;
      SlotObject["has_fixed_wide_access"] = Slot.HasFixedWideAccess;
      SlotObject["distinct_actual_origins"] = Slot.DistinctActualOrigins;
      DeallocatorSlots.push_back(std::move(SlotObject));
    }
    Object["deallocator_slots"] = std::move(DeallocatorSlots);
    Out << llvm::formatv("{0}\n", llvm::json::Value(std::move(Object)));
  }
}

// ---------------------------------------------------------------------------
// Container boundaries (see the header): the decision comes from the call
// sites, not from the body.
// ---------------------------------------------------------------------------
namespace {

// A field address the function computes: inttoptr(base + K) where the base is a
// parameter or a call result (an object the function allocated itself).
struct TrackedFieldOrigin {
  const llvm::Value *Base = nullptr;
  std::int64_t Offset = 0;
  bool FromArgument = false;
};

// Raw per-function facts of the first module sweep, before the module-wide
// callback-offset table exists.
;

// Resolve an integer value to (base, offset) while it is a parameter or a call
// result plus/minus constants.  This is the ABI-erased form of a field address:
// there are no typed GEPs in the lifted IR.
bool resolveObjectAddress(const llvm::Value *Value, const llvm::Value *&Base,
                          std::int64_t &Offset) {
  const llvm::Value *Cur = Value;
  std::int64_t Off = 0;
  for (unsigned Guard = 0; Guard < 8; ++Guard) {
    if (llvm::isa<llvm::Argument>(Cur) || llvm::isa<llvm::CallBase>(Cur)) {
      Base = Cur;
      Offset = Off;
      return true;
    }
    auto *BO = llvm::dyn_cast<llvm::BinaryOperator>(Cur);
    if (BO == nullptr) {
      return false;
    }
    const llvm::Value *L = BO->getOperand(0);
    const llvm::Value *R = BO->getOperand(1);
    if (BO->getOpcode() == llvm::Instruction::Add) {
      auto *C = llvm::dyn_cast<llvm::ConstantInt>(R);
      const llvm::Value *Next = L;
      if (C == nullptr) {
        C = llvm::dyn_cast<llvm::ConstantInt>(L);
        Next = R;
      }
      if (C == nullptr || C->getType()->getIntegerBitWidth() > 64) {
        return false;
      }
      Off += C->getSExtValue();
      Cur = Next;
      continue;
    }
    if (BO->getOpcode() == llvm::Instruction::Sub) {
      auto *C = llvm::dyn_cast<llvm::ConstantInt>(R);
      if (C == nullptr || C->getType()->getIntegerBitWidth() > 64) {
        return false;
      }
      Off -= C->getSExtValue();
      Cur = L;
      continue;
    }
    return false;
  }
  return false;
}

// Collect the field origins an indirect callee resolves to, following the
// SSA-preserving casts and phi/select merges the lifted IR uses for a merged
// function pointer.
void collectCalleeFields(
    const llvm::Value *Value,
    const std::map<const llvm::Value *, TrackedFieldOrigin> &Origins,
    llvm::SmallVectorImpl<TrackedFieldOrigin> &Out,
    llvm::SmallPtrSetImpl<const llvm::Value *> &Seen, unsigned &Budget) {
  if (Value == nullptr || Budget-- == 0 || !Seen.insert(Value).second) {
    return;
  }
  if (llvm::isa<llvm::Argument>(Value)) {
    return;
  }
  if (auto *L = llvm::dyn_cast<llvm::LoadInst>(Value)) {
    auto It = Origins.find(L->getPointerOperand());
    if (It != Origins.end()) {
      Out.push_back(It->second);
    }
    return;
  }
  if (auto *ITP = llvm::dyn_cast<llvm::IntToPtrInst>(Value)) {
    collectCalleeFields(ITP->getOperand(0), Origins, Out, Seen, Budget);
    return;
  }
  if (auto *BC = llvm::dyn_cast<llvm::BitCastInst>(Value)) {
    collectCalleeFields(BC->getOperand(0), Origins, Out, Seen, Budget);
    return;
  }
  if (auto *ASC = llvm::dyn_cast<llvm::AddrSpaceCastInst>(Value)) {
    collectCalleeFields(ASC->getOperand(0), Origins, Out, Seen, Budget);
    return;
  }
  if (auto *FI = llvm::dyn_cast<llvm::FreezeInst>(Value)) {
    collectCalleeFields(FI->getOperand(0), Origins, Out, Seen, Budget);
    return;
  }
  if (auto *PHI = llvm::dyn_cast<llvm::PHINode>(Value)) {
    for (const llvm::Value *In : PHI->incoming_values()) {
      collectCalleeFields(In, Origins, Out, Seen, Budget);
    }
    return;
  }
  if (auto *Sel = llvm::dyn_cast<llvm::SelectInst>(Value)) {
    collectCalleeFields(Sel->getTrueValue(), Origins, Out, Seen, Budget);
    collectCalleeFields(Sel->getFalseValue(), Origins, Out, Seen, Budget);
    return;
  }
}

// A value that came from outside the function (a parameter, possibly through a
// cast or a phi merge).  A parameter stored into a callback slot is how an
// initialiser installs the hasher/comparator.
bool isParameterDerived(const llvm::Value *Value) {
  llvm::SmallVector<const llvm::Value *, 8> Worklist{Value};
  llvm::SmallPtrSet<const llvm::Value *, 16> Seen;
  unsigned Budget = 32;
  while (!Worklist.empty() && Budget-- > 0) {
    const llvm::Value *V = Worklist.pop_back_val();
    if (V == nullptr || !Seen.insert(V).second) {
      continue;
    }
    if (llvm::isa<llvm::Argument>(V)) {
      return true;
    }
    if (llvm::isa<llvm::IntToPtrInst, llvm::BitCastInst,
                  llvm::AddrSpaceCastInst, llvm::FreezeInst>(V)) {
      Worklist.push_back(llvm::cast<llvm::Instruction>(V)->getOperand(0));
      continue;
    }
    if (auto *PHI = llvm::dyn_cast<llvm::PHINode>(V)) {
      for (const llvm::Value *In : PHI->incoming_values()) {
        Worklist.push_back(In);
      }
      continue;
    }
    if (auto *Sel = llvm::dyn_cast<llvm::SelectInst>(V)) {
      Worklist.push_back(Sel->getTrueValue());
      Worklist.push_back(Sel->getFalseValue());
      continue;
    }
  }
  return false;
}

// 64-bit constant store that equals the address of a lifted function.
bool isLiftedFunctionAddress(
    const llvm::Value *Value,
    const std::map<std::uint64_t, const llvm::Function *> &FunctionByAddress) {
  if (auto *CI = llvm::dyn_cast<llvm::ConstantInt>(Value)) {
    if (CI->getType()->getIntegerBitWidth() != 64) {
      return false;
    }
    return FunctionByAddress.count(CI->getZExtValue()) != 0;
  }
  if (auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(Value)) {
    if (CE->getOpcode() == llvm::Instruction::PtrToInt) {
      return llvm::isa<llvm::Function>(CE->getOperand(0));
    }
  }
  return false;
}

} // namespace

} // namespace notdec::mlsub
