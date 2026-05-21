#include "Passes/evm/SolidityPatterns.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

STATISTIC(NumNonpayableGuards, "Number of Solidity nonpayable guards found");
STATISTIC(NumSelectorPrologues, "Number of Solidity selector prologues found");
STATISTIC(NumSelectorInlinedBodies,
          "Number of Solidity selector inlined body candidates found");
STATISTIC(NumAbiReturns, "Number of Solidity ABI return sites found");
STATISTIC(NumReverts, "Number of Solidity revert sites found");
STATISTIC(NumCleanups, "Number of Solidity value cleanup hints found");
STATISTIC(NumStorageAddressing,
          "Number of Solidity storage addressing candidates found");
STATISTIC(NumMemoryObjects, "Number of Solidity memory object hints found");
STATISTIC(NumEvents, "Number of Solidity event candidates found");
STATISTIC(NumExternalCalls, "Number of Solidity external calls found");

namespace notdec::passes::evm {

const char *KIND_SOLIDITY_NONPAYABLE = "notdec.solidity.nonpayable";
const char *KIND_SOLIDITY_PAYABILITY_GUARD = "notdec.solidity.payability_guard";
const char *KIND_SOLIDITY_ENTRY_KIND = "notdec.solidity.entry_kind";
const char *KIND_SOLIDITY_SELECTOR_PROLOGUE =
    "notdec.solidity.selector_prologue";
const char *KIND_SOLIDITY_SELECTOR_INLINED_BODY =
    "notdec.solidity.selector_inlined_body";
const char *KIND_SOLIDITY_ABI_RETURN = "notdec.solidity.abi_return";
const char *KIND_SOLIDITY_REVERT = "notdec.solidity.revert";
const char *KIND_SOLIDITY_CLEANUP = "notdec.solidity.cleanup";
const char *KIND_SOLIDITY_STORAGE_ADDRESSING =
    "notdec.solidity.storage_addressing";
const char *KIND_SOLIDITY_MEMORY_OBJECT = "notdec.solidity.memory_object";
const char *KIND_SOLIDITY_EVENT = "notdec.solidity.event";
const char *KIND_SOLIDITY_EXTERNAL_CALL = "notdec.solidity.external_call";

namespace {

bool isCallTo(const Value *V, StringRef Name) {
  auto *Call = dyn_cast<CallBase>(V);
  if (Call == nullptr) {
    return false;
  }
  const Function *Callee = Call->getCalledFunction();
  return Callee != nullptr && Callee->getName() == Name;
}

bool isZero(const Value *V) {
  auto *C = dyn_cast<ConstantInt>(V);
  return C != nullptr && C->isZero();
}

bool isConstantIntValue(const Value *V, uint64_t N) {
  auto *C = dyn_cast<ConstantInt>(V);
  return C != nullptr && C->getValue() == N;
}

StringRef getCalleeName(const Value *V) {
  auto *Call = dyn_cast<CallBase>(V);
  if (Call == nullptr) {
    return "";
  }
  const Function *Callee = Call->getCalledFunction();
  return Callee == nullptr ? StringRef("") : Callee->getName();
}

CallBase *getCallValueFromPredicate(ICmpInst *Cmp) {
  if (Cmp == nullptr) {
    return nullptr;
  }
  if (Cmp->getPredicate() != ICmpInst::ICMP_EQ &&
      Cmp->getPredicate() != ICmpInst::ICMP_NE) {
    return nullptr;
  }

  for (unsigned I = 0; I < 2; ++I) {
    Value *MaybeCallValue = Cmp->getOperand(I);
    Value *MaybeZero = Cmp->getOperand(1 - I);
    if (isCallTo(MaybeCallValue, "evm_callvalue") && isZero(MaybeZero)) {
      return cast<CallBase>(MaybeCallValue);
    }
  }
  return nullptr;
}

bool isEmptyRevertBlock(BasicBlock *BB) {
  if (BB == nullptr || !isa<UnreachableInst>(BB->getTerminator())) {
    return false;
  }

  for (Instruction &I : *BB) {
    if (isa<UnreachableInst>(&I)) {
      continue;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_revert") ||
        Call->arg_size() != 3 || !isZero(Call->getArgOperand(1)) ||
        !isZero(Call->getArgOperand(2))) {
      return false;
    }
  }
  return true;
}

void addStringMetadata(LLVMContext &Ctx, Instruction &I, StringRef Kind,
                       StringRef Value) {
  I.setMetadata(Kind, MDNode::get(Ctx, {MDString::get(Ctx, Value)}));
}

void addStringMetadata(LLVMContext &Ctx, Function &F, StringRef Kind,
                       StringRef Value) {
  F.setMetadata(Kind, MDNode::get(Ctx, {MDString::get(Ctx, Value)}));
}

void markBlock(LLVMContext &Ctx, BasicBlock &BB, StringRef Kind,
               StringRef Value) {
  for (Instruction &I : BB) {
    addStringMetadata(Ctx, I, Kind, Value);
  }
}

bool isSelectorFunction(const Function &F) {
  return F.getName().starts_with("public_") &&
         F.getName().contains("function_selector");
}

bool isPublicEntryFunction(const Function &F) {
  return F.getName().starts_with("public_") && !isSelectorFunction(F);
}

bool isMstoreAt(const CallBase &Call, uint64_t Offset, uint64_t Value) {
  return isCallTo(&Call, "evm_mstore") && Call.arg_size() == 3 &&
         isConstantIntValue(Call.getArgOperand(1), Offset) &&
         isConstantIntValue(Call.getArgOperand(2), Value);
}

bool isMloadAt(const CallBase &Call, uint64_t Offset) {
  return isCallTo(&Call, "evm_mload") && Call.arg_size() == 2 &&
         isConstantIntValue(Call.getArgOperand(1), Offset);
}

bool isReturndataSize(Value *V) { return isCallTo(V, "evm_returndatasize"); }

bool isReturndataBubble(BasicBlock &BB, CallBase &Revert) {
  if (Revert.arg_size() != 3 || !isZero(Revert.getArgOperand(1)) ||
      !isReturndataSize(Revert.getArgOperand(2))) {
    return false;
  }

  for (Instruction &I : BB) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_returndatacopy") ||
        Call->arg_size() != 5) {
      continue;
    }
    if (isZero(Call->getArgOperand(2)) && isZero(Call->getArgOperand(3)) &&
        isReturndataSize(Call->getArgOperand(4))) {
      return true;
    }
  }
  return false;
}

bool hasPanicSelectorStore(BasicBlock &BB, CallBase &Revert) {
  for (Instruction &I : BB) {
    if (&I == &Revert) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_mstore") ||
        Call->arg_size() != 3 || !isZero(Call->getArgOperand(1))) {
      continue;
    }
    auto *Selector = dyn_cast<CallBase>(Call->getArgOperand(2));
    if (Selector != nullptr && isCallTo(Selector, "evm_shl") &&
        Selector->arg_size() == 2 &&
        isConstantIntValue(Selector->getArgOperand(0), 224) &&
        isConstantIntValue(Selector->getArgOperand(1), 0x4e487b71)) {
      return true;
    }
  }
  return false;
}

std::string classifyMask(const APInt &Mask) {
  unsigned ActiveBits = Mask.getActiveBits();
  if (Mask.getBitWidth() == 256 && ActiveBits == 160 &&
      Mask.popcount() == 160 && Mask.countTrailingOnes() == 160) {
    return "clean_address";
  }
  if (ActiveBits > 0 && Mask.popcount() == ActiveBits &&
      Mask.countTrailingOnes() == ActiveBits) {
    return "clean_uint" + std::to_string(ActiveBits);
  }
  if (Mask.isNegative() && Mask.countTrailingZeros() > 0) {
    return "clear_low_bits";
  }
  return "";
}

StringRef classifyExternalCall(StringRef Name) {
  return Name == "evm_call"           ? StringRef("call")
         : Name == "evm_staticcall"   ? StringRef("staticcall")
         : Name == "evm_delegatecall" ? StringRef("delegatecall")
         : Name == "evm_callcode"     ? StringRef("callcode")
                                      : StringRef("");
}

} // namespace

PreservedAnalyses SolidityPatternAnnotationPass::run(Module &M,
                                                     ModuleAnalysisManager &) {
  LLVMContext &Ctx = M.getContext();
  NamedMDNode *Node = M.getOrInsertNamedMetadata("notdec.solidity.patterns");
  Node->addOperand(MDNode::get(Ctx, {MDString::get(Ctx, "mode=metadata-only"),
                                     MDString::get(Ctx, "version=1")}));
  return PreservedAnalyses::none();
}

PreservedAnalyses
SelectorInlinedLogicExtractionPass::run(Function &F,
                                        FunctionAnalysisManager &) {
  if (!isSelectorFunction(F)) {
    return PreservedAnalyses::all();
  }

  LLVMContext &Ctx = F.getContext();
  bool Changed = false;
  addStringMetadata(Ctx, F, KIND_SOLIDITY_ENTRY_KIND, "selector");
  Changed = true;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    StringRef CalleeName = getCalleeName(Call);
    if (isMstoreAt(*Call, 64, 128)) {
      addStringMetadata(Ctx, I, KIND_SOLIDITY_SELECTOR_PROLOGUE,
                        "free_memory_pointer");
      ++NumSelectorPrologues;
      continue;
    }

    StringRef CallKind = classifyExternalCall(CalleeName);
    if (!CallKind.empty() || CalleeName.starts_with("evm_log")) {
      addStringMetadata(Ctx, I, KIND_SOLIDITY_SELECTOR_INLINED_BODY,
                        "candidate");
      ++NumSelectorInlinedBodies;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses PayabilityGuardPass::run(Function &F,
                                           FunctionAnalysisManager &) {
  if (!isPublicEntryFunction(F)) {
    return PreservedAnalyses::all();
  }

  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (BasicBlock &BB : F) {
    auto *Br = dyn_cast<BranchInst>(BB.getTerminator());
    if (Br == nullptr || !Br->isConditional()) {
      continue;
    }

    auto *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
    CallBase *CallValue = getCallValueFromPredicate(Cmp);
    if (CallValue == nullptr) {
      continue;
    }

    BasicBlock *Failure = Cmp->getPredicate() == ICmpInst::ICMP_EQ
                              ? Br->getSuccessor(1)
                              : Br->getSuccessor(0);
    if (!isEmptyRevertBlock(Failure)) {
      continue;
    }

    F.setMetadata(KIND_SOLIDITY_NONPAYABLE,
                  MDNode::get(Ctx, {MDString::get(Ctx, "true")}));
    addStringMetadata(Ctx, *CallValue, KIND_SOLIDITY_PAYABILITY_GUARD,
                      "callvalue");
    addStringMetadata(Ctx, *Cmp, KIND_SOLIDITY_PAYABILITY_GUARD, "condition");
    addStringMetadata(Ctx, *Br, KIND_SOLIDITY_PAYABILITY_GUARD, "branch");
    markBlock(Ctx, *Failure, KIND_SOLIDITY_PAYABILITY_GUARD, "revert");

    ++NumNonpayableGuards;
    Changed = true;
    break;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses AbiReturnPass::run(Function &F, FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_return") ||
        Call->arg_size() != 3) {
      continue;
    }

    StringRef Kind = "candidate";
    if (isConstantIntValue(Call->getArgOperand(2), 32)) {
      Kind = "static_1_word";
    } else if (isReturndataSize(Call->getArgOperand(2))) {
      Kind = "returndata_forward";
    }
    addStringMetadata(Ctx, I, KIND_SOLIDITY_ABI_RETURN, Kind);
    addStringMetadata(Ctx, F, KIND_SOLIDITY_ABI_RETURN, "true");
    ++NumAbiReturns;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses SolidityRevertPass::run(Function &F,
                                          FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      auto *Call = dyn_cast<CallBase>(&I);
      if (Call == nullptr || !isCallTo(Call, "evm_revert") ||
          Call->arg_size() != 3) {
        continue;
      }

      StringRef Kind = "candidate";
      if (isZero(Call->getArgOperand(1)) && isZero(Call->getArgOperand(2))) {
        Kind = "empty";
      } else if (isReturndataBubble(BB, *Call)) {
        Kind = "returndata_bubble";
      } else if (isConstantIntValue(Call->getArgOperand(2), 36) &&
                 hasPanicSelectorStore(BB, *Call)) {
        Kind = "panic";
      }

      addStringMetadata(Ctx, I, KIND_SOLIDITY_REVERT, Kind);
      ++NumReverts;
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses ValueCleanupTypeHintPass::run(Function &F,
                                                FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    if (auto *Bin = dyn_cast<BinaryOperator>(&I);
        Bin != nullptr && Bin->getOpcode() == Instruction::And) {
      for (unsigned Op = 0; Op < 2; ++Op) {
        auto *Mask = dyn_cast<ConstantInt>(Bin->getOperand(Op));
        if (Mask == nullptr) {
          continue;
        }
        std::string Kind = classifyMask(Mask->getValue());
        if (!Kind.empty()) {
          addStringMetadata(Ctx, I, KIND_SOLIDITY_CLEANUP, Kind);
          ++NumCleanups;
          Changed = true;
          break;
        }
      }
      continue;
    }

    auto *Call = dyn_cast<CallBase>(&I);
    if (Call != nullptr && isCallTo(Call, "evm_signextend") &&
        Call->arg_size() == 2) {
      addStringMetadata(Ctx, I, KIND_SOLIDITY_CLEANUP, "clean_int");
      ++NumCleanups;
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses StorageAddressingPass::run(Function &F,
                                             FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_sha3") ||
        Call->arg_size() != 3) {
      continue;
    }

    StringRef Kind = "candidate";
    if (isConstantIntValue(Call->getArgOperand(2), 64)) {
      Kind = "mapping_slot_candidate";
    } else if (isConstantIntValue(Call->getArgOperand(2), 32)) {
      Kind = "dynamic_array_data_slot_candidate";
    }
    addStringMetadata(Ctx, I, KIND_SOLIDITY_STORAGE_ADDRESSING, Kind);
    ++NumStorageAddressing;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses MemoryObjectPass::run(Function &F,
                                        FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isMloadAt(*Call, 64)) {
      addStringMetadata(Ctx, I, KIND_SOLIDITY_MEMORY_OBJECT,
                        "free_memory_pointer_load");
      ++NumMemoryObjects;
      Changed = true;
    } else if (isCallTo(Call, "evm_mstore") && Call->arg_size() == 3 &&
               isConstantIntValue(Call->getArgOperand(1), 64)) {
      StringRef Kind = isMstoreAt(*Call, 64, 128)
                           ? "free_memory_pointer_init"
                           : "free_memory_pointer_update";
      addStringMetadata(Ctx, I, KIND_SOLIDITY_MEMORY_OBJECT, Kind);
      ++NumMemoryObjects;
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

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
    addStringMetadata(Ctx, I, KIND_SOLIDITY_EVENT,
                      ("topic_count_" + Twine(Name.back())).str());
    ++NumEvents;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

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
    addStringMetadata(Ctx, I, KIND_SOLIDITY_EXTERNAL_CALL, Kind);
    ++NumExternalCalls;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
