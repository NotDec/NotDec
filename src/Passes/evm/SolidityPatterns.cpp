#include "Passes/evm/SolidityPatterns.h"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

STATISTIC(NumNonpayableGuards, "Number of Solidity nonpayable guards found");
STATISTIC(NumSelectorPrologues, "Number of Solidity selector prologues found");
STATISTIC(NumSelectorInlinedBodies,
          "Number of Solidity selector inlined body candidates found");
STATISTIC(NumAbiDecodes, "Number of Solidity ABI decode candidates found");
STATISTIC(NumAbiReturns, "Number of Solidity ABI return sites found");
STATISTIC(NumReverts, "Number of Solidity revert sites found");
STATISTIC(NumCheckedBounds,
          "Number of Solidity checked operation/bounds candidates found");
STATISTIC(NumCleanups, "Number of Solidity value cleanup hints found");
STATISTIC(NumStorageAddressing,
          "Number of Solidity storage addressing candidates found");
STATISTIC(NumPackedStorageFields,
          "Number of Solidity packed storage field candidates found");
STATISTIC(NumStorageBytesStrings,
          "Number of Solidity storage bytes/string candidates found");
STATISTIC(NumMemoryObjects, "Number of Solidity memory object hints found");
STATISTIC(NumEvents, "Number of Solidity event candidates found");
STATISTIC(NumExternalCalls, "Number of Solidity external calls found");
STATISTIC(NumSelectorOutlinedBodies,
          "Number of Solidity selector inline bodies outlined");
STATISTIC(NumSelectorOutlineSkipped,
          "Number of Solidity selector inline body outline candidates skipped");
STATISTIC(NumPayabilityCfgRewrites,
          "Number of Solidity nonpayable guards rewritten in the CFG");

namespace notdec::passes::evm {

const char *KIND_SOLIDITY_NONPAYABLE = "notdec.solidity.nonpayable";
const char *KIND_SOLIDITY_PAYABILITY_GUARD = "notdec.solidity.payability_guard";
const char *KIND_SOLIDITY_ENTRY_KIND = "notdec.solidity.entry_kind";
const char *KIND_SOLIDITY_SELECTOR_PROLOGUE =
    "notdec.solidity.selector_prologue";
const char *KIND_SOLIDITY_SELECTOR_INLINED_BODY =
    "notdec.solidity.selector_inlined_body";
const char *KIND_SOLIDITY_ABI_DECODE = "notdec.solidity.abi_decode";
const char *KIND_SOLIDITY_ABI_RETURN = "notdec.solidity.abi_return";
const char *KIND_SOLIDITY_REVERT = "notdec.solidity.revert";
const char *KIND_SOLIDITY_CHECKED_BOUNDS = "notdec.solidity.checked_bounds";
const char *KIND_SOLIDITY_CLEANUP = "notdec.solidity.cleanup";
const char *KIND_SOLIDITY_STORAGE_ADDRESSING =
    "notdec.solidity.storage_addressing";
const char *KIND_SOLIDITY_PACKED_STORAGE_FIELD =
    "notdec.solidity.packed_storage_field";
const char *KIND_SOLIDITY_STORAGE_BYTES_STRING =
    "notdec.solidity.storage_bytes_string";
const char *KIND_SOLIDITY_MEMORY_OBJECT = "notdec.solidity.memory_object";
const char *KIND_SOLIDITY_EVENT = "notdec.solidity.event";
const char *KIND_SOLIDITY_EXTERNAL_CALL = "notdec.solidity.external_call";

namespace {

constexpr StringRef KIND_SOLIDITY_SELECTOR_OUTLINED_BODY =
    "notdec.solidity.selector_outlined_body";
constexpr StringRef KIND_SOLIDITY_SELECTOR_OUTLINE_SKIPPED =
    "notdec.solidity.selector_outline_skipped";
constexpr uint64_t PANIC_SELECTOR = 0x4e487b71;
constexpr uint64_t ERROR_SELECTOR = 0x08c379a0;

// One matched revert site.  Keep the pieces here instead of re-walking the
// block in every consumer: later passes need to know whether a revert exits
// with an empty buffer, a Solidity panic payload, or forwarded returndata.
struct SolidityRevertMatch {
  StringRef Kind = "encoded_candidate";
  CallBase *Revert = nullptr;
  CallBase *SelectorStore = nullptr;
  CallBase *PanicCodeStore = nullptr;
  CallBase *ReturndataCopy = nullptr;
  std::optional<uint64_t> Selector;
  std::optional<uint64_t> PanicCode;
};

// Carries the exact pieces of one canonical nonpayable guard.  The matcher
// only proves the shape; the pass later uses this to annotate the old guard and
// replace only the terminator edge.
struct PayabilityGuardMatch {
  BasicBlock *GuardBlock = nullptr;
  BasicBlock *SuccessBlock = nullptr;
  BasicBlock *FailureBlock = nullptr;
  CallBase *CallValue = nullptr;
  ICmpInst *Condition = nullptr;
  BranchInst *Branch = nullptr;
};

bool isCallTo(const Value *V, StringRef Name) {
  if (V == nullptr) {
    return false;
  }
  auto *Call = dyn_cast<CallBase>(V);
  if (Call == nullptr) {
    return false;
  }
  const Function *Callee = Call->getCalledFunction();
  return Callee != nullptr && Callee->getName() == Name;
}

bool isZero(const Value *V) {
  if (V == nullptr) {
    return false;
  }
  auto *C = dyn_cast<ConstantInt>(V);
  return C != nullptr && C->isZero();
}

bool isConstantIntValue(const Value *V, uint64_t N) {
  if (V == nullptr) {
    return false;
  }
  auto *C = dyn_cast<ConstantInt>(V);
  return C != nullptr && C->getValue() == N;
}

StringRef getCalleeName(const Value *V) {
  if (V == nullptr) {
    return "";
  }
  auto *Call = dyn_cast<CallBase>(V);
  if (Call == nullptr) {
    return "";
  }
  const Function *Callee = Call->getCalledFunction();
  return Callee == nullptr ? StringRef("") : Callee->getName();
}

bool isRewriteMarkerCall(const Instruction &I) {
  auto *Call = dyn_cast<CallBase>(&I);
  if (Call == nullptr) {
    return false;
  }
  const Function *Callee = Call->getCalledFunction();
  return Callee != nullptr &&
         Callee->getName().starts_with("notdec_solidity_rewrite_");
}

void addPlainMetadata(LLVMContext &Ctx, Instruction &I, StringRef Kind,
                      StringRef Value) {
  I.setMetadata(Kind, MDNode::get(Ctx, {MDString::get(Ctx, Value)}));
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
    if (isRewriteMarkerCall(I)) {
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

std::optional<PayabilityGuardMatch> matchPayabilityGuard(BasicBlock &BB) {
  auto *Br = dyn_cast<BranchInst>(BB.getTerminator());
  if (Br == nullptr || !Br->isConditional()) {
    return std::nullopt;
  }

  auto *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
  CallBase *CallValue = getCallValueFromPredicate(Cmp);
  if (CallValue == nullptr) {
    return std::nullopt;
  }

  BasicBlock *Success = nullptr;
  BasicBlock *Failure = nullptr;
  if (Cmp->getPredicate() == ICmpInst::ICMP_EQ) {
    Success = Br->getSuccessor(0);
    Failure = Br->getSuccessor(1);
  } else {
    Success = Br->getSuccessor(1);
    Failure = Br->getSuccessor(0);
  }
  if (!isEmptyRevertBlock(Failure)) {
    return std::nullopt;
  }

  return PayabilityGuardMatch{&BB, Success, Failure, CallValue, Cmp, Br};
}

std::string getRewriteMarkerName(StringRef Kind) {
  std::string Name = "notdec_solidity_rewrite_";
  Kind.consume_front("notdec.solidity.");
  for (char C : Kind) {
    Name.push_back(C == '.' ? '_' : C);
  }
  return Name;
}

std::string getHiddenMetadataName(StringRef Kind) {
  std::string Name = "notdec.solidity.rewrite_hidden.";
  Kind.consume_front("notdec.solidity.");
  Name += Kind.str();
  return Name;
}

uint64_t getRewriteKindCode(StringRef Value) {
  // The marker call is a stable rewrite surface for tests and later lowering.
  // Keep the original EVM instruction alive, and carry the matched kind as a
  // small deterministic code so this pass does not need one helper per variant.
  uint64_t Hash = 1469598103934665603ULL;
  for (char C : Value) {
    Hash ^= static_cast<unsigned char>(C);
    Hash *= 1099511628211ULL;
  }
  return Hash;
}

Instruction *getFunctionRewritePoint(Function &F) {
  BasicBlock &Entry = F.getEntryBlock();
  auto It = Entry.getFirstNonPHIOrDbgOrAlloca();
  Instruction *InsertBefore = It == Entry.end() ? nullptr : &*It;
  if (InsertBefore == nullptr) {
    InsertBefore = Entry.getTerminator();
  }
  return InsertBefore;
}

void insertRewriteMarker(LLVMContext &Ctx, Instruction &I, StringRef Kind,
                         StringRef RewriteKind) {
  Module *M = I.getModule();
  FunctionCallee Marker = M->getOrInsertFunction(
      getRewriteMarkerName(Kind),
      FunctionType::get(Type::getVoidTy(Ctx), {Type::getIntNTy(Ctx, 256)},
                        false));

  IRBuilder<> Builder(Ctx);
  if (I.isTerminator()) {
    Builder.SetInsertPoint(&I);
  } else if (Instruction *Next = I.getNextNode()) {
    Builder.SetInsertPoint(Next);
  } else {
    Builder.SetInsertPoint(I.getParent());
  }

  auto *KindCode =
      ConstantInt::get(Type::getIntNTy(Ctx, 256),
                       getRewriteKindCode(RewriteKind));
  Value *Args[] = {KindCode};
  Builder.CreateCall(Marker, Args);
}

void insertHiddenMarker(LLVMContext &Ctx, Instruction &I, StringRef Kind) {
  Module *M = I.getModule();
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_rewrite_hidden",
      FunctionType::get(Type::getVoidTy(Ctx), {Type::getIntNTy(Ctx, 256)},
                        false));

  IRBuilder<> Builder(Ctx);
  if (I.isTerminator()) {
    Builder.SetInsertPoint(&I);
  } else if (Instruction *Next = I.getNextNode()) {
    Builder.SetInsertPoint(Next);
  } else {
    Builder.SetInsertPoint(I.getParent());
  }

  auto *KindCode =
      ConstantInt::get(Type::getIntNTy(Ctx, 256), getRewriteKindCode(Kind));
  Value *Args[] = {KindCode};
  Builder.CreateCall(Marker, Args);
}

void insertRewriteMarker(LLVMContext &Ctx, Function &F, StringRef Kind,
                         StringRef RewriteKind) {
  Instruction *InsertBefore = getFunctionRewritePoint(F);
  if (InsertBefore == nullptr) {
    return;
  }
  insertRewriteMarker(Ctx, *InsertBefore, Kind, RewriteKind);
}

void insertHiddenMarker(LLVMContext &Ctx, Function &F, StringRef Kind) {
  Instruction *InsertBefore = getFunctionRewritePoint(F);
  if (InsertBefore == nullptr) {
    return;
  }
  insertHiddenMarker(Ctx, *InsertBefore, Kind);
}

void insertPayabilityCfgRewriteMarker(LLVMContext &Ctx,
                                      const PayabilityGuardMatch &Match) {
  Module *M = Match.GuardBlock->getModule();
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_cfg_rewrite_payability_guard",
      FunctionType::get(Type::getVoidTy(Ctx), {Type::getIntNTy(Ctx, 256)},
                        false));

  IRBuilder<> Builder(Match.Branch);
  Value *Args[] = {
      ConstantInt::get(Type::getIntNTy(Ctx, 256),
                       getRewriteKindCode("payability_guard"))};
  Builder.CreateCall(Marker, Args);
}

void addHiddenMetadata(LLVMContext &Ctx, Instruction &I, StringRef Kind,
                       StringRef Value) {
  I.setMetadata(getHiddenMetadataName(Kind),
                MDNode::get(Ctx, {MDString::get(Ctx, Value)}));
}

void addHiddenMetadata(LLVMContext &Ctx, Function &F, StringRef Kind,
                       StringRef Value) {
  Instruction *InsertBefore = getFunctionRewritePoint(F);
  if (InsertBefore == nullptr) {
    return;
  }
  addHiddenMetadata(Ctx, *InsertBefore, Kind, Value);
}

void addStringMetadata(LLVMContext &Ctx, Instruction &I, StringRef Kind,
                       StringRef Value) {
  if (isRewriteMarkerCall(I)) {
    return;
  }
  I.setMetadata(Kind, MDNode::get(Ctx, {MDString::get(Ctx, Value)}));
  addHiddenMetadata(Ctx, I, Kind, Value);
  insertRewriteMarker(Ctx, I, Kind, Value);
  insertHiddenMarker(Ctx, I, Kind);
}

void addStringMetadata(LLVMContext &Ctx, Function &F, StringRef Kind,
                       StringRef Value) {
  bool IsNew = F.getMetadata(Kind) == nullptr;
  F.setMetadata(Kind, MDNode::get(Ctx, {MDString::get(Ctx, Value)}));
  if (IsNew) {
    addHiddenMetadata(Ctx, F, Kind, Value);
    insertRewriteMarker(Ctx, F, Kind, Value);
    insertHiddenMarker(Ctx, F, Kind);
  }
}

void markBlock(LLVMContext &Ctx, BasicBlock &BB, StringRef Kind,
               StringRef Value) {
  SmallVector<Instruction *, 8> OriginalInstructions;
  for (Instruction &I : BB) {
    if (!isRewriteMarkerCall(I)) {
      OriginalInstructions.push_back(&I);
    }
  }
  for (Instruction *I : OriginalInstructions) {
    addStringMetadata(Ctx, *I, Kind, Value);
  }
}

bool isSelectorFunction(const Function &F) {
  return F.getName().starts_with("public_") &&
         F.getName().contains("function_selector");
}

bool isPublicEntryFunction(const Function &F) {
  return F.getName().starts_with("public_") && !isSelectorFunction(F);
}

bool isAbiDecodeContext(const Function &F) {
  return isPublicEntryFunction(F) || isSelectorFunction(F);
}

bool isSelectorValueProducer(const Instruction &I) {
  auto *Call = dyn_cast<CallBase>(&I);
  if (Call == nullptr) {
    return false;
  }
  if (isCallTo(Call, "evm_calldataload") && Call->arg_size() == 2 &&
      isZero(Call->getArgOperand(1))) {
    return true;
  }
  return isCallTo(Call, "evm_shr") && Call->arg_size() == 2 &&
         isConstantIntValue(Call->getArgOperand(0), 224);
}

bool dependsOnSelectorLoad(Value *V, unsigned Depth,
                           SmallPtrSetImpl<Value *> &Seen) {
  if (V == nullptr || Depth == 0 || !Seen.insert(V).second) {
    return false;
  }
  auto *Call = dyn_cast<CallBase>(V);
  if (Call != nullptr && isCallTo(Call, "evm_calldataload") &&
      Call->arg_size() == 2 && isZero(Call->getArgOperand(1))) {
    return true;
  }
  auto *Inst = dyn_cast<Instruction>(V);
  if (Inst == nullptr) {
    return false;
  }
  for (Value *Op : Inst->operands()) {
    if (dependsOnSelectorLoad(Op, Depth - 1, Seen)) {
      return true;
    }
  }
  return false;
}

bool dependsOnSelectorLoad(Value *V, unsigned Depth = 8) {
  SmallPtrSet<Value *, 16> Seen;
  return dependsOnSelectorLoad(V, Depth, Seen);
}

bool isSelectorCompareBranch(BasicBlock &BB) {
  auto *Br = dyn_cast<BranchInst>(BB.getTerminator());
  if (Br == nullptr || !Br->isConditional()) {
    return false;
  }

  auto *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
  if (Cmp == nullptr) {
    return false;
  }
  if (Cmp->getPredicate() != ICmpInst::ICMP_EQ &&
      Cmp->getPredicate() != ICmpInst::ICMP_NE) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    if (isa<ConstantInt>(Cmp->getOperand(I)) &&
        dependsOnSelectorLoad(Cmp->getOperand(1 - I))) {
      return true;
    }
  }
  return false;
}

bool dependsOnCalldataSizeValue(Value *V, unsigned Depth,
                                SmallPtrSetImpl<Value *> &Seen) {
  if (V == nullptr || Depth == 0 || !Seen.insert(V).second) {
    return false;
  }

  auto *Call = dyn_cast<CallBase>(V);
  if (Call != nullptr) {
    return isCallTo(Call, "evm_calldatasize");
  }

  auto *Inst = dyn_cast<Instruction>(V);
  if (Inst == nullptr) {
    return false;
  }
  for (Value *Op : Inst->operands()) {
    if (dependsOnCalldataSizeValue(Op, Depth - 1, Seen)) {
      return true;
    }
  }
  return false;
}

bool dependsOnCalldataSizeValue(Value *V, unsigned Depth = 8) {
  SmallPtrSet<Value *, 16> Seen;
  return dependsOnCalldataSizeValue(V, Depth, Seen);
}

bool isSelectorSizeGate(BasicBlock &BB) {
  auto *Br = dyn_cast<BranchInst>(BB.getTerminator());
  if (Br == nullptr || !Br->isConditional()) {
    return false;
  }

  auto *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
  if (Cmp != nullptr) {
    for (unsigned I = 0; I < 2; ++I) {
      if (isa<ConstantInt>(Cmp->getOperand(I)) &&
          dependsOnCalldataSizeValue(Cmp->getOperand(1 - I))) {
        return true;
      }
    }
  }
  return dependsOnCalldataSizeValue(Br->getCondition());
}

bool isDispatcherBlock(BasicBlock &BB) {
  if (isSelectorSizeGate(BB) || isSelectorCompareBranch(BB)) {
    return true;
  }
  for (Instruction &I : BB) {
    if (isSelectorValueProducer(I)) {
      return true;
    }
  }
  return false;
}

// Selector call stubs are dispatcher leaves, not user body.  They only forward
// to an already lifted public function and then return.
bool isPublicCallStub(BasicBlock &BB) {
  CallBase *OnlyCall = nullptr;
  for (Instruction &I : BB) {
    if (isa<ReturnInst>(&I) || isa<BranchInst>(&I) || isRewriteMarkerCall(I)) {
      continue;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || OnlyCall != nullptr) {
      return false;
    }
    const Function *Callee = Call->getCalledFunction();
    if (Callee == nullptr || !Callee->getName().starts_with("public_")) {
      return false;
    }
    OnlyCall = Call;
  }
  return OnlyCall != nullptr;
}

bool isEmptyRejectBlock(BasicBlock &BB) { return isEmptyRevertBlock(&BB); }

bool isVoidReturnBlock(BasicBlock &BB) {
  for (Instruction &I : BB) {
    if (isa<ReturnInst>(&I)) {
      continue;
    }
    if (isRewriteMarkerCall(I)) {
      continue;
    }
    return false;
  }
  return isa<ReturnInst>(BB.getTerminator());
}

bool hasOutsidePredecessor(BasicBlock &BB,
                           const SmallPtrSetImpl<BasicBlock *> &Region) {
  for (BasicBlock *Pred : predecessors(&BB)) {
    if (!Region.contains(Pred)) {
      return true;
    }
  }
  return false;
}

bool hasOutsideUse(Value &V, const SmallPtrSetImpl<BasicBlock *> &Region) {
  for (User *U : V.users()) {
    auto *UserI = dyn_cast<Instruction>(U);
    if (UserI != nullptr && !Region.contains(UserI->getParent())) {
      return true;
    }
  }
  return false;
}

bool regionHasLiveOut(const SmallVectorImpl<BasicBlock *> &Blocks,
                      const SmallPtrSetImpl<BasicBlock *> &Region) {
  for (BasicBlock *BB : Blocks) {
    for (Instruction &I : *BB) {
      if (!I.getType()->isVoidTy() && hasOutsideUse(I, Region)) {
        return true;
      }
    }
  }
  return false;
}

bool hasSharedTailEntry(ArrayRef<BasicBlock *> Blocks,
                        const SmallPtrSetImpl<BasicBlock *> &Region) {
  for (size_t I = 1; I < Blocks.size(); ++I) {
    if (hasOutsidePredecessor(*Blocks[I], Region)) {
      return true;
    }
  }
  return false;
}

void collectRegionInputs(const SmallVectorImpl<BasicBlock *> &Blocks,
                         const SmallPtrSetImpl<BasicBlock *> &Region,
                         SmallVectorImpl<Instruction *> &Inputs) {
  SmallPtrSet<Instruction *, 16> Seen;
  for (BasicBlock *BB : Blocks) {
    for (Instruction &I : *BB) {
      if (auto *Phi = dyn_cast<PHINode>(&I)) {
        for (unsigned Idx = 0; Idx < Phi->getNumIncomingValues(); ++Idx) {
          if (!Region.contains(Phi->getIncomingBlock(Idx))) {
            continue;
          }
          auto *Def = dyn_cast<Instruction>(Phi->getIncomingValue(Idx));
          if (Def != nullptr && !Region.contains(Def->getParent()) &&
              Seen.insert(Def).second) {
            Inputs.push_back(Def);
          }
        }
        continue;
      }
      for (Value *Op : I.operands()) {
        auto *Def = dyn_cast<Instruction>(Op);
        if (Def == nullptr || Region.contains(Def->getParent())) {
          continue;
        }
        if (Seen.insert(Def).second) {
          Inputs.push_back(Def);
        }
      }
    }
  }
}

bool regionInputsAvailableAtEntry(ArrayRef<Instruction *> Inputs,
                                  BasicBlock *Entry,
                                  const SmallPtrSetImpl<BasicBlock *> &Region,
                                  DominatorTree &DT) {
  for (Instruction *Input : Inputs) {
    for (BasicBlock *Pred : predecessors(Entry)) {
      if (!Region.contains(Pred) &&
          !DT.dominates(Input, Pred->getTerminator())) {
        return false;
      }
    }
  }
  return true;
}

// Build the inline body region from a dispatcher successor.  Stop at dispatcher
// blocks, public call stubs and shared `ret void` exits so the cloned function
// keeps the real fallback/receive body without stealing selector return blocks.
void collectReachableBody(BasicBlock *Entry,
                          const SmallPtrSetImpl<BasicBlock *> &Dispatcher,
                          SmallVectorImpl<BasicBlock *> &Blocks,
                          SmallPtrSetImpl<BasicBlock *> &Region) {
  SmallVector<BasicBlock *, 16> Worklist;
  Worklist.push_back(Entry);

  while (!Worklist.empty()) {
    BasicBlock *BB = Worklist.pop_back_val();
    if (Region.contains(BB) || Dispatcher.contains(BB) ||
        isPublicCallStub(*BB) || isEmptyRejectBlock(*BB) ||
        isVoidReturnBlock(*BB)) {
      continue;
    }
    Region.insert(BB);
    Blocks.push_back(BB);

    for (BasicBlock *Succ : successors(BB)) {
      Worklist.push_back(Succ);
    }
  }
}

bool regionHasOutsideSuccessor(const SmallVectorImpl<BasicBlock *> &Blocks,
                               const SmallPtrSetImpl<BasicBlock *> &Region) {
  for (BasicBlock *BB : Blocks) {
    for (BasicBlock *Succ : successors(BB)) {
      if (!Region.contains(Succ) && !isVoidReturnBlock(*Succ) &&
          !isEmptyRejectBlock(*Succ)) {
        return true;
      }
    }
  }
  return false;
}

bool isSelectorOutlinedCallBlock(const BasicBlock &BB) {
  for (const Instruction &I : BB) {
    if (I.getMetadata(KIND_SOLIDITY_SELECTOR_OUTLINED_BODY) != nullptr) {
      return true;
    }
  }
  return false;
}

bool regionOnlyExitsToOutlinedCall(
    const SmallVectorImpl<BasicBlock *> &Blocks,
    const SmallPtrSetImpl<BasicBlock *> &Region) {
  bool SawOutlinedCall = false;
  for (BasicBlock *BB : Blocks) {
    for (BasicBlock *Succ : successors(BB)) {
      if (Region.contains(Succ) || isVoidReturnBlock(*Succ) ||
          isEmptyRejectBlock(*Succ)) {
        continue;
      }
      if (!isSelectorOutlinedCallBlock(*Succ)) {
        return false;
      }
      SawOutlinedCall = true;
    }
  }
  return SawOutlinedCall;
}

void mapVoidReturnExits(Function &NewF, ArrayRef<BasicBlock *> Blocks,
                        const SmallPtrSetImpl<BasicBlock *> &Region,
                        ValueToValueMapTy &VMap) {
  for (BasicBlock *BB : Blocks) {
    for (BasicBlock *Succ : successors(BB)) {
      if (Region.contains(Succ) || !isVoidReturnBlock(*Succ) ||
          VMap.count(Succ) != 0) {
        continue;
      }
      auto *Exit = BasicBlock::Create(NewF.getContext(),
                                      Succ->getName() + ".outline.ret", &NewF);
      IRBuilder<> Builder(Exit);
      Builder.CreateRetVoid();
      VMap[Succ] = Exit;
    }
  }
}

void mapEmptyRejectExits(Function &NewF, ArrayRef<BasicBlock *> Blocks,
                         const SmallPtrSetImpl<BasicBlock *> &Region,
                         ValueToValueMapTy &VMap) {
  for (BasicBlock *BB : Blocks) {
    for (BasicBlock *Succ : successors(BB)) {
      if (Region.contains(Succ) || !isEmptyRejectBlock(*Succ) ||
          VMap.count(Succ) != 0) {
        continue;
      }
      BasicBlock *NewBB = CloneBasicBlock(Succ, VMap, ".outline", &NewF);
      VMap[Succ] = NewBB;
    }
  }
}

void pruneOutsidePhiIncoming(ArrayRef<BasicBlock *> Blocks,
                             const SmallPtrSetImpl<BasicBlock *> &Region,
                             ValueToValueMapTy &VMap) {
  for (BasicBlock *BB : Blocks) {
    auto *MappedBB = dyn_cast_or_null<BasicBlock>(VMap.lookup(BB));
    if (MappedBB == nullptr) {
      continue;
    }

    for (PHINode &Phi : MappedBB->phis()) {
      for (int I = static_cast<int>(Phi.getNumIncomingValues()) - 1; I >= 0;
           --I) {
        auto *IncomingBB = Phi.getIncomingBlock(static_cast<unsigned>(I));
        if (!Region.contains(IncomingBB)) {
          Phi.removeIncomingValue(static_cast<unsigned>(I), false);
        }
      }
    }
  }
}

bool isSelectorBodySignal(const Instruction &I) {
  auto *Call = dyn_cast<CallBase>(&I);
  if (Call == nullptr) {
    return false;
  }
  StringRef Name = getCalleeName(Call);
  return Name == "evm_call" || Name == "evm_staticcall" ||
         Name == "evm_delegatecall" || Name == "evm_callcode" ||
         Name.starts_with("evm_log") || Name == "evm_return" ||
         Name == "evm_revert";
}

bool regionHasBodySignal(const SmallVectorImpl<BasicBlock *> &Blocks) {
  for (BasicBlock *BB : Blocks) {
    for (Instruction &I : *BB) {
      if (isSelectorBodySignal(I)) {
        return true;
      }
    }
  }
  return false;
}

bool hasCallTo(Function &F, StringRef Name) {
  for (Instruction &I : instructions(F)) {
    if (isCallTo(&I, Name)) {
      return true;
    }
  }
  return false;
}

bool hasWholeSelectorOutlineShape(Function &F,
                                  const SmallVectorImpl<BasicBlock *> &Blocks) {
  if (!hasCallTo(F, "evm_calldatasize") || hasCallTo(F, "evm_caller") ||
      hasCallTo(F, "evm_origin")) {
    return false;
  }
  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call != nullptr && isCallTo(Call, "evm_mstore") &&
        Call->arg_size() == 3 && isConstantIntValue(Call->getArgOperand(1), 64) &&
        isConstantIntValue(Call->getArgOperand(2), 128)) {
      return false;
    }
  }
  return regionHasBodySignal(Blocks);
}

// Keep outline conservative.  Values produced inside the region must not be
// used by the selector function after replacement.  Values produced before the
// region are passed as extra helper arguments by collectRegionInputs().
StringRef getOutlineSkipReason(const SmallVectorImpl<BasicBlock *> &Blocks,
                               const SmallPtrSetImpl<BasicBlock *> &Region) {
  if (Blocks.empty()) {
    return "empty";
  }
  for (size_t I = 1; I < Blocks.size(); ++I) {
    if (hasOutsidePredecessor(*Blocks[I], Region)) {
      // This is a shared tail.  We clone it into the helper and prune PHI
      // incoming edges from the other selector path, while keeping the original
      // shared tail in the selector for that other path.
      continue;
    }
  }
  if (regionHasLiveOut(Blocks, Region)) {
    return "live_out";
  }
  if (regionHasOutsideSuccessor(Blocks, Region)) {
    if (regionOnlyExitsToOutlinedCall(Blocks, Region)) {
      return "already_outlined_successor";
    }
    return "outside_successor";
  }
  return "";
}

// Clone the region into a standalone helper with the same EVM ABI-like
// arguments plus any SSA values produced before the region.  The selector
// function will call this helper and return.
Function *cloneSelectorRegion(Function &F, ArrayRef<BasicBlock *> Blocks,
                              const SmallPtrSetImpl<BasicBlock *> &Region,
                              BasicBlock *Entry,
                              ArrayRef<Instruction *> Inputs) {
  Module *M = F.getParent();
  SmallVector<Type *, 4> ParamTypes;
  for (Argument &Arg : F.args()) {
    ParamTypes.push_back(Arg.getType());
  }
  for (Instruction *Input : Inputs) {
    ParamTypes.push_back(Input->getType());
  }

  auto *NewFTy = FunctionType::get(F.getReturnType(), ParamTypes, false);
  Function *NewF = Function::Create(NewFTy, GlobalValue::InternalLinkage,
                                    "public__notdec_solidity_selector_inline.body",
                                    M);
  NewF->setCallingConv(F.getCallingConv());
  NewF->addFnAttr(Attribute::NoInline);
  NewF->setDSOLocal(true);

  ValueToValueMapTy VMap;
  auto NewArg = NewF->arg_begin();
  for (Argument &Arg : F.args()) {
    NewArg->setName(Arg.getName());
    VMap[&Arg] = &*NewArg++;
  }
  for (Instruction *Input : Inputs) {
    NewArg->setName(Input->getName());
    VMap[Input] = &*NewArg++;
  }

  for (BasicBlock *BB : Blocks) {
    BasicBlock *NewBB = CloneBasicBlock(BB, VMap, "", NewF);
    VMap[BB] = NewBB;
  }
  mapVoidReturnExits(*NewF, Blocks, Region, VMap);
  mapEmptyRejectExits(*NewF, Blocks, Region, VMap);
  if (hasSharedTailEntry(Blocks, Region)) {
    pruneOutsidePhiIncoming(Blocks, Region, VMap);
  }

  for (BasicBlock &BB : *NewF) {
    for (Instruction &I : BB) {
      RemapInstruction(&I, VMap,
                       RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);
    }
  }

  if (auto *MappedEntry = dyn_cast<BasicBlock>(VMap[Entry])) {
    if (MappedEntry != &NewF->getEntryBlock()) {
      MappedEntry->moveBefore(&NewF->getEntryBlock());
    }
  }

  return NewF;
}

// Replace all outside edges into the outlined region with one call block.  The
// original body blocks are removed from the selector function after cloning.
void replaceRegionWithCall(Function &F, Function &Outlined,
                           ArrayRef<BasicBlock *> Blocks,
                           const SmallPtrSetImpl<BasicBlock *> &Region,
                           BasicBlock *Entry,
                           ArrayRef<Instruction *> Inputs) {
  BasicBlock *After =
      BasicBlock::Create(F.getContext(), Entry->getName() + ".outlined", &F);
  IRBuilder<> Builder(After);
  SmallVector<Value *, 4> Args;
  for (Argument &Arg : F.args()) {
    Args.push_back(&Arg);
  }
  for (Instruction *Input : Inputs) {
    Args.push_back(Input);
  }
  Builder.CreateCall(&Outlined, Args)
      ->setMetadata(KIND_SOLIDITY_SELECTOR_OUTLINED_BODY,
                    MDNode::get(F.getContext(),
                                {MDString::get(F.getContext(), "call")}));
  Builder.CreateRetVoid();

  SmallVector<BasicBlock *, 8> OutsidePreds;
  for (BasicBlock *Pred : predecessors(Entry)) {
    if (!Region.contains(Pred)) {
      OutsidePreds.push_back(Pred);
    }
  }
  for (BasicBlock *Pred : OutsidePreds) {
    Pred->getTerminator()->replaceUsesOfWith(Entry, After);
  }

  SmallPtrSet<BasicBlock *, 16> Keep;
  SmallVector<BasicBlock *, 8> Worklist;
  for (BasicBlock *BB : Blocks) {
    if (BB != Entry && hasOutsidePredecessor(*BB, Region) &&
        Keep.insert(BB).second) {
      Worklist.push_back(BB);
    }
  }
  while (!Worklist.empty()) {
    BasicBlock *BB = Worklist.pop_back_val();
    for (BasicBlock *Succ : successors(BB)) {
      if (Region.contains(Succ) && Keep.insert(Succ).second) {
        Worklist.push_back(Succ);
      }
    }
  }

  SmallPtrSet<BasicBlock *, 16> Delete;
  for (BasicBlock *BB : Blocks) {
    if (!Keep.contains(BB)) {
      Delete.insert(BB);
    }
  }
  for (BasicBlock *BB : Blocks) {
    if (!Keep.contains(BB)) {
      continue;
    }
    for (PHINode &Phi : BB->phis()) {
      for (int I = static_cast<int>(Phi.getNumIncomingValues()) - 1; I >= 0;
           --I) {
        if (Delete.contains(Phi.getIncomingBlock(static_cast<unsigned>(I)))) {
          Phi.removeIncomingValue(static_cast<unsigned>(I), false);
        }
      }
    }
  }

  for (BasicBlock *BB : Blocks) {
    if (BB->getParent() != &F || Keep.contains(BB)) {
      continue;
    }
    BB->dropAllReferences();
  }
  for (BasicBlock *BB : Blocks) {
    if (BB->getParent() == &F && !Keep.contains(BB)) {
      BB->eraseFromParent();
    }
  }
}

void replaceWholeFunctionWithCall(Function &F, Function &Outlined) {
  BasicBlock *NewEntry = BasicBlock::Create(F.getContext(), "entry.outlined", &F);
  IRBuilder<> Builder(NewEntry);
  SmallVector<Value *, 4> Args;
  for (Argument &Arg : F.args()) {
    Args.push_back(&Arg);
  }
  Builder.CreateCall(&Outlined, Args)
      ->setMetadata(KIND_SOLIDITY_SELECTOR_OUTLINED_BODY,
                    MDNode::get(F.getContext(),
                                {MDString::get(F.getContext(), "call")}));
  Builder.CreateRetVoid();

  SmallVector<BasicBlock *, 16> OldBlocks;
  for (BasicBlock &BB : F) {
    if (&BB != NewEntry) {
      OldBlocks.push_back(&BB);
    }
  }
  for (BasicBlock *BB : OldBlocks) {
    BB->dropAllReferences();
  }
  for (BasicBlock *BB : OldBlocks) {
    BB->eraseFromParent();
  }
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

bool isSameValue(Value *LHS, Value *RHS) {
  if (LHS == RHS) {
    return true;
  }
  auto *LC = dyn_cast_or_null<ConstantInt>(LHS);
  auto *RC = dyn_cast_or_null<ConstantInt>(RHS);
  return LC != nullptr && RC != nullptr && LC->getValue() == RC->getValue();
}

std::optional<uint64_t> getUInt64Constant(Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr || C->getValue().getActiveBits() > 64) {
    return std::nullopt;
  }
  return C->getZExtValue();
}

std::optional<uint64_t> getSelectorWord(Value *V) {
  auto *Selector = dyn_cast_or_null<CallBase>(V);
  if (Selector == nullptr || !isCallTo(Selector, "evm_shl") ||
      Selector->arg_size() != 2 ||
      !isConstantIntValue(Selector->getArgOperand(0), 224)) {
    return std::nullopt;
  }
  return getUInt64Constant(Selector->getArgOperand(1));
}

bool isPanicSelectorValue(Value *V) {
  return getSelectorWord(V) == PANIC_SELECTOR;
}

CallBase *findReturndataBubbleCopy(BasicBlock &BB, CallBase &Revert) {
  if (Revert.arg_size() != 3 ||
      !isReturndataSize(Revert.getArgOperand(2))) {
    return nullptr;
  }

  for (Instruction &I : BB) {
    if (&I == &Revert) {
      break;
    }
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_returndatacopy") ||
        Call->arg_size() != 5) {
      continue;
    }
    if (isSameValue(Call->getArgOperand(2), Revert.getArgOperand(1)) &&
        isZero(Call->getArgOperand(3)) &&
        isReturndataSize(Call->getArgOperand(4))) {
      return Call;
    }
  }
  return nullptr;
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
    if (isPanicSelectorValue(Call->getArgOperand(2))) {
      return true;
    }
  }
  return false;
}

std::optional<SolidityRevertMatch> matchSolidityRevert(BasicBlock &BB,
                                                        CallBase &Revert) {
  if (Revert.arg_size() != 3) {
    return std::nullopt;
  }

  SolidityRevertMatch Match;
  Match.Revert = &Revert;

  if (isZero(Revert.getArgOperand(1)) && isZero(Revert.getArgOperand(2))) {
    Match.Kind = "empty";
    return Match;
  }

  if (CallBase *Copy = findReturndataBubbleCopy(BB, Revert)) {
    Match.Kind = "returndata_bubble";
    Match.ReturndataCopy = Copy;
    return Match;
  }

  for (Instruction &I : BB) {
    if (&I == &Revert) {
      break;
    }

    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_mstore") ||
        Call->arg_size() != 3) {
      continue;
    }

    if (isZero(Call->getArgOperand(1))) {
      if (std::optional<uint64_t> Selector =
              getSelectorWord(Call->getArgOperand(2))) {
        Match.SelectorStore = Call;
        Match.Selector = Selector;
      }
      continue;
    }

    if (isConstantIntValue(Call->getArgOperand(1), 4)) {
      if (std::optional<uint64_t> Code =
              getUInt64Constant(Call->getArgOperand(2))) {
        Match.PanicCodeStore = Call;
        Match.PanicCode = Code;
      }
    }
  }

  if (isConstantIntValue(Revert.getArgOperand(2), 36) &&
      Match.Selector.has_value() && *Match.Selector == PANIC_SELECTOR) {
    Match.Kind = "panic";
  } else if (Match.Selector.has_value() && *Match.Selector == ERROR_SELECTOR) {
    Match.Kind = "error_string";
  } else if (Match.Selector.has_value()) {
    Match.Kind = "custom_error_candidate";
  } else {
    Match.Kind = "encoded_candidate";
  }

  return Match;
}

void insertPanicRewriteMarker(LLVMContext &Ctx,
                              const SolidityRevertMatch &Match) {
  if (Match.PanicCode == std::nullopt || Match.Revert == nullptr) {
    return;
  }

  Module *M = Match.Revert->getModule();
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_rewrite_revert_panic",
      FunctionType::get(Type::getVoidTy(Ctx), {Type::getIntNTy(Ctx, 256)},
                        false));

  IRBuilder<> Builder(Ctx);
  if (Instruction *Next = Match.Revert->getNextNode()) {
    Builder.SetInsertPoint(Next);
  } else {
    Builder.SetInsertPoint(Match.Revert->getParent());
  }

  Value *Args[] = {
      ConstantInt::get(Type::getIntNTy(Ctx, 256), *Match.PanicCode)};
  Builder.CreateCall(Marker, Args);
}

void insertReturndataBubbleRewriteMarker(LLVMContext &Ctx,
                                         const SolidityRevertMatch &Match) {
  if (Match.Revert == nullptr) {
    return;
  }

  Module *M = Match.Revert->getModule();
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_rewrite_revert_returndata_bubble",
      FunctionType::get(Type::getVoidTy(Ctx), {Type::getIntNTy(Ctx, 256)},
                        false));

  IRBuilder<> Builder(Ctx);
  if (Instruction *Next = Match.Revert->getNextNode()) {
    Builder.SetInsertPoint(Next);
  } else {
    Builder.SetInsertPoint(Match.Revert->getParent());
  }

  Value *Args[] = {
      ConstantInt::get(Type::getIntNTy(Ctx, 256),
                       getRewriteKindCode("returndata_bubble"))};
  Builder.CreateCall(Marker, Args);
}

void insertSelectorRewriteMarker(LLVMContext &Ctx,
                                 const SolidityRevertMatch &Match,
                                 StringRef MarkerName) {
  if (!Match.Selector.has_value() || Match.Revert == nullptr) {
    return;
  }

  Module *M = Match.Revert->getModule();
  FunctionCallee Marker = M->getOrInsertFunction(
      MarkerName,
      FunctionType::get(Type::getVoidTy(Ctx), {Type::getIntNTy(Ctx, 256)},
                        false));

  IRBuilder<> Builder(Ctx);
  if (Instruction *Next = Match.Revert->getNextNode()) {
    Builder.SetInsertPoint(Next);
  } else {
    Builder.SetInsertPoint(Match.Revert->getParent());
  }

  Value *Args[] = {
      ConstantInt::get(Type::getIntNTy(Ctx, 256), *Match.Selector)};
  Builder.CreateCall(Marker, Args);
}

void addRevertMatchMetadata(LLVMContext &Ctx,
                            const SolidityRevertMatch &Match) {
  if (Match.Revert == nullptr) {
    return;
  }

  addStringMetadata(Ctx, *Match.Revert, KIND_SOLIDITY_REVERT, Match.Kind);
  if (Match.PanicCode.has_value()) {
    addPlainMetadata(Ctx, *Match.Revert, "notdec.solidity_revert.panic_code",
                     Twine(*Match.PanicCode).str());
  }
  if (Match.Selector.has_value()) {
    addPlainMetadata(Ctx, *Match.Revert, "notdec.solidity_revert.selector",
                     Twine::utohexstr(*Match.Selector).str());
  }
  if (Match.ReturndataCopy != nullptr) {
    addPlainMetadata(Ctx, *Match.ReturndataCopy,
                     "notdec.solidity_revert.returndata_copy",
                     "returndata_bubble");
  }
}

bool dependsOnCallTo(Value *V, StringRef Name, unsigned Depth,
                     SmallPtrSetImpl<Value *> &Seen) {
  if (V == nullptr || Depth == 0 || !Seen.insert(V).second) {
    return false;
  }
  if (isCallTo(V, Name)) {
    return true;
  }

  if (auto *Inst = dyn_cast<Instruction>(V)) {
    for (Value *Op : Inst->operands()) {
      if (dependsOnCallTo(Op, Name, Depth - 1, Seen)) {
        return true;
      }
    }
  }
  return false;
}

bool dependsOnCallTo(Value *V, StringRef Name, unsigned Depth = 8) {
  SmallPtrSet<Value *, 16> Seen;
  return dependsOnCallTo(V, Name, Depth, Seen);
}

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

bool expressionHasPackedStorageOp(Value *V, unsigned Depth = 8) {
  SmallPtrSet<Value *, 16> Seen;
  return expressionHasPackedStorageOp(V, Depth, Seen);
}

bool isLowBitMask(const ConstantInt &C) {
  return C.getValue() == 1 || C.getValue() == 31 || C.getValue() == 127;
}

bool hasLowBitMaskOperand(const BinaryOperator &Bin) {
  if (Bin.getOpcode() != Instruction::And) {
    return false;
  }
  return (isa<ConstantInt>(Bin.getOperand(0)) &&
          isLowBitMask(*cast<ConstantInt>(Bin.getOperand(0)))) ||
         (isa<ConstantInt>(Bin.getOperand(1)) &&
          isLowBitMask(*cast<ConstantInt>(Bin.getOperand(1))));
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

PreservedAnalyses
SelectorEntryOutliningPass::run(Function &F, FunctionAnalysisManager &FAM) {
  if (!isSelectorFunction(F)) {
    return PreservedAnalyses::all();
  }

  LLVMContext &Ctx = F.getContext();
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  bool Changed = false;
  while (true) {
    SmallPtrSet<BasicBlock *, 32> Dispatcher;
    SmallVector<BasicBlock *, 8> Candidates;
    SmallPtrSet<BasicBlock *, 8> CandidateSet;

    for (BasicBlock &BB : F) {
      if (isDispatcherBlock(BB)) {
        Dispatcher.insert(&BB);
      }
    }
    if (Dispatcher.empty()) {
      break;
    }

    // Keep candidate order stable by walking function blocks.  Some selector
    // bodies become outlineable only after a later shared tail has been split
    // out, so each successful rewrite restarts this search on the new CFG.
    for (BasicBlock &BB : F) {
      if (!Dispatcher.contains(&BB)) {
        continue;
      }
      for (BasicBlock *Succ : successors(&BB)) {
        if (!Dispatcher.contains(Succ) && !isPublicCallStub(*Succ) &&
            !isEmptyRejectBlock(*Succ) && CandidateSet.insert(Succ).second) {
          Candidates.push_back(Succ);
        }
      }
    }

    bool OutlinedThisRound = false;
    SmallVector<std::pair<BasicBlock *, StringRef>, 4> Skipped;
    for (BasicBlock *Entry : Candidates) {
      if (Entry->getParent() != &F) {
        continue;
      }

      SmallVector<BasicBlock *, 16> Blocks;
      SmallPtrSet<BasicBlock *, 16> Region;
      collectReachableBody(Entry, Dispatcher, Blocks, Region);

      if (!regionHasBodySignal(Blocks)) {
        continue;
      }

      StringRef SkipReason = getOutlineSkipReason(Blocks, Region);
      if (!SkipReason.empty()) {
        if (!Blocks.empty() && SkipReason != "already_outlined_successor") {
          Skipped.push_back({Blocks.front(), SkipReason});
        }
        continue;
      }

      SmallVector<Instruction *, 8> Inputs;
      collectRegionInputs(Blocks, Region, Inputs);
      if (!regionInputsAvailableAtEntry(Inputs, Entry, Region, DT)) {
        if (!Blocks.empty()) {
          Skipped.push_back({Blocks.front(), "input_not_available"});
        }
        continue;
      }

      Function *Outlined =
          cloneSelectorRegion(F, Blocks, Region, Entry, Inputs);
      Outlined->setMetadata(
          KIND_SOLIDITY_SELECTOR_OUTLINED_BODY,
          MDNode::get(Ctx, {MDString::get(Ctx, F.getName())}));
      replaceRegionWithCall(F, *Outlined, Blocks, Region, Entry, Inputs);
      ++NumSelectorOutlinedBodies;
      Changed = true;
      OutlinedThisRound = true;
      break;
    }

    if (!OutlinedThisRound) {
      for (auto [BB, SkipReason] : Skipped) {
        if (BB->getParent() == &F) {
          addPlainMetadata(Ctx, *BB->getTerminator(),
                           KIND_SOLIDITY_SELECTOR_OUTLINE_SKIPPED, SkipReason);
          ++NumSelectorOutlineSkipped;
        }
      }
      break;
    }
  }

  if (!Changed) {
    SmallVector<BasicBlock *, 16> Blocks;
    SmallPtrSet<BasicBlock *, 16> Region;
    for (BasicBlock &BB : F) {
      Blocks.push_back(&BB);
      Region.insert(&BB);
    }

    if (hasWholeSelectorOutlineShape(F, Blocks)) {
      SmallVector<Instruction *, 1> Inputs;
      Function *Outlined =
          cloneSelectorRegion(F, Blocks, Region, &F.getEntryBlock(), Inputs);
      Outlined->setMetadata(
          KIND_SOLIDITY_SELECTOR_OUTLINED_BODY,
          MDNode::get(Ctx, {MDString::get(Ctx, F.getName())}));
      replaceWholeFunctionWithCall(F, *Outlined);
      ++NumSelectorOutlinedBodies;
      Changed = true;
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
    std::optional<PayabilityGuardMatch> Match = matchPayabilityGuard(BB);
    if (!Match) {
      continue;
    }

    F.setMetadata(KIND_SOLIDITY_NONPAYABLE,
                  MDNode::get(Ctx, {MDString::get(Ctx, "true")}));
    addStringMetadata(Ctx, *Match->CallValue, KIND_SOLIDITY_PAYABILITY_GUARD,
                      "callvalue");
    addStringMetadata(Ctx, *Match->Condition, KIND_SOLIDITY_PAYABILITY_GUARD,
                      "condition");
    addStringMetadata(Ctx, *Match->Branch, KIND_SOLIDITY_PAYABILITY_GUARD,
                      "branch");
    markBlock(Ctx, *Match->FailureBlock, KIND_SOLIDITY_PAYABILITY_GUARD,
              "revert");

    // Keep the guard block as the predecessor, but consume the compiler
    // nonpayable branch so later passes do not see the outer reject wrapper.
    insertPayabilityCfgRewriteMarker(Ctx, *Match);
    IRBuilder<> Builder(Match->Branch);
    BranchInst *NewBranch = Builder.CreateBr(Match->SuccessBlock);
    NewBranch->copyMetadata(*Match->Branch);
    Match->Branch->eraseFromParent();

    ++NumNonpayableGuards;
    ++NumPayabilityCfgRewrites;
    Changed = true;
    break;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses AbiDecodePass::run(Function &F, FunctionAnalysisManager &) {
  if (!isAbiDecodeContext(F)) {
    return PreservedAnalyses::all();
  }

  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isCallTo(Call, "evm_calldataload") && Call->arg_size() == 2) {
      StringRef Kind = isConstantIntValue(Call->getArgOperand(1), 0)
                           ? "selector_word"
                           : "static_arg_word";
      addStringMetadata(Ctx, I, KIND_SOLIDITY_ABI_DECODE, Kind);
      ++NumAbiDecodes;
      Changed = true;
      continue;
    }

    if (isCallTo(Call, "evm_calldatacopy") && Call->arg_size() == 5) {
      addStringMetadata(Ctx, I, KIND_SOLIDITY_ABI_DECODE,
                        "dynamic_copy_candidate");
      ++NumAbiDecodes;
      Changed = true;
    }
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

      std::optional<SolidityRevertMatch> Match =
          matchSolidityRevert(BB, *Call);
      if (!Match.has_value()) {
        continue;
      }

      addRevertMatchMetadata(Ctx, *Match);
      if (Match->Kind == "panic") {
        insertPanicRewriteMarker(Ctx, *Match);
      } else if (Match->Kind == "returndata_bubble") {
        insertReturndataBubbleRewriteMarker(Ctx, *Match);
      } else if (Match->Kind == "error_string") {
        insertSelectorRewriteMarker(Ctx, *Match,
                                    "notdec_solidity_rewrite_revert_error_string");
      } else if (Match->Kind == "custom_error_candidate") {
        insertSelectorRewriteMarker(Ctx, *Match,
                                    "notdec_solidity_rewrite_revert_custom_error");
      }
      ++NumReverts;
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses CheckedBoundsPass::run(Function &F, FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (BasicBlock &BB : F) {
    auto *Br = dyn_cast<BranchInst>(BB.getTerminator());
    if (Br != nullptr && Br->isConditional()) {
      if ((isEmptyRevertBlock(Br->getSuccessor(0)) ||
           isEmptyRevertBlock(Br->getSuccessor(1))) &&
          !dependsOnCallTo(Br->getCondition(), "evm_callvalue")) {
        addStringMetadata(Ctx, *Br, KIND_SOLIDITY_CHECKED_BOUNDS,
                          "guard_to_empty_revert_candidate");
        ++NumCheckedBounds;
        Changed = true;
      }
    }

    for (Instruction &I : BB) {
      auto *Call = dyn_cast<CallBase>(&I);
      if (Call == nullptr || !isCallTo(Call, "evm_revert") ||
          Call->arg_size() != 3) {
        continue;
      }
      if (isConstantIntValue(Call->getArgOperand(2), 36) &&
          hasPanicSelectorStore(BB, *Call)) {
        addStringMetadata(Ctx, I, KIND_SOLIDITY_CHECKED_BOUNDS,
                          "panic_guard_candidate");
        ++NumCheckedBounds;
        Changed = true;
      }
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
          ++NumPackedStorageFields;
          Changed = true;
          break;
        }
      }
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses StorageBytesStringPass::run(Function &F,
                                              FunctionAnalysisManager &) {
  LLVMContext &Ctx = F.getContext();
  bool Changed = false;

  for (Instruction &I : instructions(F)) {
    if (auto *Bin = dyn_cast<BinaryOperator>(&I);
        Bin != nullptr && hasLowBitMaskOperand(*Bin)) {
      addStringMetadata(Ctx, I, KIND_SOLIDITY_STORAGE_BYTES_STRING,
                        "low_bit_encoding_candidate");
      ++NumStorageBytesStrings;
      Changed = true;
      continue;
    }

    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    if (isCallTo(Call, "evm_mstore8")) {
      addStringMetadata(Ctx, I, KIND_SOLIDITY_STORAGE_BYTES_STRING,
                        "byte_copy_candidate");
      ++NumStorageBytesStrings;
      Changed = true;
      continue;
    }
    if ((isCallTo(Call, "evm_shr") || isCallTo(Call, "evm_shl")) &&
        Call->arg_size() == 2 &&
        isConstantIntValue(Call->getArgOperand(0), 248)) {
      addStringMetadata(Ctx, I, KIND_SOLIDITY_STORAGE_BYTES_STRING,
                        "short_bytes_shift_candidate");
      ++NumStorageBytesStrings;
      Changed = true;
    }
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
