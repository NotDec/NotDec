#include "Passes/evm/SolidityPatternUtils.h"

#include <limits>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
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
#include <string>

using namespace llvm;

namespace notdec::passes::evm {

const char *KIND_SOLIDITY_NONPAYABLE = "notdec.solidity.nonpayable";
const char *KIND_SOLIDITY_PAYABILITY_GUARD = "notdec.solidity.payability_guard";
const char *KIND_SOLIDITY_ABI_RETURN = "notdec.solidity.abi_return";
const char *KIND_SOLIDITY_REVERT = "notdec.solidity.revert";
const char *KIND_SOLIDITY_CHECKED_BOUNDS = "notdec.solidity.checked_bounds";
const char *KIND_SOLIDITY_EVENT = "notdec.solidity.event";

namespace detail {

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

Value *getIntToPtrAddress(Value *Ptr) {
  if (isa_and_nonnull<ConstantPointerNull>(Ptr)) {
    return ConstantInt::get(Type::getIntNTy(Ptr->getContext(), 256), 0);
  }
  auto *Cast = dyn_cast_or_null<IntToPtrInst>(Ptr);
  if (Cast != nullptr) {
    return Cast->getOperand(0);
  }
  auto *ConstExpr = dyn_cast_or_null<ConstantExpr>(Ptr);
  if (ConstExpr != nullptr && ConstExpr->getOpcode() == Instruction::IntToPtr) {
    return ConstExpr->getOperand(0);
  }
  // MemoryBufferRewritePass rewrites some EVM memory objects to native LLVM
  // pointers.  Keep those direct pointer loads/stores visible to the remaining
  // Solidity pattern matchers without reconstructing old memory markers.
  if (Ptr != nullptr && Ptr->getType()->isPointerTy()) {
    return Ptr;
  }
  return nullptr;
}

std::optional<EvmMemoryLoad> matchEvmMemoryLoad(Value *V) {
  auto *Load = dyn_cast_or_null<LoadInst>(V);
  if (Load == nullptr ||
      Load->getType() != Type::getIntNTy(Load->getContext(), 256)) {
    return std::nullopt;
  }
  Value *Address = getIntToPtrAddress(Load->getPointerOperand());
  if (Address == nullptr) {
    return std::nullopt;
  }
  return EvmMemoryLoad{Load, Address, Load};
}

std::optional<EvmMemoryStore> matchEvmMemoryStore(Instruction *I) {
  auto *Store = dyn_cast_or_null<StoreInst>(I);
  if (Store == nullptr) {
    return std::nullopt;
  }
  Value *Address = getIntToPtrAddress(Store->getPointerOperand());
  if (Address == nullptr) {
    return std::nullopt;
  }
  Type *ValueType = Store->getValueOperand()->getType();
  unsigned Bits =
      ValueType->isIntegerTy() ? ValueType->getIntegerBitWidth() : 0;
  if (Bits != 8 && Bits != 256) {
    return std::nullopt;
  }
  return EvmMemoryStore{Store, Address, Store->getValueOperand(), Bits};
}

bool isPrivateHelperCall(const CallBase *Call) {
  if (Call == nullptr) {
    return false;
  }
  const Function *Callee = Call->getCalledFunction();
  return Callee != nullptr && Callee->getName().starts_with("private__");
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

bool isUInt64Limit(const Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr || C->getBitWidth() <= 64) {
    return false;
  }
  APInt Limit(C->getBitWidth(), 1);
  Limit <<= 64;
  return C->getValue() == Limit;
}

bool isUInt64LimitValue(const Value *V) {
  if (isUInt64Limit(V)) {
    return true;
  }

  auto *Shift = dyn_cast_or_null<CallBase>(V);
  return Shift != nullptr && isCallTo(Shift, "evm_shl") &&
         Shift->arg_size() == 2 &&
         isConstantIntValue(Shift->getArgOperand(0), 64) &&
         isConstantIntValue(Shift->getArgOperand(1), 1);
}

bool isUInt64Max(const Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C != nullptr) {
    if (C->getBitWidth() < 64) {
      return false;
    }
    APInt Max(C->getBitWidth(), 0);
    Max.setLowBits(64);
    return C->getValue() == Max;
  }

  auto *Add = dyn_cast_or_null<BinaryOperator>(V);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return false;
  }

  Value *MaybeShift = nullptr;
  auto *LHS = dyn_cast<ConstantInt>(Add->getOperand(0));
  auto *RHS = dyn_cast<ConstantInt>(Add->getOperand(1));
  if (LHS != nullptr && LHS->isMinusOne()) {
    MaybeShift = Add->getOperand(1);
  } else if (RHS != nullptr && RHS->isMinusOne()) {
    MaybeShift = Add->getOperand(0);
  }
  auto *Shift = dyn_cast_or_null<CallBase>(MaybeShift);
  return Shift != nullptr && isCallTo(Shift, "evm_shl") &&
         Shift->arg_size() == 2 &&
         isConstantIntValue(Shift->getArgOperand(0), 64) &&
         isConstantIntValue(Shift->getArgOperand(1), 1);
}

std::optional<uint64_t> matchUInt64LimitMinus(const Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr || C->getBitWidth() <= 64) {
    return std::nullopt;
  }

  APInt Limit(C->getBitWidth(), 1);
  Limit <<= 64;
  if (C->getValue().uge(Limit)) {
    return std::nullopt;
  }

  APInt Diff = Limit - C->getValue();
  if (Diff.getActiveBits() > 64) {
    return std::nullopt;
  }

  uint64_t Size = Diff.getZExtValue();
  if (Size == 0 || Size > 4096 || Size % 32 != 0) {
    return std::nullopt;
  }
  return Size;
}

std::optional<uint64_t> matchUInt64LimitMinusStrictUpper(const Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr || C->getBitWidth() <= 64) {
    return std::nullopt;
  }

  APInt Limit(C->getBitWidth(), 1);
  Limit <<= 64;
  if (C->getValue().uge(Limit)) {
    return std::nullopt;
  }

  APInt Diff = Limit - C->getValue() - 1;
  if (Diff.getActiveBits() > 64) {
    return std::nullopt;
  }

  uint64_t Size = Diff.getZExtValue();
  if (Size == 0 || Size > 4096 || Size % 32 != 0) {
    return std::nullopt;
  }
  return Size;
}

std::optional<uint64_t> matchUInt64LimitMinusStrictUpperAllowZero(
    const Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr || C->getBitWidth() <= 64) {
    return std::nullopt;
  }

  APInt Limit(C->getBitWidth(), 1);
  Limit <<= 64;
  if (C->getValue().uge(Limit)) {
    return std::nullopt;
  }

  APInt Diff = Limit - C->getValue() - 1;
  if (Diff.getActiveBits() > 64) {
    return std::nullopt;
  }

  uint64_t Size = Diff.getZExtValue();
  if (Size > 4096 || Size % 32 != 0) {
    return std::nullopt;
  }
  return Size;
}

std::optional<uint64_t> matchWrappingAddStrictUpper(const Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr) {
    return std::nullopt;
  }

  APInt Size = ~C->getValue();
  if (Size.getActiveBits() > 64) {
    return std::nullopt;
  }

  uint64_t Size64 = Size.getZExtValue();
  if (Size64 == 0 || Size64 > 4096 || Size64 % 32 != 0) {
    return std::nullopt;
  }
  return Size64;
}

std::optional<uint64_t> matchWrappingPointerStrictUpper(const Value *V) {
  auto *ConstExpr = dyn_cast_or_null<ConstantExpr>(V);
  if (ConstExpr == nullptr ||
      ConstExpr->getOpcode() != Instruction::IntToPtr) {
    return std::nullopt;
  }
  auto *C = dyn_cast_or_null<ConstantInt>(ConstExpr->getOperand(0));
  if (C == nullptr) {
    return std::nullopt;
  }

  APInt Size = -C->getValue();
  if (Size.getActiveBits() > 64) {
    return std::nullopt;
  }

  uint64_t Size64 = Size.getZExtValue();
  if (Size64 == 0 || Size64 > 4096 || Size64 % 32 != 0) {
    return std::nullopt;
  }
  return Size64;
}

bool isMinus32(const Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr) {
    return false;
  }
  const APInt &Value = C->getValue();
  return Value == APInt(Value.getBitWidth(), -32, true);
}

bool isAllOnes(const Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  return C != nullptr && C->isMinusOne();
}

bool isSmallUnsignedMax(const ConstantInt *C) {
  if (C == nullptr) {
    return false;
  }
  APInt Limit = C->getValue() + 1;
  return Limit.isPowerOf2() && Limit.ugt(1) &&
         Limit.getActiveBits() < C->getBitWidth();
}

bool isConstantWithin(Value *V, const APInt &Max) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  return C != nullptr && C->getValue().ule(Max);
}

bool isConstantEqual(Value *V, const APInt &Expected) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  return C != nullptr && C->getValue() == Expected;
}

bool isUnsignedCleanupToMax(Value *V, const APInt &Max) {
  if (isConstantWithin(V, Max)) {
    return true;
  }

  auto *And = dyn_cast_or_null<BinaryOperator>(V);
  if (And == nullptr || And->getOpcode() != Instruction::And) {
    return false;
  }
  return isConstantEqual(And->getOperand(0), Max) ||
         isConstantEqual(And->getOperand(1), Max);
}

bool isPowerOfTwoMinusOne(Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C != nullptr) {
    APInt Limit = C->getValue() + 1;
    return Limit.isPowerOf2() && Limit.ugt(1) &&
           Limit.getActiveBits() < C->getBitWidth();
  }

  auto *Add = dyn_cast_or_null<BinaryOperator>(V);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return false;
  }

  Value *MaybeShift = nullptr;
  auto *LHS = dyn_cast<ConstantInt>(Add->getOperand(0));
  auto *RHS = dyn_cast<ConstantInt>(Add->getOperand(1));
  if (LHS != nullptr && LHS->isMinusOne()) {
    MaybeShift = Add->getOperand(1);
  } else if (RHS != nullptr && RHS->isMinusOne()) {
    MaybeShift = Add->getOperand(0);
  }

  auto *Shift = dyn_cast_or_null<CallBase>(MaybeShift);
  if (Shift == nullptr || !isCallTo(Shift, "evm_shl") ||
      Shift->arg_size() != 2 ||
      !isConstantIntValue(Shift->getArgOperand(1), 1)) {
    return false;
  }
  auto *Bits = dyn_cast<ConstantInt>(Shift->getArgOperand(0));
  return Bits != nullptr && Bits->getValue().ugt(1) &&
         Bits->getValue().ult(256);
}

bool isUnsignedCleanupToMaxValue(Value *V, Value *Max) {
  if (V == nullptr || Max == nullptr) {
    return false;
  }
  if (auto *MaxConst = dyn_cast<ConstantInt>(Max)) {
    return isUnsignedCleanupToMax(V, MaxConst->getValue());
  }

  auto *And = dyn_cast_or_null<BinaryOperator>(V);
  return And != nullptr && And->getOpcode() == Instruction::And &&
         (And->getOperand(0) == Max || And->getOperand(1) == Max);
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

std::optional<uint64_t> getUInt64Metadata(const Instruction &I,
                                          StringRef Kind) {
  auto *Node = I.getMetadata(Kind);
  if (Node == nullptr || Node->getNumOperands() != 1) {
    return std::nullopt;
  }

  auto *Value = dyn_cast<MDString>(Node->getOperand(0));
  if (Value == nullptr) {
    return std::nullopt;
  }

  uint64_t Parsed = 0;
  if (Value->getString().getAsInteger(10, Parsed)) {
    return std::nullopt;
  }
  return Parsed;
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

  auto *KindCode = ConstantInt::get(Type::getIntNTy(Ctx, 256),
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
  Value *Args[] = {ConstantInt::get(Type::getIntNTy(Ctx, 256),
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

bool isReturndataSize(Value *V) { return isCallTo(V, "evm_returndatasize"); }

bool isFreeMemoryPointerLoad(Value *V) {
  std::optional<EvmMemoryLoad> Load = matchEvmMemoryLoad(V);
  return Load.has_value() && isConstantIntValue(Load->Address, 64);
}

bool isFreeMemoryPointerStore(Instruction *I) {
  std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(I);
  if (Store.has_value() && isConstantIntValue(Store->Address, 64)) {
    return true;
  }
  auto *Call = dyn_cast_or_null<CallBase>(I);
  return Call != nullptr && isCallTo(Call, "notdec_evm_finalize_alloc") &&
         Call->arg_size() == 2;
}

bool isFreeMemoryAllocationBase(Value *V) {
  auto *Call = dyn_cast_or_null<CallBase>(V);
  if (Call != nullptr &&
      ((isCallTo(Call, "notdec_evm_alloc") && Call->arg_size() == 1) ||
       (isCallTo(Call, "notdec_evm_alloc_unbounded") &&
        Call->arg_size() == 0))) {
    return true;
  }

  auto *PtrToInt = dyn_cast_or_null<PtrToIntInst>(V);
  Call = PtrToInt == nullptr
             ? nullptr
             : dyn_cast_or_null<CallBase>(PtrToInt->getOperand(0));
  return Call != nullptr &&
         ((isCallTo(Call, "calloc") && Call->arg_size() == 2) ||
          (isCallTo(Call, "calloc_unbounded") && Call->arg_size() == 0));
}

bool isSameOrReloadedFreeMemoryBase(Value *LHS, Value *RHS) {
  if (LHS == RHS) {
    return true;
  }
  bool LHSFreeBase =
      isFreeMemoryPointerLoad(LHS) || isFreeMemoryAllocationBase(LHS);
  bool RHSFreeBase =
      isFreeMemoryPointerLoad(RHS) || isFreeMemoryAllocationBase(RHS);
  return LHSFreeBase && RHSFreeBase;
}

bool isSameValue(Value *LHS, Value *RHS) {
  if (LHS == RHS) {
    return true;
  }
  auto *LC = dyn_cast_or_null<ConstantInt>(LHS);
  auto *RC = dyn_cast_or_null<ConstantInt>(RHS);
  if (LC != nullptr && RC != nullptr) {
    return LC->getValue() == RC->getValue();
  }

  std::optional<EvmMemoryLoad> LLoad = matchEvmMemoryLoad(LHS);
  std::optional<EvmMemoryLoad> RLoad = matchEvmMemoryLoad(RHS);
  return LLoad.has_value() && RLoad.has_value() &&
         isSameValue(LLoad->Address, RLoad->Address);
}

std::optional<uint64_t> getUInt64Constant(Value *V) {
  auto *C = dyn_cast_or_null<ConstantInt>(V);
  if (C == nullptr || C->getValue().getActiveBits() > 64) {
    return std::nullopt;
  }
  return C->getZExtValue();
}

std::optional<uint64_t> getOffsetFromBase(Value *Offset, Value *Base) {
  if (isSameValue(Offset, Base)) {
    return 0;
  }

  auto *OffsetConst = dyn_cast_or_null<ConstantInt>(Offset);
  auto *BaseConst = dyn_cast_or_null<ConstantInt>(Base);
  if (OffsetConst != nullptr && BaseConst != nullptr &&
      OffsetConst->getValue().uge(BaseConst->getValue())) {
    APInt Diff = OffsetConst->getValue() - BaseConst->getValue();
    if (Diff.getActiveBits() <= 64) {
      return Diff.getZExtValue();
    }
  }

  auto *Add = dyn_cast_or_null<BinaryOperator>(Offset);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return std::nullopt;
  }

  for (unsigned I = 0; I < 2; ++I) {
    if (!isSameValue(Add->getOperand(I), Base)) {
      continue;
    }
    return getUInt64Constant(Add->getOperand(1 - I));
  }
  return std::nullopt;
}

std::optional<uint64_t> getSelectorWord(Value *V) {
  auto *Selector = dyn_cast_or_null<CallBase>(V);
  if (Selector == nullptr || !isCallTo(Selector, "evm_shl") ||
      Selector->arg_size() != 2) {
    return std::nullopt;
  }

  auto *Shift = dyn_cast_or_null<ConstantInt>(Selector->getArgOperand(0));
  auto *Payload = dyn_cast_or_null<ConstantInt>(Selector->getArgOperand(1));
  if (Shift == nullptr || Payload == nullptr ||
      Shift->getValue().getActiveBits() > 8) {
    return std::nullopt;
  }

  // Solidity may emit PUSH3/PUSH4 plus SHL to place the 4-byte selector in the
  // top bytes of the ABI word.  Decode the stored word instead of matching one
  // fixed shift amount.
  APInt Word = Payload->getValue().zextOrTrunc(256);
  Word <<= Shift->getZExtValue();
  if (Word.trunc(224) != 0) {
    return std::nullopt;
  }
  return Word.lshr(224).getZExtValue();
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

bool dependsOnCallTo(Value *V, StringRef Name, unsigned Depth) {
  SmallPtrSet<Value *, 16> Seen;
  return dependsOnCallTo(V, Name, Depth, Seen);
}

bool dependsOnValue(Value *V, Value *Target, unsigned Depth,
                    SmallPtrSetImpl<Value *> &Seen) {
  if (V == Target) {
    return true;
  }
  if (V == nullptr || Depth == 0 || !Seen.insert(V).second) {
    return false;
  }

  auto *UserValue = dyn_cast<User>(V);
  if (UserValue == nullptr) {
    return false;
  }
  for (Value *Op : UserValue->operands()) {
    if (dependsOnValue(Op, Target, Depth - 1, Seen)) {
      return true;
    }
  }
  return false;
}

bool dependsOnValue(Value *V, Value *Target, unsigned Depth) {
  SmallPtrSet<Value *, 16> Seen;
  return dependsOnValue(V, Target, Depth, Seen);
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

bool expressionHasPackedStorageOp(Value *V, unsigned Depth) {
  SmallPtrSet<Value *, 16> Seen;
  return expressionHasPackedStorageOp(V, Depth, Seen);
}

std::optional<StorageScratchKeccakMatch>
matchStorageScratchKeccak(CallBase &Sha3) {
  if (!isCallTo(&Sha3, "evm_sha3") || Sha3.arg_size() != 3 ||
      !isConstantIntValue(Sha3.getArgOperand(1), 0) ||
      !isConstantIntValue(Sha3.getArgOperand(2), 64)) {
    return std::nullopt;
  }

  Value *Key = nullptr;
  Value *BaseSlot = nullptr;
  for (auto It = Sha3.getIterator(); It != Sha3.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call != nullptr &&
        (isCallTo(Call, "evm_sha3") ||
         !classifyExternalCall(getCalleeName(Call)).empty())) {
      break;
    }
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&*It);
    if (!Store.has_value()) {
      continue;
    }
    if (BaseSlot == nullptr && isConstantIntValue(Store->Address, 32)) {
      BaseSlot = Store->StoredValue;
      continue;
    }
    if (Key == nullptr && isConstantIntValue(Store->Address, 0)) {
      Key = Store->StoredValue;
      continue;
    }
    if (Key != nullptr && BaseSlot != nullptr) {
      break;
    }
  }

  if (Key == nullptr || BaseSlot == nullptr) {
    return std::nullopt;
  }
  return StorageScratchKeccakMatch{&Sha3, Key, BaseSlot};
}

std::optional<StorageArrayDataKeccakMatch>
matchStorageArrayDataKeccak(CallBase &Sha3) {
  if (!isCallTo(&Sha3, "evm_sha3") || Sha3.arg_size() != 3 ||
      !isConstantIntValue(Sha3.getArgOperand(1), 0) ||
      !isConstantIntValue(Sha3.getArgOperand(2), 32)) {
    return std::nullopt;
  }

  Value *BaseSlot = nullptr;
  for (auto It = Sha3.getIterator(); It != Sha3.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call != nullptr &&
        (isCallTo(Call, "evm_sha3") ||
         !classifyExternalCall(getCalleeName(Call)).empty())) {
      break;
    }
    std::optional<EvmMemoryStore> Store = matchEvmMemoryStore(&*It);
    if (Store.has_value() && isConstantIntValue(Store->Address, 0)) {
      BaseSlot = Store->StoredValue;
      break;
    }
  }

  if (BaseSlot == nullptr) {
    return std::nullopt;
  }
  return StorageArrayDataKeccakMatch{&Sha3, BaseSlot};
}

std::optional<StorageMappingAccessMatch>
matchStorageMappingAccess(CallBase &Access, Value *StorageSlot,
                          uint64_t AccessKind) {
  for (auto It = Access.getIterator(); It != Access.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call == nullptr) {
      continue;
    }
    if (isCallTo(Call, "evm_sload") || isCallTo(Call, "evm_sstore") ||
        !classifyExternalCall(getCalleeName(Call)).empty()) {
      break;
    }
    if (!isCallTo(Call, "evm_sha3") || Call->arg_size() != 3) {
      continue;
    }
    if (!dependsOnValue(StorageSlot, Call)) {
      break;
    }
    std::optional<StorageScratchKeccakMatch> Scratch =
        matchStorageScratchKeccak(*Call);
    if (!Scratch.has_value()) {
      break;
    }
    return StorageMappingAccessMatch{&Access,           Scratch->Key,
                                     Scratch->BaseSlot, Scratch->Sha3,
                                     StorageSlot,       AccessKind};
  }
  return std::nullopt;
}

std::optional<StorageArrayDataAccessMatch>
matchStorageArrayDataAccess(CallBase &Access, Value *StorageSlot,
                            uint64_t AccessKind) {
  for (auto It = Access.getIterator(); It != Access.getParent()->begin();) {
    --It;
    auto *Call = dyn_cast<CallBase>(&*It);
    if (Call == nullptr) {
      continue;
    }
    if (isCallTo(Call, "evm_sload") || isCallTo(Call, "evm_sstore") ||
        !classifyExternalCall(getCalleeName(Call)).empty()) {
      break;
    }
    if (!isCallTo(Call, "evm_sha3") || Call->arg_size() != 3) {
      continue;
    }
    if (!dependsOnValue(StorageSlot, Call)) {
      break;
    }
    std::optional<StorageArrayDataKeccakMatch> Data =
        matchStorageArrayDataKeccak(*Call);
    if (!Data.has_value()) {
      break;
    }
    return StorageArrayDataAccessMatch{&Access, Data->BaseSlot, Data->Sha3,
                                       StorageSlot, AccessKind};
  }
  return std::nullopt;
}

std::optional<PackedStorageAccessMatch>
matchPackedStorageAccess(CallBase &Access, uint64_t AccessKind) {
  if (AccessKind == 1) {
    if (!isCallTo(&Access, "evm_sload") || Access.arg_size() != 1) {
      return std::nullopt;
    }
    for (User *U : Access.users()) {
      if (expressionHasPackedStorageOp(U)) {
        return PackedStorageAccessMatch{&Access, Access.getArgOperand(0),
                                        AccessKind};
      }
    }
    return std::nullopt;
  }

  if (AccessKind == 2) {
    if (!isCallTo(&Access, "evm_sstore") || Access.arg_size() != 2) {
      return std::nullopt;
    }
    Value *Stored = Access.getArgOperand(1);
    if (dependsOnCallTo(Stored, "evm_sload") &&
        expressionHasPackedStorageOp(Stored)) {
      return PackedStorageAccessMatch{&Access, Access.getArgOperand(0),
                                      AccessKind};
    }
  }
  return std::nullopt;
}

StringRef classifyExternalCall(StringRef Name) {
  return Name == "evm_call"           ? StringRef("call")
         : Name == "evm_staticcall"   ? StringRef("staticcall")
         : Name == "evm_delegatecall" ? StringRef("delegatecall")
         : Name == "evm_callcode"     ? StringRef("callcode")
                                      : StringRef("");
}

} // namespace detail

} // namespace notdec::passes::evm
