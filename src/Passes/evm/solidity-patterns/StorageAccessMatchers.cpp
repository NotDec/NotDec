#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Instructions.h>
#include <optional>

using namespace llvm;

namespace notdec::passes::evm::detail {

bool expressionHasPackedStorageOp(Value *V, unsigned Depth,
                                  SmallPtrSetImpl<Value *> &Seen) {
  if (V == nullptr || Depth == 0 || !Seen.insert(V).second) {
    return false;
  }

  if (auto *Bin = dyn_cast<BinaryOperator>(V)) {
    if (Bin->getOpcode() == Instruction::And ||
        Bin->getOpcode() == Instruction::Or ||
        Bin->getOpcode() == Instruction::Xor ||
        Bin->getOpcode() == Instruction::Mul) {
      return true;
    }
  }

  if (auto *Call = dyn_cast<CallBase>(V)) {
    StringRef Name = getCalleeName(Call);
    if (Name == "evm_shl" || Name == "evm_shr" || Name == "evm_sar" ||
        Name == "evm_div" || Name == "evm_exp") {
      return true;
    }
  }

  if (auto *Inst = dyn_cast<Instruction>(V)) {
    for (Value *Op : Inst->operands()) {
      if (expressionHasPackedStorageOp(Op, Depth - 1, Seen)) {
        return true;
      }
    }
  }
  return false;
}

bool expressionHasPackedStorageOp(Value *V, unsigned Depth) {
  SmallPtrSet<Value *, 16> Seen;
  return expressionHasPackedStorageOp(V, Depth, Seen);
}

std::optional<StorageScratchKeccakMatch>
matchStorageScratchKeccak(CallBase &Sha3) {
  if (!isCallTo(&Sha3, "evm_sha3") || Sha3.arg_size() != 3 ||
      !isConstantIntValue(Sha3.getArgOperand(1), 0) ||
      !isConstantIntValue(Sha3.getArgOperand(2), 64)) {
    return std::nullopt;
  }

  Value *Key = nullptr;
  Value *BaseSlot = nullptr;
  ExtValuePtr KeyEvidence = static_cast<Value *>(nullptr);
  ExtValuePtr BaseSlotEvidence = static_cast<Value *>(nullptr);
  for (auto It = Sha3.getIterator(); It != Sha3.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call != nullptr &&
        (isCallTo(Call, "evm_sha3") ||
         !classifyExternalCall(getCalleeName(Call)).empty())) {
      break;
    }
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&*It);
    if (!Store.has_value()) {
      continue;
    }
    if (BaseSlot == nullptr && isConstantIntValue(Store->Address, 32)) {
      BaseSlot = Store->StoredValue;
      BaseSlotEvidence = getExtValuePtr(Store->StoredValue, Store->Inst, 0);
      continue;
    }
    if (Key == nullptr && isConstantIntValue(Store->Address, 0)) {
      Key = Store->StoredValue;
      KeyEvidence = getExtValuePtr(Store->StoredValue, Store->Inst, 0);
      continue;
    }
    if (Key != nullptr && BaseSlot != nullptr) {
      break;
    }
  }

  if (Key == nullptr || BaseSlot == nullptr) {
    return std::nullopt;
  }
  return StorageScratchKeccakMatch{&Sha3, KeyEvidence, BaseSlotEvidence};
}

std::optional<StorageArrayDataKeccakMatch>
matchStorageArrayDataKeccak(CallBase &Sha3) {
  if (!isCallTo(&Sha3, "evm_sha3") || Sha3.arg_size() != 3 ||
      !isConstantIntValue(Sha3.getArgOperand(1), 0) ||
      !isConstantIntValue(Sha3.getArgOperand(2), 32)) {
    return std::nullopt;
  }

  Value *BaseSlot = nullptr;
  ExtValuePtr BaseSlotEvidence = static_cast<Value *>(nullptr);
  for (auto It = Sha3.getIterator(); It != Sha3.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call != nullptr &&
        (isCallTo(Call, "evm_sha3") ||
         !classifyExternalCall(getCalleeName(Call)).empty())) {
      break;
    }
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&*It);
    if (Store.has_value() && isConstantIntValue(Store->Address, 0)) {
      BaseSlot = Store->StoredValue;
      BaseSlotEvidence = getExtValuePtr(Store->StoredValue, Store->Inst, 0);
      break;
    }
  }

  if (BaseSlot == nullptr) {
    return std::nullopt;
  }
  return StorageArrayDataKeccakMatch{&Sha3, BaseSlotEvidence};
}

std::optional<StorageMappingAccessMatch>
matchStorageMappingAccess(CallBase &Access, Value *StorageSlot,
                          uint64_t AccessKind) {
  for (auto It = Access.getIterator(); It != Access.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call == nullptr) {
      continue;
    }
    if (isCallTo(Call, "evm_sload") || isCallTo(Call, "evm_sstore") ||
        !classifyExternalCall(getCalleeName(Call)).empty()) {
      break;
    }
    if (!isCallTo(Call, "evm_sha3") || Call->arg_size() != 3) {
      continue;
    }
    if (!dependsOnValue(StorageSlot, Call)) {
      break;
    }
    std::optional<StorageScratchKeccakMatch> Scratch =
        matchStorageScratchKeccak(*Call);
    if (!Scratch.has_value()) {
      break;
    }
    return StorageMappingAccessMatch{&Access,           Scratch->Key,
                                     Scratch->BaseSlot, Scratch->Sha3,
                                     StorageSlot,       AccessKind};
  }
  return std::nullopt;
}

std::optional<StorageArrayDataAccessMatch>
matchStorageArrayDataAccess(CallBase &Access, Value *StorageSlot,
                            uint64_t AccessKind) {
  for (auto It = Access.getIterator(); It != Access.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call == nullptr) {
      continue;
    }
    if (isCallTo(Call, "evm_sload") || isCallTo(Call, "evm_sstore") ||
        !classifyExternalCall(getCalleeName(Call)).empty()) {
      break;
    }
    if (!isCallTo(Call, "evm_sha3") || Call->arg_size() != 3) {
      continue;
    }
    if (!dependsOnValue(StorageSlot, Call)) {
      break;
    }
    std::optional<StorageArrayDataKeccakMatch> Data =
        matchStorageArrayDataKeccak(*Call);
    if (!Data.has_value()) {
      break;
    }
    return StorageArrayDataAccessMatch{&Access, Data->BaseSlot, Data->Sha3,
                                       StorageSlot, AccessKind};
  }
  return std::nullopt;
}

std::optional<PackedStorageAccessMatch>
matchPackedStorageAccess(CallBase &Access, uint64_t AccessKind) {
  if (AccessKind == 1) {
    if (!isCallTo(&Access, "evm_sload") || Access.arg_size() != 1) {
      return std::nullopt;
    }
    for (User *U : Access.users()) {
      if (expressionHasPackedStorageOp(U)) {
        return PackedStorageAccessMatch{&Access, Access.getArgOperand(0),
                                        AccessKind};
      }
    }
    return std::nullopt;
  }

  if (AccessKind == 2) {
    if (!isCallTo(&Access, "evm_sstore") || Access.arg_size() != 2) {
      return std::nullopt;
    }
    Value *Stored = Access.getArgOperand(1);
    if (dependsOnCallTo(Stored, "evm_sload") &&
        expressionHasPackedStorageOp(Stored)) {
      return PackedStorageAccessMatch{&Access, Access.getArgOperand(0),
                                      AccessKind};
    }
  }
  return std::nullopt;
}

StringRef classifyExternalCall(StringRef Name) {
  return Name == "evm_call"           ? StringRef("call")
         : Name == "evm_staticcall"   ? StringRef("staticcall")
         : Name == "evm_delegatecall" ? StringRef("delegatecall")
         : Name == "evm_callcode"     ? StringRef("callcode")
                                      : StringRef("");
}

} // namespace notdec::passes::evm::detail
