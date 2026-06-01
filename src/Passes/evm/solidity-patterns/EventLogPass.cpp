#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumEvents, "Number of Solidity event candidates found");
STATISTIC(NumEventMemoryConsumers,
          "Number of Solidity event memory consumers found");

namespace {

CallBase *findEventConsumerMarker(BasicBlock &BB, CallBase &Log) {
  for (Instruction &I : BB) {
    if (&I == &Log) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr ||
        !isCallTo(Call, "notdec_solidity_memory_consumer") ||
        Call->arg_size() != 3 ||
        !isConstantIntValue(Call->getArgOperand(2), 3)) {
      continue;
    }
    if (Call->getArgOperand(0) == Log.getArgOperand(1) &&
        Call->getArgOperand(1) == Log.getArgOperand(2)) {
      return Call;
    }
  }
  return nullptr;
}

void insertEventMemoryConsumerMarker(LLVMContext &Ctx, CallBase &Log,
                                     CallBase &Consumer,
                                     uint64_t TopicCount) {
  Module *M = Log.getModule();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_event_memory_consumer",
      FunctionType::get(Type::getVoidTy(Ctx), {I256, I256, I256}, false));

  IRBuilder<> Builder(&Log);
  Builder.CreateCall(Marker,
                     {Consumer.getArgOperand(0), Consumer.getArgOperand(1),
                      ConstantInt::get(I256, TopicCount)});
}

} // namespace

PreservedAnalyses EventLogPass::run(Function &F, FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    StringRef Name = getCalleeName(Call);
    if (!Name.starts_with("evm_log") || Name.size() != 8) {
      continue;
    }
    if (Name.back() < '0' || Name.back() > '4') {
      continue;
    }
    uint64_t TopicCount = static_cast<uint64_t>(Name.back() - '0');
    if (CallBase *Consumer = findEventConsumerMarker(*Call->getParent(),
                                                     *Call)) {
      insertEventMemoryConsumerMarker(Ctx, *Call, *Consumer, TopicCount);
      ++NumEventMemoryConsumers;
    }
    addStringMetadata(Ctx, I, KIND_SOLIDITY_EVENT,
                      ("topic_count_" + Twine(Name.back())).str());
    ++NumEvents;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
