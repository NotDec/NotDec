#include <cassert>
#include <cstdlib>
#include <iostream>
#include <llvm/IR/InstrTypes.h>
#include <optional>
#include <string>
#include <utility>

#include <llvm/ADT/StringExtras.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/raw_ostream.h>
#include <variant>

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/StorageShapeGraph.h"
#include "Utils/Range.h"
#include "optimizers/ConstraintGenerator.h"
#include "optimizers/StackPointerFinder.h"
#include "utils.h"

#define DEBUG_TYPE "type-recovery"

namespace notdec {

using retypd::CGNode;
using retypd::OffsetLabel;

const char *ConstraintGraph::Memory = "MEMORY";
const char *TypeRecovery::DefaultPrefix = "v_";
const char *TypeRecovery::FuncPrefix = "func_";
const char *TypeRecovery::PhiPrefix = "phi_";
const char *TypeRecovery::SelectPrefix = "select_";
const char *TypeRecovery::NewPrefix = "new_";
const char *TypeRecovery::AddPrefix = "add_";
const char *TypeRecovery::SubPrefix = "sub_";

/// Visit Add/Mul chain, add the results to OffsetRange.
OffsetRange matchOffsetRange(llvm::Value *I) {
  assert(I->getType()->isIntegerTy());
  if (auto *CI = dyn_cast<llvm::ConstantInt>(I)) {
    return OffsetRange{.offset = CI->getSExtValue()};
  }
  // unknown value = 1*x
  if (!isa<llvm::BinaryOperator>(I)) {
    return OffsetRange{.offset = 0, .access = {{1, 0}}};
  }
  auto *BinOp = cast<llvm::BinaryOperator>(I);
  auto *Src1 = BinOp->getOperand(0);
  auto *Src2 = BinOp->getOperand(1);
  ensureSequence(Src1, Src2);
  // check if add or mul
  if (BinOp->getOpcode() == llvm::Instruction::Add) {
    return matchOffsetRange(Src1) + matchOffsetRange(Src2);
  } else if (BinOp->getOpcode() == llvm::Instruction::Mul) {
    return matchOffsetRange(Src1) * matchOffsetRange(Src2);
  } else {
    return OffsetRange{.offset = 0, .access = {{1, 0}}};
  }
}

PreservedAnalyses TypeRecovery::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(errs() << " ============== RetypdGenerator  ===============\n");

  LLVM_DEBUG(printModule(M, "current.ll"));

  auto SP = MAM.getResult<StackPointerFinderAnalysis>(M);
  this->StackPointer = SP.result;

  data_layout = std::move(M.getDataLayoutStr());
  pointer_size = M.getDataLayout().getPointerSizeInBits();

  // TODO!! reverse post order of functions?
  for (auto &F : M) {
    auto it = func_ctxs.try_emplace(&F, *this, F);
    assert(it.second && "TypeRecovery::run: duplicate function?");
    auto &Generator = it.first->second;
    Generator.generate();
  }

  // create a temporary directory
  SmallString<128> Path;
  std::error_code EC;
  EC = llvm::sys::fs::createUniqueDirectory("notdec_retypd", Path);
  if (!EC) {
    // Resolve any symlinks in the new directory.
    std::string UnresolvedPath(Path.str());
    EC = llvm::sys::fs::real_path(UnresolvedPath, Path);
  }
  auto TempPath = Path.str().str() + "/retypd-constrains.json";

  gen_json(TempPath);

  errs() << "constrains path: " << TempPath << "\n";

  // clean up
  // delete directory if not debug
  LLVM_DEBUG(if (!Path.empty()) {
    assert(llvm::sys::fs::remove_directories(Path.str()) == std::errc());
  });

  LLVM_DEBUG(errs() << " ============== RetypdGenerator End ===============\n");
  return PreservedAnalyses::none();
}

void ConstraintsGenerator::generate() {
  RetypdGeneratorVisitor Visitor(*this);
  Visitor.visit(Func);
  Visitor.handlePHINodes();
  callInstanceId.clear();
}

void ConstraintsGenerator::solve() { CG.solve(); }

static bool mustBePrimitive(const llvm::Type *Ty) {
  if (Ty->isFloatTy() || Ty->isDoubleTy()) {
    return true;
  }
  if (Ty->isIntegerTy()) {
    return Ty->getIntegerBitWidth() < 32;
  }
  if (Ty->isPointerTy()) {
    return false;
  }
  return false;
}

static std::string getLLVMTypeBase(Type *Ty) {
  if (Ty->isPointerTy()) {
    // 20231122 why there is pointer constant(not inttoptr constant expr)
    assert(false && "TODO can this be pointer?");
    return getLLVMTypeBase(Ty->getPointerElementType()) + "*";
  }
  if (Ty->isFloatTy()) {
    return "float";
  } else if (Ty->isDoubleTy()) {
    return "double";
  } else if (Ty->isIntegerTy()) {
    if (Ty->getIntegerBitWidth() == 1)
      return "bool";
    else
      return "int";
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: unknown integer type: " << *Ty << "\n";
    assert(false && "unknown integer type");
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: unknown constant type: " << *Ty << "\n";
    assert(false && "unknown constant type");
  }
}

static inline TypeVariable makeTv(std::string Name) {
  return retypd::TypeVariable::CreateDtv(Name);
}

static inline TypeVariable getLLVMTypeVar(Type *Ty) {
  return retypd::TypeVariable::CreatePrimitive(getLLVMTypeBase(Ty));
}

static bool is32Or64Int(const Type *Ty) {
  if (Ty->isIntegerTy()) {
    return Ty->getIntegerBitWidth() == 32 || Ty->getIntegerBitWidth() == 64;
  }
  return false;
}

// Check if a primitive type is final. Currently only int is not final for
// unknown signedness.
bool isFinal(const std::string &Name) {
  if (Name == "int") {
    return false;
  }
  return true;
}

bool hasUser(const Value *Val, const User *User) {
  for (auto U : Val->users()) {
    if (U == User) {
      return true;
    }
  }
  return false;
}

retypd::CGNode &ConstraintsGenerator::getNode(ValMapKey Val, User *User) {
  // Differentiate int32/int64 by User.
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    if (auto CI = dyn_cast<ConstantInt>(*V)) {
      if (CI->getBitWidth() == 32 || CI->getBitWidth() == 64) {
        assert(User != nullptr && "RetypdGenerator::getTypeVar: User is Null!");
        assert(hasUser(*V, User) &&
               "convertTypeVarVal: constant not used by user");
        Val = IntConstant{.Val = cast<ConstantInt>(*V), .User = User};
      }
    }
  }
  if (Val2Node.count(Val)) {
    return *Val2Node.at(Val);
  }
  auto ret = convertTypeVar(Val);
  return setTypeVar(Val, ret, User);
}

retypd::SSGNode *ConstraintsGenerator::getSSGNode(ValMapKey Val, User *User) {
  return getNode(Val, User).getLink().lookupNode();
}

const TypeVariable &ConstraintsGenerator::getTypeVar(ValMapKey Val,
                                                     User *User) {
  return getNode(Val, User).key.Base;
}

TypeVariable ConstraintsGenerator::convertTypeVar(ValMapKey Val, User *User) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    return convertTypeVarVal(*V);
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    auto tv = makeTv(Ctx.getName(*F->Func, TypeRecovery::FuncPrefix));
    tv.getLabels().push_back(retypd::OutLabel{});
    return tv;
  } else if (auto Arg = std::get_if<CallArg>(&Val)) {
    // TODO what if function pointer?
    auto tv = makeTv(
        Ctx.getName(*Arg->Call->getCalledFunction(), TypeRecovery::FuncPrefix));
    tv.getInstanceId() = Arg->InstanceId;
    tv.getLabels().push_back(retypd::InLabel{std::to_string(Arg->Index)});
    return tv;
  } else if (auto Ret = std::get_if<CallRet>(&Val)) {
    // TODO what if function pointer?
    auto tv = makeTv(
        Ctx.getName(*Ret->Call->getCalledFunction(), TypeRecovery::FuncPrefix));
    tv.getInstanceId() = Ret->InstanceId;
    tv.getLabels().push_back(retypd::OutLabel{});
    return tv;
  } else if (auto IC = std::get_if<IntConstant>(&Val)) {
    auto ret = TypeVariable::CreateIntConstant(
        OffsetRange{.offset = IC->Val->getSExtValue()}, intConstantIds.size());
    intConstantIds.push_back(*IC);
    return ret;
  }
  llvm::errs()
      << __FILE__ << ":" << __LINE__ << ": "
      << "ERROR: RetypdGenerator::convertTypeVar unhandled type of ValMapKey\n";
  std::abort();
}

TypeVariable ConstraintsGenerator::convertTypeVarVal(Value *Val, User *User) {
  if (Constant *C = dyn_cast<Constant>(Val)) {
    // check for constantExpr
    if (auto CE = dyn_cast<ConstantExpr>(C)) {
      // ignore bitcast ConstantExpr
      if (CE->getOpcode() == Instruction::BitCast) {
        return getTypeVar(CE->getOperand(0), User);
      }
      if (CE->getOpcode() == Instruction::IntToPtr) {
        if (auto Addr = dyn_cast<ConstantInt>(CE->getOperand(0))) {
          auto tv = makeTv(CG.Memory);
          setOffset(tv, OffsetRange{.offset = Addr->getSExtValue()});
          return tv;
        }
      }
    } else if (auto gv = dyn_cast<GlobalValue>(C)) { // global variable
      if (gv == Ctx.StackPointer) {
        assert(false && "convertTypeVarVal: direct use of stack pointer?, run "
                        "StackAllocationRecovery first");
      } else if (auto Func = dyn_cast<Function>(C)) {
        return makeTv(Ctx.getName(*Func, TypeRecovery::FuncPrefix));
      }
      return makeTv(gv->getName().str());
    } else if (isa<ConstantInt>(C) || isa<ConstantFP>(C)) {
      if (auto CI = dyn_cast<ConstantInt>(C)) {
        assert(CI->getBitWidth() != 32 && CI->getBitWidth() != 64 &&
               "Should be handled earlier");
      }
      auto Ty = C->getType();
      return getLLVMTypeVar(Ty);
    }
    llvm::errs()
        << __FILE__ << ":" << __LINE__ << ": "
        << "ERROR: RetypdGenerator::getTypeVar unhandled type of constant: "
        << *C << "\n";
    std::abort();
  } else if (auto arg = dyn_cast<Argument>(Val)) { // for function argument
    TypeVariable tv = getTypeVar(arg->getParent(), User);
    tv.getLabels().push_back(retypd::InLabel{std::to_string(arg->getArgNo())});
    return tv;
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "WARN: RetypdGenerator::getTypeVar unhandled value: " << *Val
               << "\n";
  return makeTv(Ctx.getName(*Val, TypeRecovery::NewPrefix));
}

// TODO: accept any character in name by using single quotes like LLVM IR.
std::string TypeRecovery::sanitize_name(std::string S) {
  std::replace(S.begin(), S.end(), '.', '_');
  std::replace(S.begin(), S.end(), '@', '_');
  return S;
}

std::string TypeRecovery::getName(Value &Val, const char *prefix) {
  if (!Val.hasName()) {
    auto Id = typeValId++;
    Val.setName(prefix + std::to_string(Id));
    return prefix + std::to_string(Id);
  }
  return Val.getName().str();
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitReturnInst(
    ReturnInst &I) {
  auto *Src = I.getReturnValue();
  if (Src == nullptr) { // ret void.
    return;
  }
  auto SrcVar = cg.getTypeVar(Src, &I);
  auto DstVar = cg.getTypeVar(ReturnValue{.Func = I.getFunction()}, &I);
  // src is a subtype of dest
  cg.addSubtype(SrcVar, DstVar);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitCallBase(CallBase &I) {
  auto Target = I.getCalledFunction();
  if (Target == nullptr) {
    // TODO indirect call
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Warn: RetypdGenerator: indirect call not supported yet\n";
    return;
  }
  // differentiate different call instances in the same function
  auto TargetName = cg.Ctx.getName(*Target, TypeRecovery::FuncPrefix);
  // Starts from 1. So that it is different from the default 0.
  auto InstanceId = ++cg.callInstanceId[TargetName];
  for (int i = 0; i < I.arg_size(); i++) {
    auto ArgVar = cg.getTypeVar(
        CallArg{.Call = &I, .InstanceId = InstanceId, .Index = i}, &I);
    auto ValVar = cg.getTypeVar(I.getArgOperand(i), &I);
    // argument is a subtype of param
    cg.addSubtype(ValVar, ArgVar);
  }
  // for return value
  auto DstVar = cg.getTypeVar(CallRet{.Call = &I}, &I);
  cg.setTypeVar(&I, DstVar, nullptr);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitSelectInst(
    SelectInst &I) {
  auto DstVar = makeTv(cg.Ctx.getName(I, TypeRecovery::SelectPrefix));
  auto *Src1 = I.getTrueValue();
  auto *Src2 = I.getFalseValue();
  auto Src1Var = cg.getTypeVar(Src1, &I);
  auto Src2Var = cg.getTypeVar(Src2, &I);
  // Not generate boolean constraints. Because it must be i1.
  cg.setTypeVar(&I, DstVar, nullptr);
  cg.addSubtype(Src1Var, DstVar);
  cg.addSubtype(Src2Var, DstVar);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitAllocaInst(
    AllocaInst &I) {
  const char *prefix = "alloca_";
  if (I.getParent()->isEntryBlock()) {
    prefix = "stack_";
  }
  auto DstVar = makeTv(cg.Ctx.getName(I, prefix));
  auto &Node = cg.setTypeVar(&I, DstVar, nullptr);
  // set as pointer type
  cg.setPointer(Node);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitPHINode(PHINode &I) {
  // Create a stub variable, defer constraints generation (and unification) to
  // handlePHINodes Because it goes across the basic block boundary. Operands
  // may be not found using getTypeVar.

  // Only Phi can create inter-block dataflow besides direct reference (valid if
  // we follow a topological sort of basic blocks)

  auto DstVar = makeTv(cg.Ctx.getName(I, TypeRecovery::PhiPrefix));
  cg.setTypeVar(&I, DstVar, nullptr);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::handlePHINodes() {
  for (auto I : phiNodes) {
    auto DstVar = cg.getTypeVar(I, nullptr);
    for (auto &Op : I->incoming_values()) {
      auto *Src = Op.get();
      auto SrcVar = cg.getTypeVar(Src, I);
      // src is a subtype of dest
      cg.addSubtype(SrcVar, DstVar);
    }
  }
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitCastInst(CastInst &I) {
  if (isa<BitCastInst>(I)) {
    // ignore cast, propagate the type of the operand.
    auto *Src = I.getOperand(0);
    auto SrcVar = cg.getTypeVar(Src, &I);
    cg.newVarSubtype(&I, SrcVar);
    return;
  } else if (isa<PtrToIntInst, IntToPtrInst, BitCastInst>(I)) {
    // ignore cast, but set value as pointer.
    auto *Src = I.getOperand(0);
    auto SrcVar = cg.getTypeVar(Src, &I);
    auto &Node = cg.newVarSubtype(&I, SrcVar);
    cg.setPointer(Node);
    return;
  } else if (isa<TruncInst, ZExtInst>(&I)) {
    auto *Src = I.getOperand(0);
    if (is32Or64Int(I.getSrcTy()) && is32Or64Int(I.getDestTy())) {
      auto SrcVar = cg.getTypeVar(Src, &I);
      cg.newVarSubtype(&I, SrcVar);
      return;
    } else {
      if (isa<ZExtInst>(&I)) {
        cg.newVarSubtype(&I, TypeVariable::CreatePrimitive("uint"));
        cg.addSubtype(cg.getTypeVar(I.getOperand(0), &I),
                      TypeVariable::CreatePrimitive("uint"));
      }
      cg.newVarSubtype(&I, getLLVMTypeVar(I.getType()));
      cg.addSubtype(cg.getTypeVar(I.getOperand(0), &I),
                    getLLVMTypeVar(I.getOperand(0)->getType()));
    }
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: RetypdGenerator::getTypeVar unhandled CastInst: " << I
               << "\n";
  std::abort();
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitGetElementPtrInst(
    GetElementPtrInst &Gep) {
  assert(false && "Gep should not exist before this pass");
  // But if we really want to support this, handle it the same way as AddInst.
  // A shortcut to create a offseted pointer. the operate type must be i8*. Just
  // like ptradd.
}

void ConstraintsGenerator::addCmpConstraint(const ValMapKey LHS,
                                            const ValMapKey RHS, ICmpInst *I) {
  getSSGNode(LHS, I)->unifyPN(*getSSGNode(RHS, I));
}

// for pointer sized int, probably is pointer comparision. So we cannot make a
// conclusion of subtype. but create a CmpConstraint.
void ConstraintsGenerator::RetypdGeneratorVisitor::visitICmpInst(ICmpInst &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addCmpConstraint(Src1, Src2, &I);

  // type the inst as bool
  assert(I.getType()->isIntegerTy(1));
  cg.newVarSubtype(&I, getLLVMTypeVar(I.getType()));
}

// #region LoadStore

// =========== begin: load/store insts and deref analysis ===========

unsigned ConstraintsGenerator::getPointerElemSize(Type *ty) {
  Type *Elem = ty->getPointerElementType();
  unsigned Size = Elem->getPrimitiveSizeInBits();
  if (Size != 0) {
    return Size;
  }
  if (Elem->isPointerTy()) {
    assert(Ctx.pointer_size != 0 &&
           "RetypdGenerator: pointer size not initialized");
    return Ctx.pointer_size;
  }
  assert(false && "unknown pointer type");
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitStoreInst(
    StoreInst &I) {
  auto DstVar =
      cg.deref(I.getPointerOperand(), &I,
               cg.getPointerElemSize(I.getPointerOperandType()), false);
  cg.addSubtype(cg.getTypeVar(I.getValueOperand(), &I), DstVar);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitLoadInst(LoadInst &I) {
  auto SrcVar =
      cg.deref(I.getPointerOperand(), &I,
               cg.getPointerElemSize(I.getPointerOperandType()), true);
  cg.setTypeVar(&I, SrcVar, nullptr);
}

std::string ConstraintsGenerator::offset(APInt Offset, int Count) {
  std::string OffsetStr = toString(Offset, 10, false);
  // case -1:
  //   OffsetStr = "0*[nobound]";
  //   break;
  // case -2:
  //   OffsetStr = "0*[nullterm]";

  if (Count > 1) {
    OffsetStr += "*" + std::to_string(Count);
  }
  return OffsetStr;
}

void ConstraintsGenerator::setOffset(TypeVariable &dtv, OffsetRange Offset) {
  OffsetLabel *current;
  if (!dtv.getLabels().empty() &&
      std::holds_alternative<OffsetLabel>(dtv.getLabels().back())) {
    // current = &std::get<OffsetLabel>(dtv.Labels.back());
    assert(false && "not implemented merging of offset labels");
  } else {
    current = &std::get<OffsetLabel>(
        dtv.getLabels().emplace_back(OffsetLabel{.range = Offset}));
  }
  assert(false && "TODO");
  (void)current;
}

// Special logics for load and store when generating type variables.
TypeVariable ConstraintsGenerator::deref(Value *Val, User *User, long BitSize,
                                         bool isLoadOrStore) {
  assert(BitSize != 0 && "RetypdGenerator::deref: zero size!?");
  // from the offset, generate a loaded type variable.
  auto DstVar = getTypeVar(Val, User);
  assert(BitSize % 8 == 0 && "size must be byte aligned");
  uint32_t Size = BitSize = BitSize / 8;
  if (isLoadOrStore) {
    DstVar.getLabels().push_back(retypd::LoadLabel{.Size = Size});
  } else {
    DstVar.getLabels().push_back(retypd::StoreLabel{.Size = Size});
  }
  return DstVar;
}

// =========== end: load/store insts ===========

// #endregion LoadStore

// =========== begin: other insts ===========

const std::map<unsigned, ConstraintsGenerator::PcodeOpType>
    ConstraintsGenerator::opTypes = {
        // for Trunc, ZExt, SExt
        {Instruction::SExt, {"sint", 1, (const char *[1]){"sint"}}},

        // other cast insts: FPToUIInst, FPToSIInst, UIToFPInst, SIToFPInst
        {Instruction::FPToUI, {"uint", 1, (const char *[1]){nullptr}}},
        {Instruction::FPToSI, {"sint", 1, (const char *[1]){nullptr}}},
        {Instruction::UIToFP, {nullptr, 1, (const char *[1]){"uint"}}},
        {Instruction::SIToFP, {nullptr, 1, (const char *[1]){"sint"}}},

        {Instruction::UDiv, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::SDiv, {"sint", 2, (const char *[2]){"sint", "sint"}}},
        {Instruction::URem, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::SRem, {"sint", 2, (const char *[2]){"sint", "sint"}}},
        {Instruction::Xor, {"uint", 2, (const char *[2]){"uint", "uint"}}},

        // {Instruction::Add, {"int", (const char *[2]){"int", "int"}, 2}},
        // {Instruction::Sub, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::Mul, {"int", 2, (const char *[2]){"int", "int"}}},

        {Instruction::Shl, {"int", 2, (const char *[2]){"int", "int"}}},
        {Instruction::LShr, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::AShr, {"sint", 2, (const char *[2]){"sint", "uint"}}},
        // {Instruction::And, {"int", (const char *[2]){"int", "int"}, 2}},
        // {Instruction::Or, {"int", (const char *[2]){"int", "int"}, 2}},
};

void ConstraintsGenerator::RetypdGeneratorVisitor::visitInstruction(
    Instruction &I) {
  // return value
  if (I.getType()->isVoidTy()) {
    // skip void type
  } else if (mustBePrimitive(I.getType())) {
    cg.newVarSubtype(&I, getLLVMTypeVar(I.getType()));
  } else if (opTypes.count(I.getOpcode()) &&
             opTypes.at(I.getOpcode()).addRetConstraint(&I, cg)) {
    // good
  } else {
    llvm::errs() << "WARN: RetypdGenerator: unhandled instruction return: " << I
                 << "\n";
  }
  // for each op
  for (unsigned Ind = 0; Ind < I.getNumOperands(); ++Ind) {
    auto Op = I.getOperand(Ind);
    if (Op->getType()->isVoidTy()) {
      // skip void type
    } else if (mustBePrimitive(Op->getType())) {
      cg.addSubtype(cg.getTypeVar(Op, &I), getLLVMTypeVar(Op->getType()));
    } else if (opTypes.count(I.getOpcode()) &&
               opTypes.at(I.getOpcode()).addOpConstraint(Ind, &I, cg)) {
      // good
    } else {
      llvm::errs() << "WARN: RetypdGenerator: unhandled instruction Op: " << I
                   << "\n";
    }
  }
}

void ConstraintsGenerator::addAddConstraint(const ValMapKey LHS,
                                            const ValMapKey RHS,
                                            BinaryOperator *I) {
  SSG.addAddCons(getSSGNode(LHS, I), getSSGNode(RHS, I),
                 getSSGNode(I, nullptr));
}

void ConstraintsGenerator::addSubConstraint(const ValMapKey LHS,
                                            const ValMapKey RHS,
                                            BinaryOperator *Result) {
  SSG.addSubCons(getSSGNode(LHS, Result), getSSGNode(RHS, Result),
                 getSSGNode(Result, nullptr));
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitAdd(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  auto DstVar = makeTv(cg.Ctx.getName(I, TypeRecovery::AddPrefix));
  cg.setTypeVar(&I, DstVar, nullptr);

  cg.addAddConstraint(Src1, Src2, &I);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitSub(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  auto DstVar = makeTv(cg.Ctx.getName(I, TypeRecovery::SubPrefix));
  cg.setTypeVar(&I, DstVar, nullptr);

  cg.addSubConstraint(Src1, Src2, &I);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitAnd(BinaryOperator &I) {
  llvm::errs() << "visiting " << __FUNCTION__ << " \n";
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  ensureSequence(Src1, Src2);

  if (auto CI = dyn_cast<ConstantInt>(Src2)) {
    // at least most of the bits are passed, View as pointer alignment.
    if ((CI->getZExtValue() & 0x3fffff00) == 0x3fffff00) {
      // act as simple assignment
      cg.newVarSubtype(&I, cg.getTypeVar(Src1, &I));
      return;
    }
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: And op without constant: " << I << "\n";
    cg.addSubtype(cg.getTypeVar(Src2, &I), getLLVMTypeVar(Src2->getType()));
  }
  // view as numeric operation?
  cg.newVarSubtype(&I, getLLVMTypeVar(I.getType()));
  cg.addSubtype(cg.getTypeVar(Src1, &I), getLLVMTypeVar(Src1->getType()));
  return;
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitOr(BinaryOperator &I) {
  llvm::errs() << "Visiting " << __FUNCTION__ << " \n";
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  ensureSequence(Src1, Src2);

  if (auto CI = dyn_cast<ConstantInt>(Src2)) {
    // at least most of the bits are passed, View as pointer alignment.
    if ((CI->getZExtValue() & 0x3fffff00) == 0) {
      // act as simple assignment
      cg.newVarSubtype(&I, cg.getTypeVar(Src1, &I));
      return;
    }
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: And op without constant: " << I << "\n";
    cg.addSubtype(cg.getTypeVar(Src2, &I), getLLVMTypeVar(Src2->getType()));
  }
  // view as numeric operation?
  cg.newVarSubtype(&I, getLLVMTypeVar(I.getType()));
  cg.addSubtype(cg.getTypeVar(Src1, &I), getLLVMTypeVar(Src1->getType()));
  return;
}

bool ConstraintsGenerator::PcodeOpType::addRetConstraint(
    Instruction *I, ConstraintsGenerator &cg) const {
  if (I->getType()->isVoidTy()) {
    return false;
  }
  const char *ty = output;
  if (ty == nullptr) {
    return false;
  }
  cg.setTypeVar(I, TypeVariable::CreatePrimitive(ty), nullptr);
  return true;
}

bool ConstraintsGenerator::PcodeOpType::addOpConstraint(
    unsigned Index, Instruction *I, ConstraintsGenerator &cg) const {
  assert(size == I->getNumOperands() && "input size not match");
  auto Op = I->getOperand(Index);
  if (Op->getType()->isVoidTy()) {
    return false;
  }
  const char *ty = inputs[Index];
  if (ty == nullptr) {
    return false;
  }
  cg.addSubtype(cg.getTypeVar(Op, I), TypeVariable::CreatePrimitive(ty));
  return true;
}

// =========== end: other insts ===========

void TypeRecovery::gen_json(std::string OutputFilename) {
  json::Object Root({{"data_layout", data_layout}});

  json::Object Constraints;
  // iterate func_constrains
  for (auto &kv : func_ctxs) {
    auto funcName = getName(*kv.first);
    json::Array FuncConstrainsJson;
    for (auto &c : kv.second.CG.toConstraints()) {
      FuncConstrainsJson.push_back(toString(c));
    }
    Constraints[funcName] = json::Value(std::move(FuncConstrainsJson));
  }
  Root["constraints"] = json::Value(std::move(Constraints));

  // write to file
  std::error_code EC;
  llvm::raw_fd_ostream os(OutputFilename, EC);
  if (EC) {
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Cannot open output json file." << std::endl;
    std::cerr << EC.message() << std::endl;
    std::abort();
  }
  json::OStream J(os, 2);
  J.value(std::move(Root));
}

} // namespace notdec
