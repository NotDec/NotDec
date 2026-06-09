#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <notdec-llvm2c/Interface/ExtValuePtr.h>

using namespace llvm;

namespace notdec::passes::evm::detail {

namespace {

Value *getValueFromExtValue(const ExtValuePtr &Value) {
  if (auto *V = std::get_if<llvm::Value *>(&Value)) {
    return *V;
  }
  return nullptr;
}

bool isPtrToIntOf(Value *MaybePtrToInt, Value *Ptr) {
  auto *Cast = dyn_cast_or_null<PtrToIntInst>(MaybePtrToInt);
  return Cast != nullptr && Cast->getOperand(0) == Ptr;
}

std::optional<uint64_t> getEvidenceOffsetFromBase(const ExtValuePtr &Addr,
                                                  Value *Base) {
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

SmallVector<Value *, 2>
getHTypeFieldStoreValues(ArrayRef<mlsub::EVMStoreEvidence> Stores,
                         ast::RecordDecl &Record, Value *Base, int64_t Offset) {
  if (!hasHTypeFieldAt(Record, Offset)) {
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
