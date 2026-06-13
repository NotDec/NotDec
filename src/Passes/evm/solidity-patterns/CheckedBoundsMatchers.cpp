#include "Passes/evm/SolidityPatternUtils.h"

#include <limits>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Operator.h>
#include <optional>

using namespace llvm;

namespace notdec::passes::evm::detail {

struct NormalizedCondition {
  ICmpInst *Cmp = nullptr;
  ICmpInst::Predicate Predicate;
};

StringRef getCheckedBoundsKindForPanicCode(uint64_t Code) {
  switch (Code) {
  case 0x00:
    return "panic_generic";
  case 0x01:
    return "panic_assert";
  case 0x11:
    return "panic_checked_arithmetic";
  case 0x12:
    return "panic_division_by_zero";
  case 0x21:
    return "panic_enum_conversion";
  case 0x22:
    return "panic_storage_encoding";
  case 0x31:
    return "panic_empty_array_pop";
  case 0x32:
    return "panic_array_out_of_bounds";
  case 0x41:
    return "panic_resource_error";
  case 0x51:
    return "panic_invalid_internal_function";
  default:
    return "panic_unknown";
  }
}

std::optional<NormalizedCondition> normalizeCondition(Value *V,
                                                      bool DesiredTrue) {
  if (auto *ZExt = dyn_cast_or_null<ZExtInst>(V)) {
    return normalizeCondition(ZExt->getOperand(0), DesiredTrue);
  }

  auto *Cmp = dyn_cast_or_null<ICmpInst>(V);
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  for (unsigned I = 0; I < 2; ++I) {
    Value *MaybeBool = Cmp->getOperand(I);
    Value *MaybeZero = Cmp->getOperand(1 - I);
    if (!isZero(MaybeZero)) {
      continue;
    }
    if (!isa<ZExtInst>(MaybeBool)) {
      continue;
    }

    bool OperandTrue = DesiredTrue;
    if (Cmp->getPredicate() == ICmpInst::ICMP_EQ) {
      OperandTrue = !DesiredTrue;
    } else if (Cmp->getPredicate() != ICmpInst::ICMP_NE) {
      break;
    }
    return normalizeCondition(MaybeBool, OperandTrue);
  }

  ICmpInst::Predicate Pred = Cmp->getPredicate();
  if (!DesiredTrue) {
    Pred = Cmp->getInversePredicate();
  }
  return NormalizedCondition{Cmp, Pred};
}

bool blockHasPhi(BasicBlock *BB) {
  return BB != nullptr && isa<PHINode>(BB->begin());
}

std::optional<SolidityRevertMatch>
getPanicRevertFromMetadata(CallBase &Revert) {
  if (!isCallTo(&Revert, "evm_revert") || Revert.arg_size() != 3) {
    return std::nullopt;
  }
  std::optional<uint64_t> PanicCode =
      getUInt64Metadata(Revert, "notdec.solidity_revert.panic_code");
  if (!PanicCode.has_value()) {
    return std::nullopt;
  }

  SolidityRevertMatch Match;
  Match.Kind = "panic";
  Match.Revert = &Revert;
  Match.PanicCode = PanicCode;
  return Match;
}

bool dependsOnCallTo(Value *V, StringRef Name, unsigned Depth);

CallBase *findPanicRevertOnLinearPath(BasicBlock *BB,
                                      SmallPtrSetImpl<BasicBlock *> &Seen,
                                      unsigned Depth = 4) {
  if (BB == nullptr || Depth == 0 || !Seen.insert(BB).second ||
      blockHasPhi(BB)) {
    return nullptr;
  }

  for (Instruction &I : *BB) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "evm_revert") ||
        Call->arg_size() != 3) {
      continue;
    }
    std::optional<SolidityRevertMatch> RevertMatch =
        getPanicRevertFromMetadata(*Call);
    if (RevertMatch.has_value() && RevertMatch->Kind == "panic") {
      return Call;
    }
  }

  auto *Br = dyn_cast<BranchInst>(BB->getTerminator());
  if (Br == nullptr || Br->isConditional()) {
    return nullptr;
  }
  return findPanicRevertOnLinearPath(Br->getSuccessor(0), Seen, Depth - 1);
}

CallBase *findPanicRevertOnLinearPath(BasicBlock *BB) {
  SmallPtrSet<BasicBlock *, 8> Seen;
  return findPanicRevertOnLinearPath(BB, Seen);
}

bool binaryOpHasOperand(BinaryOperator *Op, Value *V) {
  return Op != nullptr && (isSameValue(Op->getOperand(0), V) ||
                           isSameValue(Op->getOperand(1), V));
}

BinaryOperator *findBinaryOpInBlock(BasicBlock *BB,
                                    Instruction::BinaryOps Opcode, Value *LHS,
                                    Value *RHS) {
  if (BB == nullptr) {
    return nullptr;
  }
  for (Instruction &I : *BB) {
    auto *Op = dyn_cast<BinaryOperator>(&I);
    if (Op == nullptr || Op->getOpcode() != Opcode) {
      continue;
    }
    if (isSameValue(Op->getOperand(0), LHS) &&
        isSameValue(Op->getOperand(1), RHS)) {
      return Op;
    }
  }
  return nullptr;
}

BinaryOperator *findCommutativeBinaryOpInBlock(BasicBlock *BB,
                                               Instruction::BinaryOps Opcode,
                                               Value *LHS, Value *RHS) {
  if (BinaryOperator *Op = findBinaryOpInBlock(BB, Opcode, LHS, RHS)) {
    return Op;
  }
  return findBinaryOpInBlock(BB, Opcode, RHS, LHS);
}

Value *findMemoryAllocationShift(BasicBlock *BB, Value *Length) {
  if (BB == nullptr) {
    return nullptr;
  }
  for (Instruction &I : *BB) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call != nullptr && isCallTo(Call, "evm_shl") && Call->arg_size() == 2 &&
        isConstantIntValue(Call->getArgOperand(0), 5) &&
        isSameValue(Call->getArgOperand(1), Length)) {
      return Call;
    }
    auto *Shift = dyn_cast<BinaryOperator>(&I);
    if (Shift != nullptr && Shift->getOpcode() == Instruction::Shl &&
        isSameValue(Shift->getOperand(0), Length) &&
        isConstantIntValue(Shift->getOperand(1), 5)) {
      return Shift;
    }
  }
  return nullptr;
}

bool isMemoryAllocationShift(Value *V, Value *Length) {
  auto *Call = dyn_cast_or_null<CallBase>(V);
  if (Call != nullptr && isCallTo(Call, "evm_shl") && Call->arg_size() == 2 &&
      isConstantIntValue(Call->getArgOperand(0), 5) &&
      isSameValue(Call->getArgOperand(1), Length)) {
    return true;
  }

  auto *Shift = dyn_cast_or_null<BinaryOperator>(V);
  return Shift != nullptr && Shift->getOpcode() == Instruction::Shl &&
         isSameValue(Shift->getOperand(0), Length) &&
         isConstantIntValue(Shift->getOperand(1), 5);
}

Value *matchBitwiseNot(Value *V) {
  auto *Op = dyn_cast_or_null<BinaryOperator>(V);
  if (Op == nullptr || Op->getOpcode() != Instruction::Xor) {
    return nullptr;
  }
  if (isAllOnes(Op->getOperand(0))) {
    return Op->getOperand(1);
  }
  if (isAllOnes(Op->getOperand(1))) {
    return Op->getOperand(0);
  }
  return nullptr;
}

BinaryOperator *matchCheckedMulSuccessCondition(Value *V) {
  auto *Or = dyn_cast_or_null<BinaryOperator>(V);
  if (Or == nullptr || Or->getOpcode() != Instruction::Or) {
    return nullptr;
  }

  Value *ZeroChecked = nullptr;
  CallBase *Div = nullptr;
  Value *OtherOperand = nullptr;
  for (Value *Operand : {Or->getOperand(0), Or->getOperand(1)}) {
    auto *Cmp = dyn_cast<ICmpInst>(Operand);
    if (Cmp == nullptr || Cmp->getPredicate() != ICmpInst::ICMP_EQ) {
      return nullptr;
    }

    if (isZero(Cmp->getOperand(0)) || isZero(Cmp->getOperand(1))) {
      ZeroChecked =
          isZero(Cmp->getOperand(0)) ? Cmp->getOperand(1) : Cmp->getOperand(0);
      continue;
    }

    for (unsigned I = 0; I < 2; ++I) {
      if (auto *Call = dyn_cast<CallBase>(Cmp->getOperand(I))) {
        if (isCallTo(Call, "evm_div") && Call->arg_size() == 2) {
          Div = Call;
          OtherOperand = Cmp->getOperand(1 - I);
          break;
        }
      }
    }
  }
  if (ZeroChecked == nullptr || Div == nullptr || OtherOperand == nullptr ||
      !isSameValue(Div->getArgOperand(1), ZeroChecked)) {
    return nullptr;
  }

  auto *Product = dyn_cast<BinaryOperator>(Div->getArgOperand(0));
  if (Product == nullptr || Product->getOpcode() != Instruction::Mul) {
    return nullptr;
  }
  if (!binaryOpHasOperand(Product, ZeroChecked) ||
      !binaryOpHasOperand(Product, OtherOperand)) {
    return nullptr;
  }
  return Product;
}

BinaryOperator *matchMulByMaxDivBound(ICmpInst *Cmp, ICmpInst::Predicate Pred,
                                      BasicBlock *SuccessBlock) {
  if (Cmp == nullptr) {
    return nullptr;
  }

  Value *Factor = nullptr;
  CallBase *Div = nullptr;
  if (Pred == ICmpInst::ICMP_ULE) {
    Factor = Cmp->getOperand(0);
    Div = dyn_cast<CallBase>(Cmp->getOperand(1));
  } else if (Pred == ICmpInst::ICMP_UGE) {
    Factor = Cmp->getOperand(1);
    Div = dyn_cast<CallBase>(Cmp->getOperand(0));
  } else if (Pred == ICmpInst::ICMP_UGT) {
    Factor = Cmp->getOperand(0);
    Div = dyn_cast<CallBase>(Cmp->getOperand(1));
  } else if (Pred == ICmpInst::ICMP_ULT) {
    Factor = Cmp->getOperand(1);
    Div = dyn_cast<CallBase>(Cmp->getOperand(0));
  }

  if (Factor == nullptr || Div == nullptr || !isCallTo(Div, "evm_div") ||
      Div->arg_size() != 2 || !isAllOnes(Div->getArgOperand(0))) {
    return nullptr;
  }

  return findCommutativeBinaryOpInBlock(SuccessBlock, Instruction::Mul, Factor,
                                        Div->getArgOperand(1));
}

BinaryOperator *matchMulByStrictConstMaxDivBound(ICmpInst *Cmp,
                                                 ICmpInst::Predicate Pred,
                                                 BasicBlock *SuccessBlock) {
  if (Cmp == nullptr) {
    return nullptr;
  }

  CallBase *Div = nullptr;
  ConstantInt *ConstMinusOne = nullptr;
  if (Pred == ICmpInst::ICMP_UGT) {
    Div = dyn_cast<CallBase>(Cmp->getOperand(0));
    ConstMinusOne = dyn_cast<ConstantInt>(Cmp->getOperand(1));
  } else {
    return nullptr;
  }

  if (Div == nullptr || ConstMinusOne == nullptr || !isCallTo(Div, "evm_div") ||
      Div->arg_size() != 2 || !isAllOnes(Div->getArgOperand(0)) ||
      ConstMinusOne->isMinusOne()) {
    return nullptr;
  }

  auto FactorValue = ConstMinusOne->getValue() + 1;
  auto *Factor = ConstantInt::get(ConstMinusOne->getType(), FactorValue);
  return findCommutativeBinaryOpInBlock(SuccessBlock, Instruction::Mul, Factor,
                                        Div->getArgOperand(1));
}

BinaryOperator *matchMulByCleanedMaxDivBound(ICmpInst *Cmp,
                                             ICmpInst::Predicate Pred,
                                             BasicBlock *SuccessBlock,
                                             Value *&MaxValue) {
  if (Cmp == nullptr) {
    return nullptr;
  }

  Value *Factor = nullptr;
  CallBase *Div = nullptr;
  if (Pred == ICmpInst::ICMP_ULE) {
    Factor = Cmp->getOperand(0);
    Div = dyn_cast<CallBase>(Cmp->getOperand(1));
  } else if (Pred == ICmpInst::ICMP_UGE) {
    Factor = Cmp->getOperand(1);
    Div = dyn_cast<CallBase>(Cmp->getOperand(0));
  } else {
    return nullptr;
  }

  if (Factor == nullptr || Div == nullptr || !isCallTo(Div, "evm_div") ||
      Div->arg_size() != 2 || !isPowerOfTwoMinusOne(Div->getArgOperand(0))) {
    return nullptr;
  }

  Value *Max = Div->getArgOperand(0);
  Value *Divisor = Div->getArgOperand(1);
  if (!isUnsignedCleanupToMaxValue(Factor, Max) ||
      !isUnsignedCleanupToMaxValue(Divisor, Max)) {
    return nullptr;
  }

  BinaryOperator *Product = findCommutativeBinaryOpInBlock(
      SuccessBlock, Instruction::Mul, Factor, Divisor);
  if (Product == nullptr) {
    return nullptr;
  }

  MaxValue = Max;
  return Product;
}

BinaryOperator *
matchCheckedMulMaxDivSuccessCondition(Value *V, BasicBlock *SuccessBlock) {
  auto *Or = dyn_cast_or_null<BinaryOperator>(V);
  if (Or == nullptr || Or->getOpcode() != Instruction::Or) {
    return nullptr;
  }

  Value *ZeroChecked = nullptr;
  ICmpInst *BoundCmp = nullptr;
  ICmpInst::Predicate BoundPred = ICmpInst::BAD_ICMP_PREDICATE;
  for (Value *Operand : {Or->getOperand(0), Or->getOperand(1)}) {
    auto *Cmp = dyn_cast<ICmpInst>(Operand);
    if (Cmp == nullptr) {
      return nullptr;
    }
    if (Cmp->getPredicate() == ICmpInst::ICMP_EQ &&
        (isZero(Cmp->getOperand(0)) || isZero(Cmp->getOperand(1)))) {
      ZeroChecked =
          isZero(Cmp->getOperand(0)) ? Cmp->getOperand(1) : Cmp->getOperand(0);
      continue;
    }
    BoundCmp = Cmp;
    BoundPred = Cmp->getPredicate();
  }

  BinaryOperator *Product =
      matchMulByMaxDivBound(BoundCmp, BoundPred, SuccessBlock);
  if (Product == nullptr) {
    Product =
        matchMulByStrictConstMaxDivBound(BoundCmp, BoundPred, SuccessBlock);
  }
  if (Product == nullptr || ZeroChecked == nullptr) {
    return nullptr;
  }

  auto *Div = dyn_cast<CallBase>(BoundPred == ICmpInst::ICMP_ULE
                                     ? BoundCmp->getOperand(1)
                                     : BoundCmp->getOperand(0));
  if (Div == nullptr || !isSameValue(Div->getArgOperand(1), ZeroChecked)) {
    return nullptr;
  }
  return Product;
}

BinaryOperator *
matchCheckedMulCleanedMaxDivSuccessCondition(Value *V, BasicBlock *SuccessBlock,
                                             Value *&MaxValue) {
  auto *Or = dyn_cast_or_null<BinaryOperator>(V);
  if (Or == nullptr || Or->getOpcode() != Instruction::Or) {
    return nullptr;
  }

  Value *ZeroChecked = nullptr;
  ICmpInst *BoundCmp = nullptr;
  ICmpInst::Predicate BoundPred = ICmpInst::BAD_ICMP_PREDICATE;
  for (Value *Operand : {Or->getOperand(0), Or->getOperand(1)}) {
    auto *Cmp = dyn_cast<ICmpInst>(Operand);
    if (Cmp == nullptr) {
      return nullptr;
    }
    if (Cmp->getPredicate() == ICmpInst::ICMP_EQ &&
        (isZero(Cmp->getOperand(0)) || isZero(Cmp->getOperand(1)))) {
      ZeroChecked =
          isZero(Cmp->getOperand(0)) ? Cmp->getOperand(1) : Cmp->getOperand(0);
      continue;
    }
    BoundCmp = Cmp;
    BoundPred = Cmp->getPredicate();
  }

  BinaryOperator *Product =
      matchMulByCleanedMaxDivBound(BoundCmp, BoundPred, SuccessBlock, MaxValue);
  if (Product == nullptr || ZeroChecked == nullptr) {
    return nullptr;
  }

  auto *Div = dyn_cast<CallBase>(BoundPred == ICmpInst::ICMP_ULE
                                     ? BoundCmp->getOperand(1)
                                     : BoundCmp->getOperand(0));
  if (Div == nullptr || !isSameValue(Div->getArgOperand(1), ZeroChecked)) {
    return nullptr;
  }
  return Product;
}

std::optional<CheckedBoundsMatch>
matchCheckedSubGuard(const NormalizedCondition &FailureCond,
                     const SolidityRevertMatch &RevertMatch,
                     BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x11 ||
      FailureCond.Predicate != ICmpInst::ICMP_UGT) {
    return std::nullopt;
  }
  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *RHS = Cmp->getOperand(0);
  Value *LHS = Cmp->getOperand(1);
  BinaryOperator *Sub =
      findBinaryOpInBlock(SuccessBlock, Instruction::Sub, LHS, RHS);
  if (Sub == nullptr) {
    return std::nullopt;
  }
  return CheckedBoundsMatch{"checked_sub",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {Sub->getOperand(0), Sub->getOperand(1), Sub},
                            RevertMatch.PanicCode,
                            true};
}

std::optional<CheckedBoundsMatch>
matchCheckedConstSubLimitGuard(const NormalizedCondition &FailureCond,
                               const SolidityRevertMatch &RevertMatch,
                               BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x11) {
    return std::nullopt;
  }
  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *RHS = nullptr;
  ConstantInt *Limit = nullptr;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGE) {
    RHS = Cmp->getOperand(0);
    Limit = dyn_cast<ConstantInt>(Cmp->getOperand(1));
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE) {
    RHS = Cmp->getOperand(1);
    Limit = dyn_cast<ConstantInt>(Cmp->getOperand(0));
  } else {
    return std::nullopt;
  }
  if (RHS == nullptr || Limit == nullptr || Limit->isZero()) {
    return std::nullopt;
  }

  auto LHSValue = Limit->getValue() - 1;
  auto *LHS = ConstantInt::get(Limit->getType(), LHSValue);
  BinaryOperator *Sub =
      findBinaryOpInBlock(SuccessBlock, Instruction::Sub, LHS, RHS);
  if (Sub == nullptr) {
    return std::nullopt;
  }
  return CheckedBoundsMatch{"checked_sub",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {Sub->getOperand(0), Sub->getOperand(1), Sub},
                            RevertMatch.PanicCode,
                            true};
}

std::optional<CheckedBoundsMatch>
matchCheckedMulGuard(Value *BranchCondition, bool FailureWhenCondTrue,
                     const SolidityRevertMatch &RevertMatch,
                     BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x11) {
    return std::nullopt;
  }

  if (!FailureWhenCondTrue) {
    Value *MaxValue = nullptr;
    if (BinaryOperator *Product = matchCheckedMulCleanedMaxDivSuccessCondition(
            BranchCondition, SuccessBlock, MaxValue)) {
      return CheckedBoundsMatch{
          "checked_mul_bound",
          "",
          nullptr,
          nullptr,
          nullptr,
          RevertMatch.Revert,
          {Product->getOperand(0), Product->getOperand(1), Product, MaxValue},
          RevertMatch.PanicCode,
          true};
    }
    if (BinaryOperator *Product = matchCheckedMulMaxDivSuccessCondition(
            BranchCondition, SuccessBlock)) {
      return CheckedBoundsMatch{
          "checked_mul",
          "",
          nullptr,
          nullptr,
          nullptr,
          RevertMatch.Revert,
          {Product->getOperand(0), Product->getOperand(1), Product},
          RevertMatch.PanicCode,
          true};
    }
    if (BinaryOperator *Product =
            matchCheckedMulSuccessCondition(BranchCondition)) {
      return CheckedBoundsMatch{
          "checked_mul",
          "",
          nullptr,
          nullptr,
          nullptr,
          RevertMatch.Revert,
          {Product->getOperand(0), Product->getOperand(1), Product},
          RevertMatch.PanicCode,
          true};
    }
  }

  std::optional<NormalizedCondition> SuccessCond =
      normalizeCondition(BranchCondition, !FailureWhenCondTrue);
  if (!SuccessCond.has_value() || SuccessCond->Cmp == nullptr ||
      SuccessCond->Predicate != ICmpInst::ICMP_NE ||
      !isZero(SuccessCond->Cmp->getOperand(1))) {
    return std::nullopt;
  }

  BinaryOperator *Product =
      matchCheckedMulSuccessCondition(SuccessCond->Cmp->getOperand(0));
  if (Product == nullptr) {
    return std::nullopt;
  }
  return CheckedBoundsMatch{
      "checked_mul",
      "",
      nullptr,
      nullptr,
      nullptr,
      RevertMatch.Revert,
      {Product->getOperand(0), Product->getOperand(1), Product},
      RevertMatch.PanicCode,
      true};
}

bool matchSmallUnsignedBoundedResult(const NormalizedCondition &FailureCond,
                                     BinaryOperator *&Op,
                                     ConstantInt *&MaxValue) {
  Op = nullptr;
  MaxValue = nullptr;
  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return false;
  }

  Value *Result = nullptr;
  Value *Bound = nullptr;
  bool BoundIsLimit = false;
  switch (FailureCond.Predicate) {
  case ICmpInst::ICMP_UGT:
    Result = Cmp->getOperand(0);
    Bound = Cmp->getOperand(1);
    break;
  case ICmpInst::ICMP_ULT:
    Result = Cmp->getOperand(1);
    Bound = Cmp->getOperand(0);
    break;
  case ICmpInst::ICMP_UGE:
    Result = Cmp->getOperand(0);
    Bound = Cmp->getOperand(1);
    BoundIsLimit = true;
    break;
  case ICmpInst::ICMP_ULE:
    Result = Cmp->getOperand(1);
    Bound = Cmp->getOperand(0);
    BoundIsLimit = true;
    break;
  default:
    return false;
  }

  Op = dyn_cast_or_null<BinaryOperator>(Result);
  if (Op == nullptr || (Op->getOpcode() != Instruction::Add &&
                        Op->getOpcode() != Instruction::Sub &&
                        Op->getOpcode() != Instruction::Mul)) {
    return false;
  }

  auto *BoundConst = dyn_cast_or_null<ConstantInt>(Bound);
  if (BoundConst == nullptr) {
    return false;
  }

  APInt Max = BoundConst->getValue();
  if (BoundIsLimit) {
    if (Max.isZero()) {
      return false;
    }
    Max -= 1;
  }
  MaxValue =
      dyn_cast<ConstantInt>(ConstantInt::get(BoundConst->getType(), Max));
  if (!isSmallUnsignedMax(MaxValue)) {
    return false;
  }

  return isUnsignedCleanupToMax(Op->getOperand(0), Max) &&
         isUnsignedCleanupToMax(Op->getOperand(1), Max);
}

bool matchUnsignedBoundedResult(Value *Result, Value *Max,
                                Instruction::BinaryOps Opcode,
                                BinaryOperator *&Op) {
  Op = dyn_cast_or_null<BinaryOperator>(Result);
  if (Op == nullptr || Op->getOpcode() != Opcode ||
      !isPowerOfTwoMinusOne(Max)) {
    return false;
  }
  return isUnsignedCleanupToMaxValue(Op->getOperand(0), Max) &&
         isUnsignedCleanupToMaxValue(Op->getOperand(1), Max);
}

bool matchUnsignedBoundedResult(const NormalizedCondition &FailureCond,
                                Instruction::BinaryOps Opcode,
                                BinaryOperator *&Op, Value *&MaxValue) {
  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return false;
  }

  Value *Result = nullptr;
  Value *Max = nullptr;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGT) {
    Result = Cmp->getOperand(0);
    Max = Cmp->getOperand(1);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULT) {
    Result = Cmp->getOperand(1);
    Max = Cmp->getOperand(0);
  } else {
    return false;
  }

  if (!matchUnsignedBoundedResult(Result, Max, Opcode, Op)) {
    return false;
  }
  MaxValue = Max;
  return true;
}

bool matchUnsignedAddWithSubtractedMax(const NormalizedCondition &FailureCond,
                                       BasicBlock *SuccessBlock,
                                       BinaryOperator *&Add, Value *&LHS,
                                       Value *&RHS, Value *&MaxValue) {
  Add = nullptr;
  LHS = nullptr;
  RHS = nullptr;
  MaxValue = nullptr;

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr || (FailureCond.Predicate != ICmpInst::ICMP_UGT &&
                         FailureCond.Predicate != ICmpInst::ICMP_ULT)) {
    return false;
  }

  Value *MaybeLHS = FailureCond.Predicate == ICmpInst::ICMP_UGT
                        ? Cmp->getOperand(0)
                        : Cmp->getOperand(1);
  auto *LimitSub = dyn_cast<BinaryOperator>(
      FailureCond.Predicate == ICmpInst::ICMP_UGT ? Cmp->getOperand(1)
                                                  : Cmp->getOperand(0));
  if (LimitSub == nullptr || LimitSub->getOpcode() != Instruction::Sub) {
    return false;
  }

  Value *Max = LimitSub->getOperand(0);
  Value *MaybeRHS = LimitSub->getOperand(1);
  if (!isPowerOfTwoMinusOne(Max) ||
      !isUnsignedCleanupToMaxValue(MaybeLHS, Max) ||
      !isUnsignedCleanupToMaxValue(MaybeRHS, Max)) {
    return false;
  }

  Add = findCommutativeBinaryOpInBlock(SuccessBlock, Instruction::Add, MaybeLHS,
                                       MaybeRHS);
  if (Add == nullptr) {
    return false;
  }

  LHS = MaybeLHS;
  RHS = MaybeRHS;
  MaxValue = Max;
  return true;
}

bool matchUnsignedBoundedMulCleanup(const NormalizedCondition &FailureCond,
                                    BinaryOperator *&Mul, Value *&MaxValue) {
  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr || FailureCond.Predicate != ICmpInst::ICMP_NE) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    auto *CandidateMul = dyn_cast<BinaryOperator>(Cmp->getOperand(I));
    auto *Cleanup = dyn_cast<BinaryOperator>(Cmp->getOperand(1 - I));
    if (CandidateMul == nullptr ||
        CandidateMul->getOpcode() != Instruction::Mul || Cleanup == nullptr ||
        Cleanup->getOpcode() != Instruction::And ||
        !binaryOpHasOperand(Cleanup, CandidateMul)) {
      continue;
    }

    Value *Max = Cleanup->getOperand(0) == CandidateMul
                     ? Cleanup->getOperand(1)
                     : Cleanup->getOperand(0);
    if (matchUnsignedBoundedResult(CandidateMul, Max, Instruction::Mul, Mul)) {
      MaxValue = Max;
      return true;
    }
  }
  return false;
}

BinaryOperator *findCheckedStepResult(BasicBlock *SuccessBlock,
                                      Instruction *GuardInst, Value *Input,
                                      Value *Step) {
  BinaryOperator *Add =
      findBinaryOpInBlock(SuccessBlock, Instruction::Add, Input, Step);
  if (Add == nullptr) {
    Add = findBinaryOpInBlock(SuccessBlock, Instruction::Add, Step, Input);
  }
  if (Add != nullptr) {
    return Add;
  }

  // Some Solidity helper blocks compute value +/- 1 before the overflow check
  // and branch to panic afterwards. Accept only the same checked input and the
  // fixed step constant, so this does not turn arbitrary pre-branch arithmetic
  // into a checked operation.
  BasicBlock *GuardBlock =
      GuardInst == nullptr ? nullptr : GuardInst->getParent();
  Add = findBinaryOpInBlock(GuardBlock, Instruction::Add, Input, Step);
  if (Add == nullptr) {
    Add = findBinaryOpInBlock(GuardBlock, Instruction::Add, Step, Input);
  }
  return Add;
}

Value *getUnsignedCleanupSource(Value *V, Value *Max) {
  auto *And = dyn_cast_or_null<BinaryOperator>(V);
  if (And == nullptr || And->getOpcode() != Instruction::And) {
    return nullptr;
  }
  if (isSameValue(And->getOperand(0), Max)) {
    return And->getOperand(1);
  }
  if (isSameValue(And->getOperand(1), Max)) {
    return And->getOperand(0);
  }
  return nullptr;
}

BinaryOperator *findCheckedCleanedStepResult(BasicBlock *SuccessBlock,
                                             Instruction *GuardInst,
                                             Value *Cleaned, Value *Step,
                                             Value *Max) {
  if (BinaryOperator *Add =
          findCheckedStepResult(SuccessBlock, GuardInst, Cleaned, Step)) {
    return Add;
  }

  // Packed fields at offset zero can be guarded as `and(raw, max) == bound`,
  // then updated as `raw +/- 1` before the later mask writes the field back.
  // Keep this tied to the exact cleanup source and power-of-two mask.
  Value *Raw = getUnsignedCleanupSource(Cleaned, Max);
  if (Raw == nullptr || !isPowerOfTwoMinusOne(Max)) {
    return nullptr;
  }
  return findCheckedStepResult(SuccessBlock, GuardInst, Raw, Step);
}

bool matchCleanedUnsignedIncrement(const NormalizedCondition &FailureCond,
                                   BasicBlock *SuccessBlock,
                                   BinaryOperator *&Add, Value *&Input,
                                   Value *&MaxValue) {
  Add = nullptr;
  Input = nullptr;
  MaxValue = nullptr;
  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr || FailureCond.Predicate != ICmpInst::ICMP_EQ) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    Value *Cleaned = Cmp->getOperand(I);
    Value *Max = Cmp->getOperand(1 - I);
    if (!isPowerOfTwoMinusOne(Max) ||
        !isUnsignedCleanupToMaxValue(Cleaned, Max)) {
      continue;
    }

    auto *One = ConstantInt::get(Cleaned->getType(), 1);
    Add = findCheckedCleanedStepResult(SuccessBlock, Cmp, Cleaned, One, Max);
    if (Add != nullptr) {
      Input = Cleaned;
      MaxValue = Max;
      return true;
    }
  }

  return false;
}

bool matchCleanedUnsignedDecrement(const NormalizedCondition &FailureCond,
                                   BasicBlock *SuccessBlock,
                                   BinaryOperator *&Add, Value *&Input,
                                   Value *&MaxValue) {
  Add = nullptr;
  Input = nullptr;
  MaxValue = nullptr;
  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr || FailureCond.Predicate != ICmpInst::ICMP_EQ) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    Value *Cleaned = Cmp->getOperand(I);
    if (!isZero(Cmp->getOperand(1 - I))) {
      continue;
    }

    auto *And = dyn_cast_or_null<BinaryOperator>(Cleaned);
    if (And == nullptr || And->getOpcode() != Instruction::And) {
      continue;
    }
    Value *Max = nullptr;
    if (isPowerOfTwoMinusOne(And->getOperand(0))) {
      Max = And->getOperand(0);
    } else if (isPowerOfTwoMinusOne(And->getOperand(1))) {
      Max = And->getOperand(1);
    } else {
      continue;
    }
    if (!isUnsignedCleanupToMaxValue(Cleaned, Max)) {
      continue;
    }

    Add = findCheckedCleanedStepResult(SuccessBlock, Cmp, Cleaned, Max, Max);
    if (Add != nullptr) {
      Input = Cleaned;
      MaxValue = Max;
      return true;
    }
  }

  return false;
}

bool isShiftLeft255One(Value *V) {
  auto *Call = dyn_cast_or_null<CallBase>(V);
  return Call != nullptr && isCallTo(Call, "evm_shl") &&
         Call->arg_size() == 2 &&
         isConstantIntValue(Call->getArgOperand(0), 255) &&
         isConstantIntValue(Call->getArgOperand(1), 1);
}

bool isSignedInt256MinValue(Value *V) {
  if (isShiftLeft255One(V)) {
    return true;
  }

  auto *Sub = dyn_cast_or_null<BinaryOperator>(V);
  return Sub != nullptr && Sub->getOpcode() == Instruction::Sub &&
         isZero(Sub->getOperand(0)) && isShiftLeft255One(Sub->getOperand(1));
}

bool isCleanedConstantFalseIncrementOverflowCondition(Value *V) {
  auto *Cmp = dyn_cast_or_null<ICmpInst>(V);
  if (Cmp == nullptr || Cmp->getPredicate() != ICmpInst::ICMP_EQ) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    Value *MaybeShift = Cmp->getOperand(I);
    Value *MaybeConstant = Cmp->getOperand(1 - I);
    if (isShiftLeft255One(MaybeShift) &&
        (isAllOnes(MaybeConstant) || isConstantIntValue(MaybeConstant, 1))) {
      return true;
    }
  }
  return false;
}

// Solidity's uint256 checked increment guard is `if eq(i, not(0)) panic`.
// In one-iteration loops, earlier cleanup can fold the loop index to zero and
// leave only an always-false `shl(255, 1) == {1,-1}` condition.  Keep this
// matcher tied to the latch/header shape so arbitrary dead panic branches do
// not become checked-add rewrites.
bool hasSingleIterationLoopHeader(BasicBlock *Header, BasicBlock *Latch) {
  if (Header == nullptr || Latch == nullptr) {
    return false;
  }
  auto *Br = dyn_cast_or_null<BranchInst>(Header->getTerminator());
  auto *Cond = Br != nullptr && Br->isConditional()
                   ? dyn_cast<PHINode>(Br->getCondition())
                   : nullptr;
  if (Cond == nullptr || !Cond->getType()->isIntegerTy(1)) {
    return false;
  }

  bool BranchesBackToLatch = false;
  for (BasicBlock *Succ : successors(Header)) {
    if (Succ == Latch) {
      BranchesBackToLatch = true;
      break;
    }
  }

  bool LatchMakesHeaderExit = false;
  bool EntryStartsLoop = false;
  for (unsigned I = 0; I < Cond->getNumIncomingValues(); ++I) {
    auto *Incoming = dyn_cast<ConstantInt>(Cond->getIncomingValue(I));
    if (Incoming == nullptr) {
      continue;
    }
    if (Cond->getIncomingBlock(I) == Latch && Incoming->isOne()) {
      LatchMakesHeaderExit = true;
    } else if (Cond->getIncomingBlock(I) != Latch && Incoming->isZero()) {
      EntryStartsLoop = true;
    }
  }

  return BranchesBackToLatch && LatchMakesHeaderExit && EntryStartsLoop;
}

std::optional<CheckedBoundsMatch> matchCleanedConstantFalseIncrementGuard(
    Value *BranchCondition, bool FailureWhenCondTrue,
    const SolidityRevertMatch &RevertMatch, BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x11 ||
      !FailureWhenCondTrue ||
      !isCleanedConstantFalseIncrementOverflowCondition(BranchCondition)) {
    return std::nullopt;
  }

  auto *GuardInst = dyn_cast<Instruction>(BranchCondition);
  BasicBlock *GuardBlock =
      GuardInst == nullptr ? nullptr : GuardInst->getParent();
  if (!hasSingleIterationLoopHeader(SuccessBlock, GuardBlock)) {
    return std::nullopt;
  }

  Type *I256 = Type::getIntNTy(BranchCondition->getContext(), 256);
  auto *Zero = ConstantInt::get(I256, 0);
  auto *One = ConstantInt::get(I256, 1);
  return CheckedBoundsMatch{"checked_add",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {Zero, One, One},
                            RevertMatch.PanicCode,
                            true};
}

bool matchSignedNegationGuard(const NormalizedCondition &FailureCond,
                              BasicBlock *SuccessBlock, BinaryOperator *&Sub,
                              Value *&Input) {
  Sub = nullptr;
  Input = nullptr;
  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr || FailureCond.Predicate != ICmpInst::ICMP_EQ) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    Value *MaybeInput = Cmp->getOperand(I);
    if (!isSignedInt256MinValue(Cmp->getOperand(1 - I))) {
      continue;
    }
    Sub = findBinaryOpInBlock(SuccessBlock, Instruction::Sub,
                              ConstantInt::get(MaybeInput->getType(), 0),
                              MaybeInput);
    if (Sub != nullptr) {
      Input = MaybeInput;
      return true;
    }
  }
  return false;
}

bool matchSignedNarrowSubRangeGuard(const NormalizedCondition &FailureCond,
                                    BinaryOperator *&Sub, Value *&MaxValue) {
  Sub = nullptr;
  MaxValue = nullptr;
  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr || FailureCond.Predicate != ICmpInst::ICMP_UGE) {
    return false;
  }

  Value *MaybeAdd = nullptr;
  Value *MaybeRange = nullptr;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGE) {
    MaybeAdd = Cmp->getOperand(0);
    MaybeRange = Cmp->getOperand(1);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE) {
    MaybeAdd = Cmp->getOperand(1);
    MaybeRange = Cmp->getOperand(0);
  } else {
    return false;
  }

  auto *Range = dyn_cast<ConstantInt>(MaybeRange);
  auto *Add = dyn_cast<BinaryOperator>(MaybeAdd);
  if (Range == nullptr || Add == nullptr ||
      Add->getOpcode() != Instruction::Add || !Range->getValue().isPowerOf2()) {
    return false;
  }

  auto HalfRange = Range->getValue().lshr(1);
  for (unsigned I = 0; I < 2; ++I) {
    auto *Offset = dyn_cast<ConstantInt>(Add->getOperand(I));
    auto *CandidateSub = dyn_cast<BinaryOperator>(Add->getOperand(1 - I));
    if (Offset == nullptr || Offset->getValue() != HalfRange ||
        CandidateSub == nullptr ||
        CandidateSub->getOpcode() != Instruction::Sub) {
      continue;
    }

    Sub = CandidateSub;
    MaxValue = ConstantInt::get(Range->getType(), HalfRange - 1);
    return true;
  }
  return false;
}

// Solidity signed add/sub guards can be lowered as a success condition feeding
// the non-revert edge. Keep these matchers tied to that codegen shape.
bool matchSignedNegative(Value *V, Value *&Input) {
  auto *Cmp = dyn_cast_or_null<ICmpInst>(V);
  if (Cmp == nullptr || Cmp->getPredicate() != ICmpInst::ICMP_SLT ||
      !isZero(Cmp->getOperand(1))) {
    return false;
  }
  Input = Cmp->getOperand(0);
  return true;
}

bool matchSignedNonNegative(Value *V, Value *&Input) {
  auto *Cmp = dyn_cast_or_null<ICmpInst>(V);
  if (Cmp == nullptr) {
    return false;
  }
  if (Cmp->getPredicate() == ICmpInst::ICMP_SGE && isZero(Cmp->getOperand(1))) {
    Input = Cmp->getOperand(0);
    return true;
  }
  if (Cmp->getPredicate() == ICmpInst::ICMP_SGT &&
      isAllOnes(Cmp->getOperand(1))) {
    Input = Cmp->getOperand(0);
    return true;
  }
  return false;
}

bool matchSignedSubSuccessFirstOr(Value *V, BinaryOperator *&Sub, Value *&X,
                                  Value *&Y) {
  auto *Or = dyn_cast_or_null<BinaryOperator>(V);
  if (Or == nullptr || Or->getOpcode() != Instruction::Or) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    Value *MaybeY = nullptr;
    if (!matchSignedNegative(Or->getOperand(I), MaybeY)) {
      continue;
    }
    auto *Cmp = dyn_cast<ICmpInst>(Or->getOperand(1 - I));
    if (Cmp == nullptr || Cmp->getPredicate() != ICmpInst::ICMP_SLE) {
      continue;
    }
    auto *MaybeSub = dyn_cast<BinaryOperator>(Cmp->getOperand(0));
    if (MaybeSub == nullptr || MaybeSub->getOpcode() != Instruction::Sub ||
        !isSameValue(MaybeSub->getOperand(1), MaybeY) ||
        !isSameValue(Cmp->getOperand(1), MaybeSub->getOperand(0))) {
      continue;
    }
    Sub = MaybeSub;
    X = MaybeSub->getOperand(0);
    Y = MaybeY;
    return true;
  }
  return false;
}

bool matchSignedSubSuccessSecondOr(Value *V, BinaryOperator *Sub, Value *X,
                                   Value *Y) {
  auto *Or = dyn_cast_or_null<BinaryOperator>(V);
  if (Or == nullptr || Or->getOpcode() != Instruction::Or) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    Value *MaybeY = nullptr;
    if (!matchSignedNonNegative(Or->getOperand(I), MaybeY) ||
        !isSameValue(MaybeY, Y)) {
      continue;
    }
    auto *Cmp = dyn_cast<ICmpInst>(Or->getOperand(1 - I));
    if (Cmp == nullptr || Cmp->getPredicate() != ICmpInst::ICMP_SGE ||
        !isSameValue(Cmp->getOperand(0), Sub) ||
        !isSameValue(Cmp->getOperand(1), X)) {
      continue;
    }
    return true;
  }
  return false;
}

BinaryOperator *matchSignedSubSuccessCondition(Value *V) {
  auto *And = dyn_cast_or_null<BinaryOperator>(V);
  if (And == nullptr || And->getOpcode() != Instruction::And) {
    return nullptr;
  }

  for (unsigned I = 0; I < 2; ++I) {
    BinaryOperator *Sub = nullptr;
    Value *X = nullptr;
    Value *Y = nullptr;
    if (matchSignedSubSuccessFirstOr(And->getOperand(I), Sub, X, Y) &&
        matchSignedSubSuccessSecondOr(And->getOperand(1 - I), Sub, X, Y)) {
      return Sub;
    }
  }
  return nullptr;
}

bool matchSignBit(Value *V, Value *Input) {
  auto *Shift = dyn_cast_or_null<BinaryOperator>(V);
  return Shift != nullptr && Shift->getOpcode() == Instruction::LShr &&
         isSameValue(Shift->getOperand(0), Input) &&
         isConstantIntValue(Shift->getOperand(1), 255);
}

bool matchSignedAddFailureZExt(Value *V, BinaryOperator *&Add, Value *&X,
                               Value *&Y, ICmpInst *&SumLessY) {
  auto *ZExt = dyn_cast_or_null<ZExtInst>(V);
  auto *And =
      ZExt == nullptr ? nullptr : dyn_cast<BinaryOperator>(ZExt->getOperand(0));
  if (And == nullptr || And->getOpcode() != Instruction::And) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    auto *Cmp = dyn_cast<ICmpInst>(And->getOperand(I));
    Value *MaybeX = nullptr;
    if (!matchSignedNonNegative(And->getOperand(1 - I), MaybeX) ||
        Cmp == nullptr || Cmp->getPredicate() != ICmpInst::ICMP_SLT) {
      continue;
    }
    auto *MaybeAdd = dyn_cast<BinaryOperator>(Cmp->getOperand(0));
    if (MaybeAdd == nullptr || MaybeAdd->getOpcode() != Instruction::Add ||
        !binaryOpHasOperand(MaybeAdd, MaybeX) ||
        !binaryOpHasOperand(MaybeAdd, Cmp->getOperand(1))) {
      continue;
    }
    Add = MaybeAdd;
    X = MaybeX;
    Y = Cmp->getOperand(1);
    SumLessY = Cmp;
    return true;
  }
  return false;
}

bool matchSignedAddFailureSelect(Value *V, BinaryOperator *Add, Value *X,
                                 ICmpInst *SumLessY) {
  auto *Select = dyn_cast_or_null<SelectInst>(V);
  if (Select == nullptr || !isSameValue(Select->getCondition(), SumLessY) ||
      !isZero(Select->getTrueValue())) {
    return false;
  }
  return matchSignBit(Select->getFalseValue(), X) &&
         isSameValue(SumLessY->getOperand(0), Add);
}

BinaryOperator *matchSignedAddSuccessCondition(Value *V) {
  auto *Cmp = dyn_cast_or_null<ICmpInst>(V);
  if (Cmp == nullptr || Cmp->getPredicate() != ICmpInst::ICMP_EQ ||
      !isZero(Cmp->getOperand(1))) {
    return nullptr;
  }
  auto *Or = dyn_cast<BinaryOperator>(Cmp->getOperand(0));
  if (Or == nullptr || Or->getOpcode() != Instruction::Or) {
    return nullptr;
  }

  for (unsigned I = 0; I < 2; ++I) {
    BinaryOperator *Add = nullptr;
    Value *X = nullptr;
    Value *Y = nullptr;
    ICmpInst *SumLessY = nullptr;
    if (matchSignedAddFailureZExt(Or->getOperand(I), Add, X, Y, SumLessY) &&
        matchSignedAddFailureSelect(Or->getOperand(1 - I), Add, X, SumLessY)) {
      return Add;
    }
  }
  return nullptr;
}

bool matchSignedMulMinValueSpecialCase(Value *V, BinaryOperator *&Mul,
                                       Value *&X, Value *&Y) {
  auto *Or = dyn_cast_or_null<BinaryOperator>(V);
  if (Or == nullptr || Or->getOpcode() != Instruction::Or) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    auto *NonNegative = dyn_cast<ICmpInst>(Or->getOperand(I));
    auto *NotMin = dyn_cast<ICmpInst>(Or->getOperand(1 - I));
    if (NonNegative == nullptr ||
        NonNegative->getPredicate() != ICmpInst::ICMP_SGT ||
        !isAllOnes(NonNegative->getOperand(1)) || NotMin == nullptr ||
        NotMin->getPredicate() != ICmpInst::ICMP_NE ||
        !isShiftLeft255One(NotMin->getOperand(1))) {
      continue;
    }

    auto *CandidateMul =
        findBinaryOpInBlock(cast<Instruction>(V)->getParent(), Instruction::Mul,
                            NonNegative->getOperand(0), NotMin->getOperand(0));
    if (CandidateMul == nullptr) {
      CandidateMul = findBinaryOpInBlock(
          cast<Instruction>(V)->getParent(), Instruction::Mul,
          NotMin->getOperand(0), NonNegative->getOperand(0));
    }
    if (CandidateMul == nullptr) {
      continue;
    }
    Mul = CandidateMul;
    X = NonNegative->getOperand(0);
    Y = NotMin->getOperand(0);
    return true;
  }
  return false;
}

bool matchSignedMulDivisionCheck(Value *V, BinaryOperator *&Mul, Value *&X,
                                 Value *&Y) {
  auto *Or = dyn_cast_or_null<BinaryOperator>(V);
  if (Or == nullptr || Or->getOpcode() != Instruction::Or) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    auto *Zero = dyn_cast<ICmpInst>(Or->getOperand(I));
    auto *Eq = dyn_cast<ICmpInst>(Or->getOperand(1 - I));
    if (Zero == nullptr || Zero->getPredicate() != ICmpInst::ICMP_EQ ||
        !isZero(Zero->getOperand(1)) || Eq == nullptr ||
        Eq->getPredicate() != ICmpInst::ICMP_EQ) {
      continue;
    }
    auto *Div = dyn_cast<CallBase>(Eq->getOperand(1));
    if (Div == nullptr || !isCallTo(Div, "evm_sdiv") || Div->arg_size() != 2 ||
        !isSameValue(Div->getArgOperand(1), Zero->getOperand(0))) {
      continue;
    }
    auto *CandidateMul = dyn_cast<BinaryOperator>(Div->getArgOperand(0));
    if (CandidateMul == nullptr ||
        CandidateMul->getOpcode() != Instruction::Mul ||
        !binaryOpHasOperand(CandidateMul, Zero->getOperand(0)) ||
        !binaryOpHasOperand(CandidateMul, Eq->getOperand(0))) {
      continue;
    }
    Mul = CandidateMul;
    X = Zero->getOperand(0);
    Y = Eq->getOperand(0);
    return true;
  }
  return false;
}

bool matchSignedDivMinValueOverflow(Value *V, Value *&X, Value *&Y) {
  auto *Or = dyn_cast_or_null<BinaryOperator>(V);
  if (Or == nullptr || Or->getOpcode() != Instruction::Or) {
    return false;
  }

  for (unsigned I = 0; I < 2; ++I) {
    auto *NotMinusOne = dyn_cast<ICmpInst>(Or->getOperand(I));
    auto *NotMin = dyn_cast<ICmpInst>(Or->getOperand(1 - I));
    if (NotMinusOne == nullptr ||
        NotMinusOne->getPredicate() != ICmpInst::ICMP_NE ||
        !isAllOnes(NotMinusOne->getOperand(1)) || NotMin == nullptr ||
        NotMin->getPredicate() != ICmpInst::ICMP_NE ||
        !isShiftLeft255One(NotMin->getOperand(1))) {
      continue;
    }
    X = NotMin->getOperand(0);
    Y = NotMinusOne->getOperand(0);
    return true;
  }
  return false;
}

std::optional<CheckedBoundsMatch>
matchSignedCheckedArithmeticGuard(Value *BranchCondition,
                                  bool FailureWhenCondTrue,
                                  const SolidityRevertMatch &RevertMatch) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x11 ||
      FailureWhenCondTrue) {
    return std::nullopt;
  }

  if (BinaryOperator *Sub = matchSignedSubSuccessCondition(BranchCondition)) {
    return CheckedBoundsMatch{"checked_sub",
                              "",
                              nullptr,
                              nullptr,
                              nullptr,
                              RevertMatch.Revert,
                              {Sub->getOperand(0), Sub->getOperand(1), Sub},
                              RevertMatch.PanicCode,
                              true};
  }

  if (BinaryOperator *Add = matchSignedAddSuccessCondition(BranchCondition)) {
    return CheckedBoundsMatch{"checked_add",
                              "",
                              nullptr,
                              nullptr,
                              nullptr,
                              RevertMatch.Revert,
                              {Add->getOperand(0), Add->getOperand(1), Add},
                              RevertMatch.PanicCode,
                              true};
  }

  BinaryOperator *Mul = nullptr;
  Value *X = nullptr;
  Value *Y = nullptr;
  if (matchSignedMulMinValueSpecialCase(BranchCondition, Mul, X, Y) ||
      matchSignedMulDivisionCheck(BranchCondition, Mul, X, Y)) {
    return CheckedBoundsMatch{"checked_mul", "",
                              nullptr,       nullptr,
                              nullptr,       RevertMatch.Revert,
                              {X, Y, Mul},   RevertMatch.PanicCode,
                              true};
  }

  if (matchSignedDivMinValueOverflow(BranchCondition, X, Y)) {
    return CheckedBoundsMatch{"checked_div", "",
                              nullptr,       nullptr,
                              nullptr,       RevertMatch.Revert,
                              {X, Y},        RevertMatch.PanicCode,
                              true};
  }

  return std::nullopt;
}

std::optional<CheckedBoundsMatch>
matchCheckedArithmetic(const NormalizedCondition &FailureCond,
                       const SolidityRevertMatch &RevertMatch,
                       BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value()) {
    return std::nullopt;
  }

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *LHS = Cmp->getOperand(0);
  Value *RHS = Cmp->getOperand(1);
  ICmpInst::Predicate Pred = FailureCond.Predicate;

  if (*RevertMatch.PanicCode == 0x11) {
    BinaryOperator *BoundedOp = nullptr;
    ConstantInt *MaxValue = nullptr;
    Value *NegatedInput = nullptr;
    if (matchSignedNegationGuard(FailureCond, SuccessBlock, BoundedOp,
                                 NegatedInput)) {
      auto *Zero = ConstantInt::get(BoundedOp->getType(), 0);
      return CheckedBoundsMatch{"checked_sub",
                                "",
                                nullptr,
                                nullptr,
                                nullptr,
                                RevertMatch.Revert,
                                {Zero, NegatedInput, BoundedOp},
                                RevertMatch.PanicCode,
                                true};
    }

    Value *DynamicMax = nullptr;
    if (matchSignedNarrowSubRangeGuard(FailureCond, BoundedOp, DynamicMax)) {
      return CheckedBoundsMatch{"checked_sub_bound",
                                "",
                                nullptr,
                                nullptr,
                                nullptr,
                                RevertMatch.Revert,
                                {BoundedOp->getOperand(0),
                                 BoundedOp->getOperand(1), BoundedOp,
                                 DynamicMax},
                                RevertMatch.PanicCode,
                                true};
    }

    if (matchSmallUnsignedBoundedResult(FailureCond, BoundedOp, MaxValue)) {
      StringRef Kind = "";
      if (BoundedOp->getOpcode() == Instruction::Add) {
        Kind = "checked_add_bound";
      } else if (BoundedOp->getOpcode() == Instruction::Sub) {
        Kind = "checked_sub_bound";
      } else {
        Kind = "checked_mul_bound";
      }
      return CheckedBoundsMatch{Kind,
                                "",
                                nullptr,
                                nullptr,
                                nullptr,
                                RevertMatch.Revert,
                                {BoundedOp->getOperand(0),
                                 BoundedOp->getOperand(1), BoundedOp, MaxValue},
                                RevertMatch.PanicCode,
                                true};
    }

    if (matchUnsignedBoundedResult(FailureCond, Instruction::Add, BoundedOp,
                                   DynamicMax)) {
      return CheckedBoundsMatch{"checked_add_bound",
                                "",
                                nullptr,
                                nullptr,
                                nullptr,
                                RevertMatch.Revert,
                                {BoundedOp->getOperand(0),
                                 BoundedOp->getOperand(1), BoundedOp,
                                 DynamicMax},
                                RevertMatch.PanicCode,
                                true};
    }
    if (matchUnsignedBoundedResult(FailureCond, Instruction::Sub, BoundedOp,
                                   DynamicMax)) {
      return CheckedBoundsMatch{"checked_sub_bound",
                                "",
                                nullptr,
                                nullptr,
                                nullptr,
                                RevertMatch.Revert,
                                {BoundedOp->getOperand(0),
                                 BoundedOp->getOperand(1), BoundedOp,
                                 DynamicMax},
                                RevertMatch.PanicCode,
                                true};
    }
    Value *BoundedLHS = nullptr;
    Value *BoundedRHS = nullptr;
    if (matchUnsignedAddWithSubtractedMax(FailureCond, SuccessBlock, BoundedOp,
                                          BoundedLHS, BoundedRHS, DynamicMax)) {
      return CheckedBoundsMatch{"checked_add_bound",
                                "",
                                nullptr,
                                nullptr,
                                nullptr,
                                RevertMatch.Revert,
                                {BoundedLHS, BoundedRHS, BoundedOp, DynamicMax},
                                RevertMatch.PanicCode,
                                true};
    }
    if (matchUnsignedBoundedMulCleanup(FailureCond, BoundedOp, DynamicMax)) {
      return CheckedBoundsMatch{"checked_mul_bound",
                                "",
                                nullptr,
                                nullptr,
                                nullptr,
                                RevertMatch.Revert,
                                {BoundedOp->getOperand(0),
                                 BoundedOp->getOperand(1), BoundedOp,
                                 DynamicMax},
                                RevertMatch.PanicCode,
                                true};
    }
    Value *BoundedInput = nullptr;
    if (matchCleanedUnsignedIncrement(FailureCond, SuccessBlock, BoundedOp,
                                      BoundedInput, DynamicMax)) {
      auto *One = ConstantInt::get(BoundedOp->getType(), 1);
      return CheckedBoundsMatch{"checked_add_bound",
                                "",
                                nullptr,
                                nullptr,
                                nullptr,
                                RevertMatch.Revert,
                                {BoundedInput, One, BoundedOp, DynamicMax},
                                RevertMatch.PanicCode,
                                true};
    }
    if (matchCleanedUnsignedDecrement(FailureCond, SuccessBlock, BoundedOp,
                                      BoundedInput, DynamicMax)) {
      auto *One = ConstantInt::get(BoundedOp->getType(), 1);
      return CheckedBoundsMatch{"checked_sub_bound",
                                "",
                                nullptr,
                                nullptr,
                                nullptr,
                                RevertMatch.Revert,
                                {BoundedInput, One, BoundedOp, DynamicMax},
                                RevertMatch.PanicCode,
                                true};
    }

    if (Pred == ICmpInst::ICMP_UGT || Pred == ICmpInst::ICMP_ULT) {
      Value *MaybeOriginal = Pred == ICmpInst::ICMP_UGT ? LHS : RHS;
      Value *MaybeResult = Pred == ICmpInst::ICMP_UGT ? RHS : LHS;
      auto *Op = dyn_cast<BinaryOperator>(MaybeResult);
      if (Op != nullptr && Op->getOpcode() == Instruction::Add &&
          binaryOpHasOperand(Op, MaybeOriginal)) {
        return CheckedBoundsMatch{"checked_add",
                                  "",
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  RevertMatch.Revert,
                                  {Op->getOperand(0), Op->getOperand(1), Op},
                                  RevertMatch.PanicCode,
                                  true};
      }
      if (Op != nullptr && Op->getOpcode() == Instruction::Sub &&
          binaryOpHasOperand(Op, MaybeOriginal)) {
        return CheckedBoundsMatch{"checked_sub",
                                  "",
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  RevertMatch.Revert,
                                  {Op->getOperand(0), Op->getOperand(1), Op},
                                  RevertMatch.PanicCode,
                                  true};
      }
    }

    if (Pred == ICmpInst::ICMP_EQ) {
      Value *MaybeInput = nullptr;
      if (isAllOnes(LHS)) {
        MaybeInput = RHS;
      } else if (isAllOnes(RHS)) {
        MaybeInput = LHS;
      }
      if (MaybeInput != nullptr) {
        auto *One = ConstantInt::get(MaybeInput->getType(), 1);
        BinaryOperator *Add =
            findCheckedStepResult(SuccessBlock, Cmp, MaybeInput, One);
        if (Add != nullptr) {
          return CheckedBoundsMatch{
              "checked_add",
              "",
              nullptr,
              nullptr,
              nullptr,
              RevertMatch.Revert,
              {Add->getOperand(0), Add->getOperand(1), Add},
              RevertMatch.PanicCode,
              true};
        }
      }

      MaybeInput = nullptr;
      if (isZero(LHS)) {
        MaybeInput = RHS;
      } else if (isZero(RHS)) {
        MaybeInput = LHS;
      }
      if (MaybeInput != nullptr) {
        auto *MinusOne = ConstantInt::getAllOnesValue(MaybeInput->getType());
        BinaryOperator *Add =
            findCheckedStepResult(SuccessBlock, Cmp, MaybeInput, MinusOne);
        if (Add != nullptr) {
          auto *One = ConstantInt::get(MaybeInput->getType(), 1);
          return CheckedBoundsMatch{"checked_sub",
                                    "",
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    RevertMatch.Revert,
                                    {MaybeInput, One, Add},
                                    RevertMatch.PanicCode,
                                    true};
        }
      }
    }

    if (Pred == ICmpInst::ICMP_UGT) {
      if (BinaryOperator *Mul =
              matchMulByMaxDivBound(Cmp, Pred, SuccessBlock)) {
        return CheckedBoundsMatch{"checked_mul",
                                  "",
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  RevertMatch.Revert,
                                  {Mul->getOperand(0), Mul->getOperand(1), Mul},
                                  RevertMatch.PanicCode,
                                  true};
      }

      Value *AddLHS = LHS;
      Value *AddRHS = matchBitwiseNot(RHS);
      if (AddRHS != nullptr) {
        BinaryOperator *Add =
            findBinaryOpInBlock(SuccessBlock, Instruction::Add, AddLHS, AddRHS);
        if (Add == nullptr) {
          Add = findBinaryOpInBlock(SuccessBlock, Instruction::Add, AddRHS,
                                    AddLHS);
        }
        if (Add != nullptr) {
          return CheckedBoundsMatch{
              "checked_add",
              "",
              nullptr,
              nullptr,
              nullptr,
              RevertMatch.Revert,
              {Add->getOperand(0), Add->getOperand(1), Add},
              RevertMatch.PanicCode,
              true};
        }
      }
    }

    if (Pred == ICmpInst::ICMP_ULT) {
      BinaryOperator *Sub =
          findBinaryOpInBlock(SuccessBlock, Instruction::Sub, LHS, RHS);
      if (Sub != nullptr) {
        return CheckedBoundsMatch{"checked_sub",
                                  "",
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  RevertMatch.Revert,
                                  {Sub->getOperand(0), Sub->getOperand(1), Sub},
                                  RevertMatch.PanicCode,
                                  true};
      }
    }
  }

  if (*RevertMatch.PanicCode != 0x12) {
    return std::nullopt;
  }

  Value *Divisor = nullptr;
  if ((Pred == ICmpInst::ICMP_EQ || Pred == ICmpInst::ICMP_ULE ||
       Pred == ICmpInst::ICMP_SLE) &&
      isZero(RHS)) {
    Divisor = LHS;
  } else if ((Pred == ICmpInst::ICMP_EQ || Pred == ICmpInst::ICMP_UGE ||
              Pred == ICmpInst::ICMP_SGE) &&
             isZero(LHS)) {
    Divisor = RHS;
  }
  if (Divisor == nullptr) {
    return std::nullopt;
  }

  for (User *U : Divisor->users()) {
    auto *Call = dyn_cast<CallBase>(U);
    if (Call == nullptr || Call->arg_size() != 2 ||
        !isSameValue(Call->getArgOperand(1), Divisor)) {
      continue;
    }
    StringRef Callee = getCalleeName(Call);
    if (Callee == "evm_div" || Callee == "evm_sdiv") {
      return CheckedBoundsMatch{
          "checked_div",
          "",
          nullptr,
          nullptr,
          nullptr,
          RevertMatch.Revert,
          {Call->getArgOperand(1)},
          RevertMatch.PanicCode,
          true};
    }
    if (Callee == "evm_mod" || Callee == "evm_smod") {
      return CheckedBoundsMatch{
          "checked_mod",
          "",
          nullptr,
          nullptr,
          nullptr,
          RevertMatch.Revert,
          {Call->getArgOperand(1)},
          RevertMatch.PanicCode,
          true};
    }
  }

  return std::nullopt;
}

bool isCalldataArrayIndexScale(Value *V, Value *Index) {
  auto *Call = dyn_cast_or_null<CallBase>(V);
  if (Call != nullptr && isCallTo(Call, "evm_shl") && Call->arg_size() == 2) {
    return isConstantIntValue(Call->getArgOperand(0), 5) &&
           isSameValue(Call->getArgOperand(1), Index);
  }

  auto *Op = dyn_cast_or_null<BinaryOperator>(V);
  if (Op == nullptr) {
    return false;
  }
  if (Op->getOpcode() == Instruction::Shl) {
    return isSameValue(Op->getOperand(0), Index) &&
           isConstantIntValue(Op->getOperand(1), 5);
  }
  if (Op->getOpcode() == Instruction::Mul) {
    return (isSameValue(Op->getOperand(0), Index) &&
            isConstantIntValue(Op->getOperand(1), 32)) ||
           (isSameValue(Op->getOperand(1), Index) &&
            isConstantIntValue(Op->getOperand(0), 32));
  }
  return false;
}

bool isCalldataArrayElementOffset(Value *V, Value *Index) {
  if (isCalldataArrayIndexScale(V, Index)) {
    return true;
  }

  auto *Add = dyn_cast_or_null<BinaryOperator>(V);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return false;
  }
  return isCalldataArrayIndexScale(Add->getOperand(0), Index) ||
         isCalldataArrayIndexScale(Add->getOperand(1), Index);
}

bool isCalldataPointer(Value *Ptr) {
  Ptr = Ptr == nullptr ? nullptr : Ptr->stripPointerCasts();
  if (auto *Arg = dyn_cast_or_null<Argument>(Ptr)) {
    return Arg->getName() == "calldata";
  }

  auto *Call = dyn_cast_or_null<CallBase>(Ptr);
  if (Call != nullptr && isCallTo(Call, "notdec_evm_calldata_min_size") &&
      Call->arg_size() >= 1) {
    return isCalldataPointer(Call->getArgOperand(0));
  }

  auto *GEP = dyn_cast_or_null<GEPOperator>(Ptr);
  return GEP != nullptr && isCalldataPointer(GEP->getPointerOperand());
}

bool isCalldataAddress(Value *V) {
  if (V == nullptr) {
    return false;
  }
  V = V->stripPointerCasts();
  if (isCalldataPointer(V)) {
    return true;
  }

  if (auto *PTI = dyn_cast<PtrToIntInst>(V)) {
    return isCalldataPointer(PTI->getOperand(0));
  }

  if (auto *BO = dyn_cast<BinaryOperator>(V)) {
    if (BO->getOpcode() != Instruction::Add) {
      return false;
    }
    return isCalldataAddress(BO->getOperand(0)) ||
           isCalldataAddress(BO->getOperand(1));
  }

  if (auto *ITP = dyn_cast<IntToPtrInst>(V)) {
    return isCalldataAddress(ITP->getOperand(0));
  }

  return false;
}

bool isCalldataLoad(Value *V) {
  auto *Load = dyn_cast_or_null<LoadInst>(V);
  return Load != nullptr && Load->getType()->isIntegerTy(256) &&
         isCalldataAddress(Load->getPointerOperand());
}

bool calldataAddressUsesArrayElementOffset(Value *V, Value *Index) {
  if (V == nullptr) {
    return false;
  }
  V = V->stripPointerCasts();

  if (isCalldataArrayElementOffset(V, Index)) {
    return true;
  }

  if (auto *ITP = dyn_cast<IntToPtrInst>(V)) {
    return calldataAddressUsesArrayElementOffset(ITP->getOperand(0), Index);
  }

  if (auto *BO = dyn_cast<BinaryOperator>(V)) {
    if (BO->getOpcode() != Instruction::Add) {
      return false;
    }
    return calldataAddressUsesArrayElementOffset(BO->getOperand(0), Index) ||
           calldataAddressUsesArrayElementOffset(BO->getOperand(1), Index);
  }

  auto *GEP = dyn_cast<GEPOperator>(V);
  if (GEP == nullptr) {
    return false;
  }
  for (Value *Offset : GEP->indices()) {
    if (isCalldataArrayElementOffset(Offset, Index)) {
      return true;
    }
  }
  return calldataAddressUsesArrayElementOffset(GEP->getPointerOperand(), Index);
}

bool hasCalldataArrayElementLoad(BasicBlock *SuccessBlock, Value *Index) {
  if (SuccessBlock == nullptr || Index == nullptr) {
    return false;
  }

  for (Instruction &I : *SuccessBlock) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call != nullptr && isCallTo(Call, "evm_calldataload") &&
        Call->arg_size() == 2 &&
        isCalldataArrayElementOffset(Call->getArgOperand(1), Index)) {
      return true;
    }
    auto *Load = dyn_cast<LoadInst>(&I);
    if (Load != nullptr && isCalldataAddress(Load->getPointerOperand()) &&
        calldataAddressUsesArrayElementOffset(Load->getPointerOperand(),
                                             Index)) {
      return true;
    }
  }
  return false;
}

bool isMemoryArrayIndexScale(Value *V, Value *Index) {
  auto *Op = dyn_cast_or_null<BinaryOperator>(V);
  if (Op == nullptr) {
    return false;
  }
  if (Op->getOpcode() == Instruction::Shl) {
    return isSameValue(Op->getOperand(0), Index) &&
           isConstantIntValue(Op->getOperand(1), 5);
  }
  if (Op->getOpcode() != Instruction::Mul) {
    return false;
  }

  ConstantInt *Scale = nullptr;
  if (isSameValue(Op->getOperand(0), Index)) {
    Scale = dyn_cast<ConstantInt>(Op->getOperand(1));
  } else if (isSameValue(Op->getOperand(1), Index)) {
    Scale = dyn_cast<ConstantInt>(Op->getOperand(0));
  }
  if (Scale == nullptr) {
    return false;
  }

  const APInt &ScaleValue = Scale->getValue();
  return ScaleValue.uge(32) && ScaleValue.urem(32) == 0;
}

bool valueUsesMemoryArrayIndexScale(Value *V, Value *Scale, unsigned Depth,
                                    SmallPtrSetImpl<Value *> &Seen) {
  if (V == nullptr || Scale == nullptr || Depth == 0 ||
      !Seen.insert(V).second) {
    return false;
  }
  if (isSameValue(V, Scale)) {
    return true;
  }

  auto *Add = dyn_cast_or_null<BinaryOperator>(V);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return false;
  }
  return valueUsesMemoryArrayIndexScale(Add->getOperand(0), Scale, Depth - 1,
                                        Seen) ||
         valueUsesMemoryArrayIndexScale(Add->getOperand(1), Scale, Depth - 1,
                                        Seen);
}

bool valueUsesMemoryArrayIndexScale(Value *V, Value *Scale) {
  SmallPtrSet<Value *, 8> Seen;
  return valueUsesMemoryArrayIndexScale(V, Scale, 4, Seen);
}

bool hasMemoryArrayElementAccess(BasicBlock *SuccessBlock, Value *Index) {
  if (SuccessBlock == nullptr || Index == nullptr) {
    return false;
  }

  for (Instruction &I : *SuccessBlock) {
    if (!isMemoryArrayIndexScale(&I, Index)) {
      continue;
    }

    for (Instruction &UseI : *SuccessBlock) {
      auto *Call = dyn_cast<CallBase>(&UseI);
      if (Call == nullptr) {
        continue;
      }
      for (Value *Arg : Call->args()) {
        if (valueUsesMemoryArrayIndexScale(Arg, &I)) {
          return true;
        }
      }
    }
  }
  return false;
}

ConstantInt *getArrayLengthFromMaxIndex(Value *V) {
  auto *MaxIndex = dyn_cast_or_null<ConstantInt>(V);
  if (MaxIndex == nullptr || MaxIndex->getValue().isAllOnes()) {
    return nullptr;
  }
  return cast<ConstantInt>(
      ConstantInt::get(MaxIndex->getType(), MaxIndex->getValue() + 1));
}

bool isStorageArrayIndexScale(Value *V, Value *Index) {
  if (isSameValue(V, Index)) {
    return true;
  }

  auto *Op = dyn_cast_or_null<BinaryOperator>(V);
  if (Op == nullptr) {
    return false;
  }

  if (Op->getOpcode() == Instruction::Mul) {
    auto *RHS = dyn_cast<ConstantInt>(Op->getOperand(1));
    auto *LHS = dyn_cast<ConstantInt>(Op->getOperand(0));
    return (isSameValue(Op->getOperand(0), Index) && RHS != nullptr &&
            !RHS->isZero()) ||
           (isSameValue(Op->getOperand(1), Index) && LHS != nullptr &&
            !LHS->isZero());
  }

  if (Op->getOpcode() == Instruction::UDiv) {
    auto *Divisor = dyn_cast<ConstantInt>(Op->getOperand(1));
    return isSameValue(Op->getOperand(0), Index) && Divisor != nullptr &&
           !Divisor->isZero();
  }

  return false;
}

bool isFixedStorageArraySlotOffset(Value *V, Value *Index) {
  auto *Add = dyn_cast_or_null<BinaryOperator>(V);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return false;
  }

  // Fixed storage arrays use a constant data slot plus an index-derived offset.
  // Dynamic arrays are handled through their sload length guard above.
  return (isStorageArrayIndexScale(Add->getOperand(0), Index) &&
          isa<ConstantInt>(Add->getOperand(1))) ||
         (isStorageArrayIndexScale(Add->getOperand(1), Index) &&
          isa<ConstantInt>(Add->getOperand(0)));
}

bool hasFixedStorageArrayElementAccess(BasicBlock *SuccessBlock, Value *Index) {
  if (SuccessBlock == nullptr || Index == nullptr) {
    return false;
  }

  for (Instruction &I : *SuccessBlock) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }

    if (isCallTo(Call, "evm_sload") && Call->arg_size() == 1 &&
        isFixedStorageArraySlotOffset(Call->getArgOperand(0), Index)) {
      return true;
    }

    if (isCallTo(Call, "evm_sstore") && Call->arg_size() == 2 &&
        isFixedStorageArraySlotOffset(Call->getArgOperand(0), Index)) {
      return true;
    }
  }
  return false;
}

std::optional<CheckedBoundsMatch>
matchArrayBounds(const NormalizedCondition &FailureCond,
                 const SolidityRevertMatch &RevertMatch,
                 BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x32) {
    return std::nullopt;
  }

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *Index = nullptr;
  Value *Length = nullptr;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGE) {
    Index = Cmp->getOperand(0);
    Length = Cmp->getOperand(1);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE) {
    Index = Cmp->getOperand(1);
    Length = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_UGT) {
    Index = Cmp->getOperand(0);
    Length = getArrayLengthFromMaxIndex(Cmp->getOperand(1));
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULT) {
    Index = Cmp->getOperand(1);
    Length = getArrayLengthFromMaxIndex(Cmp->getOperand(0));
  } else if (FailureCond.Predicate == ICmpInst::ICMP_EQ &&
             (isZero(Cmp->getOperand(0)) || isZero(Cmp->getOperand(1)))) {
    Length =
        isZero(Cmp->getOperand(0)) ? Cmp->getOperand(1) : Cmp->getOperand(0);
    Index = ConstantInt::get(Length->getType(), 0);
  } else {
    return std::nullopt;
  }

  StringRef Kind = "array_bounds_unknown";
  if (isCalldataLoad(Length)) {
    Kind = "array_bounds_calldata";
  } else if (std::optional<EvmMemoryLoad> Load = matchEvmMemoryLoad(Length)) {
    Kind = isCalldataAddress(Load->Address) ? "array_bounds_calldata"
                                            : "array_bounds_memory";
  } else if (auto *LengthCall = dyn_cast<CallBase>(Length)) {
    if (isCallTo(LengthCall, "evm_calldataload")) {
      Kind = "array_bounds_calldata";
    } else if (isCallTo(LengthCall, "evm_sload")) {
      Kind = "array_bounds_storage";
    }
  }

  if (Kind == "array_bounds_unknown") {
    if (hasCalldataArrayElementLoad(SuccessBlock, Index)) {
      Kind = "array_bounds_calldata";
    } else if (hasMemoryArrayElementAccess(SuccessBlock, Index)) {
      Kind = "array_bounds_memory";
    } else if (hasFixedStorageArrayElementAccess(SuccessBlock, Index)) {
      Kind = "array_bounds_storage";
    } else {
      return std::nullopt;
    }
  }

  return CheckedBoundsMatch{Kind,
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {Index, Length},
                            RevertMatch.PanicCode,
                            true};
}

bool isSmallEnumMemberCount(Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr) {
    return false;
  }
  const APInt &Count = C->getValue();
  return !Count.isZero() && Count.ule(256);
}

ConstantInt *getEnumMemberCountFromMax(Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr || C->getValue().uge(255)) {
    return nullptr;
  }

  return cast<ConstantInt>(ConstantInt::get(C->getType(), C->getValue() + 1));
}

std::optional<CheckedBoundsMatch>
matchEnumConversion(const NormalizedCondition &FailureCond,
                    const SolidityRevertMatch &RevertMatch) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x21) {
    return std::nullopt;
  }

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *EnumValue = nullptr;
  Value *MemberCount = nullptr;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGE) {
    EnumValue = Cmp->getOperand(0);
    MemberCount = Cmp->getOperand(1);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE) {
    EnumValue = Cmp->getOperand(1);
    MemberCount = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_UGT) {
    EnumValue = Cmp->getOperand(0);
    MemberCount = getEnumMemberCountFromMax(Cmp->getOperand(1));
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULT) {
    EnumValue = Cmp->getOperand(1);
    MemberCount = getEnumMemberCountFromMax(Cmp->getOperand(0));
  } else {
    return std::nullopt;
  }

  if (!isSmallEnumMemberCount(MemberCount)) {
    return std::nullopt;
  }

  return CheckedBoundsMatch{"enum_conversion",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {EnumValue, MemberCount},
                            RevertMatch.PanicCode,
                            true};
}

bool blockComputesLengthMinusOne(BasicBlock *BB, Value *Length) {
  if (BB == nullptr || Length == nullptr) {
    return false;
  }

  for (Instruction &I : *BB) {
    auto *Add = dyn_cast<BinaryOperator>(&I);
    if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
      continue;
    }

    Value *Other = nullptr;
    if (isAllOnes(Add->getOperand(0))) {
      Other = Add->getOperand(1);
    } else if (isAllOnes(Add->getOperand(1))) {
      Other = Add->getOperand(0);
    } else {
      continue;
    }

    if (isSameValue(Other, Length)) {
      return true;
    }

    auto *BaseAdd = dyn_cast<BinaryOperator>(Other);
    if (BaseAdd != nullptr && BaseAdd->getOpcode() == Instruction::Add &&
        binaryOpHasOperand(BaseAdd, Length)) {
      return true;
    }
  }
  return false;
}

std::optional<CheckedBoundsMatch>
matchEmptyArrayPop(const NormalizedCondition &FailureCond,
                   const SolidityRevertMatch &RevertMatch,
                   BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x31) {
    return std::nullopt;
  }

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr || FailureCond.Predicate != ICmpInst::ICMP_EQ) {
    return std::nullopt;
  }

  Value *Length = nullptr;
  if (isZero(Cmp->getOperand(0))) {
    Length = Cmp->getOperand(1);
  } else if (isZero(Cmp->getOperand(1))) {
    Length = Cmp->getOperand(0);
  } else {
    return std::nullopt;
  }

  auto *LengthCall = dyn_cast_or_null<CallBase>(Length);
  if (LengthCall == nullptr || !isCallTo(LengthCall, "evm_sload") ||
      !blockComputesLengthMinusOne(SuccessBlock, Length)) {
    return std::nullopt;
  }

  return CheckedBoundsMatch{"empty_array_pop_storage",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {Length},
                            RevertMatch.PanicCode,
                            true};
}

Instruction *findFreeMemoryPointerStore(BasicBlock *BB, Value *NewPtr);
Instruction *findFinalizeAllocCall(BasicBlock *BB, Value *Base, Value *Size);
bool callHasArg(CallBase *Call, Value *Needle);
bool isSameAllocationHeaderPointer(Value *HeaderPtr, Value *Base);

bool hasMemoryArrayAllocationComputation(BasicBlock *SuccessBlock,
                                         Value *Length) {
  Value *Shift = findMemoryAllocationShift(SuccessBlock, Length);
  if (Shift == nullptr) {
    return false;
  }

  BinaryOperator *Size =
      findCommutativeBinaryOpInBlock(SuccessBlock, Instruction::Add, Shift,
                                     ConstantInt::get(Length->getType(), 32));
  if (Size == nullptr) {
    return false;
  }

  for (Instruction &I : *SuccessBlock) {
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (!Store.has_value() || !isSameValue(Store->StoredValue, Length)) {
      continue;
    }
    if (isFreeMemoryPointerLoad(Store->Address)) {
      return true;
    }
  }
  return false;
}

bool isMaskClearingLowFiveBits(Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr) {
    return false;
  }
  const APInt &Value = C->getValue();
  return Value.countTrailingZeros() >= 5;
}

bool isRoundedByteLength(Value *V, Value *Length) {
  auto *Rounded = dyn_cast_or_null<BinaryOperator>(V);
  if (Rounded == nullptr || Rounded->getOpcode() != Instruction::And) {
    return false;
  }

  Value *Base = nullptr;
  if (isMaskClearingLowFiveBits(Rounded->getOperand(0))) {
    Base = Rounded->getOperand(1);
  } else if (isMaskClearingLowFiveBits(Rounded->getOperand(1))) {
    Base = Rounded->getOperand(0);
  }
  auto *Add = dyn_cast_or_null<BinaryOperator>(Base);
  return Add != nullptr && Add->getOpcode() == Instruction::Add &&
         binaryOpHasOperand(Add, Length) &&
         (isConstantIntValue(Add->getOperand(0), 31) ||
          isConstantIntValue(Add->getOperand(1), 31));
}

bool isByteAllocationSize(Value *V, Value *Length) {
  auto *SizeAdd = dyn_cast_or_null<BinaryOperator>(V);
  if (SizeAdd == nullptr || SizeAdd->getOpcode() != Instruction::Add) {
    return false;
  }
  Value *Rounded = nullptr;
  if (isConstantIntValue(SizeAdd->getOperand(0), 32)) {
    Rounded = SizeAdd->getOperand(1);
  } else if (isConstantIntValue(SizeAdd->getOperand(1), 32)) {
    Rounded = SizeAdd->getOperand(0);
  }
  return Rounded != nullptr && isRoundedByteLength(Rounded, Length);
}

bool isPreIncrementByteAllocationSize(Value *V, Value *Length) {
  // roundUp(base + 1) + 32 is simplified to (base & -32) + 64.
  auto *LengthAdd = dyn_cast_or_null<BinaryOperator>(Length);
  if (LengthAdd == nullptr || LengthAdd->getOpcode() != Instruction::Add) {
    return false;
  }

  Value *Base = nullptr;
  if (isConstantIntValue(LengthAdd->getOperand(0), 1)) {
    Base = LengthAdd->getOperand(1);
  } else if (isConstantIntValue(LengthAdd->getOperand(1), 1)) {
    Base = LengthAdd->getOperand(0);
  }
  if (Base == nullptr) {
    return false;
  }

  auto *SizeAdd = dyn_cast_or_null<BinaryOperator>(V);
  if (SizeAdd == nullptr || SizeAdd->getOpcode() != Instruction::Add) {
    return false;
  }

  Value *RoundedBase = nullptr;
  if (isConstantIntValue(SizeAdd->getOperand(0), 64)) {
    RoundedBase = SizeAdd->getOperand(1);
  } else if (isConstantIntValue(SizeAdd->getOperand(1), 64)) {
    RoundedBase = SizeAdd->getOperand(0);
  }

  auto *Rounded = dyn_cast_or_null<BinaryOperator>(RoundedBase);
  return Rounded != nullptr && Rounded->getOpcode() == Instruction::And &&
         binaryOpHasOperand(Rounded, Base) &&
         (isMaskClearingLowFiveBits(Rounded->getOperand(0)) ||
          isMaskClearingLowFiveBits(Rounded->getOperand(1)));
}

bool isArrayAllocationSize(Value *V, Value *Length, BasicBlock *BB) {
  auto *SizeAdd = dyn_cast_or_null<BinaryOperator>(V);
  if (SizeAdd == nullptr || SizeAdd->getOpcode() != Instruction::Add) {
    return false;
  }
  Value *Shift = findMemoryAllocationShift(BB, Length);
  if (Shift != nullptr && binaryOpHasOperand(SizeAdd, Shift) &&
      (isConstantIntValue(SizeAdd->getOperand(0), 32) ||
       isConstantIntValue(SizeAdd->getOperand(1), 32))) {
    return true;
  }
  return (isConstantIntValue(SizeAdd->getOperand(0), 32) &&
          isMemoryAllocationShift(SizeAdd->getOperand(1), Length)) ||
         (isConstantIntValue(SizeAdd->getOperand(1), 32) &&
          isMemoryAllocationShift(SizeAdd->getOperand(0), Length));
}

bool isRoundedByteAllocationSize(Value *V) {
  auto *RoundedSize = dyn_cast_or_null<BinaryOperator>(V);
  if (RoundedSize == nullptr || RoundedSize->getOpcode() != Instruction::And) {
    return false;
  }

  Value *RoundedBase = nullptr;
  if (isMaskClearingLowFiveBits(RoundedSize->getOperand(0))) {
    RoundedBase = RoundedSize->getOperand(1);
  } else if (isMaskClearingLowFiveBits(RoundedSize->getOperand(1))) {
    RoundedBase = RoundedSize->getOperand(0);
  }

  auto *Add = dyn_cast_or_null<BinaryOperator>(RoundedBase);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return false;
  }

  Value *InnerRounded = nullptr;
  if (isConstantIntValue(Add->getOperand(0), 63)) {
    InnerRounded = Add->getOperand(1);
  } else if (isConstantIntValue(Add->getOperand(1), 63)) {
    InnerRounded = Add->getOperand(0);
  }

  auto *InnerAnd = dyn_cast_or_null<BinaryOperator>(InnerRounded);
  if (InnerAnd == nullptr || InnerAnd->getOpcode() != Instruction::And) {
    return false;
  }

  Value *InnerBase = nullptr;
  if (isMaskClearingLowFiveBits(InnerAnd->getOperand(0))) {
    InnerBase = InnerAnd->getOperand(1);
  } else if (isMaskClearingLowFiveBits(InnerAnd->getOperand(1))) {
    InnerBase = InnerAnd->getOperand(0);
  }

  auto *LengthAdd = dyn_cast_or_null<BinaryOperator>(InnerBase);
  return LengthAdd != nullptr && LengthAdd->getOpcode() == Instruction::Add &&
         (isConstantIntValue(LengthAdd->getOperand(0), 31) ||
          isConstantIntValue(LengthAdd->getOperand(1), 31));
}

bool isRoundedByteAllocationSizeForLength(Value *V, Value *Length) {
  auto *RoundedSize = dyn_cast_or_null<BinaryOperator>(V);
  if (RoundedSize == nullptr || RoundedSize->getOpcode() != Instruction::And) {
    return false;
  }

  Value *RoundedBase = nullptr;
  if (isMaskClearingLowFiveBits(RoundedSize->getOperand(0))) {
    RoundedBase = RoundedSize->getOperand(1);
  } else if (isMaskClearingLowFiveBits(RoundedSize->getOperand(1))) {
    RoundedBase = RoundedSize->getOperand(0);
  }

  auto *Add = dyn_cast_or_null<BinaryOperator>(RoundedBase);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return false;
  }

  Value *InnerRounded = nullptr;
  if (isConstantIntValue(Add->getOperand(0), 63)) {
    InnerRounded = Add->getOperand(1);
  } else if (isConstantIntValue(Add->getOperand(1), 63)) {
    InnerRounded = Add->getOperand(0);
  }

  auto *InnerAnd = dyn_cast_or_null<BinaryOperator>(InnerRounded);
  if (InnerAnd == nullptr || InnerAnd->getOpcode() != Instruction::And) {
    return false;
  }

  Value *InnerBase = nullptr;
  if (isMaskClearingLowFiveBits(InnerAnd->getOperand(0))) {
    InnerBase = InnerAnd->getOperand(1);
  } else if (isMaskClearingLowFiveBits(InnerAnd->getOperand(1))) {
    InnerBase = InnerAnd->getOperand(0);
  }

  auto *LengthAdd = dyn_cast_or_null<BinaryOperator>(InnerBase);
  return LengthAdd != nullptr && LengthAdd->getOpcode() == Instruction::Add &&
         binaryOpHasOperand(LengthAdd, Length) &&
         (isConstantIntValue(LengthAdd->getOperand(0), 31) ||
          isConstantIntValue(LengthAdd->getOperand(1), 31));
}

bool isDynamicAllocationSize(Value *V, Value *Length, BasicBlock *BB) {
  return isByteAllocationSize(V, Length) ||
         isPreIncrementByteAllocationSize(V, Length) ||
         isRoundedByteAllocationSizeForLength(V, Length) ||
         isArrayAllocationSize(V, Length, BB);
}

bool isPreIncrementFinalizeAllocationSize(Value *V, Value *Length) {
  // Some Solidity paths guard the pre-incremented byte length with
  // base < uint64max, then allocate roundUp(base + 1) + 32. InstCombine may
  // simplify that to (base & mask) + 64.
  auto *SizeAdd = dyn_cast_or_null<BinaryOperator>(V);
  if (SizeAdd == nullptr || SizeAdd->getOpcode() != Instruction::Add) {
    return false;
  }

  Value *RoundedBase = nullptr;
  if (isConstantIntValue(SizeAdd->getOperand(0), 64)) {
    RoundedBase = SizeAdd->getOperand(1);
  } else if (isConstantIntValue(SizeAdd->getOperand(1), 64)) {
    RoundedBase = SizeAdd->getOperand(0);
  }

  auto *Rounded = dyn_cast_or_null<BinaryOperator>(RoundedBase);
  return Rounded != nullptr && Rounded->getOpcode() == Instruction::And &&
         binaryOpHasOperand(Rounded, Length) &&
         (isMaskClearingLowFiveBits(Rounded->getOperand(0)) ||
          isMaskClearingLowFiveBits(Rounded->getOperand(1)));
}

bool isRoundedMemoryAllocationSize(Value *V) {
  auto *RoundedSize = dyn_cast_or_null<BinaryOperator>(V);
  if (RoundedSize == nullptr || RoundedSize->getOpcode() != Instruction::And) {
    return false;
  }

  Value *RoundedBase = nullptr;
  if (isMaskClearingLowFiveBits(RoundedSize->getOperand(0))) {
    RoundedBase = RoundedSize->getOperand(1);
  } else if (isMaskClearingLowFiveBits(RoundedSize->getOperand(1))) {
    RoundedBase = RoundedSize->getOperand(0);
  }

  auto *Add = dyn_cast_or_null<BinaryOperator>(RoundedBase);
  return Add != nullptr && Add->getOpcode() == Instruction::Add &&
         (isConstantIntValue(Add->getOperand(0), 31) ||
          isConstantIntValue(Add->getOperand(1), 31));
}

bool isRoundedMemoryAllocationSizeFromBase(Value *RoundedSize,
                                           Value *RoundedBase) {
  auto *And = dyn_cast_or_null<BinaryOperator>(RoundedSize);
  if (And == nullptr || And->getOpcode() != Instruction::And) {
    return false;
  }
  if (isMaskClearingLowFiveBits(And->getOperand(0))) {
    return isSameValue(And->getOperand(1), RoundedBase);
  }
  if (isMaskClearingLowFiveBits(And->getOperand(1))) {
    return isSameValue(And->getOperand(0), RoundedBase);
  }
  return false;
}

bool hasPowerOfTwoExpComputation(BasicBlock *SuccessBlock, Value *Exponent) {
  if (SuccessBlock == nullptr || Exponent == nullptr) {
    return false;
  }

  for (Instruction &I : *SuccessBlock) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || Call->arg_size() != 2) {
      continue;
    }
    if (isCallTo(Call, "evm_shl") &&
        isSameValue(Call->getArgOperand(0), Exponent) &&
        isConstantIntValue(Call->getArgOperand(1), 1)) {
      return true;
    }
    if (isCallTo(Call, "evm_exp") &&
        isConstantIntValue(Call->getArgOperand(0), 2) &&
        isSameValue(Call->getArgOperand(1), Exponent)) {
      return true;
    }
  }
  return false;
}

bool hasBytesAllocationStores(BasicBlock *BB, Value *OldPtr, Value *NewPtr,
                              Value *Size, Value *Length) {
  if (BB == nullptr) {
    return false;
  }

  bool StoresLength = false;
  bool StoresFreePtr = false;
  bool FinalizesAlloc = findFinalizeAllocCall(BB, OldPtr, Size) != nullptr;
  for (Instruction &I : *BB) {
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (!Store.has_value()) {
      continue;
    }
    if (isSameValue(Store->Address, OldPtr) &&
        isSameValue(Store->StoredValue, Length)) {
      StoresLength = true;
    }
    if (isConstantIntValue(Store->Address, 64) &&
        isSameValue(Store->StoredValue, NewPtr)) {
      StoresFreePtr = true;
    }
  }
  return StoresLength && (StoresFreePtr || FinalizesAlloc);
}

bool hasBytesAllocationStoresOnLocalPath(BasicBlock *SuccessBlock,
                                         Value *OldPtr, Value *NewPtr, Value *Size,
                                         Value *Length) {
  if (hasBytesAllocationStores(SuccessBlock, OldPtr, NewPtr, Size, Length)) {
    return true;
  }

  // Solidity may split the length guard and the header stores with a pointer
  // overflow guard. Keep the search local so unrelated later stores do not
  // justify rewriting the length check.
  auto *Term =
      SuccessBlock == nullptr ? nullptr : SuccessBlock->getTerminator();
  if (Term == nullptr) {
    return false;
  }
  for (BasicBlock *Succ : successors(Term)) {
    if (hasBytesAllocationStores(Succ, OldPtr, NewPtr, Size, Length)) {
      return true;
    }
  }
  return false;
}

bool hasAllocationHelperHeaderStore(BasicBlock *SuccessBlock, Value *Ptr,
                                    Value *Length) {
  if (SuccessBlock == nullptr || Ptr == nullptr) {
    return false;
  }
  for (Instruction &I : *SuccessBlock) {
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (!Store.has_value()) {
      continue;
    }
    if (isSameValue(Store->Address, Ptr) &&
        isSameValue(Store->StoredValue, Length)) {
      return true;
    }
  }
  return false;
}

bool hasMemoryAllocationHelperCall(BasicBlock *SuccessBlock, Value *Length) {
  if (SuccessBlock == nullptr || Length == nullptr) {
    return false;
  }

  for (Instruction &I : *SuccessBlock) {
    auto *Call = dyn_cast<CallBase>(&I);
    // Solidity emits private allocation helpers for ABI decoder/allocation
    // paths. Only accept them when the returned pointer is immediately used as
    // a dynamic array header; a private helper call alone is too broad.
    if (!isPrivateHelperCall(Call) || !Call->getType()->isIntegerTy(256) ||
        !hasAllocationHelperHeaderStore(SuccessBlock, Call, Length)) {
      continue;
    }
    for (Value *Arg : Call->args()) {
      if (isDynamicAllocationSize(Arg, Length, SuccessBlock)) {
        return true;
      }
    }
  }
  return false;
}

bool hasVoidMemoryAllocationHelperCall(BasicBlock *SuccessBlock,
                                       Value *Length) {
  if (SuccessBlock == nullptr || Length == nullptr) {
    return false;
  }

  SmallVector<Value *, 4> HeaderPtrs;
  for (Instruction &I : *SuccessBlock) {
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (Store.has_value() && isSameValue(Store->StoredValue, Length)) {
      HeaderPtrs.push_back(Store->Address);
    }
  }

  for (Instruction &I : *SuccessBlock) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (!isPrivateHelperCall(Call) || !Call->getType()->isVoidTy()) {
      continue;
    }

    bool HasSize = false;
    for (Value *Arg : Call->args()) {
      HasSize |= isDynamicAllocationSize(Arg, Length, SuccessBlock);
    }
    if (!HasSize) {
      continue;
    }

    for (Value *HeaderPtr : HeaderPtrs) {
      for (Value *Arg : Call->args()) {
        if (isSameAllocationHeaderPointer(HeaderPtr, Arg)) {
          return true;
        }
      }
    }
  }
  return false;
}

bool hasMemoryAllocationSizeReturn(BasicBlock *SuccessBlock, Value *Length) {
  if (SuccessBlock == nullptr || Length == nullptr) {
    return false;
  }
  // arrayAllocationSizeFunction returns only the checked allocation size. There
  // is no local mstore/free-pointer update to use as proof in these helpers.
  auto *Ret = dyn_cast_or_null<ReturnInst>(SuccessBlock->getTerminator());
  if (Ret == nullptr || Ret->getNumOperands() != 1) {
    return false;
  }
  return isDynamicAllocationSize(Ret->getReturnValue(), Length, SuccessBlock);
}

bool isSameAllocationHeaderPointer(Value *HeaderPtr, Value *Base) {
  if (isSameValue(HeaderPtr, Base)) {
    return true;
  }

  auto *PtrToInt = dyn_cast_or_null<PtrToIntInst>(Base);
  if (PtrToInt != nullptr &&
      isSameValue(HeaderPtr, PtrToInt->getOperand(0))) {
    return true;
  }

  auto *ConstExpr = dyn_cast_or_null<ConstantExpr>(Base);
  return ConstExpr != nullptr &&
         ConstExpr->getOpcode() == Instruction::PtrToInt &&
         isSameValue(HeaderPtr, ConstExpr->getOperand(0));
}

bool hasSolidityMemoryAllocationMarker(BasicBlock *SuccessBlock,
                                       Value *Length) {
  if (SuccessBlock == nullptr || Length == nullptr) {
    return false;
  }

  SmallVector<Value *, 4> HeaderPtrs;
  for (Instruction &I : *SuccessBlock) {
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (Store.has_value() && isSameValue(Store->StoredValue, Length)) {
      HeaderPtrs.push_back(Store->Address);
    }
  }

  for (Instruction &I : *SuccessBlock) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr ||
        !isCallTo(Call, "notdec_solidity_memory_allocation") ||
        Call->arg_size() != 2) {
      continue;
    }

    Value *Base = Call->getArgOperand(0);
    Value *Size = Call->getArgOperand(1);
    // The allocation marker already binds this size to the local calloc base.
    // In optimized IR, the size expression may have been computed in an
    // earlier block and reused by several guarded allocations.
    if (!isFreeMemoryAllocationBase(Base) ||
        !isDynamicAllocationSize(Size, Length, nullptr)) {
      continue;
    }

    for (Value *HeaderPtr : HeaderPtrs) {
      if (isSameAllocationHeaderPointer(HeaderPtr, Base)) {
        return true;
      }
    }
  }
  return false;
}

std::optional<CheckedBoundsMatch>
matchPowerOfTwoExpGuard(const NormalizedCondition &FailureCond,
                        const SolidityRevertMatch &RevertMatch,
                        BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x11) {
    return std::nullopt;
  }

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *Exponent = nullptr;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGE &&
      isConstantIntValue(Cmp->getOperand(1), 256)) {
    Exponent = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE &&
             isConstantIntValue(Cmp->getOperand(0), 256)) {
    Exponent = Cmp->getOperand(1);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_UGT &&
             isConstantIntValue(Cmp->getOperand(1), 255)) {
    Exponent = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULT &&
             isConstantIntValue(Cmp->getOperand(0), 255)) {
    Exponent = Cmp->getOperand(1);
  }

  if (Exponent == nullptr ||
      !hasPowerOfTwoExpComputation(SuccessBlock, Exponent)) {
    return std::nullopt;
  }

  Value *Base = ConstantInt::get(Exponent->getType(), 2);
  return CheckedBoundsMatch{"checked_exp",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {Base, Exponent},
                            RevertMatch.PanicCode,
                            true};
}

bool hasMemoryBytesAllocationComputation(BasicBlock *SuccessBlock,
                                         Value *Length) {
  if (SuccessBlock == nullptr) {
    return false;
  }

  SmallVector<Value *, 4> OldPtrs;
  for (Instruction &I : *SuccessBlock) {
    if (isFreeMemoryPointerLoad(&I) || isFreeMemoryAllocationBase(&I)) {
      OldPtrs.push_back(&I);
    }
  }

  for (Value *OldPtr : OldPtrs) {
    for (Instruction &I : *SuccessBlock) {
      auto *NewPtr = dyn_cast<BinaryOperator>(&I);
      if (NewPtr == nullptr || NewPtr->getOpcode() != Instruction::Add ||
          !binaryOpHasOperand(NewPtr, OldPtr)) {
        continue;
      }
      Value *Size = isSameValue(NewPtr->getOperand(0), OldPtr)
                        ? NewPtr->getOperand(1)
                        : NewPtr->getOperand(0);
      if (!isDynamicAllocationSize(Size, Length, SuccessBlock)) {
        continue;
      }
      if (hasBytesAllocationStoresOnLocalPath(SuccessBlock, OldPtr, NewPtr,
                                              Size, Length)) {
        return true;
      }
    }
  }
  return false;
}

bool blockFinalizesDynamicAllocationSize(BasicBlock *BB, Value *Length,
                                         BasicBlock *SizeBlock) {
  if (BB == nullptr || Length == nullptr) {
    return false;
  }

  for (Instruction &I : *BB) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !isCallTo(Call, "notdec_evm_finalize_alloc") ||
        Call->arg_size() != 2) {
      continue;
    }
    Value *Size = Call->getArgOperand(1);
    if (isDynamicAllocationSize(Size, Length, SizeBlock) ||
        isPreIncrementFinalizeAllocationSize(Size, Length)) {
      return true;
    }
  }
  return false;
}

bool hasDynamicFinalizeAllocOnLocalPath(BasicBlock *SuccessBlock,
                                        Value *Length) {
  if (blockFinalizesDynamicAllocationSize(SuccessBlock, Length, SuccessBlock)) {
    return true;
  }

  auto *Term =
      SuccessBlock == nullptr ? nullptr : SuccessBlock->getTerminator();
  if (Term == nullptr) {
    return false;
  }
  for (BasicBlock *Succ : successors(Term)) {
    if (blockFinalizesDynamicAllocationSize(Succ, Length, SuccessBlock)) {
      return true;
    }
  }
  return false;
}

bool hasMemoryAllocationSizeComputation(BasicBlock *SuccessBlock,
                                        Value *Length) {
  Value *Shift = findMemoryAllocationShift(SuccessBlock, Length);
  if (Shift == nullptr) {
    return hasMemoryBytesAllocationComputation(SuccessBlock, Length) ||
           hasDynamicFinalizeAllocOnLocalPath(SuccessBlock, Length) ||
           hasSolidityMemoryAllocationMarker(SuccessBlock, Length) ||
           hasMemoryAllocationHelperCall(SuccessBlock, Length) ||
           hasVoidMemoryAllocationHelperCall(SuccessBlock, Length) ||
           hasMemoryAllocationSizeReturn(SuccessBlock, Length);
  }

  if (auto *RoundedBase = findCommutativeBinaryOpInBlock(
          SuccessBlock, Instruction::Add, Shift,
          ConstantInt::get(Length->getType(), 63))) {
    if (findCommutativeBinaryOpInBlock(
            SuccessBlock, Instruction::And, RoundedBase,
            ConstantInt::get(Length->getType(), -32, true)) != nullptr) {
      return true;
    }
  }

  return hasMemoryArrayAllocationComputation(SuccessBlock, Length) ||
         hasMemoryBytesAllocationComputation(SuccessBlock, Length) ||
         hasDynamicFinalizeAllocOnLocalPath(SuccessBlock, Length) ||
         hasSolidityMemoryAllocationMarker(SuccessBlock, Length) ||
         hasMemoryAllocationHelperCall(SuccessBlock, Length) ||
         hasVoidMemoryAllocationHelperCall(SuccessBlock, Length) ||
         hasMemoryAllocationSizeReturn(SuccessBlock, Length);
}

bool callHasArg(CallBase *Call, Value *Needle) {
  if (Call == nullptr || Needle == nullptr) {
    return false;
  }
  for (Value *Arg : Call->args()) {
    if (isSameValue(Arg, Needle)) {
      return true;
    }
  }
  return false;
}

bool hasLengthGreaterThan31Branch(BasicBlock *BB, Value *Length) {
  if (BB == nullptr || Length == nullptr) {
    return false;
  }
  auto *Br = dyn_cast_or_null<BranchInst>(BB->getTerminator());
  if (Br == nullptr || !Br->isConditional()) {
    return false;
  }
  auto *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
  if (Cmp == nullptr) {
    return false;
  }
  return (Cmp->getPredicate() == ICmpInst::ICMP_UGT &&
          isSameValue(Cmp->getOperand(0), Length) &&
          isConstantIntValue(Cmp->getOperand(1), 31)) ||
         (Cmp->getPredicate() == ICmpInst::ICMP_ULT &&
          isConstantIntValue(Cmp->getOperand(0), 31) &&
          isSameValue(Cmp->getOperand(1), Length));
}

bool hasStorageByteArrayCopyToStorageSuccess(BasicBlock *SuccessBlock,
                                             Value *Length) {
  if (SuccessBlock == nullptr || Length == nullptr ||
      !hasLengthGreaterThan31Branch(SuccessBlock, Length)) {
    return false;
  }

  for (Instruction &I : *SuccessBlock) {
    auto *SLoad = dyn_cast<CallBase>(&I);
    if (SLoad == nullptr || !isCallTo(SLoad, "evm_sload") ||
        SLoad->arg_size() != 1) {
      continue;
    }
    Value *Slot = SLoad->getArgOperand(0);

    for (Instruction &OldLenI : *SuccessBlock) {
      auto *OldLen = dyn_cast<CallBase>(&OldLenI);
      if (!isPrivateHelperCall(OldLen) ||
          !OldLen->getType()->isIntegerTy(256) || !callHasArg(OldLen, SLoad)) {
        continue;
      }

      for (Instruction &CopyI : *SuccessBlock) {
        auto *Copy = dyn_cast<CallBase>(&CopyI);
        if (!isPrivateHelperCall(Copy) || !Copy->getType()->isVoidTy()) {
          continue;
        }
        if (callHasArg(Copy, Slot) && callHasArg(Copy, OldLen) &&
            callHasArg(Copy, Length)) {
          return true;
        }
      }
    }
  }
  return false;
}

bool isShiftedStorageBytesLength(Value *V, Value *Length) {
  auto *Call = dyn_cast_or_null<CallBase>(V);
  return Call != nullptr && isCallTo(Call, "evm_shl") &&
         Call->arg_size() == 2 &&
         isConstantIntValue(Call->getArgOperand(0), 1) &&
         isSameValue(Call->getArgOperand(1), Length);
}

bool valueContainsShiftedStorageBytesLength(Value *V, Value *Length,
                                            unsigned Depth,
                                            SmallPtrSetImpl<Value *> &Seen) {
  if (V == nullptr || Length == nullptr || Depth == 0 ||
      !Seen.insert(V).second) {
    return false;
  }
  if (isShiftedStorageBytesLength(V, Length)) {
    return true;
  }
  auto *Inst = dyn_cast<Instruction>(V);
  if (Inst == nullptr) {
    return false;
  }
  for (Value *Operand : Inst->operands()) {
    if (valueContainsShiftedStorageBytesLength(Operand, Length, Depth - 1,
                                               Seen)) {
      return true;
    }
  }
  return false;
}

bool valueContainsShiftedStorageBytesLength(Value *V, Value *Length) {
  SmallPtrSet<Value *, 16> Seen;
  return valueContainsShiftedStorageBytesLength(V, Length, 8, Seen);
}

bool hasOldStorageBytesLengthBranch(BasicBlock *BB, CallBase *SLoad) {
  if (BB == nullptr || SLoad == nullptr) {
    return false;
  }
  auto *Br = dyn_cast_or_null<BranchInst>(BB->getTerminator());
  if (Br == nullptr || !Br->isConditional()) {
    return false;
  }
  auto *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
  if (Cmp == nullptr || Cmp->getPredicate() != ICmpInst::ICMP_UGT ||
      !isConstantIntValue(Cmp->getOperand(1), 31)) {
    return false;
  }
  auto *OldLen = dyn_cast<CallBase>(Cmp->getOperand(0));
  return isPrivateHelperCall(OldLen) && OldLen->getType()->isIntegerTy(256) &&
         callHasArg(OldLen, SLoad);
}

void collectReachableBlocks(BasicBlock *BB, unsigned Depth,
                            SmallPtrSetImpl<BasicBlock *> &Seen,
                            SmallVectorImpl<BasicBlock *> &Blocks) {
  if (BB == nullptr || Depth == 0 || !Seen.insert(BB).second) {
    return;
  }
  Blocks.push_back(BB);
  auto *Br = dyn_cast_or_null<BranchInst>(BB->getTerminator());
  if (Br == nullptr) {
    return;
  }
  for (BasicBlock *Succ : successors(BB)) {
    collectReachableBlocks(Succ, Depth - 1, Seen, Blocks);
  }
}

bool hasDirectStorageByteArrayWrite(BasicBlock *SuccessBlock, Value *Length,
                                    Value *Slot) {
  SmallPtrSet<BasicBlock *, 16> Seen;
  SmallVector<BasicBlock *, 16> Blocks;
  collectReachableBlocks(SuccessBlock, 16, Seen, Blocks);

  bool SawLengthBranch = false;
  bool SawEncodedLengthStore = false;
  for (BasicBlock *BB : Blocks) {
    if (hasLengthGreaterThan31Branch(BB, Length)) {
      SawLengthBranch = true;
    }
    for (Instruction &I : *BB) {
      auto *Store = dyn_cast<CallBase>(&I);
      if (Store == nullptr || !isCallTo(Store, "evm_sstore") ||
          Store->arg_size() != 2 ||
          !isSameValue(Store->getArgOperand(0), Slot)) {
        continue;
      }
      if (valueContainsShiftedStorageBytesLength(Store->getArgOperand(1),
                                                 Length)) {
        SawEncodedLengthStore = true;
      }
    }
  }
  return SawLengthBranch && SawEncodedLengthStore;
}

bool hasDirectStorageByteArrayCopyToStorageSuccess(BasicBlock *SuccessBlock,
                                                   Value *Length) {
  if (SuccessBlock == nullptr || Length == nullptr) {
    return false;
  }
  for (Instruction &I : *SuccessBlock) {
    auto *SLoad = dyn_cast<CallBase>(&I);
    if (SLoad == nullptr || !isCallTo(SLoad, "evm_sload") ||
        SLoad->arg_size() != 1 ||
        !hasOldStorageBytesLengthBranch(SuccessBlock, SLoad)) {
      continue;
    }
    if (hasDirectStorageByteArrayWrite(SuccessBlock, Length,
                                       SLoad->getArgOperand(0))) {
      return true;
    }
  }
  return false;
}

bool isMemoryAllocationSize(Value *V) {
  auto *RoundedSize = dyn_cast_or_null<BinaryOperator>(V);
  if (RoundedSize == nullptr || RoundedSize->getOpcode() != Instruction::And) {
    return false;
  }
  if (isRoundedByteAllocationSize(V)) {
    return true;
  }

  Value *RoundedBase = nullptr;
  if (isMinus32(RoundedSize->getOperand(0))) {
    RoundedBase = RoundedSize->getOperand(1);
  } else if (isMinus32(RoundedSize->getOperand(1))) {
    RoundedBase = RoundedSize->getOperand(0);
  }
  if (RoundedBase == nullptr) {
    return false;
  }

  auto *Add = dyn_cast<BinaryOperator>(RoundedBase);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return false;
  }

  Value *Shift = nullptr;
  if (isConstantIntValue(Add->getOperand(0), 63)) {
    Shift = Add->getOperand(1);
  } else if (isConstantIntValue(Add->getOperand(1), 63)) {
    Shift = Add->getOperand(0);
  }
  auto *ShiftCall = dyn_cast_or_null<CallBase>(Shift);
  if (ShiftCall != nullptr && isCallTo(ShiftCall, "evm_shl") &&
      ShiftCall->arg_size() == 2 &&
      isConstantIntValue(ShiftCall->getArgOperand(0), 5)) {
    return true;
  }
  return isRoundedByteAllocationSize(V);
}

bool isSmallFixedMemoryAllocationSize(Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr) {
    return false;
  }
  const APInt &Size = C->getValue();
  if (Size.isZero() || Size.ugt(4096)) {
    return false;
  }
  uint64_t Size64 = Size.getZExtValue();
  return Size64 % 32 == 0;
}

ConstantInt *getUniformConstantArgument(Value *V) {
  auto *Arg = dyn_cast_or_null<Argument>(V);
  if (Arg == nullptr) {
    return nullptr;
  }

  Function *F = Arg->getParent();
  ConstantInt *Const = nullptr;
  for (User *U : F->users()) {
    auto *Call = dyn_cast<CallBase>(U);
    if (Call == nullptr || Call->getCalledFunction() != F ||
        Call->arg_size() <= Arg->getArgNo()) {
      return nullptr;
    }
    auto *ArgConst =
        dyn_cast<ConstantInt>(Call->getArgOperand(Arg->getArgNo()));
    if (ArgConst == nullptr) {
      return nullptr;
    }
    if (Const == nullptr) {
      Const = ArgConst;
      continue;
    }
    if (Const->getValue() != ArgConst->getValue()) {
      return nullptr;
    }
  }
  return Const;
}

bool isConstantIntValueOrUniformArg(Value *V, uint64_t N) {
  if (isConstantIntValue(V, N)) {
    return true;
  }
  auto *Uniform = getUniformConstantArgument(V);
  return Uniform != nullptr && Uniform->getValue() == N;
}

bool isSupportedMemoryAllocationSize(Value *V) {
  return isMemoryAllocationSize(V) || isRoundedMemoryAllocationSize(V) ||
         isSmallFixedMemoryAllocationSize(V);
}

bool isSupportedMemoryAllocationSizeWithUniformArg(Value *V) {
  if (isSupportedMemoryAllocationSize(V)) {
    return true;
  }
  return isSmallFixedMemoryAllocationSize(getUniformConstantArgument(V));
}

Value *getMemoryPointerLoadSlot(Value *V) {
  std::optional<EvmMemoryLoad> Load = matchEvmMemoryLoad(V);
  return Load.has_value() ? Load->Address : nullptr;
}

Instruction *findFreeMemoryPointerStore(BasicBlock *BB, Value *NewPtr) {
  if (BB == nullptr) {
    return nullptr;
  }
  for (Instruction &I : *BB) {
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (Store.has_value() && isConstantIntValue(Store->Address, 64) &&
        isSameValue(Store->StoredValue, NewPtr)) {
      return Store->Inst;
    }
  }
  return nullptr;
}

Instruction *findUniformFreeMemoryPointerStore(BasicBlock *BB, Value *NewPtr) {
  if (BB == nullptr || NewPtr == nullptr) {
    return nullptr;
  }
  for (Instruction &I : *BB) {
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (Store.has_value() &&
        isConstantIntValueOrUniformArg(Store->Address, 64) &&
        isSameValue(Store->StoredValue, NewPtr)) {
      return Store->Inst;
    }
  }
  return nullptr;
}

Instruction *findFinalizeAllocCall(BasicBlock *BB, Value *Base, Value *Size) {
  if (BB == nullptr || Base == nullptr || Size == nullptr) {
    return nullptr;
  }
  for (Instruction &I : *BB) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call != nullptr && isCallTo(Call, "notdec_evm_finalize_alloc") &&
        Call->arg_size() == 2 && isSameValue(Call->getArgOperand(0), Base) &&
        isSameValue(Call->getArgOperand(1), Size)) {
      return Call;
    }
  }
  return nullptr;
}

Instruction *findSolidityMemoryAllocationCall(BasicBlock *BB, Value *Base,
                                              Value *Size) {
  if (BB == nullptr || Base == nullptr || Size == nullptr) {
    return nullptr;
  }
  for (Instruction &I : *BB) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call != nullptr &&
        isCallTo(Call, "notdec_solidity_memory_allocation") &&
        Call->arg_size() == 2 && isSameValue(Call->getArgOperand(0), Base) &&
        isSameValue(Call->getArgOperand(1), Size)) {
      return Call;
    }
  }
  return nullptr;
}

Instruction *findMemoryPointerStoreForLoad(BasicBlock *BB, Value *OldPtr,
                                           Value *NewPtr) {
  Value *Slot = getMemoryPointerLoadSlot(OldPtr);
  if (BB == nullptr || Slot == nullptr) {
    return nullptr;
  }
  for (Instruction &I : *BB) {
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (Store.has_value() && isSameValue(Store->Address, Slot) &&
        isSameValue(Store->StoredValue, NewPtr)) {
      return Store->Inst;
    }
  }
  return nullptr;
}

// After a previous allocation was rewritten, the next finalizeAllocation guard
// can see old/new free pointers as two offsets from the same original mload.
// Require the old pointer to be stored before the guard and the new pointer to
// be stored to the same slot on success, so this stays tied to free-pointer
// updates rather than arbitrary address arithmetic.
Value *findMemoryPointerStoreSlot(BasicBlock *BB, Value *Ptr) {
  if (BB == nullptr || Ptr == nullptr) {
    return nullptr;
  }
  for (Instruction &I : *BB) {
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (Store.has_value() && isSameValue(Store->StoredValue, Ptr)) {
      return Store->Address;
    }
  }
  return nullptr;
}

Instruction *findMemoryPointerStoreToSlot(BasicBlock *BB, Value *Slot,
                                          Value *NewPtr) {
  if (BB == nullptr || Slot == nullptr || NewPtr == nullptr) {
    return nullptr;
  }
  for (Instruction &I : *BB) {
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (Store.has_value() && isSameValue(Store->Address, Slot) &&
        isSameValue(Store->StoredValue, NewPtr)) {
      return Store->Inst;
    }
  }
  return nullptr;
}

bool hasMemoryPointerStoreTransition(BasicBlock *GuardBlock,
                                     BasicBlock *SuccessBlock, Value *OldPtr,
                                     Value *NewPtr) {
  Value *Slot = findMemoryPointerStoreSlot(GuardBlock, OldPtr);
  return Slot != nullptr &&
         findMemoryPointerStoreToSlot(SuccessBlock, Slot, NewPtr) != nullptr;
}

bool isSmallFixedMemoryAllocationDelta(uint64_t Size) {
  return Size != 0 && Size <= 4096 && Size % 32 == 0;
}

bool matchConstantOffsetAllocationDelta(Value *NewPtr, Value *OldPtr,
                                        Value *&Base, uint64_t &Size) {
  auto *NewAdd = dyn_cast_or_null<BinaryOperator>(NewPtr);
  if (NewAdd == nullptr || NewAdd->getOpcode() != Instruction::Add) {
    return false;
  }

  for (Value *CandidateBase : {NewAdd->getOperand(0), NewAdd->getOperand(1)}) {
    std::optional<uint64_t> NewOffset =
        getOffsetFromBase(NewPtr, CandidateBase);
    std::optional<uint64_t> OldOffset =
        getOffsetFromBase(OldPtr, CandidateBase);
    if (!NewOffset.has_value() || !OldOffset.has_value() ||
        *NewOffset <= *OldOffset) {
      continue;
    }

    uint64_t Delta = *NewOffset - *OldOffset;
    if (!isSmallFixedMemoryAllocationDelta(Delta)) {
      continue;
    }
    Base = CandidateBase;
    Size = Delta;
    return true;
  }
  return false;
}

Value *findPtrToIntInBlock(BasicBlock *BB, Value *Ptr) {
  if (BB == nullptr || Ptr == nullptr) {
    return nullptr;
  }
  for (Instruction &I : *BB) {
    auto *Cast = dyn_cast<PtrToIntInst>(&I);
    if (Cast != nullptr && isSameValue(Cast->getOperand(0), Ptr)) {
      return Cast;
    }
  }
  return nullptr;
}

Value *getOrCreatePtrToIntBeforeTerminator(BasicBlock *BB, Value *Ptr) {
  if (Value *Existing = findPtrToIntInBlock(BB, Ptr)) {
    return Existing;
  }
  if (BB == nullptr || Ptr == nullptr || !Ptr->getType()->isPointerTy()) {
    return nullptr;
  }

  IRBuilder<> Builder(BB->getTerminator());
  return Builder.CreatePtrToInt(Ptr, Type::getIntNTy(BB->getContext(), 256),
                                "evm.alloc.addr.rewrite");
}

std::optional<uint64_t> matchPointerUInt64LimitMinus(Value *PtrLimit,
                                                     bool StrictUpper,
                                                     bool AllowZero) {
  Value *Limit = getIntToPtrAddress(PtrLimit);
  std::optional<uint64_t> Size =
      StrictUpper ? matchUInt64LimitMinusStrictUpperAllowZero(Limit)
                  : matchUInt64LimitMinus(Limit);
  if (!Size.has_value() || (!AllowZero && *Size == 0)) {
    return std::nullopt;
  }
  return Size;
}

std::optional<uint64_t> getFixedCallocAllocationSize(Value *Ptr) {
  auto *Call = dyn_cast_or_null<CallBase>(Ptr);
  if (Call == nullptr || !isCallTo(Call, "calloc") || Call->arg_size() != 2 ||
      !isConstantIntValue(Call->getArgOperand(0), 1)) {
    return std::nullopt;
  }

  auto *Size = dyn_cast<ConstantInt>(Call->getArgOperand(1));
  if (Size == nullptr || !isSmallFixedMemoryAllocationSize(Size)) {
    return std::nullopt;
  }
  return Size->getZExtValue();
}

bool isAllocationPointer(Value *Ptr) {
  auto *Call = dyn_cast_or_null<CallBase>(Ptr);
  return Call != nullptr &&
         ((isCallTo(Call, "calloc") && Call->arg_size() == 2) ||
          (isCallTo(Call, "calloc_unbounded") && Call->arg_size() == 0));
}

bool callUsesPtrToInt(CallBase *Call, unsigned ArgNo, Value *Ptr) {
  if (Call == nullptr || Call->arg_size() <= ArgNo) {
    return false;
  }
  auto *PtrToInt = dyn_cast<PtrToIntInst>(Call->getArgOperand(ArgNo));
  return PtrToInt != nullptr && isSameValue(PtrToInt->getOperand(0), Ptr);
}

bool findSolidityMemoryAllocationForPointer(BasicBlock *BB, Value *Ptr,
                                            Value *Size) {
  if (BB == nullptr || Ptr == nullptr || Size == nullptr) {
    return false;
  }
  for (Instruction &I : *BB) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call != nullptr &&
        isCallTo(Call, "notdec_solidity_memory_allocation") &&
        Call->arg_size() == 2 && callUsesPtrToInt(Call, 0, Ptr) &&
        isSameValue(Call->getArgOperand(1), Size)) {
      return true;
    }
  }
  return false;
}

bool findFreeMemoryPointerStoreForPointer(BasicBlock *BB, Value *Ptr) {
  if (BB == nullptr || Ptr == nullptr) {
    return false;
  }
  for (Instruction &I : *BB) {
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&I);
    if (!Store.has_value() || !isConstantIntValue(Store->Address, 64)) {
      continue;
    }
    auto *PtrToInt = dyn_cast<PtrToIntInst>(Store->StoredValue);
    if (PtrToInt != nullptr && isSameValue(PtrToInt->getOperand(0), Ptr)) {
      return true;
    }
  }
  return false;
}

std::optional<CheckedBoundsMatch>
matchDirectAllocationPointerBounds(const NormalizedCondition &FailureCond,
                                   const SolidityRevertMatch &RevertMatch,
                                   BasicBlock *SuccessBlock) {
  // After memory allocation recovery, Solidity's fixed-size allocation guard
  // can become a direct pointer comparison on the calloc result.  Keep this
  // matcher tied to allocation calls and success-side allocation evidence so it
  // does not rewrite unrelated native pointer comparisons.
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x41) {
    return std::nullopt;
  }

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *Ptr = nullptr;
  std::optional<uint64_t> Size;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGT &&
      Cmp->getOperand(0)->getType()->isPointerTy()) {
    Ptr = Cmp->getOperand(0);
    Size = matchPointerUInt64LimitMinus(Cmp->getOperand(1), true, true);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULT &&
             Cmp->getOperand(1)->getType()->isPointerTy()) {
    Ptr = Cmp->getOperand(1);
    Size = matchPointerUInt64LimitMinus(Cmp->getOperand(0), true, true);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_UGE &&
             Cmp->getOperand(0)->getType()->isPointerTy()) {
    Ptr = Cmp->getOperand(0);
    Size = matchPointerUInt64LimitMinus(Cmp->getOperand(1), false, false);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE &&
             Cmp->getOperand(1)->getType()->isPointerTy()) {
    Ptr = Cmp->getOperand(1);
    Size = matchPointerUInt64LimitMinus(Cmp->getOperand(0), false, false);
  }

  if (Ptr == nullptr || !Size.has_value() || !isAllocationPointer(Ptr)) {
    return std::nullopt;
  }

  Value *SizeValue = ConstantInt::get(Type::getIntNTy(Cmp->getContext(), 256),
                                      *Size);
  bool HasAllocationMarker =
      *Size != 0 &&
      findSolidityMemoryAllocationForPointer(SuccessBlock, Ptr, SizeValue);
  bool HasMatchingCallocSize =
      getFixedCallocAllocationSize(Ptr).value_or(
          std::numeric_limits<uint64_t>::max()) == *Size;
  bool HasFreePointerStore =
      *Size == 0 && findFreeMemoryPointerStoreForPointer(SuccessBlock, Ptr);
  if (!HasAllocationMarker && !HasMatchingCallocSize && !HasFreePointerStore) {
    return std::nullopt;
  }

  Value *PtrInt = getOrCreatePtrToIntBeforeTerminator(Cmp->getParent(), Ptr);
  if (PtrInt == nullptr) {
    return std::nullopt;
  }
  return CheckedBoundsMatch{"memory_allocation_pointer_bounds",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {PtrInt, SizeValue, PtrInt},
                            RevertMatch.PanicCode,
                            true};
}

// Solidity encodes short storage bytes/string values in the slot itself. This
// only accepts the canonical decoded-length expression from that same slot.
bool matchStorageBytesLength(Value *Length, Value *Slot) {
  auto *Select = dyn_cast_or_null<SelectInst>(Length);
  if (Select == nullptr) {
    return false;
  }

  auto *ShortCond = dyn_cast<ICmpInst>(Select->getCondition());
  if (ShortCond == nullptr || ShortCond->getPredicate() != ICmpInst::ICMP_EQ ||
      !isZero(ShortCond->getOperand(1))) {
    return false;
  }

  auto *LowBit = dyn_cast<BinaryOperator>(ShortCond->getOperand(0));
  if (LowBit == nullptr || LowBit->getOpcode() != Instruction::And ||
      !binaryOpHasOperand(LowBit, Slot) ||
      !(isConstantIntValue(LowBit->getOperand(0), 1) ||
        isConstantIntValue(LowBit->getOperand(1), 1))) {
    return false;
  }

  auto *ShortLen = dyn_cast<BinaryOperator>(Select->getTrueValue());
  auto *FullLen = dyn_cast<CallBase>(Select->getFalseValue());
  if (ShortLen == nullptr || ShortLen->getOpcode() != Instruction::And ||
      FullLen == nullptr || FullLen->arg_size() != 2) {
    return false;
  }

  bool IsDivByTwo = isCallTo(FullLen, "evm_div") &&
                    isSameValue(FullLen->getArgOperand(0), Slot) &&
                    isConstantIntValue(FullLen->getArgOperand(1), 2);
  bool IsShrByOne = isCallTo(FullLen, "evm_shr") &&
                    isConstantIntValue(FullLen->getArgOperand(0), 1) &&
                    isSameValue(FullLen->getArgOperand(1), Slot);
  if (!IsDivByTwo && !IsShrByOne) {
    return false;
  }

  return binaryOpHasOperand(ShortLen, FullLen) &&
         (isConstantIntValue(ShortLen->getOperand(0), 127) ||
          isConstantIntValue(ShortLen->getOperand(1), 127));
}

std::optional<CheckedBoundsMatch>
matchMemoryAllocationBounds(const NormalizedCondition &FailureCond,
                            const SolidityRevertMatch &RevertMatch,
                            BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x41) {
    return std::nullopt;
  }

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *Length = nullptr;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGE &&
      isUInt64Limit(Cmp->getOperand(1))) {
    Length = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE &&
             isUInt64Limit(Cmp->getOperand(0))) {
    Length = Cmp->getOperand(1);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_UGE &&
             isUInt64Max(Cmp->getOperand(1))) {
    Length = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE &&
             isUInt64Max(Cmp->getOperand(0))) {
    Length = Cmp->getOperand(1);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_UGT &&
             isUInt64Max(Cmp->getOperand(1))) {
    Length = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULT &&
             isUInt64Max(Cmp->getOperand(0))) {
    Length = Cmp->getOperand(1);
  } else {
    return std::nullopt;
  }

  if (!hasMemoryAllocationSizeComputation(SuccessBlock, Length)) {
    return std::nullopt;
  }

  return CheckedBoundsMatch{"memory_allocation_bounds",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {Length},
                            RevertMatch.PanicCode,
                            true};
}

std::optional<CheckedBoundsMatch>
matchStorageByteArrayLengthBounds(const NormalizedCondition &FailureCond,
                                  const SolidityRevertMatch &RevertMatch,
                                  BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x41) {
    return std::nullopt;
  }

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *Length = nullptr;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGE &&
      isUInt64Limit(Cmp->getOperand(1))) {
    Length = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE &&
             isUInt64Limit(Cmp->getOperand(0))) {
    Length = Cmp->getOperand(1);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_UGT &&
             isUInt64Max(Cmp->getOperand(1))) {
    Length = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULT &&
             isUInt64Max(Cmp->getOperand(0))) {
    Length = Cmp->getOperand(1);
  } else {
    return std::nullopt;
  }

  if (!hasStorageByteArrayCopyToStorageSuccess(SuccessBlock, Length) &&
      !hasDirectStorageByteArrayCopyToStorageSuccess(SuccessBlock, Length)) {
    return std::nullopt;
  }

  return CheckedBoundsMatch{"storage_byte_array_length_bounds",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {Length},
                            RevertMatch.PanicCode,
                            true};
}

BinaryOperator *findStorageArrayLengthIncrementStore(BasicBlock *SuccessBlock,
                                                     Value *Length) {
  // Solidity guards dynamic storage array push by checking the old length
  // before writing oldLength + 1 back to the same storage length slot.
  auto *LengthLoad = dyn_cast_or_null<CallBase>(Length);
  if (SuccessBlock == nullptr || LengthLoad == nullptr ||
      !isCallTo(LengthLoad, "evm_sload") || LengthLoad->arg_size() != 1) {
    return nullptr;
  }

  Value *Slot = LengthLoad->getArgOperand(0);
  Value *One = ConstantInt::get(Length->getType(), 1);
  BinaryOperator *Increment = findCommutativeBinaryOpInBlock(
      SuccessBlock, Instruction::Add, Length, One);
  if (Increment == nullptr) {
    return nullptr;
  }

  for (Instruction &I : *SuccessBlock) {
    auto *Store = dyn_cast<CallBase>(&I);
    if (Store == nullptr || !isCallTo(Store, "evm_sstore") ||
        Store->arg_size() != 2) {
      continue;
    }
    if (isSameValue(Store->getArgOperand(0), Slot) &&
        isSameValue(Store->getArgOperand(1), Increment)) {
      return Increment;
    }
  }
  return nullptr;
}

std::optional<CheckedBoundsMatch>
matchStorageArrayLengthBounds(const NormalizedCondition &FailureCond,
                              const SolidityRevertMatch &RevertMatch,
                              BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x41) {
    return std::nullopt;
  }

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *Length = nullptr;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGE &&
      isUInt64LimitValue(Cmp->getOperand(1))) {
    Length = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE &&
             isUInt64LimitValue(Cmp->getOperand(0))) {
    Length = Cmp->getOperand(1);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_UGT &&
             isUInt64Max(Cmp->getOperand(1))) {
    Length = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULT &&
             isUInt64Max(Cmp->getOperand(0))) {
    Length = Cmp->getOperand(1);
  } else {
    return std::nullopt;
  }

  BinaryOperator *Increment =
      findStorageArrayLengthIncrementStore(SuccessBlock, Length);
  if (Increment == nullptr) {
    return std::nullopt;
  }

  return CheckedBoundsMatch{
      "storage_array_length_bounds",
      "",
      nullptr,
      nullptr,
      nullptr,
      RevertMatch.Revert,
      {Length, ConstantInt::get(Length->getType(), 1), Increment},
      RevertMatch.PanicCode,
      true};
}

std::optional<CheckedBoundsMatch>
matchFixedMemoryAllocationPointerBounds(const NormalizedCondition &FailureCond,
                                        const SolidityRevertMatch &RevertMatch,
                                        BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x41) {
    return std::nullopt;
  }

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *OldPtr = nullptr;
  std::optional<uint64_t> Size;
  bool StrictUpperBound = false;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGE) {
    OldPtr = Cmp->getOperand(0);
    Size = matchUInt64LimitMinus(Cmp->getOperand(1));
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE) {
    OldPtr = Cmp->getOperand(1);
    Size = matchUInt64LimitMinus(Cmp->getOperand(0));
  } else if (FailureCond.Predicate == ICmpInst::ICMP_UGT) {
    OldPtr = Cmp->getOperand(0);
    Size = matchUInt64LimitMinusStrictUpper(Cmp->getOperand(1));
    StrictUpperBound = true;
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULT) {
    OldPtr = Cmp->getOperand(1);
    Size = matchUInt64LimitMinusStrictUpper(Cmp->getOperand(0));
    StrictUpperBound = true;
  }

  if (OldPtr == nullptr || !Size.has_value()) {
    return std::nullopt;
  }
  if (!StrictUpperBound && !isFreeMemoryPointerLoad(OldPtr)) {
    return std::nullopt;
  }

  Value *SizeValue = ConstantInt::get(OldPtr->getType(), *Size);
  BinaryOperator *NewPtr = findCommutativeBinaryOpInBlock(
      SuccessBlock, Instruction::Add, OldPtr, SizeValue);
  if (NewPtr == nullptr) {
    NewPtr = findCommutativeBinaryOpInBlock(Cmp->getParent(), Instruction::Add,
                                            OldPtr, SizeValue);
  }
  if (NewPtr == nullptr ||
      (findFreeMemoryPointerStore(SuccessBlock, NewPtr) == nullptr &&
       findMemoryPointerStoreForLoad(SuccessBlock, OldPtr, NewPtr) ==
           nullptr)) {
    return std::nullopt;
  }

  return CheckedBoundsMatch{"memory_allocation_pointer_bounds",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {OldPtr, SizeValue, NewPtr},
                            RevertMatch.PanicCode,
                            true};
}

std::optional<CheckedBoundsMatch> matchZeroSizeMemoryAllocationPointerBounds(
    const NormalizedCondition &FailureCond,
    const SolidityRevertMatch &RevertMatch, BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x41) {
    return std::nullopt;
  }

  ICmpInst *Cmp = FailureCond.Cmp;
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  Value *NewPtr = nullptr;
  if (FailureCond.Predicate == ICmpInst::ICMP_UGT &&
      isUInt64Max(Cmp->getOperand(1))) {
    NewPtr = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULT &&
             isUInt64Max(Cmp->getOperand(0))) {
    NewPtr = Cmp->getOperand(1);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_UGE &&
             isUInt64Limit(Cmp->getOperand(1))) {
    NewPtr = Cmp->getOperand(0);
  } else if (FailureCond.Predicate == ICmpInst::ICMP_ULE &&
             isUInt64Limit(Cmp->getOperand(0))) {
    NewPtr = Cmp->getOperand(1);
  } else {
    return std::nullopt;
  }

  if (findFreeMemoryPointerStore(SuccessBlock, NewPtr) == nullptr) {
    return std::nullopt;
  }

  Value *ZeroSize = ConstantInt::get(NewPtr->getType(), 0);
  return CheckedBoundsMatch{"memory_allocation_pointer_bounds",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {NewPtr, ZeroSize, NewPtr},
                            RevertMatch.PanicCode,
                            true};
}

std::optional<CheckedBoundsMatch>
matchStorageBytesEncoding(Value *BranchCondition, bool FailureWhenCondTrue,
                          const SolidityRevertMatch &RevertMatch) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x22) {
    return std::nullopt;
  }

  auto *Xor = dyn_cast_or_null<BinaryOperator>(BranchCondition);
  if (Xor == nullptr || Xor->getOpcode() != Instruction::Xor ||
      !Xor->getType()->isIntegerTy(1)) {
    return std::nullopt;
  }

  ICmpInst *LenCmp = nullptr;
  TruncInst *SlotLowBit = nullptr;
  for (Value *Operand : {Xor->getOperand(0), Xor->getOperand(1)}) {
    if (auto *Cmp = dyn_cast<ICmpInst>(Operand)) {
      LenCmp = Cmp;
      continue;
    }
    if (auto *Trunc = dyn_cast<TruncInst>(Operand)) {
      SlotLowBit = Trunc;
      continue;
    }
    return std::nullopt;
  }

  if (LenCmp == nullptr || SlotLowBit == nullptr ||
      !SlotLowBit->getType()->isIntegerTy(1)) {
    return std::nullopt;
  }

  Value *Length = nullptr;
  bool ConditionMeansFailure = false;
  if (LenCmp->getPredicate() == ICmpInst::ICMP_UGT &&
      isConstantIntValue(LenCmp->getOperand(1), 31)) {
    Length = LenCmp->getOperand(0);
    ConditionMeansFailure = true;
  } else if (LenCmp->getPredicate() == ICmpInst::ICMP_ULT &&
             isConstantIntValue(LenCmp->getOperand(1), 32)) {
    Length = LenCmp->getOperand(0);
    ConditionMeansFailure = false;
  } else {
    return std::nullopt;
  }

  if (ConditionMeansFailure != FailureWhenCondTrue ||
      !matchStorageBytesLength(Length, SlotLowBit->getOperand(0))) {
    return std::nullopt;
  }

  return CheckedBoundsMatch{"storage_bytes_encoding",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {SlotLowBit->getOperand(0), Length},
                            RevertMatch.PanicCode,
                            true};
}

std::optional<CheckedBoundsMatch> matchMemoryAllocationPointerBounds(
    Value *BranchCondition, bool FailureWhenCondTrue,
    const SolidityRevertMatch &RevertMatch, BasicBlock *SuccessBlock) {
  if (!RevertMatch.PanicCode.has_value() || *RevertMatch.PanicCode != 0x41) {
    return std::nullopt;
  }

  auto *Combiner = dyn_cast_or_null<BinaryOperator>(BranchCondition);
  if (Combiner == nullptr || !Combiner->getType()->isIntegerTy(1)) {
    return std::nullopt;
  }

  Instruction::BinaryOps ExpectedCombiner =
      FailureWhenCondTrue ? Instruction::Or : Instruction::And;
  if (Combiner->getOpcode() != ExpectedCombiner) {
    return std::nullopt;
  }

  Value *RangeCheckedPtr = nullptr;
  Value *RangeCheckedBase = nullptr;
  Value *NoWrapCheckedPtr = nullptr;
  Value *OldPtr = nullptr;
  Value *NoWrapLimit = nullptr;
  Value *NoWrapInitialFreePointerRoundedBase = nullptr;
  std::optional<uint64_t> RangeCheckedTotalSize;
  std::optional<uint64_t> NoWrapStrictUpperSize;
  for (Value *Operand : {Combiner->getOperand(0), Combiner->getOperand(1)}) {
    auto *Cmp = dyn_cast<ICmpInst>(Operand);
    if (Cmp == nullptr) {
      return std::nullopt;
    }
    if (FailureWhenCondTrue) {
      // Solidity finalizeAllocation uses:
      //   or(gt(newFreePtr, uint64max), lt(newFreePtr, memPtr))
      if (Cmp->getPredicate() == ICmpInst::ICMP_UGT &&
          isUInt64Max(Cmp->getOperand(1))) {
        RangeCheckedPtr = Cmp->getOperand(0);
        continue;
      }
      if (Cmp->getPredicate() == ICmpInst::ICMP_UGT) {
        std::optional<uint64_t> TotalSize =
            matchUInt64LimitMinusStrictUpper(Cmp->getOperand(1));
        if (TotalSize.has_value()) {
          RangeCheckedBase = Cmp->getOperand(0);
          RangeCheckedTotalSize = TotalSize;
          continue;
        }
      }
      if (Cmp->getPredicate() == ICmpInst::ICMP_UGT) {
        std::optional<uint64_t> Size =
            matchWrappingAddStrictUpper(Cmp->getOperand(1));
        if (Size.has_value()) {
          if (*Size == 128) {
            OldPtr = ConstantInt::get(Cmp->getOperand(0)->getType(), 128);
            NoWrapInitialFreePointerRoundedBase = Cmp->getOperand(0);
            continue;
          }
          OldPtr = Cmp->getOperand(0);
          NoWrapStrictUpperSize = Size;
          continue;
        }
      }
      if (Cmp->getPredicate() == ICmpInst::ICMP_UGE &&
          isUInt64Limit(Cmp->getOperand(1))) {
        RangeCheckedPtr = Cmp->getOperand(0);
        continue;
      }
      if (Cmp->getPredicate() == ICmpInst::ICMP_ULT) {
        NoWrapCheckedPtr = Cmp->getOperand(0);
        OldPtr = Cmp->getOperand(1);
        continue;
      }
    } else {
      if (Cmp->getPredicate() == ICmpInst::ICMP_ULT &&
          isUInt64Limit(Cmp->getOperand(1))) {
        RangeCheckedPtr = Cmp->getOperand(0);
        continue;
      }
      if (Cmp->getPredicate() == ICmpInst::ICMP_ULE &&
          isUInt64Max(Cmp->getOperand(1))) {
        RangeCheckedPtr = Cmp->getOperand(0);
        continue;
      }
      if (Cmp->getPredicate() == ICmpInst::ICMP_UGE) {
        NoWrapCheckedPtr = Cmp->getOperand(0);
        OldPtr = Cmp->getOperand(1);
        continue;
      }
      if (Cmp->getPredicate() == ICmpInst::ICMP_ULT &&
          Cmp->getOperand(0)->getType()->isPointerTy()) {
        std::optional<uint64_t> Size =
            matchWrappingPointerStrictUpper(Cmp->getOperand(1));
        Value *OldPtrInt = findPtrToIntInBlock(Combiner->getParent(),
                                               Cmp->getOperand(0));
        if (Size.has_value() && OldPtrInt != nullptr) {
          OldPtr = OldPtrInt;
          NoWrapStrictUpperSize = Size;
          continue;
        }
      }
      if (Cmp->getPredicate() == ICmpInst::ICMP_ULT) {
        OldPtr = Cmp->getOperand(0);
        NoWrapLimit = Cmp->getOperand(1);
        continue;
      }
    }
    return std::nullopt;
  }

  if ((RangeCheckedPtr == nullptr && RangeCheckedBase == nullptr) ||
      OldPtr == nullptr) {
    return std::nullopt;
  }
  if (!FailureWhenCondTrue && !isFreeMemoryPointerLoad(OldPtr) &&
      !isFreeMemoryAllocationBase(OldPtr)) {
    return std::nullopt;
  }

  BasicBlock *GuardBlock = Combiner->getParent();
  Value *NewPtr = RangeCheckedPtr;
  Value *Size = nullptr;
  Value *SharedBase = nullptr;
  uint64_t OffsetDelta = 0;
  bool UsesOffsetPair = false;

  if (NewPtr == nullptr) {
    if (!RangeCheckedTotalSize.has_value()) {
      return std::nullopt;
    }
    Value *TotalSizeValue =
        ConstantInt::get(RangeCheckedBase->getType(), *RangeCheckedTotalSize);
    auto *FoundNewPtr = findCommutativeBinaryOpInBlock(
        GuardBlock, Instruction::Add, RangeCheckedBase, TotalSizeValue);
    if (FoundNewPtr == nullptr) {
      FoundNewPtr = findCommutativeBinaryOpInBlock(
          SuccessBlock, Instruction::Add, RangeCheckedBase, TotalSizeValue);
    }
    NewPtr = FoundNewPtr;
  }

  auto *NewPtrAdd = dyn_cast_or_null<BinaryOperator>(NewPtr);
  if (NewPtrAdd == nullptr || NewPtrAdd->getOpcode() != Instruction::Add) {
    return std::nullopt;
  }

  if (binaryOpHasOperand(NewPtrAdd, OldPtr)) {
    Size = isSameValue(NewPtrAdd->getOperand(0), OldPtr)
               ? NewPtrAdd->getOperand(1)
               : NewPtrAdd->getOperand(0);
  } else if (matchConstantOffsetAllocationDelta(NewPtr, OldPtr, SharedBase,
                                                OffsetDelta)) {
    Size = ConstantInt::get(NewPtr->getType(), OffsetDelta);
    UsesOffsetPair = true;
  } else {
    return std::nullopt;
  }

  if (RangeCheckedBase != nullptr) {
    if (SharedBase == nullptr) {
      std::optional<uint64_t> NewOffset =
          getOffsetFromBase(NewPtr, RangeCheckedBase);
      std::optional<uint64_t> OldOffset =
          getOffsetFromBase(OldPtr, RangeCheckedBase);
      if (!NewOffset.has_value() || !OldOffset.has_value() ||
          *NewOffset <= *OldOffset) {
        return std::nullopt;
      }
      SharedBase = RangeCheckedBase;
    }
    if (!isSameValue(SharedBase, RangeCheckedBase)) {
      return std::nullopt;
    }
  }

  if (NoWrapCheckedPtr != nullptr && !isSameValue(NewPtr, NoWrapCheckedPtr)) {
    return std::nullopt;
  }

  if (NoWrapLimit != nullptr) {
    auto *SizeConst = dyn_cast<ConstantInt>(Size);
    auto *LimitConst = dyn_cast<ConstantInt>(NoWrapLimit);
    if (SizeConst == nullptr || LimitConst == nullptr) {
      return std::nullopt;
    }
    APInt ExpectedLimit = -SizeConst->getValue();
    if (LimitConst->getValue() != ExpectedLimit) {
      return std::nullopt;
    }
  }
  if (NoWrapInitialFreePointerRoundedBase != nullptr &&
      !isRoundedMemoryAllocationSizeFromBase(
          Size, NoWrapInitialFreePointerRoundedBase)) {
    return std::nullopt;
  }
  if (NoWrapStrictUpperSize.has_value()) {
    auto *SizeConst = dyn_cast<ConstantInt>(Size);
    if (SizeConst == nullptr ||
        SizeConst->getValue() != *NoWrapStrictUpperSize) {
      return std::nullopt;
    }
  }

  bool HasFreePointerStore =
      findFreeMemoryPointerStore(SuccessBlock, NewPtr) != nullptr;
  bool HasUniformInitialFreePointerStore =
      isConstantIntValueOrUniformArg(OldPtr, 128) &&
      findUniformFreeMemoryPointerStore(SuccessBlock, NewPtr) != nullptr;
  bool HasFinalizeAlloc = isFreeMemoryAllocationBase(OldPtr) &&
                          findFinalizeAllocCall(SuccessBlock, OldPtr, Size) !=
                              nullptr;
  bool HasSolidityAllocationMarker =
      isFreeMemoryAllocationBase(OldPtr) &&
      findSolidityMemoryAllocationCall(SuccessBlock, OldPtr, Size) != nullptr;
  if (!isSupportedMemoryAllocationSizeWithUniformArg(Size) ||
      (!HasFreePointerStore && !HasUniformInitialFreePointerStore &&
       !HasFinalizeAlloc && !HasSolidityAllocationMarker &&
       findMemoryPointerStoreForLoad(SuccessBlock, OldPtr, NewPtr) == nullptr &&
       !(UsesOffsetPair && hasMemoryPointerStoreTransition(
                               GuardBlock, SuccessBlock, OldPtr, NewPtr)))) {
    return std::nullopt;
  }

  return CheckedBoundsMatch{"memory_allocation_pointer_bounds",
                            "",
                            nullptr,
                            nullptr,
                            nullptr,
                            RevertMatch.Revert,
                            {OldPtr, Size, NewPtr},
                            RevertMatch.PanicCode,
                            true};
}

std::optional<CheckedBoundsMatch> matchCheckedBoundsGuard(BasicBlock &BB) {
  auto *Br = dyn_cast<BranchInst>(BB.getTerminator());
  if (Br == nullptr || !Br->isConditional() ||
      dependsOnCallTo(Br->getCondition(), "evm_callvalue", 8)) {
    return std::nullopt;
  }

  for (unsigned SuccIdx = 0; SuccIdx < 2; ++SuccIdx) {
    BasicBlock *Failure = Br->getSuccessor(SuccIdx);
    CallBase *Revert = findPanicRevertOnLinearPath(Failure);
    if (Revert == nullptr) {
      continue;
    }

    std::optional<SolidityRevertMatch> RevertMatch =
        getPanicRevertFromMetadata(*Revert);
    if (!RevertMatch.has_value() || RevertMatch->Kind != "panic") {
      continue;
    }

    bool FailureWhenCondTrue = SuccIdx == 0;
    std::optional<NormalizedCondition> FailureCond =
        normalizeCondition(Br->getCondition(), FailureWhenCondTrue);

    CheckedBoundsMatch Match{
        RevertMatch->PanicCode.has_value()
            ? getCheckedBoundsKindForPanicCode(*RevertMatch->PanicCode)
            : StringRef("panic_unknown"),
        "unrecognized_operands",
        Br,
        Br->getSuccessor(1 - SuccIdx),
        Failure,
        Revert,
        {},
        RevertMatch->PanicCode,
        false};

    if (std::optional<CheckedBoundsMatch> MemoryPointerBounds =
            matchMemoryAllocationPointerBounds(
                Br->getCondition(), FailureWhenCondTrue, *RevertMatch,
                Br->getSuccessor(1 - SuccIdx))) {
      MemoryPointerBounds->Branch = Br;
      MemoryPointerBounds->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      MemoryPointerBounds->FailureBlock = Failure;
      return MemoryPointerBounds;
    }

    if (std::optional<CheckedBoundsMatch> StorageEncoding =
            matchStorageBytesEncoding(Br->getCondition(), FailureWhenCondTrue,
                                      *RevertMatch)) {
      StorageEncoding->Branch = Br;
      StorageEncoding->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      StorageEncoding->FailureBlock = Failure;
      return StorageEncoding;
    }

    if (std::optional<CheckedBoundsMatch> Mul =
            matchCheckedMulGuard(Br->getCondition(), FailureWhenCondTrue,
                                 *RevertMatch, Br->getSuccessor(1 - SuccIdx))) {
      Mul->Branch = Br;
      Mul->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      Mul->FailureBlock = Failure;
      return Mul;
    }

    if (std::optional<CheckedBoundsMatch> SignedArithmetic =
            matchSignedCheckedArithmeticGuard(
                Br->getCondition(), FailureWhenCondTrue, *RevertMatch)) {
      SignedArithmetic->Branch = Br;
      SignedArithmetic->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      SignedArithmetic->FailureBlock = Failure;
      return SignedArithmetic;
    }

    if (std::optional<CheckedBoundsMatch> ConstantFalseIncrement =
            matchCleanedConstantFalseIncrementGuard(
                Br->getCondition(), FailureWhenCondTrue, *RevertMatch,
                Br->getSuccessor(1 - SuccIdx))) {
      ConstantFalseIncrement->Branch = Br;
      ConstantFalseIncrement->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      ConstantFalseIncrement->FailureBlock = Failure;
      return ConstantFalseIncrement;
    }

    if (!FailureCond.has_value()) {
      Match.SkipReason = "unsupported_condition";
      return Match;
    }

    if (std::optional<CheckedBoundsMatch> Sub = matchCheckedSubGuard(
            *FailureCond, *RevertMatch, Br->getSuccessor(1 - SuccIdx))) {
      Sub->Branch = Br;
      Sub->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      Sub->FailureBlock = Failure;
      return Sub;
    }
    if (std::optional<CheckedBoundsMatch> ConstSub =
            matchCheckedConstSubLimitGuard(*FailureCond, *RevertMatch,
                                           Br->getSuccessor(1 - SuccIdx))) {
      ConstSub->Branch = Br;
      ConstSub->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      ConstSub->FailureBlock = Failure;
      return ConstSub;
    }

    if (std::optional<CheckedBoundsMatch> Arithmetic = matchCheckedArithmetic(
            *FailureCond, *RevertMatch, Br->getSuccessor(1 - SuccIdx))) {
      Arithmetic->Branch = Br;
      Arithmetic->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      Arithmetic->FailureBlock = Failure;
      return Arithmetic;
    }

    if (std::optional<CheckedBoundsMatch> Exp = matchPowerOfTwoExpGuard(
            *FailureCond, *RevertMatch, Br->getSuccessor(1 - SuccIdx))) {
      Exp->Branch = Br;
      Exp->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      Exp->FailureBlock = Failure;
      return Exp;
    }

    if (std::optional<CheckedBoundsMatch> Bounds = matchArrayBounds(
            *FailureCond, *RevertMatch, Br->getSuccessor(1 - SuccIdx))) {
      Bounds->Branch = Br;
      Bounds->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      Bounds->FailureBlock = Failure;
      return Bounds;
    }

    if (std::optional<CheckedBoundsMatch> EnumConversion =
            matchEnumConversion(*FailureCond, *RevertMatch)) {
      EnumConversion->Branch = Br;
      EnumConversion->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      EnumConversion->FailureBlock = Failure;
      return EnumConversion;
    }

    if (std::optional<CheckedBoundsMatch> EmptyArrayPop = matchEmptyArrayPop(
            *FailureCond, *RevertMatch, Br->getSuccessor(1 - SuccIdx))) {
      EmptyArrayPop->Branch = Br;
      EmptyArrayPop->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      EmptyArrayPop->FailureBlock = Failure;
      return EmptyArrayPop;
    }

    if (std::optional<CheckedBoundsMatch> MemoryBounds =
            matchMemoryAllocationBounds(*FailureCond, *RevertMatch,
                                        Br->getSuccessor(1 - SuccIdx))) {
      MemoryBounds->Branch = Br;
      MemoryBounds->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      MemoryBounds->FailureBlock = Failure;
      return MemoryBounds;
    }

    if (std::optional<CheckedBoundsMatch> StorageByteArrayLength =
            matchStorageByteArrayLengthBounds(*FailureCond, *RevertMatch,
                                              Br->getSuccessor(1 - SuccIdx))) {
      StorageByteArrayLength->Branch = Br;
      StorageByteArrayLength->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      StorageByteArrayLength->FailureBlock = Failure;
      return StorageByteArrayLength;
    }

    if (std::optional<CheckedBoundsMatch> StorageArrayLength =
            matchStorageArrayLengthBounds(*FailureCond, *RevertMatch,
                                          Br->getSuccessor(1 - SuccIdx))) {
      StorageArrayLength->Branch = Br;
      StorageArrayLength->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      StorageArrayLength->FailureBlock = Failure;
      return StorageArrayLength;
    }

    if (std::optional<CheckedBoundsMatch> DirectMemoryPointerBounds =
            matchDirectAllocationPointerBounds(
                *FailureCond, *RevertMatch, Br->getSuccessor(1 - SuccIdx))) {
      DirectMemoryPointerBounds->Branch = Br;
      DirectMemoryPointerBounds->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      DirectMemoryPointerBounds->FailureBlock = Failure;
      return DirectMemoryPointerBounds;
    }

    if (std::optional<CheckedBoundsMatch> FixedMemoryPointerBounds =
            matchFixedMemoryAllocationPointerBounds(
                *FailureCond, *RevertMatch, Br->getSuccessor(1 - SuccIdx))) {
      FixedMemoryPointerBounds->Branch = Br;
      FixedMemoryPointerBounds->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      FixedMemoryPointerBounds->FailureBlock = Failure;
      return FixedMemoryPointerBounds;
    }

    if (std::optional<CheckedBoundsMatch> ZeroSizeMemoryPointerBounds =
            matchZeroSizeMemoryAllocationPointerBounds(
                *FailureCond, *RevertMatch, Br->getSuccessor(1 - SuccIdx))) {
      ZeroSizeMemoryPointerBounds->Branch = Br;
      ZeroSizeMemoryPointerBounds->SuccessBlock = Br->getSuccessor(1 - SuccIdx);
      ZeroSizeMemoryPointerBounds->FailureBlock = Failure;
      return ZeroSizeMemoryPointerBounds;
    }

    return Match;
  }

  return std::nullopt;
}

} // namespace notdec::passes::evm::detail
