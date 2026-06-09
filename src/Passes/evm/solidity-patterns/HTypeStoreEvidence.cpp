#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <notdec-llvm2c/Interface/ExtValuePtr.h>

using namespace llvm;

namespace notdec::passes::evm::detail {

namespace {

std::optional<uint64_t> getUInt64FromConstant(const Constant *C) {
  auto *CI = dyn_cast_or_null<ConstantInt>(C);
  if (CI == nullptr || CI->getValue().getActiveBits() > 64) {
    return std::nullopt;
  }
  return CI->getZExtValue();
}

Value *getValueFromExtValue(const ExtValuePtr &Value) {
  if (auto *V = std::get_if<llvm::Value *>(&Value)) {
    return *V;
  }
  return nullptr;
}

std::optional<uint64_t> getConstantOffsetFromExtValue(const ExtValuePtr &Ext) {
  if (auto *Addr = std::get_if<ConstantAddr>(&Ext)) {
    return getUInt64FromConstant(Addr->Val);
  }
  if (auto *C = std::get_if<UConstant>(&Ext)) {
    if (isa<ConstantPointerNull>(C->Val)) {
      return 0;
    }
    return getUInt64FromConstant(C->Val);
  }
  if (Value *V = getValueFromExtValue(Ext)) {
    if (isa<ConstantPointerNull>(V)) {
      return 0;
    }
    if (auto *Cast = dyn_cast<IntToPtrInst>(V)) {
      V = Cast->getOperand(0);
    }
    return getUInt64Constant(V);
  }
  return std::nullopt;
}

bool isPtrToIntOf(Value *MaybePtrToInt, Value *Ptr) {
  auto *Cast = dyn_cast_or_null<PtrToIntInst>(MaybePtrToInt);
  return Cast != nullptr && Cast->getOperand(0) == Ptr;
}

std::optional<uint64_t> getEvidenceOffsetFromBase(const ExtValuePtr &Addr,
                                                  Value *Base) {
  if (std::optional<uint64_t> BaseValue = getUInt64Constant(Base)) {
    if (std::optional<uint64_t> AddrValue =
            getConstantOffsetFromExtValue(Addr)) {
      if (*AddrValue >= *BaseValue) {
        return *AddrValue - *BaseValue;
      }
    }
  }

  Value *AddrValue = getValueFromExtValue(Addr);
  if (AddrValue == nullptr) {
    return std::nullopt;
  }

  if (isPtrToIntOf(Base, AddrValue)) {
    return 0;
  }
  if (auto *Cast = dyn_cast<IntToPtrInst>(AddrValue)) {
    AddrValue = Cast->getOperand(0);
  }
  return getOffsetFromBase(AddrValue, Base);
}

} // namespace

bool hasHTypeFieldAt(ast::RecordDecl &Record, int64_t Offset) {
  return Record.getFieldAt(Offset) != nullptr;
}

bool hasHTypeBufferFieldAt(const HTypeBufferView &View, int64_t Offset) {
  if (View.Record != nullptr) {
    return hasHTypeFieldAt(*View.Record, Offset);
  }
  return Offset == 0 && View.HasTransparentOffset0Field;
}

SmallVector<Value *, 2>
getHTypeFieldStoreValues(ArrayRef<mlsub::EVMStoreEvidence> Stores,
                         ast::RecordDecl &Record, Value *Base, int64_t Offset) {
  if (!hasHTypeFieldAt(Record, Offset)) {
    return {};
  }
  return getHTypeStoreValuesAtOffset(Stores, Base, Offset);
}

SmallVector<Value *, 2>
getHTypeBufferFieldStoreValues(ArrayRef<mlsub::EVMStoreEvidence> Stores,
                               const HTypeBufferView &View, Value *Base,
                               int64_t Offset) {
  if (!hasHTypeBufferFieldAt(View, Offset)) {
    return {};
  }
  return getHTypeStoreValuesAtOffset(Stores, Base, Offset);
}

SmallVector<Value *, 2>
getHTypeStoreValuesAtOffset(ArrayRef<mlsub::EVMStoreEvidence> Stores,
                            Value *Base, int64_t Offset) {
  SmallVector<Value *, 2> Values;
  for (const mlsub::EVMStoreEvidence &Store : Stores) {
    if (Store.BitSize != 256 || Store.StoredValue == nullptr) {
      continue;
    }
    std::optional<uint64_t> StoreOffset =
        getEvidenceOffsetFromBase(Store.Addr, Base);
    if (StoreOffset.has_value() &&
        *StoreOffset == static_cast<uint64_t>(Offset)) {
      Values.push_back(Store.StoredValue);
    }
  }
  return Values;
}

SmallVector<Value *, 2>
getHTypeStoreValuesAtOffsetBefore(ArrayRef<mlsub::EVMStoreEvidence> Stores,
                                  Value *Base, int64_t Offset,
                                  Instruction &Before) {
  SmallVector<Value *, 2> Values;
  for (const mlsub::EVMStoreEvidence &Store : Stores) {
    if (Store.BitSize != 256 || Store.StoredValue == nullptr ||
        Store.Source == nullptr ||
        Store.Source->getParent() != Before.getParent() ||
        !Store.Source->comesBefore(&Before)) {
      continue;
    }
    std::optional<uint64_t> StoreOffset =
        getEvidenceOffsetFromBase(Store.Addr, Base);
    if (StoreOffset.has_value() &&
        *StoreOffset == static_cast<uint64_t>(Offset)) {
      Values.push_back(Store.StoredValue);
    }
  }
  return Values;
}

std::optional<uint64_t> getUniqueUInt64FieldValue(ArrayRef<Value *> Values,
                                                  bool &Conflict) {
  std::optional<uint64_t> Result;
  for (Value *V : Values) {
    std::optional<uint64_t> Candidate = getUInt64Constant(V);
    if (!Candidate.has_value()) {
      continue;
    }
    if (Result.has_value() && *Result != *Candidate) {
      Conflict = true;
      return std::nullopt;
    }
    Result = Candidate;
  }
  return Result;
}

} // namespace notdec::passes::evm::detail
