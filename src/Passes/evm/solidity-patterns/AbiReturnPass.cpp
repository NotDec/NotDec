#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumAbiReturns, "Number of Solidity ABI return sites found");

namespace {

StringRef classifyAbiReturnSize(Value *Size) {
  if (isConstantIntValue(Size, 32)) {
    return "static_1_word";
  }
  if (isReturndataSize(Size)) {
    return "returndata_forward";
  }
  return "candidate";
}

uint64_t getAbiReturnKindCode(StringRef Kind) {
  if (Kind == "static_1_word") {
    return 1;
  }
  if (Kind == "returndata_forward") {
    return 2;
  }
  return 0;
}

CallBase *findReturnConsumerMarker(BasicBlock &BB, CallBase &Return) {
  for (Instruction &I : BB) {
    if (&I == &Return) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr ||
        !isCallTo(Call, "notdec_solidity_memory_consumer") ||
        Call->arg_size() != 3 ||
        !isConstantIntValue(Call->getArgOperand(2), 1)) {
      continue;
    }
    if (Call->getArgOperand(0) == Return.getArgOperand(1) &&
        Call->getArgOperand(1) == Return.getArgOperand(2)) {
      return Call;
    }
  }
  return nullptr;
}

void insertAbiReturnMemoryConsumerMarker(LLVMContext &Ctx, CallBase &Return,
                                         CallBase &Consumer, StringRef Kind) {
  Module *M = Return.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_abi_return_memory_consumer",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256}, false));

  IRBuilder<> Builder(&Return);
  Builder.CreateCall(Marker,
                     {Consumer.getArgOperand(0), Consumer.getArgOperand(1),
                      ConstantInt::get(I256, getAbiReturnKindCode(Kind))});
}

} // namespace

PreservedAnalyses AbiReturnPass::run(Function &F, FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_return") ||
        Call->arg_size() != 3) {
      continue;
    }

    StringRef Kind = classifyAbiReturnSize(Call->getArgOperand(2));
    if (CallBase *Consumer = findReturnConsumerMarker(*Call->getParent(),
                                                      *Call)) {
      Kind = classifyAbiReturnSize(Consumer->getArgOperand(1));
      insertAbiReturnMemoryConsumerMarker(Ctx, *Call, *Consumer, Kind);
    }
    addStringMetadata(Ctx, I, KIND_SOLIDITY_ABI_RETURN, Kind);
    addStringMetadata(Ctx, F, KIND_SOLIDITY_ABI_RETURN, "true");
    ++NumAbiReturns;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
