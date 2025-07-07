

#include "Passes/StackBreaker.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Transforms/Utils/Local.h>
#include <string>
#include <vector>

#define DEBUG_TYPE "stackbreaker"

namespace notdec {

const NewAllocas *StackBreakerRewriter::getNA(OffsetTy Off) {
  for (auto &NA : NAs) {
    if (NA.R.containsOffset(Off)) {
      return &NA;
    }
  }
  return nullptr;
}

void StackBreakerRewriter::visitBinaryOperator(BinaryOperator &I) {
  if (I.use_empty())
    return markAsDead(I);

  if (I.getOpcode() == BinaryOperator::BinaryOps::Add) {
    auto L = I.getOperand(0);
    auto R = I.getOperand(1);
    assert(!isa<ConstantInt>(L));
    // If not constant, then the offset is unknown
    if (!isa<ConstantInt>(R)) {
      IsOffsetKnown = false;
    }

    auto Off = retypd::matchOffsetRangeNoNegativeAccess(R);
    Offset += APInt(Offset.getBitWidth(), Off.offset, true);

    auto Entry = getNA(Offset.getSExtValue());
    assert(Entry != nullptr);
    auto NewAI = Entry->NewAI;

    // rebase the addition on this new alloca.
    IRBuilder<> Builder(NewAI->getContext());
    Builder.SetInsertPoint(NewAI->getNextNode());

    // replace left op
    auto NewLeft = Builder.CreatePtrToInt(NewAI, I.getType());
    auto NewRight = Builder.CreateSub(R, ConstantInt::get(I.getType(), Offset));
    auto NewAdd = Builder.CreateAdd(NewLeft, NewRight, I.getName());

    I.replaceAllUsesWith(NewAdd);
    markAsDead(I);
  } else {
    llvm::errs() << "No rewriting rule for this inst: " << I << "\n";
    assert(false && "No rewriting rule for this inst");
  }
}

bool StackBreaker::runOnAlloca(AllocaInst &AI, SCCTypeResult &HT) {
  auto M = AI.getModule();
  auto &DL = M->getDataLayout();
  // Skip alloca forms that this analysis can't handle.
  auto *AT = AI.getAllocatedType();
  if (AI.isArrayAllocation() || !AT->isSized() || isa<ScalableVectorType>(AT) ||
      DL.getTypeAllocSize(AT).getFixedSize() == 0)
    return false;

  auto AllocaSize = DL.getTypeAllocSize(AT).getFixedSize();

  auto StackHT = HT.ValueTypes.at(&AI);

  // skip if alloca is not a struct
  if (!StackHT->isPointerType()) {
    return false;
  }
  if (!StackHT->getPointeeType()->isRecordType()) {
    return false;
  }
  auto RD = StackHT->getPointeeType()->getAsRecordDecl();

  // break alloca inst according to HType.
  IRBuilder<> Builder(M->getContext());
  Builder.SetInsertPoint(AI.getNextNode());

  std::vector<NewAllocas> NAs;
  bool isNegative = true;
  auto StartOffset = 0;
  auto EndOffset = 0;
  if (isNegative) {
    StartOffset = -AllocaSize;
  } else {
    EndOffset = AllocaSize;
  }

  auto getArrayBySize = [&](uint64_t Size) {
    return ArrayType::get(Builder.getInt8Ty(), Size);
  };

  auto Current = StartOffset;
  // Fields' offset are in increasing order
  for (auto Field : RD->getFields()) {
    // add paddings begin
    if (Current < Field.R.Start) {
      auto R1 = SimpleRange{.Start = Current, .Size = Field.R.Start - Current};
      // Create NewAllocaInst and insert into NAs
      auto NewAlloca = Builder.CreateAlloca(getArrayBySize(R1.Size), nullptr,
                                            "stack_" + std::to_string(Current));
      // TODO add debug info to prevent from deleting.
      NAs.push_back({.R = R1, .NewAI = NewAlloca});
      Current = Field.R.Start;
    }
    // if field is fully contained in the range.
    if (Field.R.Start >= StartOffset && Field.R.end() <= EndOffset) {
      // Create NewAllocaInst and insert into NAs
      auto NewAlloca =
          Builder.CreateAlloca(getArrayBySize(Field.R.Size), nullptr,
                               "stack_" + std::to_string(Field.R.Start));
      // TODO add debug info to prevent from deleting.
      NAs.push_back({.R = Field.R, .NewAI = NewAlloca});
      Current = Field.R.end();
    } else if (Field.R.end() <= StartOffset || Field.R.Start >= EndOffset) {
      // fully out of the range.
      // do nothing, skip
    } else {
      assert(false && "intersecting field?");
    }
  }
  // Add padding in the end
  if (Current < EndOffset) {
    auto R1 = SimpleRange{.Start = Current, .Size = EndOffset - Current};
    // Create NewAllocaInst and insert into NAs
    auto NewAlloca = Builder.CreateAlloca(getArrayBySize(R1.Size), nullptr,
                                          "stack_" + std::to_string(Current));
    // TODO add debug info to prevent from deleting.
    NAs.push_back({.R = R1, .NewAI = NewAlloca});
    Current = EndOffset;
  }

  // Use rewriter to rewrite all accesses.
  StackBreakerRewriter SBR(DL, NAs);
  auto PI = SBR.visitPtr(AI);
  assert(!PI.isAborted());
  assert(!PI.isEscaped());

  assert(AI.getNumUses() == 0);
  AI.eraseFromParent();
  return true;
}

bool StackBreakerRewriter::deleteDeadInstructions() {
  bool Changed = false;
  while (!DeadInsts.empty()) {
    // remove in reverse order
    Instruction *I = dyn_cast_or_null<Instruction>(DeadInsts.pop_back_val());
    if (!I)
      continue;
    llvm::errs() << "Deleting dead instruction: " << *I << "\n";
    LLVM_DEBUG(dbgs() << "Deleting dead instruction: " << *I << "\n");

    // If the instruction is an alloca, find the possible dbg.declare connected
    // to it, and remove it too. We must do this before calling RAUW or we will
    // not be able to find it.
    if (AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
      for (DbgVariableIntrinsic *OldDII : FindDbgAddrUses(AI))
        OldDII->eraseFromParent();
    }

    if (I->getNumUses()) {
      llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                   << "Warning: deleting inst with uses: " << *I << "\n";
    }

    I->replaceAllUsesWith(UndefValue::get(I->getType()));

    for (Use &Operand : I->operands())
      if (Instruction *U = dyn_cast<Instruction>(Operand)) {
        // Zero out the operand and see if it becomes trivially dead.
        Operand = nullptr;
        if (isInstructionTriviallyDead(U) && U != OldAI)
          DeadInsts.push_back(U);
      }

    I->eraseFromParent();
    Changed = true;
  }
  return Changed;
}

} // namespace notdec
