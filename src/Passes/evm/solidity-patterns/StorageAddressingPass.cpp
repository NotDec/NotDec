#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumStorageAddressing,
          "Number of Solidity storage addressing candidates found");
STATISTIC(NumStorageScratchKeccak,
          "Number of Solidity storage scratch keccak markers found");
STATISTIC(NumStorageArrayDataKeccak,
          "Number of Solidity storage array data keccak markers found");
STATISTIC(NumStorageArrayDataAccess,
          "Number of Solidity storage array data access markers found");

namespace {

struct ScratchKeccakMatch {
  CallBase *KeyStore = nullptr;
  CallBase *SlotStore = nullptr;
};

struct ArrayDataKeccakMatch {
  CallBase *BaseSlotStore = nullptr;
};

struct ArrayDataAccessMatch {
  CallBase *DataMarker = nullptr;
};

bool dependsOnValue(Value *V, Value *Target, unsigned Depth = 8) {
  if (V == Target) {
    return true;
  }
  if (Depth == 0) {
    return false;
  }
  auto *UserValue = dyn_cast_or_null<User>(V);
  if (UserValue == nullptr) {
    return false;
  }
  for (Value *Op : UserValue->operands()) {
    if (dependsOnValue(Op, Target, Depth - 1)) {
      return true;
    }
  }
  return false;
}

std::optional<ScratchKeccakMatch> matchStorageScratchKeccak(CallBase &Sha3) {
  if (!isConstantIntValue(Sha3.getArgOperand(1), 0) ||
      !isConstantIntValue(Sha3.getArgOperand(2), 64)) {
    return std::nullopt;
  }

  ScratchKeccakMatch Match;
  for (auto It = Sha3.getIterator(); It != Sha3.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call == nullptr) {
      continue;
    }
    if (isCallTo(Call, "evm_sha3") ||
        !classifyExternalCall(getCalleeName(Call)).empty()) {
      break;
    }
    if (!isCallTo(Call, "evm_mstore") || Call->arg_size() != 3) {
      continue;
    }
    if (Match.SlotStore == nullptr &&
        isConstantIntValue(Call->getArgOperand(1), 32)) {
      Match.SlotStore = Call;
      continue;
    }
    if (Match.KeyStore == nullptr &&
        isConstantIntValue(Call->getArgOperand(1), 0)) {
      Match.KeyStore = Call;
      continue;
    }
    if (Match.KeyStore != nullptr && Match.SlotStore != nullptr) {
      break;
    }
  }

  if (Match.KeyStore == nullptr || Match.SlotStore == nullptr) {
    return std::nullopt;
  }
  return Match;
}

std::optional<ArrayDataKeccakMatch> matchStorageArrayDataKeccak(
    CallBase &Sha3) {
  if (!isConstantIntValue(Sha3.getArgOperand(1), 0) ||
      !isConstantIntValue(Sha3.getArgOperand(2), 32)) {
    return std::nullopt;
  }

  ArrayDataKeccakMatch Match;
  for (auto It = Sha3.getIterator(); It != Sha3.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call == nullptr) {
      continue;
    }
    if (isCallTo(Call, "evm_sha3") ||
        !classifyExternalCall(getCalleeName(Call)).empty()) {
      break;
    }
    if (!isCallTo(Call, "evm_mstore") || Call->arg_size() != 3) {
      continue;
    }
    if (isConstantIntValue(Call->getArgOperand(1), 0)) {
      Match.BaseSlotStore = Call;
      break;
    }
  }

  if (Match.BaseSlotStore == nullptr) {
    return std::nullopt;
  }
  return Match;
}

std::optional<ArrayDataAccessMatch>
matchStorageArrayDataAccess(CallBase &Access, Value *StorageSlot) {
  ArrayDataAccessMatch Match;
  for (auto It = Access.getIterator(); It != Access.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call == nullptr) {
      continue;
    }
    if (isCallTo(Call, "evm_sha3") ||
        !classifyExternalCall(getCalleeName(Call)).empty()) {
      break;
    }
    if (!isCallTo(Call, "notdec_solidity_storage_array_data_keccak") ||
        Call->arg_size() != 2) {
      continue;
    }
    if (dependsOnValue(StorageSlot, Call->getArgOperand(1))) {
      Match.DataMarker = Call;
      return Match;
    }
  }
  return std::nullopt;
}

bool insertStorageScratchKeccakMarker(LLVMContext &Ctx, CallBase &Sha3,
                                      const ScratchKeccakMatch &Match) {
  Instruction *InsertBefore = Sha3.getNextNode();
  if (InsertBefore == nullptr) {
    return false;
  }

  Module *M = Sha3.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_storage_scratch_keccak",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256}, false));

  IRBuilder<> Builder(InsertBefore);
  Builder.CreateCall(Marker, {Match.KeyStore->getArgOperand(2),
                              Match.SlotStore->getArgOperand(2), &Sha3});
  return true;
}

bool insertStorageArrayDataKeccakMarker(
    LLVMContext &Ctx, CallBase &Sha3, const ArrayDataKeccakMatch &Match) {
  Instruction *InsertBefore = Sha3.getNextNode();
  if (InsertBefore == nullptr) {
    return false;
  }

  Module *M = Sha3.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_storage_array_data_keccak",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256}, false));

  IRBuilder<> Builder(InsertBefore);
  Builder.CreateCall(Marker, {Match.BaseSlotStore->getArgOperand(2), &Sha3});
  return true;
}

bool insertStorageArrayDataAccessMarker(LLVMContext &Ctx, CallBase &Access,
                                        const ArrayDataAccessMatch &Match,
                                        Value *StorageSlot,
                                        uint64_t AccessKind) {
  Module *M = Access.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_storage_array_data_access",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256, I256},
                        false));

  IRBuilder<> Builder(&Access);
  Builder.CreateCall(
      Marker, {Match.DataMarker->getArgOperand(0),
               Match.DataMarker->getArgOperand(1), StorageSlot,
               ConstantInt::get(I256, AccessKind)});
  return true;
}

} // namespace

PreservedAnalyses StorageAddressingPass::run(Function &F,
                                             FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isCallTo(Call, "evm_sload") && Call->arg_size() == 1) {
      if (std::optional<ArrayDataAccessMatch> Match =
              matchStorageArrayDataAccess(*Call, Call->getArgOperand(0))) {
        if (insertStorageArrayDataAccessMarker(
                Ctx, *Call, *Match, Call->getArgOperand(0), 1)) {
          ++NumStorageArrayDataAccess;
          Changed = true;
        }
      }
      continue;
    }

    if (isCallTo(Call, "evm_sstore") && Call->arg_size() == 2) {
      if (std::optional<ArrayDataAccessMatch> Match =
              matchStorageArrayDataAccess(*Call, Call->getArgOperand(0))) {
        if (insertStorageArrayDataAccessMarker(
                Ctx, *Call, *Match, Call->getArgOperand(0), 2)) {
          ++NumStorageArrayDataAccess;
          Changed = true;
        }
      }
      continue;
    }

    if (!isCallTo(Call, "evm_sha3") || Call->arg_size() != 3) {
      continue;
    }

    StringRef Kind = "candidate";
    if (isConstantIntValue(Call->getArgOperand(2), 64)) {
      Kind = "mapping_slot_candidate";
    } else if (isConstantIntValue(Call->getArgOperand(2), 32)) {
      Kind = "dynamic_array_data_slot_candidate";
    }
    if (std::optional<ScratchKeccakMatch> Match =
            matchStorageScratchKeccak(*Call)) {
      if (insertStorageScratchKeccakMarker(Ctx, *Call, *Match)) {
        ++NumStorageScratchKeccak;
      }
    } else if (std::optional<ArrayDataKeccakMatch> Match =
                   matchStorageArrayDataKeccak(*Call)) {
      if (insertStorageArrayDataKeccakMarker(Ctx, *Call, *Match)) {
        ++NumStorageArrayDataKeccak;
      }
    }
    addStringMetadata(Ctx, I, KIND_SOLIDITY_STORAGE_ADDRESSING, Kind);
    ++NumStorageAddressing;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
