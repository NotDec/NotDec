#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumPackedStorageFields,
          "Number of Solidity packed storage field candidates found");
STATISTIC(NumPackedStorageAccess,
          "Number of Solidity packed storage accesses with storage roots found");

namespace {

struct StorageAccessMarker {
  CallBase *Marker = nullptr;
  uint64_t RootKind = 0;
};

std::optional<StorageAccessMarker> findStorageAccessMarker(CallBase &Access,
                                                           Value *StorageSlot,
                                                           uint64_t AccessKind) {
  for (auto It = Access.getIterator(); It != Access.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call == nullptr) {
      continue;
    }

    if (isCallTo(Call, "evm_sload") || isCallTo(Call, "evm_sstore") ||
        isCallTo(Call, "evm_sha3") ||
        !classifyExternalCall(getCalleeName(Call)).empty()) {
      break;
    }

    if (isCallTo(Call, "notdec_solidity_storage_mapping_access") &&
        Call->arg_size() == 5 && Call->getArgOperand(3) == StorageSlot &&
        isConstantIntValue(Call->getArgOperand(4), AccessKind)) {
      return StorageAccessMarker{Call, 1};
    }

    if (isCallTo(Call, "notdec_solidity_storage_array_data_access") &&
        Call->arg_size() == 4 && Call->getArgOperand(2) == StorageSlot &&
        isConstantIntValue(Call->getArgOperand(3), AccessKind)) {
      return StorageAccessMarker{Call, 2};
    }
  }

  return std::nullopt;
}

void insertPackedStorageAccessMarker(LLVMContext &Ctx, CallBase &Access,
                                     const StorageAccessMarker &Marker,
                                     Value *StorageSlot,
                                     uint64_t AccessKind) {
  Module *M = Access.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Callee = M->getOrInsertFunction(
      "notdec_solidity_packed_storage_access",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256}, false));

  IRBuilder<> Builder(&Access);
  Builder.CreateCall(Callee, {StorageSlot, ConstantInt::get(I256, AccessKind),
                              ConstantInt::get(I256, Marker.RootKind)});
}

} // namespace

PreservedAnalyses PackedStorageFieldPass::run(Function &F,
                                              FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call != nullptr && isCallTo(Call, "evm_sstore") &&
        Call->arg_size() == 2) {
      Value *Stored = Call->getArgOperand(1);
      if (dependsOnCallTo(Stored, "evm_sload") &&
          expressionHasPackedStorageOp(Stored)) {
        addStringMetadata(Ctx, I, KIND_SOLIDITY_PACKED_STORAGE_FIELD,
                          "packed_store_candidate");
        if (std::optional<StorageAccessMarker> Marker =
                findStorageAccessMarker(*Call, Call->getArgOperand(0), 2)) {
          insertPackedStorageAccessMarker(Ctx, *Call, *Marker,
                                          Call->getArgOperand(0), 2);
          ++NumPackedStorageAccess;
        }
        ++NumPackedStorageFields;
        Changed = true;
      }
      continue;
    }

    if (Call != nullptr && isCallTo(Call, "evm_sload")) {
      for (User *U : Call->users()) {
        if (expressionHasPackedStorageOp(U)) {
          addStringMetadata(Ctx, I, KIND_SOLIDITY_PACKED_STORAGE_FIELD,
                            "packed_load_candidate");
          if (std::optional<StorageAccessMarker> Marker =
                  findStorageAccessMarker(*Call, Call->getArgOperand(0), 1)) {
            insertPackedStorageAccessMarker(Ctx, *Call, *Marker,
                                            Call->getArgOperand(0), 1);
            ++NumPackedStorageAccess;
          }
          ++NumPackedStorageFields;
          Changed = true;
          break;
        }
      }
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
