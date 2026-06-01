#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/CFG.h>
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
STATISTIC(NumExternalCallOutputCopyWrites,
          "Number of Solidity external call output copy writes found");
STATISTIC(NumExternalCallOutputAbiDecodeBuffers,
          "Number of Solidity external call output ABI decode buffers found");
STATISTIC(NumExternalCallOutputWordReads,
          "Number of Solidity external call output word reads found");
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

std::optional<uint64_t> getOffsetFromBase(Value *Ptr, Value *Base) {
  if (Ptr == Base) {
    return 0;
  }

  std::optional<uint64_t> PtrConst = getUInt64Constant(Ptr);
  std::optional<uint64_t> BaseConst = getUInt64Constant(Base);
  if (PtrConst.has_value() && BaseConst.has_value() && *PtrConst >= *BaseConst) {
    return *PtrConst - *BaseConst;
  }

  auto *Add = dyn_cast_or_null<BinaryOperator>(Ptr);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return std::nullopt;
  }

  for (unsigned I = 0; I < 2; ++I) {
    Value *AddBase = Add->getOperand(I);
    if (AddBase != Base &&
        !(isFreeMemoryPointerLoad(AddBase) && isFreeMemoryPointerLoad(Base))) {
      continue;
    }
    return getUInt64Constant(Add->getOperand(1 - I));
  }
  return std::nullopt;
}

bool outputSizeCoversWord(Value *OutputSize, uint64_t Offset) {
  std::optional<uint64_t> Size = getUInt64Constant(OutputSize);
  if (!Size.has_value()) {
    return true;
  }
  return *Size >= 32 && Offset <= *Size - 32;
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

CallBase *findExternalCallOutputCopyWriteMarker(BasicBlock &BB,
                                                CallBase &ExternalCall,
                                                Value *OutputBase) {
  bool SeenExternalCall = false;
  auto IsMatchingOutputCopy = [&](CallBase *Call) {
    if (!isCallTo(Call, "notdec_solidity_memory_copy_write") ||
        Call->arg_size() != 5 ||
        !isConstantIntValue(Call->getArgOperand(1), 0) ||
        !isConstantIntValue(Call->getArgOperand(2), 0) ||
        !isConstantIntValue(Call->getArgOperand(4), 3)) {
      return false;
    }

    Value *CopyBase = Call->getArgOperand(0);
    return isSameOrReloadedFreeMemoryBase(CopyBase, OutputBase);
  };

  for (Instruction &I : BB) {
    if (&I == &ExternalCall) {
      SeenExternalCall = true;
      continue;
    }
    if (!SeenExternalCall) {
      continue;
    }

    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isFreeMemoryPointerStore(Call) ||
        !classifyExternalCall(getCalleeName(Call)).empty()) {
      return nullptr;
    }

    if (IsMatchingOutputCopy(Call)) {
      return Call;
    }
  }

  for (BasicBlock *Succ : successors(&BB)) {
    for (Instruction &I : *Succ) {
      auto *Call = dyn_cast<CallBase>(&I);
      if (Call == nullptr) {
        continue;
      }
      if (isFreeMemoryPointerStore(Call) ||
          !classifyExternalCall(getCalleeName(Call)).empty()) {
        break;
      }
      if (IsMatchingOutputCopy(Call)) {
        return Call;
      }
    }
  }

  return nullptr;
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

void collectExternalCallOutputWordReadMarkers(
    BasicBlock &BB, CallBase &ExternalCall, Value *OutputBase, Value *OutputSize,
    SmallVectorImpl<CallBase *> &Reads) {
  auto IsMatchingOutputRead = [&](CallBase *Call) {
    std::optional<uint64_t> Offset;
    if (isCallTo(Call, "notdec_solidity_memory_read") && Call->arg_size() == 3) {
      Value *ReadBase = Call->getArgOperand(0);
      if (!isSameOrReloadedFreeMemoryBase(ReadBase, OutputBase)) {
        return false;
      }
      Offset = getUInt64Constant(Call->getArgOperand(1));
    } else if (isCallTo(Call, "evm_mload") && Call->arg_size() == 2) {
      auto *NextCall = dyn_cast_or_null<CallBase>(Call->getNextNode());
      if (NextCall != nullptr &&
          isCallTo(NextCall, "notdec_solidity_memory_read") &&
          NextCall->arg_size() == 3 && NextCall->getArgOperand(2) == Call &&
          isSameOrReloadedFreeMemoryBase(NextCall->getArgOperand(0),
                                         OutputBase)) {
        return false;
      }
      Offset = getOffsetFromBase(Call->getArgOperand(1), OutputBase);
    } else {
      return false;
    }
    return Offset.has_value() && (*Offset % 32) == 0 &&
           outputSizeCoversWord(OutputSize, *Offset);
  };

  bool SeenExternalCall = false;
  for (Instruction &I : BB) {
    if (&I == &ExternalCall) {
      SeenExternalCall = true;
      continue;
    }
    if (!SeenExternalCall) {
      continue;
    }

    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    if (isFreeMemoryPointerStore(Call) ||
        !classifyExternalCall(getCalleeName(Call)).empty()) {
      return;
    }
    if (IsMatchingOutputRead(Call)) {
      Reads.push_back(Call);
    }
  }

  for (BasicBlock *Succ : successors(&BB)) {
    for (Instruction &I : *Succ) {
      auto *Call = dyn_cast<CallBase>(&I);
      if (Call == nullptr) {
        continue;
      }
      if (isFreeMemoryPointerStore(Call) ||
          !classifyExternalCall(getCalleeName(Call)).empty()) {
        break;
      }
      if (IsMatchingOutputRead(Call)) {
        Reads.push_back(Call);
      }
    }
  }
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

void insertExternalCallOutputCopyWriteMarker(LLVMContext &Ctx,
                                             CallBase &ExternalCall,
                                             CallBase &CopyWrite,
                                             uint64_t CallKind) {
  Module *M = ExternalCall.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_external_call_output_copy_write",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {I256, I256, I256, I256, I256}, false));

  IRBuilder<> Builder(&CopyWrite);
  Builder.CreateCall(Marker,
                     {CopyWrite.getArgOperand(0), CopyWrite.getArgOperand(2),
                      CopyWrite.getArgOperand(3), CopyWrite.getArgOperand(4),
                      ConstantInt::get(I256, CallKind)});
}

void insertExternalCallOutputAbiDecodeBufferMarker(LLVMContext &Ctx,
                                                   Instruction &InsertBefore,
                                                   Value *Base, Value *Size,
                                                   uint64_t CallKind) {
  Module *M = InsertBefore.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_external_call_output_abi_decode_buffer",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256}, false));

  IRBuilder<> Builder(&InsertBefore);
  Builder.CreateCall(Marker, {Base, Size,
                              ConstantInt::get(I256, CallKind)});
}

void insertExternalCallOutputAbiDecodeBufferMarker(LLVMContext &Ctx,
                                                   CallBase &CopyWrite,
                                                   uint64_t CallKind) {
  insertExternalCallOutputAbiDecodeBufferMarker(Ctx, CopyWrite,
                                                CopyWrite.getArgOperand(0),
                                                CopyWrite.getArgOperand(3),
                                                CallKind);
}

bool insertExternalCallOutputWordReadMarker(LLVMContext &Ctx,
                                            CallBase &MemoryRead,
                                            Value *OutputBase,
                                            uint64_t CallKind) {
  Module *M = MemoryRead.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_external_call_output_word_read",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256, I256},
                        false));

  Value *Base = nullptr;
  Value *OffsetValue = nullptr;
  Value *ReadValue = nullptr;
  if (isCallTo(&MemoryRead, "notdec_solidity_memory_read") &&
      MemoryRead.arg_size() == 3) {
    Base = MemoryRead.getArgOperand(0);
    OffsetValue = MemoryRead.getArgOperand(1);
    ReadValue = MemoryRead.getArgOperand(2);
  } else if (isCallTo(&MemoryRead, "evm_mload") && MemoryRead.arg_size() == 2) {
    std::optional<uint64_t> Offset =
        getOffsetFromBase(MemoryRead.getArgOperand(1), OutputBase);
    if (!Offset.has_value()) {
      return false;
    }
    Base = OutputBase;
    OffsetValue = ConstantInt::get(I256, *Offset);
    ReadValue = &MemoryRead;
  } else {
    return false;
  }

  Instruction *InsertBefore = MemoryRead.getNextNode();
  if (InsertBefore == nullptr) {
    return false;
  }

  IRBuilder<> Builder(InsertBefore);
  Builder.CreateCall(Marker,
                     {Base, OffsetValue, ReadValue,
                      ConstantInt::get(I256, CallKind)});
  return true;
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
      bool InsertedOutputDecodeBuffer = false;
      if (CallBase *CopyWrite = findExternalCallOutputCopyWriteMarker(
              *Call->getParent(), *Call, Args->OutputBase)) {
        insertExternalCallOutputCopyWriteMarker(Ctx, *Call, *CopyWrite,
                                                CallKind);
        ++NumExternalCallOutputCopyWrites;
        insertExternalCallOutputAbiDecodeBufferMarker(Ctx, *CopyWrite,
                                                      CallKind);
        ++NumExternalCallOutputAbiDecodeBuffers;
        InsertedOutputDecodeBuffer = true;
      }
      SmallVector<CallBase *, 8> OutputWordReads;
      collectExternalCallOutputWordReadMarkers(
          *Call->getParent(), *Call, Args->OutputBase, Args->OutputSize,
          OutputWordReads);
      if (!InsertedOutputDecodeBuffer && !OutputWordReads.empty()) {
        insertExternalCallOutputAbiDecodeBufferMarker(
            Ctx, *OutputWordReads.front(), Args->OutputBase, Args->OutputSize,
            CallKind);
        ++NumExternalCallOutputAbiDecodeBuffers;
      }
      for (CallBase *WordRead : OutputWordReads) {
        if (insertExternalCallOutputWordReadMarker(Ctx, *WordRead,
                                                   Args->OutputBase,
                                                   CallKind)) {
          ++NumExternalCallOutputWordReads;
        }
      }
    }
    addStringMetadata(Ctx, I, KIND_SOLIDITY_EXTERNAL_CALL, Kind);
    ++NumExternalCalls;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
