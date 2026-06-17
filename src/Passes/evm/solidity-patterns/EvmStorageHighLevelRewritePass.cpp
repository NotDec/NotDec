#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "evm-storage-high-level-rewrite"

namespace notdec::passes::evm {
namespace {

FunctionCallee getStorageSlotFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.slot", I256, I256);
}

FunctionCallee getStorageFieldFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.field", I256, I256, I256);
}

FunctionCallee getStorageMapValueFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.map.value", I256, I256, I256);
}

FunctionCallee getStorageStaticArrayElemFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.static_array.elem", I256, I256,
                               I256);
}

FunctionCallee getStorageDynamicArrayElemFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.dynamic_array.elem", I256, I256,
                               I256);
}

FunctionCallee getStorageDynamicArrayLengthLoadFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.dynamic_array.length.load", I256,
                               I256);
}

FunctionCallee getStorageDynamicArrayLengthStoreFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.dynamic_array.length.store",
                               Type::getVoidTy(Ctx), I256, I256);
}

FunctionCallee getStoragePackedLoadFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.packed.load", I256, I256, I256,
                               I256);
}

FunctionCallee getStoragePackedStoreFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.packed.store",
                               Type::getVoidTy(Ctx), I256, I256, I256, I256);
}

FunctionCallee getStorageBytesLengthLoadFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.bytes.length.load", I256, I256);
}

FunctionCallee getStorageBytesShortDataLoadFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.bytes.short_data.load", I256, I256);
}

FunctionCallee getStorageBytesLongElemLoadFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.bytes.long_elem.load", I256, I256,
                               I256);
}

FunctionCallee getStorageLoadFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.load", I256, I256);
}

FunctionCallee getStorageStoreFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.storage.store", Type::getVoidTy(Ctx), I256,
                               I256);
}

ConstantInt *getI256(LLVMContext &Ctx, uint64_t Value) {
  return ConstantInt::get(Type::getIntNTy(Ctx, 256), Value);
}

std::optional<uint64_t> getUInt64Constant(ConstantInt *C) {
  if (C == nullptr || C->getValue().getActiveBits() > 64) {
    return std::nullopt;
  }
  return C->getZExtValue();
}

Value *getEvidenceValue(const ExtValuePtr &Evidence) {
  if (auto *V = std::get_if<Value *>(&Evidence)) {
    return *V;
  }
  if (auto *C = std::get_if<UConstant>(&Evidence)) {
    return C->Val;
  }
  return nullptr;
}

Value *buildStorageRef(Value *Slot, Instruction &InsertBefore,
                       SmallPtrSetImpl<Value *> &Seen);

void debugRewrite(StringRef Kind, const Instruction &OldInst, const Value &Ref) {
  LLVM_DEBUG(dbgs() << "evm storage rewrite: " << Kind << " in "
                    << OldInst.getFunction()->getName() << ": " << OldInst
                    << " -> " << Ref << "\n");
}

void debugSkip(StringRef Reason, const Instruction &Inst) {
  LLVM_DEBUG(dbgs() << "evm storage rewrite: skip " << Reason << " in "
                    << Inst.getFunction()->getName() << ": " << Inst << "\n");
}

bool callUsesStorageSlotAsDynamicArrayData(Function &F, Value *BaseSlot) {
  if (BaseSlot == nullptr) {
    return false;
  }
  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    Value *StorageSlot = nullptr;
    uint64_t AccessKind = 0;
    if (detail::isCallTo(Call, "evm_sload") && Call->arg_size() == 1) {
      StorageSlot = Call->getArgOperand(0);
      AccessKind = 1;
    } else if (detail::isCallTo(Call, "evm_sstore") && Call->arg_size() == 2) {
      StorageSlot = Call->getArgOperand(0);
      AccessKind = 2;
    } else {
      continue;
    }
    std::optional<detail::StorageArrayDataAccessMatch> Match =
        detail::matchStorageArrayDataAccess(*Call, StorageSlot, AccessKind);
    if (Match.has_value() &&
        detail::isSameValue(getEvidenceValue(Match->BaseSlot), BaseSlot)) {
      return true;
    }
  }
  return false;
}

bool isOneBitMask(Value *V) {
  return detail::isConstantIntValue(V, 1);
}

bool isShiftByOne(Value *V) {
  return detail::isConstantIntValue(V, 1);
}

bool isBytesLengthFullLen(Value *V, Value *Load) {
  if (auto *Shift = dyn_cast_or_null<BinaryOperator>(V)) {
    return Shift->getOpcode() == Instruction::LShr &&
           detail::isSameValue(Shift->getOperand(0), Load) &&
           isShiftByOne(Shift->getOperand(1));
  }
  auto *Call = dyn_cast_or_null<CallBase>(V);
  return Call != nullptr && detail::isCallTo(Call, "evm_shr") &&
         Call->arg_size() == 2 && isShiftByOne(Call->getArgOperand(0)) &&
         detail::isSameValue(Call->getArgOperand(1), Load);
}

bool isBytesLengthShortLen(Value *V, Value *FullLen) {
  auto *And = dyn_cast_or_null<BinaryOperator>(V);
  return And != nullptr && And->getOpcode() == Instruction::And &&
         ((detail::isSameValue(And->getOperand(0), FullLen) &&
           detail::isConstantIntValue(And->getOperand(1), 127)) ||
          (detail::isSameValue(And->getOperand(1), FullLen) &&
           detail::isConstantIntValue(And->getOperand(0), 127)));
}

bool matchBytesLengthValue(Value *V, Value *Load) {
  auto *Select = dyn_cast_or_null<SelectInst>(V);
  if (Select == nullptr) {
    return false;
  }

  auto *Cond = dyn_cast<ICmpInst>(Select->getCondition());
  if (Cond == nullptr || Cond->getPredicate() != ICmpInst::ICMP_EQ ||
      !detail::isConstantIntValue(Cond->getOperand(1), 0)) {
    return false;
  }

  auto *LowBit = dyn_cast<BinaryOperator>(Cond->getOperand(0));
  if (LowBit == nullptr || LowBit->getOpcode() != Instruction::And ||
      !((detail::isSameValue(LowBit->getOperand(0), Load) &&
         isOneBitMask(LowBit->getOperand(1))) ||
        (detail::isSameValue(LowBit->getOperand(1), Load) &&
         isOneBitMask(LowBit->getOperand(0))))) {
    return false;
  }

  Value *ShortLen = Select->getTrueValue();
  Value *FullLen = Select->getFalseValue();
  return isBytesLengthFullLen(FullLen, Load) &&
         isBytesLengthShortLen(ShortLen, FullLen);
}

bool hasBytesLengthValue(CallBase &Load) {
  SmallVector<Value *, 8> Worklist;
  SmallPtrSet<Value *, 16> Seen;
  for (User *User : Load.users()) {
    if (auto *V = dyn_cast<Value>(User)) {
      Worklist.push_back(V);
    }
  }

  unsigned Steps = 0;
  while (!Worklist.empty() && Steps++ < 32) {
    Value *V = Worklist.pop_back_val();
    if (V == nullptr || !Seen.insert(V).second) {
      continue;
    }
    if (matchBytesLengthValue(V, &Load)) {
      return true;
    }
    for (User *User : V->users()) {
      if (auto *UserValue = dyn_cast<Value>(User)) {
        Worklist.push_back(UserValue);
      }
    }
  }
  return false;
}

bool looksLikeBytesLengthDecode(Value *Load) {
  if (Load == nullptr) {
    return false;
  }
  for (User *U : Load->users()) {
    if (auto *Bin = dyn_cast<BinaryOperator>(U)) {
      if (Bin->getOpcode() == Instruction::And &&
          (isOneBitMask(Bin->getOperand(0)) ||
           isOneBitMask(Bin->getOperand(1)))) {
        return true;
      }
      if (Bin->getOpcode() == Instruction::LShr &&
          detail::isSameValue(Bin->getOperand(0), Load) &&
          isShiftByOne(Bin->getOperand(1))) {
        return true;
      }
    }
    auto *Call = dyn_cast<CallBase>(U);
    if (Call == nullptr || Call->arg_size() != 2) {
      continue;
    }
    if (detail::isCallTo(Call, "evm_and") &&
        (isOneBitMask(Call->getArgOperand(0)) ||
         isOneBitMask(Call->getArgOperand(1)))) {
      return true;
    }
    if (detail::isCallTo(Call, "evm_shr") &&
        isShiftByOne(Call->getArgOperand(0)) &&
        detail::isSameValue(Call->getArgOperand(1), Load)) {
      return true;
    }
  }
  return false;
}

bool isDynamicArrayLengthAccess(CallBase &Call) {
  if (Call.getFunction() == nullptr ||
      !detail::isCallTo(&Call, "evm_sload") || Call.arg_size() != 1 ||
      looksLikeBytesLengthDecode(&Call)) {
    return false;
  }
  return callUsesStorageSlotAsDynamicArrayData(*Call.getFunction(),
                                               Call.getArgOperand(0));
}

std::optional<unsigned> getLowMaskBitWidth(Value *V) {
  auto *CI = dyn_cast_or_null<ConstantInt>(V);
  if (CI == nullptr) {
    return std::nullopt;
  }
  const APInt &Mask = CI->getValue();
  unsigned Width = Mask.getActiveBits();
  if (Width == 0 || Width >= Mask.getBitWidth()) {
    return std::nullopt;
  }
  for (unsigned I = 0; I < Mask.getBitWidth(); ++I) {
    bool Expected = I < Width;
    if (Mask[I] != Expected) {
      return std::nullopt;
    }
  }
  return Width;
}

struct PackedBitRange {
  unsigned Offset = 0;
  unsigned Width = 0;
};

std::optional<PackedBitRange> getClearMaskBitRange(Value *V) {
  auto *CI = dyn_cast_or_null<ConstantInt>(V);
  if (CI == nullptr) {
    return std::nullopt;
  }

  const APInt &Mask = CI->getValue();
  if (Mask.isAllOnes() || Mask.isZero()) {
    return std::nullopt;
  }

  std::optional<unsigned> Start;
  unsigned End = 0;
  for (unsigned I = 0; I < Mask.getBitWidth(); ++I) {
    if (!Mask[I]) {
      if (!Start.has_value()) {
        Start = I;
      }
      End = I + 1;
    } else if (Start.has_value()) {
      for (unsigned J = I + 1; J < Mask.getBitWidth(); ++J) {
        if (!Mask[J]) {
          return std::nullopt;
        }
      }
      break;
    }
  }

  if (!Start.has_value() || End <= *Start) {
    return std::nullopt;
  }
  return PackedBitRange{.Offset = *Start, .Width = End - *Start};
}

bool matchPackedReadSource(Value *V, CallBase &Load, unsigned &Offset) {
  if (detail::isSameValue(V, &Load)) {
    Offset = 0;
    return true;
  }
  if (auto *Call = dyn_cast_or_null<CallBase>(V)) {
    if (detail::isCallTo(Call, "evm_shr") && Call->arg_size() == 2 &&
        detail::isSameValue(Call->getArgOperand(1), &Load)) {
      auto Shift = getUInt64Constant(dyn_cast<ConstantInt>(Call->getArgOperand(0)));
      if (Shift.has_value() && *Shift < 256) {
        Offset = static_cast<unsigned>(*Shift);
        return true;
      }
    }
  }
  if (auto *Shift = dyn_cast_or_null<BinaryOperator>(V)) {
    if (Shift->getOpcode() == Instruction::LShr &&
        detail::isSameValue(Shift->getOperand(0), &Load)) {
      auto Amount = getUInt64Constant(dyn_cast<ConstantInt>(Shift->getOperand(1)));
      if (Amount.has_value() && *Amount < 256) {
        Offset = static_cast<unsigned>(*Amount);
        return true;
      }
    }
  }
  return false;
}

bool matchPackedReadExtract(Value *V, CallBase &Load, unsigned &Offset,
                            unsigned &Width) {
  auto MatchOperands = [&](Value *LHS, Value *RHS) {
    if (auto MaskWidth = getLowMaskBitWidth(LHS)) {
      Width = *MaskWidth;
      return matchPackedReadSource(RHS, Load, Offset);
    }
    if (auto MaskWidth = getLowMaskBitWidth(RHS)) {
      Width = *MaskWidth;
      return matchPackedReadSource(LHS, Load, Offset);
    }
    return false;
  };

  if (auto *And = dyn_cast_or_null<BinaryOperator>(V)) {
    return And->getOpcode() == Instruction::And &&
           MatchOperands(And->getOperand(0), And->getOperand(1));
  }
  if (auto *Call = dyn_cast_or_null<CallBase>(V)) {
    return detail::isCallTo(Call, "evm_and") && Call->arg_size() == 2 &&
           MatchOperands(Call->getArgOperand(0), Call->getArgOperand(1));
  }
  return false;
}

Value *findPackedReadExtract(CallBase &Load, unsigned &Offset,
                             unsigned &Width) {
  for (User *LoadUser : Load.users()) {
    if (auto *V = dyn_cast<Value>(LoadUser)) {
      if (matchPackedReadExtract(V, Load, Offset, Width)) {
        return V;
      }
      for (User *NestedUser : V->users()) {
        auto *NestedValue = dyn_cast<Value>(NestedUser);
        if (matchPackedReadExtract(NestedValue, Load, Offset, Width)) {
          return NestedValue;
        }
      }
    }
  }
  return nullptr;
}

Value *getAndOtherOperandForClearMask(Value *V, PackedBitRange &Range) {
  auto Match = [&](Value *LHS, Value *RHS) -> Value * {
    if (auto LHSRange = getClearMaskBitRange(LHS)) {
      Range = *LHSRange;
      return RHS;
    }
    if (auto RHSRange = getClearMaskBitRange(RHS)) {
      Range = *RHSRange;
      return LHS;
    }
    return nullptr;
  };

  if (auto *And = dyn_cast_or_null<BinaryOperator>(V)) {
    if (And->getOpcode() == Instruction::And) {
      return Match(And->getOperand(0), And->getOperand(1));
    }
  }
  if (auto *Call = dyn_cast_or_null<CallBase>(V)) {
    if (detail::isCallTo(Call, "evm_and") && Call->arg_size() == 2) {
      return Match(Call->getArgOperand(0), Call->getArgOperand(1));
    }
  }
  return nullptr;
}

bool getOrOperands(Value *V, Value *&LHS, Value *&RHS) {
  if (auto *Or = dyn_cast_or_null<BinaryOperator>(V)) {
    if (Or->getOpcode() == Instruction::Or) {
      LHS = Or->getOperand(0);
      RHS = Or->getOperand(1);
      return true;
    }
  }
  if (auto *Call = dyn_cast_or_null<CallBase>(V)) {
    if (detail::isCallTo(Call, "evm_or") && Call->arg_size() == 2) {
      LHS = Call->getArgOperand(0);
      RHS = Call->getArgOperand(1);
      return true;
    }
  }
  return false;
}

Value *getShiftedWriteValue(Value *V, unsigned &Offset) {
  if (auto *Call = dyn_cast_or_null<CallBase>(V)) {
    if (detail::isCallTo(Call, "evm_shl") && Call->arg_size() == 2) {
      auto Shift = getUInt64Constant(dyn_cast<ConstantInt>(Call->getArgOperand(0)));
      if (Shift.has_value() && *Shift < 256) {
        Offset = static_cast<unsigned>(*Shift);
        return Call->getArgOperand(1);
      }
    }
  }

  if (auto *Shift = dyn_cast_or_null<BinaryOperator>(V)) {
    if (Shift->getOpcode() == Instruction::Shl) {
      auto Amount = getUInt64Constant(dyn_cast<ConstantInt>(Shift->getOperand(1)));
      if (Amount.has_value() && *Amount < 256) {
        Offset = static_cast<unsigned>(*Amount);
        return Shift->getOperand(0);
      }
    }
  }

  Offset = 0;
  return V;
}

CallBase *getSameSlotSLoad(Value *V, Value *StorageSlot) {
  auto *Load = dyn_cast_or_null<CallBase>(V);
  if (Load == nullptr || !detail::isCallTo(Load, "evm_sload") ||
      Load->arg_size() != 1 ||
      !detail::isSameValue(Load->getArgOperand(0), StorageSlot)) {
    return nullptr;
  }
  return Load;
}

bool isBytesShortDataMask(Value *V) {
  auto *CI = dyn_cast_or_null<ConstantInt>(V);
  if (CI == nullptr) {
    return false;
  }
  const APInt &Mask = CI->getValue();
  if (Mask.getBitWidth() <= 8) {
    return false;
  }
  for (unsigned I = 0; I < Mask.getBitWidth(); ++I) {
    if (Mask[I] != (I >= 8)) {
      return false;
    }
  }
  return true;
}

Value *findBytesShortDataExtract(CallBase &Load) {
  SmallVector<Value *, 8> Worklist;
  SmallPtrSet<Value *, 16> Seen;
  for (User *User : Load.users()) {
    if (auto *V = dyn_cast<Value>(User)) {
      Worklist.push_back(V);
    }
  }

  unsigned Steps = 0;
  while (!Worklist.empty() && Steps++ < 32) {
    Value *V = Worklist.pop_back_val();
    if (V == nullptr || !Seen.insert(V).second) {
      continue;
    }
    auto *And = dyn_cast<BinaryOperator>(V);
    if (And != nullptr && And->getOpcode() == Instruction::And &&
        ((detail::isSameValue(And->getOperand(0), &Load) &&
          isBytesShortDataMask(And->getOperand(1))) ||
         (detail::isSameValue(And->getOperand(1), &Load) &&
          isBytesShortDataMask(And->getOperand(0))))) {
      return V;
    }
    for (User *User : V->users()) {
      if (auto *UserValue = dyn_cast<Value>(User)) {
        Worklist.push_back(UserValue);
      }
    }
  }
  return nullptr;
}

bool sameAsAnyValue(Value *Needle, ArrayRef<Value *> Values) {
  for (Value *V : Values) {
    if (detail::isSameValue(Needle, V)) {
      return true;
    }
  }
  return false;
}

void eraseIfUnused(Value *V) {
  auto *I = dyn_cast_or_null<Instruction>(V);
  if (I != nullptr && I->use_empty()) {
    I->eraseFromParent();
  }
}

void eraseDeadTree(Value *V, unsigned Depth = 8) {
  auto *I = dyn_cast_or_null<Instruction>(V);
  if (I == nullptr || !I->use_empty() || Depth == 0) {
    return;
  }

  SmallVector<Value *, 4> Operands;
  for (Use &Op : I->operands()) {
    Operands.push_back(Op.get());
  }
  I->eraseFromParent();
  for (Value *Operand : Operands) {
    eraseDeadTree(Operand, Depth - 1);
  }
}

bool rewritePackedRead(CallBase &Load) {
  if (looksLikeBytesLengthDecode(&Load)) {
    return false;
  }

  unsigned Offset = 0;
  unsigned Width = 0;
  Value *Extract = findPackedReadExtract(Load, Offset, Width);
  if (Extract == nullptr) {
    return false;
  }

  SmallPtrSet<Value *, 8> Seen;
  Value *Ref = buildStorageRef(Load.getArgOperand(0), Load, Seen);
  if (Ref == nullptr) {
    return false;
  }

  Module &M = *Load.getModule();
  LLVMContext &Ctx = M.getContext();
  IRBuilder<> B(cast<Instruction>(Extract));
  CallInst *PackedLoad =
      B.CreateCall(getStoragePackedLoadFn(M),
                   {Ref, getI256(Ctx, Offset), getI256(Ctx, Width)});
  PackedLoad->copyMetadata(Load);
  PackedLoad->takeName(Extract);
  debugRewrite("packed.load", Load, *PackedLoad);

  SmallVector<Instruction *, 4> MaybeDead;
  SmallPtrSet<Instruction *, 4> SeenDead;
  auto AddMaybeDead = [&](Value *V) {
    auto *I = dyn_cast_or_null<Instruction>(V);
    if (I != nullptr && SeenDead.insert(I).second) {
      MaybeDead.push_back(I);
    }
  };
  AddMaybeDead(Extract);
  for (User *User : Load.users()) {
    if (auto *V = dyn_cast<Value>(User)) {
      AddMaybeDead(V);
    }
  }

  Extract->replaceAllUsesWith(PackedLoad);
  for (Instruction *I : MaybeDead) {
    eraseIfUnused(I);
  }
  eraseIfUnused(&Load);
  return true;
}

bool matchPackedStoreValue(Value *StorageSlot, Value *StoredValue,
                           unsigned &Offset, unsigned &Width,
                           Value *&WriteValue) {
  Value *OrLHS = nullptr;
  Value *OrRHS = nullptr;
  if (!getOrOperands(StoredValue, OrLHS, OrRHS)) {
    return false;
  }

  auto TryMatch = [&](Value *ClearPart, Value *WritePart) {
    PackedBitRange Range;
    Value *OldValue = getAndOtherOperandForClearMask(ClearPart, Range);
    if (getSameSlotSLoad(OldValue, StorageSlot) == nullptr) {
      return false;
    }

    unsigned ShiftOffset = 0;
    Value *ShiftedValue = getShiftedWriteValue(WritePart, ShiftOffset);
    if (ShiftOffset != Range.Offset || Range.Width == 0) {
      return false;
    }

    Offset = Range.Offset;
    Width = Range.Width;
    WriteValue = ShiftedValue;
    return true;
  };

  return TryMatch(OrLHS, OrRHS) || TryMatch(OrRHS, OrLHS);
}

bool isPackedWritePreservedWordLoad(CallBase &Load) {
  if (!detail::isCallTo(&Load, "evm_sload") || Load.arg_size() != 1) {
    return false;
  }

  for (User *AndUser : Load.users()) {
    PackedBitRange Range;
    if (getAndOtherOperandForClearMask(dyn_cast<Value>(AndUser), Range) ==
        nullptr) {
      continue;
    }
    for (User *OrUser : AndUser->users()) {
      auto *StoredValue = dyn_cast<Value>(OrUser);
      if (StoredValue == nullptr) {
        continue;
      }
      for (User *StoreUser : OrUser->users()) {
        auto *Store = dyn_cast<CallBase>(StoreUser);
        unsigned Offset = 0;
        unsigned Width = 0;
        Value *WriteValue = nullptr;
        if (Store != nullptr && detail::isCallTo(Store, "evm_sstore") &&
            Store->arg_size() == 2 &&
            detail::isSameValue(Store->getArgOperand(0),
                                Load.getArgOperand(0)) &&
            detail::isSameValue(Store->getArgOperand(1), StoredValue) &&
            matchPackedStoreValue(Store->getArgOperand(0),
                                  Store->getArgOperand(1), Offset, Width,
                                  WriteValue)) {
          return true;
        }
      }
    }
  }
  return false;
}

bool rewritePackedStore(CallBase &Store) {
  if (!detail::isCallTo(&Store, "evm_sstore") || Store.arg_size() != 2) {
    return false;
  }

  unsigned Offset = 0;
  unsigned Width = 0;
  Value *WriteValue = nullptr;
  if (!matchPackedStoreValue(Store.getArgOperand(0), Store.getArgOperand(1),
                             Offset, Width, WriteValue)) {
    return false;
  }

  SmallPtrSet<Value *, 8> Seen;
  Value *Ref = buildStorageRef(Store.getArgOperand(0), Store, Seen);
  if (Ref == nullptr) {
    return false;
  }

  Module &M = *Store.getModule();
  LLVMContext &Ctx = M.getContext();
  IRBuilder<> B(&Store);
  CallInst *PackedStore =
      B.CreateCall(getStoragePackedStoreFn(M),
                   {Ref, getI256(Ctx, Offset), getI256(Ctx, Width), WriteValue});
  PackedStore->copyMetadata(Store);
  debugRewrite("packed.store", Store, *PackedStore);

  Value *StoredValue = Store.getArgOperand(1);
  Store.eraseFromParent();
  eraseDeadTree(StoredValue);
  return true;
}

Value *buildStorageRefFromAdd(BinaryOperator &Add, Instruction &InsertBefore,
                              SmallPtrSetImpl<Value *> &Seen) {
  Module &M = *InsertBefore.getModule();
  IRBuilder<> B(&InsertBefore);
  Value *LHS = Add.getOperand(0);
  Value *RHS = Add.getOperand(1);

  auto BuildDynamicElem = [&](CallBase *Sha3, Value *Index) -> Value * {
    std::optional<detail::StorageArrayDataKeccakMatch> Data =
        detail::matchStorageArrayDataKeccak(*Sha3);
    if (!Data.has_value()) {
      return nullptr;
    }
    Value *Base =
        buildStorageRef(getEvidenceValue(Data->BaseSlot), InsertBefore, Seen);
    if (Base == nullptr) {
      return nullptr;
    }
    return B.CreateCall(getStorageDynamicArrayElemFn(M), {Base, Index});
  };

  if (auto *Call = dyn_cast<CallBase>(LHS)) {
    if (Value *Ref = BuildDynamicElem(Call, RHS)) {
      return Ref;
    }
  }
  if (auto *Call = dyn_cast<CallBase>(RHS)) {
    if (Value *Ref = BuildDynamicElem(Call, LHS)) {
      return Ref;
    }
  }

  if (auto *Offset = dyn_cast<ConstantInt>(RHS)) {
    Value *Base = buildStorageRef(LHS, InsertBefore, Seen);
    if (Base != nullptr) {
      std::optional<uint64_t> OffsetValue = getUInt64Constant(Offset);
      if (!OffsetValue.has_value()) {
        return nullptr;
      }
      return B.CreateCall(
          getStorageFieldFn(M),
          {Base, getI256(M.getContext(), *OffsetValue)});
    }
    Value *ArrayBase = buildStorageRef(RHS, InsertBefore, Seen);
    if (ArrayBase != nullptr) {
      return B.CreateCall(getStorageStaticArrayElemFn(M), {ArrayBase, LHS});
    }
    return nullptr;
  }
  if (auto *Offset = dyn_cast<ConstantInt>(LHS)) {
    Value *Base = buildStorageRef(RHS, InsertBefore, Seen);
    if (Base != nullptr) {
      std::optional<uint64_t> OffsetValue = getUInt64Constant(Offset);
      if (!OffsetValue.has_value()) {
        return nullptr;
      }
      return B.CreateCall(
          getStorageFieldFn(M),
          {Base, getI256(M.getContext(), *OffsetValue)});
    }
    Value *ArrayBase = buildStorageRef(LHS, InsertBefore, Seen);
    if (ArrayBase != nullptr) {
      return B.CreateCall(getStorageStaticArrayElemFn(M), {ArrayBase, RHS});
    }
    return nullptr;
  }

  return nullptr;
}

Value *buildStorageRef(Value *Slot, Instruction &InsertBefore,
                       SmallPtrSetImpl<Value *> &Seen) {
  if (Slot == nullptr || !Seen.insert(Slot).second) {
    return nullptr;
  }

  Module &M = *InsertBefore.getModule();
  LLVMContext &Ctx = M.getContext();
  IRBuilder<> B(&InsertBefore);

  if (auto *C = dyn_cast<ConstantInt>(Slot)) {
    std::optional<uint64_t> Value = getUInt64Constant(C);
    if (!Value.has_value()) {
      return nullptr;
    }
    return B.CreateCall(getStorageSlotFn(M), {getI256(Ctx, *Value)});
  }

  if (auto *Add = dyn_cast<BinaryOperator>(Slot)) {
    if (Add->getOpcode() == Instruction::Add) {
      return buildStorageRefFromAdd(*Add, InsertBefore, Seen);
    }
  }

  auto *Call = dyn_cast<CallBase>(Slot);
  if (Call == nullptr) {
    return nullptr;
  }

  if (std::optional<detail::StorageScratchKeccakMatch> Scratch =
          detail::matchStorageScratchKeccak(*Call)) {
    Value *Base =
        buildStorageRef(getEvidenceValue(Scratch->BaseSlot), InsertBefore, Seen);
    Value *Key = getEvidenceValue(Scratch->Key);
    if (Base == nullptr) {
      return nullptr;
    }
    if (Key == nullptr) {
      return nullptr;
    }
    return B.CreateCall(getStorageMapValueFn(M), {Base, Key});
  }

  if (std::optional<detail::StorageArrayDataKeccakMatch> Data =
          detail::matchStorageArrayDataKeccak(*Call)) {
    Value *Base =
        buildStorageRef(getEvidenceValue(Data->BaseSlot), InsertBefore, Seen);
    if (Base == nullptr) {
      return nullptr;
    }
    return B.CreateCall(getStorageDynamicArrayElemFn(M), {Base, getI256(Ctx, 0)});
  }

  return nullptr;
}

Value *buildStorageRef(Value *Slot, Instruction &InsertBefore) {
  SmallPtrSet<Value *, 8> Seen;
  return buildStorageRef(Slot, InsertBefore, Seen);
}

bool rewriteBytesShortData(CallBase &Load, Value *Ref) {
  Value *ShortData = findBytesShortDataExtract(Load);
  if (ShortData == nullptr) {
    return false;
  }

  Module &M = *Load.getModule();
  IRBuilder<> B(cast<Instruction>(ShortData));
  CallInst *NewLoad = B.CreateCall(getStorageBytesShortDataLoadFn(M), {Ref});
  NewLoad->copyMetadata(Load);
  NewLoad->takeName(ShortData);
  debugRewrite("bytes.short_data.load", *cast<Instruction>(ShortData),
               *NewLoad);
  ShortData->replaceAllUsesWith(NewLoad);
  eraseIfUnused(ShortData);
  return true;
}

Value *getDynamicDataIndex(Value *StorageSlot, Value *DataHash) {
  if (detail::isSameValue(StorageSlot, DataHash)) {
    return nullptr;
  }

  auto getIndexFromOperands = [&](Value *LHS, Value *RHS) -> Value * {
    bool LHSIsBase = detail::dependsOnValue(LHS, DataHash);
    bool RHSIsBase = detail::dependsOnValue(RHS, DataHash);
    if (LHSIsBase == RHSIsBase) {
      return nullptr;
    }
    return LHSIsBase ? RHS : LHS;
  };

  if (auto *Add = dyn_cast<BinaryOperator>(StorageSlot)) {
    if (Add->getOpcode() == Instruction::Add) {
      return getIndexFromOperands(Add->getOperand(0), Add->getOperand(1));
    }
  }

  if (auto *Call = dyn_cast<CallBase>(StorageSlot)) {
    if (detail::isCallTo(Call, "evm_add") && Call->arg_size() == 2) {
      return getIndexFromOperands(Call->getArgOperand(0),
                                  Call->getArgOperand(1));
    }
  }

  return nullptr;
}

bool rewriteBytesLongElem(CallBase &Load, ArrayRef<Value *> BytesBaseSlots) {
  std::optional<detail::StorageArrayDataAccessMatch> Match =
      detail::matchStorageArrayDataAccess(Load, Load.getArgOperand(0),
                                          /*AccessKind=*/1);
  if (!Match.has_value() ||
      !sameAsAnyValue(getEvidenceValue(Match->BaseSlot), BytesBaseSlots)) {
    return false;
  }

  Value *Index = getDynamicDataIndex(Load.getArgOperand(0), Match->DataHash);
  if (Index == nullptr) {
    return false;
  }

  SmallPtrSet<Value *, 8> Seen;
  Value *Ref = buildStorageRef(getEvidenceValue(Match->BaseSlot), Load, Seen);
  if (Ref == nullptr) {
    return false;
  }

  Module &M = *Load.getModule();
  IRBuilder<> B(&Load);
  CallInst *NewLoad =
      B.CreateCall(getStorageBytesLongElemLoadFn(M), {Ref, Index});
  NewLoad->copyMetadata(Load);
  NewLoad->takeName(&Load);
  debugRewrite("bytes.long_elem.load", Load, *NewLoad);
  Load.replaceAllUsesWith(NewLoad);
  Load.eraseFromParent();
  return true;
}

bool rewriteStorageAccess(CallBase &Call,
                          const SmallPtrSetImpl<CallBase *> &LengthLoads,
                          ArrayRef<Value *> BytesBaseSlots) {
  Module &M = *Call.getModule();
  IRBuilder<> B(&Call);

  if (detail::isCallTo(&Call, "evm_sload") && Call.arg_size() == 1) {
    if (rewriteBytesLongElem(Call, BytesBaseSlots)) {
      return true;
    }
    if (isPackedWritePreservedWordLoad(Call)) {
      debugSkip("packed-store preserved word load", Call);
      return false;
    }
    if (rewritePackedRead(Call)) {
      return true;
    }
    Value *Ref = buildStorageRef(Call.getArgOperand(0), Call);
    if (Ref == nullptr) {
      debugSkip("unmatched sload slot", Call);
      return false;
    }
    if (sameAsAnyValue(Call.getArgOperand(0), BytesBaseSlots)) {
      rewriteBytesShortData(Call, Ref);
      CallInst *Load = B.CreateCall(getStorageBytesLengthLoadFn(M), {Ref});
      Load->copyMetadata(Call);
      Load->takeName(&Call);
      debugRewrite("bytes.length.load", Call, *Load);
      Call.replaceAllUsesWith(Load);
      Call.eraseFromParent();
      return true;
    }
    FunctionCallee LoadFn = LengthLoads.contains(&Call)
                                ? getStorageDynamicArrayLengthLoadFn(M)
                                : getStorageLoadFn(M);
    CallInst *Load = B.CreateCall(LoadFn, {Ref});
    Load->copyMetadata(Call);
    Load->takeName(&Call);
    debugRewrite(LengthLoads.contains(&Call) ? "dynamic_array.length.load"
                                             : "load",
                 Call, *Load);
    Call.replaceAllUsesWith(Load);
    Call.eraseFromParent();
    return true;
  }

  if (detail::isCallTo(&Call, "evm_sstore") && Call.arg_size() == 2) {
    if (rewritePackedStore(Call)) {
      return true;
    }
    Value *Ref = buildStorageRef(Call.getArgOperand(0), Call);
    if (Ref == nullptr) {
      debugSkip("unmatched sstore slot", Call);
      return false;
    }
    bool IsArrayLengthStore =
        callUsesStorageSlotAsDynamicArrayData(*Call.getFunction(),
                                              Call.getArgOperand(0));
    FunctionCallee StoreFn = IsArrayLengthStore
                                 ? getStorageDynamicArrayLengthStoreFn(M)
                                 : getStorageStoreFn(M);
    CallInst *Store = B.CreateCall(StoreFn, {Ref, Call.getArgOperand(1)});
    Store->copyMetadata(Call);
    debugRewrite(IsArrayLengthStore ? "dynamic_array.length.store" : "store",
                 Call, *Store);
    Call.eraseFromParent();
    return true;
  }

  return false;
}

} // namespace

PreservedAnalyses
EvmStorageHighLevelRewritePass::run(Module &M, ModuleAnalysisManager &MAM) {
  (void)MAM;
  std::unique_ptr<mlsub::MLsubRecovery::Result> &HighTypes = TR.ResultVal;
  if (HighTypes == nullptr || HighTypes->StorageType == nullptr) {
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  SmallVector<CallBase *, 32> Accesses;
  SmallPtrSet<CallBase *, 8> DynamicArrayLengthLoads;
  SmallVector<Value *, 8> BytesBaseSlots;
  for (Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    for (Instruction &I : instructions(F)) {
      auto *Call = dyn_cast<CallBase>(&I);
      if (Call != nullptr &&
          (detail::isCallTo(Call, "evm_sload") ||
           detail::isCallTo(Call, "evm_sstore"))) {
        Accesses.push_back(Call);
        if (isDynamicArrayLengthAccess(*Call)) {
          DynamicArrayLengthLoads.insert(Call);
        }
        if (detail::isCallTo(Call, "evm_sload") && Call->arg_size() == 1 &&
            hasBytesLengthValue(*Call) &&
            !sameAsAnyValue(Call->getArgOperand(0), BytesBaseSlots)) {
          BytesBaseSlots.push_back(Call->getArgOperand(0));
        }
      }
    }
  }

  for (CallBase *Call : Accesses) {
    Changed |= rewriteStorageAccess(*Call, DynamicArrayLengthLoads,
                                    BytesBaseSlots);
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
