#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumCheckedBounds,
          "Number of Solidity checked operation/bounds candidates found");

namespace {

uint64_t getRewriteKindCode(StringRef Value) {
  // Keep marker payloads stable without adding one marker overload per skip
  // reason.
  uint64_t Hash = 1469598103934665603ULL;
  for (char C : Value) {
    Hash ^= static_cast<unsigned char>(C);
    Hash *= 1099511628211ULL;
  }
  return Hash;
}

StringRef getCheckedBoundsRewriteMarkerName(StringRef Kind) {
  if (Kind == "checked_add") {
    return "notdec_solidity_rewrite_checked_add";
  }
  if (Kind == "checked_sub") {
    return "notdec_solidity_rewrite_checked_sub";
  }
  if (Kind == "checked_mul") {
    return "notdec_solidity_rewrite_checked_mul";
  }
  if (Kind == "checked_add_bound") {
    return "notdec_solidity_rewrite_checked_add_bound";
  }
  if (Kind == "checked_sub_bound") {
    return "notdec_solidity_rewrite_checked_sub_bound";
  }
  if (Kind == "checked_mul_bound") {
    return "notdec_solidity_rewrite_checked_mul_bound";
  }
  if (Kind == "checked_div") {
    return "notdec_solidity_rewrite_checked_div";
  }
  if (Kind == "checked_mod") {
    return "notdec_solidity_rewrite_checked_mod";
  }
  if (Kind == "checked_exp") {
    return "notdec_solidity_rewrite_checked_exp";
  }
  if (Kind == "array_bounds_memory") {
    return "notdec_solidity_rewrite_array_bounds_memory";
  }
  if (Kind == "array_bounds_calldata") {
    return "notdec_solidity_rewrite_array_bounds_calldata";
  }
  if (Kind == "array_bounds_storage") {
    return "notdec_solidity_rewrite_array_bounds_storage";
  }
  if (Kind == "memory_allocation_bounds") {
    return "notdec_solidity_rewrite_memory_allocation_bounds";
  }
  if (Kind == "memory_allocation_pointer_bounds") {
    return "notdec_solidity_rewrite_memory_allocation_pointer_bounds";
  }
  if (Kind == "storage_bytes_encoding") {
    return "notdec_solidity_rewrite_storage_bytes_encoding";
  }
  if (Kind == "storage_byte_array_length_bounds") {
    return "notdec_solidity_rewrite_storage_byte_array_length_bounds";
  }
  if (Kind == "storage_array_length_bounds") {
    return "notdec_solidity_rewrite_storage_array_length_bounds";
  }
  if (Kind == "enum_conversion") {
    return "notdec_solidity_rewrite_enum_conversion";
  }
  if (Kind == "empty_array_pop_storage") {
    return "notdec_solidity_rewrite_empty_array_pop_storage";
  }
  return "";
}

bool checkedBoundsValueAvailableAtBranch(Value *V, BranchInst *Branch,
                                         DominatorTree &DT) {
  auto *Inst = dyn_cast_or_null<Instruction>(V);
  return Inst == nullptr || DT.dominates(Inst, Branch);
}

bool canRematerializeCheckedBoundsOperand(Value *V, BranchInst *Branch,
                                          DominatorTree &DT) {
  auto *BinOp = dyn_cast_or_null<BinaryOperator>(V);
  if (BinOp == nullptr || BinOp->getOpcode() != Instruction::And) {
    return false;
  }
  return checkedBoundsValueAvailableAtBranch(BinOp->getOperand(0), Branch,
                                             DT) &&
         checkedBoundsValueAvailableAtBranch(BinOp->getOperand(1), Branch, DT);
}

bool checkedBoundsOperandsDominateBranch(const CheckedBoundsMatch &Match,
                                         DominatorTree &DT) {
  if (!Match.Rewrite || Match.Branch == nullptr) {
    return true;
  }
  if (Match.Kind == "memory_allocation_pointer_bounds") {
    return true;
  }

  for (Value *Operand : Match.Operands) {
    if (checkedBoundsValueAvailableAtBranch(Operand, Match.Branch, DT)) {
      continue;
    }
    if ((Match.Kind == "checked_add" || Match.Kind == "checked_sub" ||
         Match.Kind == "checked_mul" || Match.Kind == "checked_add_bound" ||
         Match.Kind == "checked_sub_bound" ||
         Match.Kind == "checked_mul_bound" ||
         Match.Kind == "storage_array_length_bounds") &&
        Match.Operands.size() == 3 && Operand == Match.Operands[2]) {
      continue;
    }
    if ((Match.Kind == "checked_add_bound" ||
         Match.Kind == "checked_sub_bound" ||
         Match.Kind == "checked_mul_bound") &&
        Match.Operands.size() == 4 && Operand == Match.Operands[2]) {
      continue;
    }
    if ((Match.Kind == "checked_div" || Match.Kind == "checked_mod") &&
        canRematerializeCheckedBoundsOperand(Operand, Match.Branch, DT)) {
      continue;
    }
    return false;
  }
  return true;
}

void downgradeCheckedBoundsRewrite(CheckedBoundsMatch &Match,
                                   StringRef SkipReason) {
  Match.Rewrite = false;
  Match.SkipReason = SkipReason;
  Match.Operands.clear();
}

void insertCheckedBoundsSemanticMarker(LLVMContext &Ctx,
                                       const CheckedBoundsMatch &Match) {
  if (!Match.Rewrite || Match.Branch == nullptr) {
    return;
  }

  StringRef MarkerName = getCheckedBoundsRewriteMarkerName(Match.Kind);
  if (MarkerName.empty()) {
    return;
  }

  Module *M = Match.Branch->getModule();
  IRBuilder<> Builder(Match.Branch);
  SmallVector<Value *, 3> Operands(Match.Operands.begin(),
                                   Match.Operands.end());
  if (Match.Kind == "checked_div" || Match.Kind == "checked_mod") {
    for (Value *&Operand : Operands) {
      auto *BinOp = dyn_cast_or_null<BinaryOperator>(Operand);
      if (BinOp != nullptr && BinOp->getParent() != Match.Branch->getParent() &&
          BinOp->getOpcode() == Instruction::And) {
        Operand = Builder.CreateAnd(BinOp->getOperand(0), BinOp->getOperand(1),
                                    BinOp->getName() + ".rewrite");
      }
    }
  }
  if ((Match.Kind == "checked_add" || Match.Kind == "checked_sub" ||
       Match.Kind == "checked_mul" || Match.Kind == "checked_add_bound" ||
       Match.Kind == "checked_sub_bound" || Match.Kind == "checked_mul_bound" ||
       Match.Kind == "storage_array_length_bounds") &&
      (Operands.size() == 3 || Operands.size() == 4)) {
    auto *ResultInst = dyn_cast<Instruction>(Operands[2]);
    if (ResultInst != nullptr &&
        ResultInst->getParent() != Match.Branch->getParent()) {
      if (Match.Kind == "checked_add" || Match.Kind == "checked_add_bound" ||
          Match.Kind == "storage_array_length_bounds") {
        Operands[2] = Builder.CreateAdd(Operands[0], Operands[1],
                                        ResultInst->getName() + ".rewrite");
      } else if (Match.Kind == "checked_sub" ||
                 Match.Kind == "checked_sub_bound") {
        Operands[2] = Builder.CreateSub(Operands[0], Operands[1],
                                        ResultInst->getName() + ".rewrite");
      } else {
        Operands[2] = Builder.CreateMul(Operands[0], Operands[1],
                                        ResultInst->getName() + ".rewrite");
      }
    }
  }
  if (Match.Kind == "memory_allocation_pointer_bounds" &&
      Operands.size() == 3) {
    auto *NewPtrInst = dyn_cast<Instruction>(Operands[2]);
    if (NewPtrInst != nullptr &&
        NewPtrInst->getParent() != Match.Branch->getParent()) {
      Operands[2] = Builder.CreateAdd(Operands[0], Operands[1],
                                      NewPtrInst->getName() + ".rewrite");
    }
  }

  SmallVector<Type *, 3> ArgTypes;
  for (Value *Operand : Operands) {
    ArgTypes.push_back(Operand->getType());
  }
  FunctionCallee Marker = M->getOrInsertFunction(
      MarkerName, FunctionType::get(Type::getVoidTy(Ctx), ArgTypes, false));

  Builder.CreateCall(Marker, Operands);
}

void insertCheckedBoundsSkipMarker(LLVMContext &Ctx,
                                   const CheckedBoundsMatch &Match) {
  if (Match.Branch == nullptr || Match.SkipReason.empty()) {
    return;
  }

  Module *M = Match.Branch->getModule();
  FunctionCallee Marker = M->getOrInsertFunction(
      "notdec_solidity_rewrite_checked_bounds_skip",
      FunctionType::get(Type::getVoidTy(Ctx),
                        {Type::getIntNTy(Ctx, 256), Type::getIntNTy(Ctx, 256)},
                        false));

  IRBuilder<> Builder(Match.Branch);
  Value *Args[] = {
      ConstantInt::get(Type::getIntNTy(Ctx, 256), Match.PanicCode.value_or(0)),
      ConstantInt::get(Type::getIntNTy(Ctx, 256),
                       getRewriteKindCode(Match.SkipReason))};
  Builder.CreateCall(Marker, Args);
}

void addCheckedBoundsMetadata(LLVMContext &Ctx,
                              const CheckedBoundsMatch &Match) {
  if (Match.Branch != nullptr) {
    addPlainMetadata(Ctx, *Match.Branch, KIND_SOLIDITY_CHECKED_BOUNDS,
                     Match.Kind);
    if (Match.PanicCode.has_value()) {
      addPlainMetadata(Ctx, *Match.Branch,
                       "notdec.solidity_checked_bounds.panic_code",
                       Twine(*Match.PanicCode).str());
    }
    if (!Match.SkipReason.empty()) {
      addPlainMetadata(Ctx, *Match.Branch,
                       "notdec.solidity_checked_bounds.skipped",
                       Match.SkipReason);
    }
  }
  if (Match.Rewrite) {
    insertCheckedBoundsSemanticMarker(Ctx, Match);
  } else {
    insertCheckedBoundsSkipMarker(Ctx, Match);
  }
}

void rewriteCheckedBoundsGuard(const CheckedBoundsMatch &Match) {
  if (!Match.Rewrite || Match.Branch == nullptr ||
      Match.SuccessBlock == nullptr) {
    return;
  }

  IRBuilder<> Builder(Match.Branch);
  BranchInst *NewBranch = Builder.CreateBr(Match.SuccessBlock);
  NewBranch->copyMetadata(*Match.Branch);
  Match.Branch->eraseFromParent();
}

} // namespace

PreservedAnalyses CheckedBoundsPass::run(Function &F,
                                         FunctionAnalysisManager &FAM) {
  if (isCalldataAccessCloneFunction(F)) {
    return PreservedAnalyses::all();
  }

  LLVMContext &Ctx = F.getContext();
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  bool Changed = false;
  SmallVector<CheckedBoundsMatch, 8> Matches;

  for (BasicBlock &BB : F) {
    std::optional<CheckedBoundsMatch> Match = matchCheckedBoundsGuard(BB);
    if (!Match.has_value()) {
      continue;
    }
    Matches.push_back(*Match);
  }

  for (CheckedBoundsMatch &Match : Matches) {
    if (!checkedBoundsOperandsDominateBranch(Match, DT)) {
      downgradeCheckedBoundsRewrite(Match, "operand_not_available_at_guard");
    }
    addCheckedBoundsMetadata(Ctx, Match);
    rewriteCheckedBoundsGuard(Match);
    ++NumCheckedBounds;
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
