#include "Passes/FreePhiSplitPass.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

using namespace llvm;

namespace notdec {

namespace {

bool isDirectFreeCall(const CallInst &Call) {
  auto *Callee = Call.getCalledFunction();
  return Callee != nullptr && Callee->getName() == "free" &&
         Call.arg_size() == 1;
}

bool isFirstRealInstructionInBlock(const Instruction &Inst) {
  for (const Instruction &Cur : *Inst.getParent()) {
    if (&Cur == &Inst) {
      return true;
    }
    if (isa<PHINode>(Cur) || isa<DbgInfoIntrinsic>(Cur)) {
      continue;
    }
    return false;
  }
  return false;
}

SmallVector<unsigned, 2> successorIndicesTo(const BasicBlock &Pred,
                                            const BasicBlock &Succ) {
  SmallVector<unsigned, 2> Indices;
  const Instruction *Term = Pred.getTerminator();
  for (unsigned I = 0, E = Term->getNumSuccessors(); I != E; ++I) {
    if (Term->getSuccessor(I) == &Succ) {
      Indices.push_back(I);
    }
  }
  return Indices;
}

bool splitFreePhiCall(CallInst &Call, PHINode &Phi,
                      unsigned &InsertedCalls) {
  BasicBlock *Join = Call.getParent();
  DenseMap<BasicBlock *, Value *> IncomingByPred;
  SmallVector<std::pair<BasicBlock *, Value *>, 8> Incoming;

  for (unsigned I = 0, E = Phi.getNumIncomingValues(); I != E; ++I) {
    auto *Pred = Phi.getIncomingBlock(I);
    auto *Value = Phi.getIncomingValue(I);
    if (Pred == Join) {
      return false;
    }
    auto It = IncomingByPred.find(Pred);
    if (It != IncomingByPred.end()) {
      if (It->second != Value) {
        return false;
      }
      continue;
    }
    IncomingByPred.insert({Pred, Value});
    Incoming.push_back({Pred, Value});
  }

  SmallVector<std::pair<BasicBlock *, Value *>, 8> EdgeBlocks;
  for (auto [Pred, Value] : Incoming) {
    auto SuccIndices = successorIndicesTo(*Pred, *Join);
    if (SuccIndices.empty()) {
      return false;
    }

    auto *Term = Pred->getTerminator();
    if (Term->getNumSuccessors() == 1) {
      EdgeBlocks.push_back({Pred, Value});
      continue;
    }

    for (unsigned SuccIndex : SuccIndices) {
      BasicBlock *Split = SplitCriticalEdge(Term, SuccIndex);
      if (Split == nullptr) {
        return false;
      }
      EdgeBlocks.push_back({Split, Value});
    }
  }

  for (auto [EdgeBlock, Value] : EdgeBlocks) {
    Instruction *Term = EdgeBlock->getTerminator();
    auto *NewCall = cast<CallInst>(Call.clone());
    NewCall->setArgOperand(0, Value);
    NewCall->insertBefore(Term->getIterator());
    ++InsertedCalls;
  }

  Call.eraseFromParent();
  if (Phi.use_empty()) {
    Phi.eraseFromParent();
  }
  return true;
}

} // namespace

PreservedAnalyses FreePhiSplitPass::run(Function &F, FunctionAnalysisManager &) {
  SmallVector<CallInst *, 8> Worklist;
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      auto *Call = dyn_cast<CallInst>(&I);
      if (Call == nullptr || !isDirectFreeCall(*Call)) {
        continue;
      }
      auto *Phi = dyn_cast<PHINode>(Call->getArgOperand(0));
      if (Phi == nullptr || Phi->getParent() != &BB || !Phi->hasOneUse() ||
          !isFirstRealInstructionInBlock(*Call)) {
        continue;
      }
      Worklist.push_back(Call);
    }
  }

  bool Changed = false;
  unsigned SplitCalls = 0;
  unsigned InsertedCalls = 0;
  for (CallInst *Call : Worklist) {
    auto *Phi = cast<PHINode>(Call->getArgOperand(0));
    if (!splitFreePhiCall(*Call, *Phi, InsertedCalls)) {
      continue;
    }
    ++SplitCalls;
    Changed = true;
  }

  if (SplitCalls != 0) {
    errs() << "Info: split free(phi) in " << F.getName() << ": " << SplitCalls
           << " call(s), " << InsertedCalls << " edge-local free call(s)\n";
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec
