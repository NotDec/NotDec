#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/Metadata.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Pass.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Transforms/Utils/Local.h>

#include <limits>

#define DEBUG_TYPE "evm-calldata-access"

using namespace llvm;

STATISTIC(NumCalldataLoadsRewritten,
          "Number of evm_calldataload calls rewritten to LLVM loads");
STATISTIC(NumCalldataCopiesRewritten,
          "Number of evm_calldatacopy calls rewritten to LLVM memcpy calls");
STATISTIC(NumCalldataCallsiteConstantOffsets,
          "Number of calldata offsets resolved from unique callsite constants");
STATISTIC(NumCalldataPolymorphicHelpers,
          "Number of private calldata helpers marked as polymorphic");
STATISTIC(NumCalldataMinSizeGuards,
          "Number of calldata minimum size guards rewritten");

namespace notdec::passes::evm {
namespace {

// A matched guard creates one checked alias on the non-revert edge.  Later
// calldata reads use the strongest alias whose checked block dominates them.
struct CalldataMinSizeGuard {
  BasicBlock *NormalSuccessor = nullptr;
  uint64_t MinSize = 0;
  CallInst *CheckedCalldata = nullptr;
};

bool shouldRewriteFunction(Function &F) {
  return detail::isPublicEntryFunction(F) || detail::isSelectorFunction(F) ||
         F.getName().starts_with("private__");
}

bool shouldMarkPolymorphicFunction(Function &F) {
  return F.getName().starts_with("private__") && !F.isDeclaration();
}

Value *getCalldataArg(Function &F) {
  if (!shouldRewriteFunction(F) || F.arg_size() < 2) {
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

bool usesCurrentCalldata(CallBase &Call, unsigned ArgIndex, Value *Calldata) {
  return Call.arg_size() > ArgIndex && Call.getArgOperand(ArgIndex) == Calldata;
}

std::optional<uint64_t> getUInt64Constant(Value *V) {
  auto *Constant = dyn_cast_or_null<ConstantInt>(V);
  if (Constant == nullptr || Constant->getValue().getActiveBits() > 64) {
    return std::nullopt;
  }
  return Constant->getZExtValue();
}

bool isCalldataSize(Value *V, Value *Calldata) {
  auto *Call = dyn_cast_or_null<CallBase>(V);
  return Call != nullptr && detail::isCallTo(Call, "evm_calldatasize") &&
         usesCurrentCalldata(*Call, 0, Calldata);
}

std::optional<uint64_t> addOne(uint64_t N) {
  if (N == std::numeric_limits<uint64_t>::max()) {
    return std::nullopt;
  }
  return N + 1;
}

std::optional<uint64_t> matchCalldataTooShortICmp(ICmpInst &Cmp,
                                                  bool TakenWhenTrue,
                                                  Value *Calldata) {
  Value *LHS = Cmp.getOperand(0);
  Value *RHS = Cmp.getOperand(1);
  ICmpInst::Predicate Pred = Cmp.getPredicate();

  if (isCalldataSize(LHS, Calldata)) {
    std::optional<uint64_t> Bound = getUInt64Constant(RHS);
    if (!Bound.has_value()) {
      return std::nullopt;
    }
    if ((Pred == ICmpInst::ICMP_ULT && TakenWhenTrue) ||
        (Pred == ICmpInst::ICMP_UGE && !TakenWhenTrue)) {
      return *Bound;
    }
    if ((Pred == ICmpInst::ICMP_ULE && TakenWhenTrue) ||
        (Pred == ICmpInst::ICMP_UGT && !TakenWhenTrue)) {
      return addOne(*Bound);
    }
    return std::nullopt;
  }

  if (isCalldataSize(RHS, Calldata)) {
    std::optional<uint64_t> Bound = getUInt64Constant(LHS);
    if (!Bound.has_value()) {
      return std::nullopt;
    }
    if ((Pred == ICmpInst::ICMP_UGT && TakenWhenTrue) ||
        (Pred == ICmpInst::ICMP_ULE && !TakenWhenTrue)) {
      return *Bound;
    }
    if ((Pred == ICmpInst::ICMP_UGE && TakenWhenTrue) ||
        (Pred == ICmpInst::ICMP_ULT && !TakenWhenTrue)) {
      return addOne(*Bound);
    }
  }

  return std::nullopt;
}

std::optional<uint64_t> matchCalldataTooShort(Value *V, bool TakenWhenTrue,
                                              Value *Calldata,
                                              unsigned Depth = 0) {
  if (V == nullptr || Depth > 8) {
    return std::nullopt;
  }

  if (auto *Cast = dyn_cast<ZExtInst>(V)) {
    return matchCalldataTooShort(Cast->getOperand(0), TakenWhenTrue, Calldata,
                                 Depth + 1);
  }

  auto *Cmp = dyn_cast<ICmpInst>(V);
  if (Cmp == nullptr) {
    return std::nullopt;
  }

  if (Cmp->getType()->isIntegerTy(1)) {
    for (unsigned ZeroIndex = 0; ZeroIndex < 2; ++ZeroIndex) {
      if (!detail::isConstantIntValue(Cmp->getOperand(ZeroIndex), 0)) {
        continue;
      }
      Value *BoolOperand = Cmp->getOperand(1 - ZeroIndex);
      if (BoolOperand->getType()->isIntegerTy(1) ||
          BoolOperand->getType()->isIntegerTy(256)) {
        if (Cmp->getPredicate() == ICmpInst::ICMP_EQ) {
          return matchCalldataTooShort(BoolOperand, !TakenWhenTrue, Calldata,
                                       Depth + 1);
        }
        if (Cmp->getPredicate() == ICmpInst::ICMP_NE) {
          return matchCalldataTooShort(BoolOperand, TakenWhenTrue, Calldata,
                                       Depth + 1);
        }
      }
    }
  }

  return matchCalldataTooShortICmp(*Cmp, TakenWhenTrue, Calldata);
}

bool isNoPayloadRevertBlock(BasicBlock *BB) {
  if (BB == nullptr) {
    return false;
  }
  for (Instruction &I : *BB) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr || !detail::isCallTo(Call, "evm_revert") ||
        Call->arg_size() != 3) {
      continue;
    }
    return detail::isConstantIntValue(Call->getArgOperand(1), 0) &&
           detail::isConstantIntValue(Call->getArgOperand(2), 0);
  }
  return false;
}

Function *getOrDeclareCalldataMinSize(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *PtrTy = PointerType::get(Ctx, 0);
  Type *I256 = Type::getIntNTy(Ctx, 256);
  FunctionType *FTy = FunctionType::get(PtrTy, {PtrTy, I256}, false);
  FunctionCallee Callee =
      M.getOrInsertFunction("notdec_evm_calldata_min_size", FTy);
  return cast<Function>(Callee.getCallee());
}

CallInst *insertCalldataMinSizeCall(BranchInst &Branch,
                                    BasicBlock &NormalSuccessor,
                                    Value *Calldata, uint64_t MinSize) {
  Module *M = Branch.getModule();
  Function *Callee = getOrDeclareCalldataMinSize(*M);
  IRBuilder<> Builder(&*NormalSuccessor.getFirstInsertionPt());
  Value *Size =
      ConstantInt::get(Type::getIntNTy(M->getContext(), 256), MinSize);
  auto *Call = Builder.CreateCall(Callee, {Calldata, Size},
                                  "calldata.checked");
  Call->copyMetadata(Branch, {LLVMContext::MD_dbg, LLVMContext::MD_annotation});
  ++NumCalldataMinSizeGuards;
  return Call;
}

SmallVector<CalldataMinSizeGuard, 4>
rewriteCalldataMinSizeGuards(Function &F, Value *Calldata) {
  SmallVector<CalldataMinSizeGuard, 4> Guards;

  for (Instruction &I : instructions(F)) {
    auto *Branch = dyn_cast<BranchInst>(&I);
    if (Branch == nullptr || !Branch->isConditional()) {
      continue;
    }

    for (unsigned FailIndex = 0; FailIndex < 2; ++FailIndex) {
      BasicBlock *Fail = Branch->getSuccessor(FailIndex);
      if (!isNoPayloadRevertBlock(Fail)) {
        continue;
      }

      bool FailWhenTrue = FailIndex == 0;
      std::optional<uint64_t> MinSize =
          matchCalldataTooShort(Branch->getCondition(), FailWhenTrue, Calldata);
      if (!MinSize.has_value()) {
        continue;
      }

      BasicBlock *Normal = Branch->getSuccessor(1 - FailIndex);
      if (Normal->getUniquePredecessor() != Branch->getParent()) {
        continue;
      }
      Guards.push_back(CalldataMinSizeGuard{
          .NormalSuccessor = Normal,
          .MinSize = *MinSize,
          .CheckedCalldata =
              insertCalldataMinSizeCall(*Branch, *Normal, Calldata, *MinSize)});
      break;
    }
  }

  return Guards;
}

std::optional<APInt> resolveOffsetConstant(Value *Offset, unsigned Depth = 0) {
  if (!Offset->getType()->isIntegerTy() || Depth > 4) {
    return std::nullopt;
  }

  if (auto *Constant = dyn_cast<ConstantInt>(Offset)) {
    return Constant->getValue();
  }

  if (std::optional<uint64_t> Constant =
          detail::getUniqueCallsiteArgUInt64Constant(Offset)) {
    return APInt(Offset->getType()->getIntegerBitWidth(), *Constant);
  }

  auto *Add = dyn_cast<BinaryOperator>(Offset);
  if (Add == nullptr || Add->getOpcode() != Instruction::Add) {
    return std::nullopt;
  }

  // Solidity helpers often compute a field address as "formal offset + 32".
  // Fold that only when the formal offset itself is constant across all direct
  // callsites; conflicting call contexts stay as a dynamic offset.
  std::optional<APInt> LHS =
      resolveOffsetConstant(Add->getOperand(0), Depth + 1);
  std::optional<APInt> RHS =
      resolveOffsetConstant(Add->getOperand(1), Depth + 1);
  if (!LHS.has_value() || !RHS.has_value()) {
    return std::nullopt;
  }
  return *LHS + *RHS;
}

Value *resolveCallsiteConstantOffset(Value *Offset) {
  std::optional<APInt> Constant = resolveOffsetConstant(Offset);
  if (!Constant.has_value() || isa<ConstantInt>(Offset)) {
    return Offset;
  }

  ++NumCalldataCallsiteConstantOffsets;
  return ConstantInt::get(Offset->getType(), *Constant);
}

Value *asPointer(IRBuilder<> &Builder, Value *V, const Twine &Name) {
  if (V->getType()->isPointerTy()) {
    return V;
  }
  return Builder.CreateIntToPtr(V, PointerType::get(Builder.getContext(), 0),
                                Name);
}

Value *calldataBytePtr(IRBuilder<> &Builder, Value *Calldata, Value *Offset,
                       const Twine &Name) {
  Type *OffsetTy = Offset->getType();
  Value *Base = Builder.CreatePtrToInt(Calldata, OffsetTy, Name + ".base");
  Value *Addr = Builder.CreateAdd(Base, Offset, Name + ".addr");
  return Builder.CreateIntToPtr(Addr, PointerType::get(Builder.getContext(), 0),
                                Name);
}

void collectArgumentUses(Value *V, SmallVectorImpl<Argument *> &Args,
                         unsigned Depth = 0) {
  if (V == nullptr || Depth > 8) {
    return;
  }
  if (auto *Arg = dyn_cast<Argument>(V)) {
    if (!Arg->getType()->isPointerTy()) {
      Args.push_back(Arg);
    }
    return;
  }
  auto *Inst = dyn_cast<Instruction>(V);
  if (Inst == nullptr) {
    return;
  }
  for (Use &Operand : Inst->operands()) {
    collectArgumentUses(Operand.get(), Args, Depth + 1);
  }
}

SmallVector<unsigned, 4> getCalldataOffsetArgNos(Function &F,
                                                 Value *Calldata) {
  SmallVector<Argument *, 8> Args;
  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    if (detail::isCallTo(Call, "evm_calldataload") &&
        usesCurrentCalldata(*Call, 0, Calldata) && Call->arg_size() == 2) {
      collectArgumentUses(Call->getArgOperand(1), Args);
    } else if (detail::isCallTo(Call, "evm_calldatacopy") &&
               usesCurrentCalldata(*Call, 1, Calldata) &&
               Call->arg_size() == 5) {
      collectArgumentUses(Call->getArgOperand(3), Args);
    }
  }

  SmallVector<unsigned, 4> ArgNos;
  llvm::sort(Args, [](Argument *LHS, Argument *RHS) {
    return LHS->getArgNo() < RHS->getArgNo();
  });
  for (Argument *Arg : Args) {
    if (ArgNos.empty() || ArgNos.back() != Arg->getArgNo()) {
      ArgNos.push_back(Arg->getArgNo());
    }
  }
  return ArgNos;
}

bool hasPolymorphicCallsiteOffset(Function &F, unsigned ArgNo) {
  std::optional<APInt> FirstConstant;
  bool SawConstant = false;
  bool SawDynamic = false;

  for (Use &U : F.uses()) {
    auto *Call = dyn_cast<CallBase>(U.getUser());
    if (Call == nullptr || Call->getCalledFunction() != &F ||
        ArgNo >= Call->arg_size()) {
      continue;
    }

    auto *Constant = dyn_cast<ConstantInt>(Call->getArgOperand(ArgNo));
    if (Constant == nullptr) {
      SawDynamic = true;
      continue;
    }

    if (FirstConstant.has_value() &&
        Constant->getValue() != *FirstConstant) {
      return true;
    }
    FirstConstant = Constant->getValue();
    SawConstant = true;
  }

  return SawConstant && SawDynamic;
}

bool markPolymorphicHelpers(Module &M) {
  bool Changed = false;

  for (Function &F : M) {
    if (!shouldMarkPolymorphicFunction(F)) {
      continue;
    }
    Value *Calldata = getCalldataArg(F);
    if (Calldata == nullptr) {
      continue;
    }
    SmallVector<unsigned, 4> OffsetArgNos = getCalldataOffsetArgNos(F, Calldata);
    bool Polymorphic = false;
    for (unsigned ArgNo : OffsetArgNos) {
      if (hasPolymorphicCallsiteOffset(F, ArgNo)) {
        Polymorphic = true;
        break;
      }
    }
    if (!Polymorphic ||
        F.getMetadata(mlsub::KIND_MLSUB_POLYMORPHIC_FUNCTION) != nullptr) {
      continue;
    }

    F.setMetadata(mlsub::KIND_MLSUB_POLYMORPHIC_FUNCTION,
                  MDNode::get(F.getContext(),
                              {MDString::get(F.getContext(),
                                             "calldata_offset")}));
    ++NumCalldataPolymorphicHelpers;
    Changed = true;
  }

  return Changed;
}

bool rewriteCalldataLoad(CallBase &Call, Value *Calldata, Value *Base) {
  if (!detail::isCallTo(&Call, "evm_calldataload") ||
      !usesCurrentCalldata(Call, 0, Calldata) || Call.arg_size() != 2) {
    return false;
  }

  IRBuilder<> Builder(&Call);
  Value *Offset = resolveCallsiteConstantOffset(Call.getArgOperand(1));
  Value *Addr = calldataBytePtr(Builder, Base, Offset, "calldata.ptr");
  auto *Load = Builder.CreateLoad(Builder.getIntNTy(256), Addr,
                                  Call.getName() + ".load");
  Load->setAlignment(Align(1));
  Load->copyMetadata(Call, {LLVMContext::MD_dbg, LLVMContext::MD_annotation});
  Call.replaceAllUsesWith(Load);
  Call.eraseFromParent();
  ++NumCalldataLoadsRewritten;
  return true;
}

bool rewriteCalldataCopy(CallBase &Call, Value *Calldata, Value *Base) {
  if (!detail::isCallTo(&Call, "evm_calldatacopy") ||
      !usesCurrentCalldata(Call, 1, Calldata) || Call.arg_size() != 5) {
    return false;
  }

  IRBuilder<> Builder(&Call);
  Value *Dst = asPointer(Builder, Call.getArgOperand(2), "calldata.copy.dst");
  Value *SrcOffset = resolveCallsiteConstantOffset(Call.getArgOperand(3));
  Value *Src =
      calldataBytePtr(Builder, Base, SrcOffset, "calldata.copy.src");
  Value *Len = Call.getArgOperand(4);
  CallInst *Memcpy =
      Builder.CreateMemCpy(Dst, Align(1), Src, Align(1), Len, false);
  Memcpy->copyMetadata(Call,
                       {LLVMContext::MD_dbg, LLVMContext::MD_annotation});
  Call.eraseFromParent();
  ++NumCalldataCopiesRewritten;
  return true;
}

bool rewriteFunction(Function &F) {
  Value *Calldata = getCalldataArg(F);
  if (Calldata == nullptr) {
    return false;
  }

  DominatorTree DT(F);
  SmallVector<CalldataMinSizeGuard, 4> Guards =
      rewriteCalldataMinSizeGuards(F, Calldata);
  SmallVector<CallBase *, 16> Calls;
  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    if (detail::isCallTo(Call, "evm_calldataload") ||
        detail::isCallTo(Call, "evm_calldatacopy")) {
      Calls.push_back(Call);
    }
  }

  bool Changed = false;
  Changed |= !Guards.empty();
  for (CallBase *Call : Calls) {
    if (Call->getParent() == nullptr) {
      continue;
    }
    Value *Base = Calldata;
    uint64_t BestMinSize = 0;
    for (const CalldataMinSizeGuard &Guard : Guards) {
      if (Guard.CheckedCalldata != nullptr && Guard.MinSize >= BestMinSize &&
          DT.dominates(Guard.NormalSuccessor, Call->getParent())) {
        Base = Guard.CheckedCalldata;
        BestMinSize = Guard.MinSize;
      }
    }
    if (rewriteCalldataLoad(*Call, Calldata, Base)) {
      Changed = true;
      continue;
    }
    if (rewriteCalldataCopy(*Call, Calldata, Base)) {
      Changed = true;
    }
  }

  return Changed;
}

} // namespace

PreservedAnalyses EvmCalldataAccessPass::run(Module &M,
                                             ModuleAnalysisManager &) {
  bool Changed = markPolymorphicHelpers(M);
  SmallVector<Function *, 32> Functions;
  for (Function &F : M) {
    if (!F.isDeclaration()) {
      Functions.push_back(&F);
    }
  }
  for (Function *F : Functions) {
    Changed |= rewriteFunction(*F);
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
