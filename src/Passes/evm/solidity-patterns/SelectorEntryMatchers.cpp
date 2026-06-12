#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
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

using namespace llvm;

namespace notdec::passes::evm::detail {

bool isSelectorFunction(const Function &F) {
  return F.getName().starts_with("public_") &&
         F.getName().contains("function_selector");
}

bool isPublicEntryFunction(const Function &F) {
  return F.getName().starts_with("public_") && !isSelectorFunction(F);
}

bool isCalldataWordLoadAt(const Value *V, uint64_t Offset) {
  auto *Call = dyn_cast_or_null<CallBase>(V);
  return Call != nullptr && isCallTo(Call, "evm_calldataload") &&
         Call->arg_size() == 2 &&
         isConstantIntValue(Call->getArgOperand(1), Offset);
}

bool isSelectorValueProducer(const Instruction &I) {
  auto *Call = dyn_cast<CallBase>(&I);
  if (Call == nullptr) {
    return false;
  }
  if (isCalldataWordLoadAt(Call, 0)) {
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
  if (isCalldataWordLoadAt(Call, 0)) {
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
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (Store.has_value() && isConstantIntValue(Store->Address, 64) &&
        isConstantIntValue(Store->StoredValue, 128)) {
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
  Function *NewF =
      Function::Create(NewFTy, GlobalValue::InternalLinkage,
                       "public__notdec_solidity_selector_inline.body", M);
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
                           BasicBlock *Entry, ArrayRef<Instruction *> Inputs) {
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
      ->setMetadata(
          KIND_SOLIDITY_SELECTOR_OUTLINED_BODY,
          MDNode::get(F.getContext(), {MDString::get(F.getContext(), "call")}));
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
  BasicBlock *NewEntry =
      BasicBlock::Create(F.getContext(), "entry.outlined", &F);
  IRBuilder<> Builder(NewEntry);
  SmallVector<Value *, 4> Args;
  for (Argument &Arg : F.args()) {
    Args.push_back(&Arg);
  }
  Builder.CreateCall(&Outlined, Args)
      ->setMetadata(
          KIND_SOLIDITY_SELECTOR_OUTLINED_BODY,
          MDNode::get(F.getContext(), {MDString::get(F.getContext(), "call")}));
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

} // namespace notdec::passes::evm::detail
