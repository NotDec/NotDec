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
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <string>

using namespace llvm;

namespace notdec::passes::evm {

const char *KIND_SOLIDITY_NONPAYABLE = "notdec.solidity.nonpayable";
const char *KIND_SOLIDITY_PAYABILITY_GUARD = "notdec.solidity.payability_guard";
const char *KIND_SOLIDITY_ABI_RETURN = "notdec.solidity.abi_return";
const char *KIND_SOLIDITY_REVERT = "notdec.solidity.revert";
const char *KIND_SOLIDITY_CHECKED_BOUNDS = "notdec.solidity.checked_bounds";
const char *KIND_SOLIDITY_EVENT = "notdec.solidity.event";
const char *KIND_SOLIDITY_CALLDATA_ARG_INDEX =
    "notdec.solidity.calldata.index";
const char *KIND_EVM_YUL_SUMMARY = "notdec.evm.yul_function_summary";

namespace {

// Affine byte offset relative to a function parameter: Param == -1 means a
// plain constant, otherwise Param is the argument index and Constant a delta.
struct YulAffineOffset {
  int Param = -1;
  int64_t Constant = 0;
};

std::optional<YulAffineOffset> resolveYulAffineOffset(Value *V,
                                                      const Function &F,
                                                      unsigned Depth = 0) {
  if (V == nullptr || Depth > 6) {
    return std::nullopt;
  }
  V = V->stripPointerCasts();
  if (auto *C = dyn_cast<ConstantInt>(V)) {
    const APInt &A = C->getValue();
    if (A.getActiveBits() > 62) {
      return std::nullopt;
    }
    return YulAffineOffset{-1, static_cast<int64_t>(A.getSExtValue())};
  }
  if (auto *Arg = dyn_cast<Argument>(V)) {
    if (Arg->getParent() != &F || !Arg->getType()->isIntegerTy()) {
      return std::nullopt;
    }
    return YulAffineOffset{static_cast<int>(Arg->getArgNo()), 0};
  }
  auto *BO = dyn_cast<BinaryOperator>(V);
  if (BO == nullptr) {
    return std::nullopt;
  }
  auto L = resolveYulAffineOffset(BO->getOperand(0), F, Depth + 1);
  auto R = resolveYulAffineOffset(BO->getOperand(1), F, Depth + 1);
  if (!L.has_value() || !R.has_value()) {
    return std::nullopt;
  }
  if (BO->getOpcode() == Instruction::Add) {
    if (L->Param >= 0 && R->Param >= 0) {
      return std::nullopt;
    }
    YulAffineOffset Out = L->Param >= 0 ? *L : *R;
    Out.Constant += (L->Param >= 0 ? R->Constant : L->Constant);
    return Out;
  }
  if (BO->getOpcode() == Instruction::Sub && L->Param >= 0 && R->Param < 0) {
    return YulAffineOffset{L->Param, L->Constant - R->Constant};
  }
  return std::nullopt;
}

std::string formatYulOffset(const YulAffineOffset &Off) {
  std::string Text = Off.Param >= 0 ? ("p" + std::to_string(Off.Param)) : "";
  if (Off.Constant != 0 || Off.Param < 0) {
    if (Off.Constant < 0) {
      Text += "-" + std::to_string(-Off.Constant);
    } else {
      if (!Text.empty()) {
        Text += "+";
      }
      Text += std::to_string(Off.Constant);
    }
  }
  return Text.empty() ? std::string("0") : Text;
}

struct YulFunctionShape {
  bool Recognized = false;
  int ReturnLabelArg = -1;
  unsigned RealParams = 0;
};

YulFunctionShape classifyYulFunction(Function &F) {
  YulFunctionShape Shape;
  if (F.isDeclaration() || !F.hasInternalLinkage() || F.arg_size() < 5) {
    return Shape;
  }
  for (unsigned I = 0; I < 4; ++I) {
    if (!F.getArg(I)->getType()->isPointerTy()) {
      return Shape;
    }
  }
  if (F.getArg(1)->getName() != "calldata") {
    return Shape;
  }
  // The EVM return label survives as a trailing argument with no dataflow use
  // in the lifted body.  Find the last such integer argument.
  for (int I = static_cast<int>(F.arg_size()) - 1; I >= 4; --I) {
    Argument *A = F.getArg(static_cast<unsigned>(I));
    if (A->getType()->isIntegerTy() && A->use_empty()) {
      Shape.ReturnLabelArg = I;
      break;
    }
  }
  Shape.Recognized = true;
  Shape.RealParams = F.arg_size() - 4 - (Shape.ReturnLabelArg >= 0 ? 1 : 0);
  return Shape;
}

std::map<unsigned, Value *> getYulReturnedElements(Function &F) {
  std::map<unsigned, Value *> Elements;
  if (F.getReturnType()->isVoidTy()) {
    return Elements;
  }
  if (!F.getReturnType()->isAggregateType()) {
    for (BasicBlock &BB : F) {
      if (auto *RI = dyn_cast<ReturnInst>(BB.getTerminator())) {
        if (Value *RV = RI->getReturnValue()) {
          Elements[0] = RV;
        }
      }
    }
    return Elements;
  }
  Value *Agg = nullptr;
  for (BasicBlock &BB : F) {
    if (auto *RI = dyn_cast<ReturnInst>(BB.getTerminator())) {
      if (RI->getReturnValue() != nullptr) {
        Agg = RI->getReturnValue();
        break;
      }
    }
  }
  if (Agg == nullptr) {
    return Elements;
  }
  std::function<void(Value *)> Walk = [&](Value *V) {
    auto *IV = dyn_cast<InsertValueInst>(V);
    if (IV == nullptr) {
      return;
    }
    Walk(IV->getAggregateOperand());
    if (!IV->getIndices().empty()) {
      Elements[IV->getIndices()[0]] = IV->getInsertedValueOperand();
    }
  };
  Walk(Agg);
  return Elements;
}

} // namespace

PreservedAnalyses EvmYulFunctionSummaryPass::run(Module &M,
                                                 ModuleAnalysisManager &) {
  std::map<const Function *, YulFunctionShape> Shapes;
  for (Function &F : M) {
    YulFunctionShape Shape = classifyYulFunction(F);
    if (Shape.Recognized) {
      Shapes.emplace(&F, Shape);
    }
  }

  bool Changed = false;
  for (Function &F : M) {
    auto ShapeIt = Shapes.find(&F);
    if (ShapeIt == Shapes.end()) {
      continue;
    }
    const YulFunctionShape &Shape = ShapeIt->second;
    Argument *Calldata = F.getArg(1);

    std::vector<std::pair<YulAffineOffset, const Value *>> Effects;
    unsigned UnresolvedLoads = 0;
    unsigned TotalLoads = 0;
    for (Instruction &I : instructions(F)) {
      auto *Call = dyn_cast<CallBase>(&I);
      if (Call == nullptr || !detail::isCallTo(Call, "evm_calldataload") ||
          Call->arg_size() != 2) {
        continue;
      }
      if (Call->getArgOperand(0)->stripPointerCasts() != Calldata) {
        continue;
      }
      ++TotalLoads;
      auto Off = resolveYulAffineOffset(Call->getArgOperand(1), F);
      if (!Off.has_value()) {
        ++UnresolvedLoads;
        continue;
      }
      Effects.emplace_back(*Off, Call);
    }

    std::map<unsigned, Value *> ReturnedElements = getYulReturnedElements(F);
    std::map<const Value *, unsigned> ElementOfValue;
    for (const auto &Ent : ReturnedElements) {
      ElementOfValue.emplace(Ent.second, Ent.first);
    }

    unsigned CallSites = 0;
    unsigned ConstLabelCalls = 0;
    if (Shape.ReturnLabelArg >= 0) {
      for (User *U : F.users()) {
        auto *CB = dyn_cast<CallBase>(U);
        if (CB == nullptr || CB->getCalledFunction() != &F ||
            static_cast<unsigned>(Shape.ReturnLabelArg) >= CB->arg_size()) {
          continue;
        }
        ++CallSites;
        if (isa<ConstantInt>(CB->getArgOperand(Shape.ReturnLabelArg))) {
          ++ConstLabelCalls;
        }
      }
    }

    unsigned YulCalls = 0;
    for (Instruction &I : instructions(F)) {
      auto *CB = dyn_cast<CallBase>(&I);
      if (CB == nullptr) {
        continue;
      }
      Function *Callee = CB->getCalledFunction();
      if (Callee != nullptr && Shapes.count(Callee) != 0) {
        ++YulCalls;
      }
    }

    std::ostringstream OS;
    OS << "v1;params=" << Shape.RealParams
       << ";ret=" << ReturnedElements.size()
       << ";label=" << Shape.ReturnLabelArg
       << ";loads=" << TotalLoads
       << ";unresolved=" << UnresolvedLoads
       << ";yulcalls=" << YulCalls
       << ";labelconst=" << ConstLabelCalls << "/" << CallSites
       << ";calldata=";
    bool First = true;
    for (const auto &[Off, Load] : Effects) {
      if (!First) {
        OS << ",";
      }
      First = false;
      OS << formatYulOffset(Off);
      auto It = ElementOfValue.find(Load);
      if (It != ElementOfValue.end()) {
        OS << ":r" << It->second;
      }
    }

    F.setMetadata(KIND_EVM_YUL_SUMMARY,
                  MDNode::get(M.getContext(),
                              MDString::get(M.getContext(), OS.str())));
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
PreservedAnalyses EvmCalldataViewCleanupPass::run(Module &M,
                                                   ModuleAnalysisManager &) {
  bool Changed = false;
  for (Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    SmallVector<CallInst *, 4> Views;
    for (Instruction &I : instructions(F)) {
      auto *Call = dyn_cast<CallInst>(&I);
      if (Call != nullptr && detail::isCallTo(Call, detail::kEvmCalldataViewName)) {
        Views.push_back(Call);
      }
    }
    for (CallInst *Call : Views) {
      Call->replaceAllUsesWith(Call->getArgOperand(0));
      Call->eraseFromParent();
      Changed = true;
    }
  }
  if (Function *Decl = M.getFunction(detail::kEvmCalldataViewName)) {
    if (Decl->isDeclaration() && Decl->use_empty()) {
      Decl->eraseFromParent();
      Changed = true;
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

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

Argument *getEvmCalldataArgument(Function &F) {
  if (F.arg_size() < 2) {
    return nullptr;
  }
  auto It = F.arg_begin();
  ++It;
  Argument &Arg = *It;
  if (!Arg.getType()->isPointerTy() || Arg.getName() != "calldata") {
    return nullptr;
  }
  return &Arg;
}

CallInst *getEvmCalldataViewCall(Function &F) {
  if (F.isDeclaration()) {
    return nullptr;
  }
  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallInst>(&I);
    if (Call != nullptr && isCallTo(Call, kEvmCalldataViewName)) {
      return Call;
    }
  }
  return nullptr;
}

Value *getEvmCalldataRoot(Function &F) {
  Argument *Arg = getEvmCalldataArgument(F);
  if (Arg == nullptr) {
    return nullptr;
  }
  if (CallInst *View = getEvmCalldataViewCall(F)) {
    return View;
  }
  return Arg;
}

bool isEvmPrivateHelperFunction(const Function &F) {
  if (F.isDeclaration() || F.getName().starts_with("public_")) {
    return false;
  }
  return F.hasInternalLinkage();
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
  return Callee != nullptr && isEvmPrivateHelperFunction(*Callee);
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
  // InstCombine folds the selector shl into the stored ABI word, so accept a
  // folded constant as well as the pre-fold evm_shl call.  A zero word is not a
  // selector: panic code 0 also looks like a low-224-zero constant and must stay
  // available as payload evidence.
  if (const auto *Constant = dyn_cast_or_null<ConstantInt>(V)) {
    APInt Word = Constant->getValue().zextOrTrunc(256);
    if (Word.trunc(224) != 0) {
      return std::nullopt;
    }
    uint64_t Selector = Word.lshr(224).getZExtValue();
    if (Selector == 0) {
      return std::nullopt;
    }
    return Selector;
  }

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

} // namespace detail

} // namespace notdec::passes::evm
