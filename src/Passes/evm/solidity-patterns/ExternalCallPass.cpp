#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumExternalCalls, "Number of Solidity external calls found");
STATISTIC(NumExternalCallMemoryConsumers,
          "Number of Solidity external call memory consumers found");
STATISTIC(NumExternalCallInputCopyWrites,
          "Number of Solidity external call input copy writes found");
STATISTIC(NumExternalCallInputWordWrites,
          "Number of Solidity external call input word writes found");
STATISTIC(NumExternalCallInputAbiHeadWrites,
          "Number of Solidity external call input ABI head writes found");

namespace {

struct ExternalCallMemoryArgs {
  Value *InputBase = nullptr;
  Value *InputSize = nullptr;
  Value *OutputBase = nullptr;
  Value *OutputSize = nullptr;
};

std::optional<ExternalCallMemoryArgs> getExternalCallMemoryArgs(CallBase *Call) {
  StringRef Name = getCalleeName(Call);
  if ((Name == "evm_call" || Name == "evm_callcode") &&
      Call->arg_size() == 10) {
    return ExternalCallMemoryArgs{Call->getArgOperand(6),
                                  Call->getArgOperand(7),
                                  Call->getArgOperand(8),
                                  Call->getArgOperand(9)};
  }
  if ((Name == "evm_delegatecall" || Name == "evm_staticcall") &&
      Call->arg_size() == 9) {
    return ExternalCallMemoryArgs{Call->getArgOperand(5),
                                  Call->getArgOperand(6),
                                  Call->getArgOperand(7),
                                  Call->getArgOperand(8)};
  }
  return std::nullopt;
}

bool isExternalAbiHeadOffset(Value *V) {
  std::optional<uint64_t> Offset = getUInt64Constant(V);
  return Offset.has_value() && *Offset >= 4 && ((*Offset - 4) % 32) == 0;
}

uint64_t getExternalCallKindCode(StringRef Kind) {
  if (Kind == "call") {
    return 1;
  }
  if (Kind == "staticcall") {
    return 2;
  }
  if (Kind == "delegatecall") {
    return 3;
  }
  if (Kind == "callcode") {
    return 4;
  }
  return 0;
}

CallBase *findExternalCallConsumerMarker(BasicBlock &BB, CallBase &ExternalCall,
                                         Value *Base, Value *Size,
                                         uint64_t ConsumerKind) {
  for (Instruction &I : BB) {
    if (&I == &ExternalCall) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr ||
        !isCallTo(Call, "notdec_solidity_memory_consumer") ||
        Call->arg_size() != 3 ||
        !isConstantIntValue(Call->getArgOperand(2), ConsumerKind)) {
      continue;
    }
    if (Call->getArgOperand(0) == Base && Call->getArgOperand(1) == Size) {
      return Call;
    }
  }
  return nullptr;
}

CallBase *findExternalCallInputCopyWriteMarker(BasicBlock &BB,
                                               CallBase &ExternalCall,
                                               Value *InputBase) {
  CallBase *Candidate = nullptr;

  for (Instruction &I : BB) {
    if (&I == &ExternalCall) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isFreeMemoryPointerStore(Call)) {
      Candidate = nullptr;
      continue;
    }

    if (!isCallTo(Call, "notdec_solidity_memory_copy_write") ||
        Call->arg_size() != 5 ||
        !isConstantIntValue(Call->getArgOperand(1), 0)) {
      continue;
    }

    Value *CopyBase = Call->getArgOperand(0);
    if (isSameOrReloadedFreeMemoryBase(CopyBase, InputBase)) {
      Candidate = Call;
    }
  }

  return Candidate;
}

CallBase *findExternalCallInputWordWriteMarker(BasicBlock &BB,
                                               CallBase &ExternalCall,
                                               Value *InputBase) {
  CallBase *Candidate = nullptr;

  for (Instruction &I : BB) {
    if (&I == &ExternalCall) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isFreeMemoryPointerStore(Call)) {
      Candidate = nullptr;
      continue;
    }

    if (!isCallTo(Call, "notdec_solidity_memory_write") ||
        Call->arg_size() != 3 ||
        !isConstantIntValue(Call->getArgOperand(1), 0)) {
      continue;
    }

    Value *WriteBase = Call->getArgOperand(0);
    if (isSameOrReloadedFreeMemoryBase(WriteBase, InputBase)) {
      Candidate = Call;
    }
  }

  return Candidate;
}

void collectExternalCallInputAbiHeadWriteMarkers(
    BasicBlock &BB, CallBase &ExternalCall, Value *InputBase,
    SmallVectorImpl<CallBase *> &Writes) {
  SmallVector<CallBase *, 8> Candidates;

  for (Instruction &I : BB) {
    if (&I == &ExternalCall) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isFreeMemoryPointerStore(Call)) {
      Candidates.clear();
      continue;
    }

    if (!isCallTo(Call, "notdec_solidity_memory_write") ||
        Call->arg_size() != 3 ||
        !isExternalAbiHeadOffset(Call->getArgOperand(1))) {
      continue;
    }

    Value *WriteBase = Call->getArgOperand(0);
    if (isSameOrReloadedFreeMemoryBase(WriteBase, InputBase)) {
      Candidates.push_back(Call);
    }
  }

  Writes.append(Candidates.begin(), Candidates.end());
}

void insertExternalCallMemoryConsumerMarker(LLVMContext &Ctx,
                                            CallBase &ExternalCall,
                                            CallBase &Consumer, uint64_t Role,
                                            uint64_t CallKind) {
  Module *M = ExternalCall.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_external_call_memory_consumer",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256, I256},
                        false));

  IRBuilder<> Builder(&ExternalCall);
  Builder.CreateCall(Marker,
                     {Consumer.getArgOperand(0), Consumer.getArgOperand(1),
                      ConstantInt::get(I256, Role),
                      ConstantInt::get(I256, CallKind)});
}

void insertExternalCallInputCopyWriteMarker(LLVMContext &Ctx,
                                            CallBase &ExternalCall,
                                            CallBase &CopyWrite,
                                            uint64_t CallKind) {
  Module *M = ExternalCall.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_external_call_input_copy_write",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&ExternalCall);
  Builder.CreateCall(Marker,
                     {CopyWrite.getArgOperand(0), CopyWrite.getArgOperand(2),
                      CopyWrite.getArgOperand(3), CopyWrite.getArgOperand(4),
                      ConstantInt::get(I256, CallKind)});
}

void insertExternalCallInputWordWriteMarker(LLVMContext &Ctx,
                                            CallBase &ExternalCall,
                                            CallBase &WordWrite,
                                            uint64_t CallKind) {
  Module *M = ExternalCall.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_external_call_input_word_write",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256, I256},
                        false));

  IRBuilder<> Builder(&ExternalCall);
  Builder.CreateCall(Marker,
                     {WordWrite.getArgOperand(0), WordWrite.getArgOperand(1),
                      WordWrite.getArgOperand(2),
                      ConstantInt::get(I256, CallKind)});
}

void insertExternalCallInputAbiHeadWriteMarker(LLVMContext &Ctx,
                                               CallBase &ExternalCall,
                                               CallBase &WordWrite,
                                               uint64_t CallKind) {
  Module *M = ExternalCall.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_external_call_input_abi_head_write",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256, I256},
                        false));

  IRBuilder<> Builder(&ExternalCall);
  Builder.CreateCall(Marker,
                     {WordWrite.getArgOperand(0), WordWrite.getArgOperand(1),
                      WordWrite.getArgOperand(2),
                      ConstantInt::get(I256, CallKind)});
}

} // namespace

PreservedAnalyses ExternalCallPass::run(Function &F,
                                        FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    StringRef Kind = classifyExternalCall(getCalleeName(Call));
    if (Kind.empty()) {
      continue;
    }
    std::optional<ExternalCallMemoryArgs> Args = getExternalCallMemoryArgs(Call);
    if (Args.has_value()) {
      uint64_t CallKind = getExternalCallKindCode(Kind);
      if (CallBase *Consumer = findExternalCallConsumerMarker(
              *Call->getParent(), *Call, Args->InputBase, Args->InputSize, 4)) {
        insertExternalCallMemoryConsumerMarker(Ctx, *Call, *Consumer, 1,
                                               CallKind);
        ++NumExternalCallMemoryConsumers;
      }
      if (CallBase *CopyWrite = findExternalCallInputCopyWriteMarker(
              *Call->getParent(), *Call, Args->InputBase)) {
        insertExternalCallInputCopyWriteMarker(Ctx, *Call, *CopyWrite,
                                               CallKind);
        ++NumExternalCallInputCopyWrites;
      }
      if (CallBase *WordWrite = findExternalCallInputWordWriteMarker(
              *Call->getParent(), *Call, Args->InputBase)) {
        insertExternalCallInputWordWriteMarker(Ctx, *Call, *WordWrite,
                                               CallKind);
        ++NumExternalCallInputWordWrites;
      }
      SmallVector<CallBase *, 8> AbiHeadWrites;
      collectExternalCallInputAbiHeadWriteMarkers(
          *Call->getParent(), *Call, Args->InputBase, AbiHeadWrites);
      for (CallBase *AbiHeadWrite : AbiHeadWrites) {
        insertExternalCallInputAbiHeadWriteMarker(Ctx, *Call, *AbiHeadWrite,
                                                  CallKind);
        ++NumExternalCallInputAbiHeadWrites;
      }
      if (CallBase *Consumer = findExternalCallConsumerMarker(
              *Call->getParent(), *Call, Args->OutputBase, Args->OutputSize,
              5)) {
        insertExternalCallMemoryConsumerMarker(Ctx, *Call, *Consumer, 2,
                                               CallKind);
        ++NumExternalCallMemoryConsumers;
      }
    }
    addStringMetadata(Ctx, I, KIND_SOLIDITY_EXTERNAL_CALL, Kind);
    ++NumExternalCalls;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
