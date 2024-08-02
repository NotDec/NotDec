#include <cassert>
#include <cstdlib>
#include <iostream>
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

#include "TypeRecovery/Schema.h"
#include "TypeRecovery/StorageShapeGraph.h"
#include "Utils/Range.h"
#include "optimizers/ConstraintGenerator.h"
#include "optimizers/StackPointerFinder.h"
#include "utils.h"

#define DEBUG_TYPE "retypd"

namespace notdec {

using retypd::OffsetLabel;

const char *RetypdGenerator::Stack = "STACK";
const char *RetypdGenerator::Memory = "MEMORY";

/// Uses ptrtoint to ensure type correctness.
PreservedAnalyses Retypd::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(errs() << " ============== RetypdGenerator  ===============\n");

  LLVM_DEBUG(printModule(M, "current.ll"));

  auto SP = MAM.getResult<StackPointerFinderAnalysis>(M);
  this->StackPointer = SP.result;

  data_layout = std::move(M.getDataLayoutStr());
  pointer_size = M.getDataLayout().getPointerSizeInBits();

  // TODO!! reverse post order
  for (auto &F : M) {
    auto it = func_ctxs.emplace(&F, *this);
    auto &Generator = it.first->second;
    Generator.run(F);
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

void RetypdGenerator::run(Function &F) {
  RetypdGeneratorVisitor Visitor(*this);
  Visitor.visit(F);
  Visitor.handlePHINodes();
  callInstanceId.clear();
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
    if (Ty->getIntegerBitWidth() == 8)
      return "uint8";
    if (Ty->getIntegerBitWidth() == 16)
      return "uint16";
    if (Ty->getIntegerBitWidth() == 32)
      return "uint32";
    if (Ty->getIntegerBitWidth() == 64)
      return "uint64";
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: unknown integer type: " << *Ty << "\n";
    assert(false && "unknown integer type");
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: unknown constant type: " << *Ty << "\n";
    assert(false && "unknown constant type");
  }
}

static inline DerivedTypeVariable makeTv(std::string Name) {
  retypd::DerivedTypeVariable ret{.name = Name};
  return ret;
}

static inline DerivedTypeVariable getLLVMTypeVar(Type *Ty) {
  return makeTv("#" + getLLVMTypeBase(Ty));
}

const DerivedTypeVariable &RetypdGenerator::getTypeVar(ValMapKey Val) {
  if (Val2Node.count(Val)) {
    return Val2Node.at(Val)->key.Base;
  }
  auto ret = convertTypeVar(Val);
  return setTypeVar(Val, ret).key.Base;
}

DerivedTypeVariable RetypdGenerator::convertTypeVar(ValMapKey Val) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    return convertTypeVarVal(*V);
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    auto tv = makeTv(Ctx.getName(*F->Func, "func_"));
    tv.Labels.push_back(retypd::OutLabel{});
    return tv;
  } else if (auto Arg = std::get_if<CallArg>(&Val)) {
    // TODO what if function pointer?
    auto tv = makeTv(Ctx.getName(*Arg->Call->getCalledFunction(), "func_"));
    tv.instanceId = Arg->InstanceId;
    tv.Labels.push_back(retypd::InLabel{std::to_string(Arg->Index)});
    return tv;
  } else if (auto Ret = std::get_if<CallRet>(&Val)) {
    // TODO what if function pointer?
    auto tv = makeTv(Ctx.getName(*Ret->Call->getCalledFunction(), "func_"));
    tv.instanceId = Ret->InstanceId;
    tv.Labels.push_back(retypd::OutLabel{});
    return tv;
  }
  llvm::errs()
      << __FILE__ << ":" << __LINE__ << ": "
      << "ERROR: RetypdGenerator::convertTypeVar unhandled type of ValMapKey\n";
  std::abort();
}

DerivedTypeVariable RetypdGenerator::convertTypeVarVal(Value *Val) {
  if (Constant *C = dyn_cast<Constant>(Val)) {
    // check for constantExpr
    if (auto CE = dyn_cast<ConstantExpr>(C)) {
      // ignore bitcast
      if (CE->getOpcode() == Instruction::BitCast) {
        return convertTypeVarVal(CE->getOperand(0));
      }
      if (CE->getOpcode() == Instruction::IntToPtr) {
        if (auto Addr = dyn_cast<ConstantInt>(CE->getOperand(0))) {
          auto tv = makeTv(Memory);
          setOffset(tv, OffsetRange{.offset = static_cast<uint64_t>(
                                        Addr->getValue().getZExtValue())});
          return tv;
        }
      }
    } else if (auto gv = dyn_cast<GlobalValue>(C)) { // global variable
      if (gv == Ctx.StackPointer) {
        return makeTv(Stack);
      } else if (auto Func = dyn_cast<Function>(C)) {
        return makeTv(Ctx.getName(*Func, "func_"));
      }
      return makeTv(gv->getName().str());
    } else if (isa<ConstantInt>(C) || isa<ConstantFP>(C)) {
      auto Ty = C->getType();
      return getLLVMTypeVar(Ty);
    }
    llvm::errs()
        << __FILE__ << ":" << __LINE__ << ": "
        << "ERROR: RetypdGenerator::getTypeVar unhandled type of constant: "
        << *C << "\n";
    std::abort();
  } else if (auto arg = dyn_cast<Argument>(Val)) { // for function argument
    DerivedTypeVariable tv = getTypeVar(arg->getParent());
    tv.Labels.push_back(retypd::InLabel{std::to_string(arg->getArgNo())});
    return tv;
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "WARN: RetypdGenerator::getTypeVar unhandled value: " << *Val
               << "\n";
  return makeTv(Ctx.getName(*Val, "new_"));
}

// TODO: accept any character in name by using single quotes like LLVM IR.
std::string Retypd::sanitize_name(std::string S) {
  std::replace(S.begin(), S.end(), '.', '_');
  std::replace(S.begin(), S.end(), '@', '_');
  return S;
}

std::string Retypd::getName(Value &Val, const char *prefix) {
  if (!Val.hasName()) {
    auto Id = typeValId++;
    Val.setName(prefix + std::to_string(Id));
    return prefix + std::to_string(Id);
  }
  return Val.getName().str();
}

void RetypdGenerator::RetypdGeneratorVisitor::visitReturnInst(ReturnInst &I) {
  auto *Src = I.getReturnValue();
  if (Src == nullptr) { // ret void.
    return;
  }
  auto SrcVar = cg.getTypeVar(Src);
  auto DstVar = cg.getTypeVar(ReturnValue{.Func = I.getFunction()});
  // src is a subtype of dest
  cg.addSubtype(SrcVar, DstVar);
}

void RetypdGenerator::RetypdGeneratorVisitor::visitCallBase(CallBase &I) {
  auto Target = I.getCalledFunction();
  if (Target == nullptr) {
    // TODO indirect call
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Warn: RetypdGenerator: indirect call not supported yet\n";
    return;
  }
  // differentiate different call instances in the same function
  auto TargetName = cg.Ctx.getName(*Target, "func_");
  // Starts from 1. So that it is different from the default 0.
  auto InstanceId = ++cg.callInstanceId[TargetName];
  for (int i = 0; i < I.arg_size(); i++) {
    auto ArgVar = cg.getTypeVar(
        CallArg{.Call = &I, .InstanceId = InstanceId, .Index = i});
    auto ValVar = cg.getTypeVar(I.getArgOperand(i));
    // argument is a subtype of param
    cg.addSubtype(ValVar, ArgVar);
  }
  // for return value
  auto DstVar = cg.getTypeVar(CallRet{.Call = &I});
  cg.setTypeVar(&I, DstVar);
}

void RetypdGenerator::RetypdGeneratorVisitor::visitSelectInst(SelectInst &I) {
  auto DstVar = makeTv(cg.Ctx.getName(I, "select_"));
  auto *Src1 = I.getTrueValue();
  auto *Src2 = I.getFalseValue();
  auto Src1Var = cg.getTypeVar(Src1);
  auto Src2Var = cg.getTypeVar(Src2);
  // Not generate boolean constraints. Because it must be i1.
  // constrains.push_back("bool <= " + cg.getTypeVar(I.getCondition()));
  cg.addSubtype(Src1Var, DstVar);
  cg.addSubtype(Src2Var, DstVar);
  cg.setTypeVar(&I, DstVar);
}

void RetypdGenerator::RetypdGeneratorVisitor::visitAllocaInst(AllocaInst &I) {
  auto DstVar = makeTv(cg.Ctx.getName(I, "alloca_"));
  auto &Node = cg.setTypeVar(&I, DstVar);
  // set as pointer type
  Node.Link.lookupNode()->setStorageShape(retypd::Pointer{});
}

void RetypdGenerator::RetypdGeneratorVisitor::visitPHINode(PHINode &I) {
  // Create a stub variable, defer constraints generation (and unification) to
  // handlePHINodes Because it goes across the basic block boundary. Operands
  // may be not found using getTypeVar.

  // Only Phi can create inter-block dataflow besides direct reference (valid if
  // we follow a topological sort of basic blocks)

  auto DstVar = makeTv(cg.Ctx.getName(I, "phi_"));
  cg.setTypeVar(&I, DstVar);
}

void RetypdGenerator::RetypdGeneratorVisitor::handlePHINodes() {
  for (auto I : phiNodes) {
    auto DstVar = cg.getTypeVar(I);
    for (auto &Op : I->incoming_values()) {
      auto *Src = Op.get();
      auto SrcVar = cg.getTypeVar(Src);
      // src is a subtype of dest
      cg.addSubtype(SrcVar, DstVar);
    }
  }
}

void RetypdGenerator::RetypdGeneratorVisitor::visitCastInst(CastInst &I) {
  if (isa<PtrToIntInst, IntToPtrInst, BitCastInst>(I)) {
    // ignore cast, propagate the type of the operand.
    auto *Src = I.getOperand(0);
    auto SrcVar = cg.getTypeVar(Src);
    cg.setTypeVar(&I, SrcVar);
    return;
  } else if (auto trunc = dyn_cast<TruncInst>(&I)) {
    auto *Src = trunc->getOperand(0);
    auto SrcVar = cg.getTypeVar(Src);
    std::optional<DerivedTypeVariable> DstVar;
    // TODO
    if (trunc->getType()->isIntegerTy(1)) {
      DstVar = getLLVMTypeVar(trunc->getType());
    }
    if (DstVar.has_value()) {
      cg.addSubtype(SrcVar, *DstVar);
      return;
    }
  } else if (auto ZExt = dyn_cast<ZExtInst>(&I)) {
    // cast boolean to integer?
    if (is_size_t(ZExt, *I.getModule())) {
      auto *Src = ZExt->getOperand(0);
      auto SrcVar = cg.getTypeVar(Src);
      cg.setTypeVar(&I, SrcVar);
      return;
    }
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: RetypdGenerator::getTypeVar unhandled CastInst: " << I
               << "\n";
  std::abort();
}

void RetypdGenerator::RetypdGeneratorVisitor::visitGetElementPtrInst(
    GetElementPtrInst &Gep) {
  assert(false && "Gep should not exist before this pass");
  // But if we really want to support this, handle it the same way as AddInst.
}

// FCMP implies float?
// for pointer sized int, probably is pointer comparision. So we cannot make a
// conclusion.
void RetypdGenerator::RetypdGeneratorVisitor::visitICmpInst(ICmpInst &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  // for pointer
  if (Src1->getType()->isPointerTy()) {
    assert(Src2->getType()->isPointerTy() &&
           "RetypdGenerator: ICmpInst: Src2 is not pointer");

  } else if (is_size_t(Src1, *I.getModule())) {
    // for pointer-sized integer, the comparision may not be integer
    // comparision.
    assert(is_size_t(Src2, *I.getModule()) &&
           "RetypdGenerator: ICmpInst: Src2 is not size_t");
    if (isa<ConstantInt>(Src1) && !isa<ConstantInt>(Src2)) {
      // 2023.11.22 because of InstCombine, this should not happen?
      assert(false && "constant cannot be at the left side of cmp");
      std::swap(Src1, Src2);
    }
    // constant usually is at the right side.
    if (auto C = dyn_cast<ConstantInt>(Src2)) {
      // TODO no conclusion for now.
      // if (C->getValue().isZero()) {
    } else {
      // TODO no conclusion for now.
      // cg.addConstraint(Src2Var, Src1Var);
      // cg.addConstraint(Src1Var, Src2Var);
    }
  } else {
    auto Src2Var = cg.getTypeVar(Src2);
    auto Src1Var = cg.getTypeVar(Src1);

    cg.addSubtype(Src2Var, getLLVMTypeVar(Src1->getType()));
    cg.addSubtype(Src2Var, getLLVMTypeVar(Src2->getType()));
  }

  assert(I.getType()->isIntegerTy(1));
  cg.setTypeVar(&I, getLLVMTypeVar(I.getType()));
}

void RetypdGenerator::RetypdGeneratorVisitor::visitFCmpInst(FCmpInst &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  auto Src2Var = cg.getTypeVar(Src2);
  auto Src1Var = cg.getTypeVar(Src1);
  cg.addSubtype(Src2Var, getLLVMTypeVar(Src1->getType()));
  cg.addSubtype(Src2Var, getLLVMTypeVar(Src2->getType()));
  assert(I.getType()->isIntegerTy(1));
  cg.setTypeVar(&I, getLLVMTypeVar(I.getType()));
}

// #region LoadStore

// =========== begin: load/store insts and deref analysis ===========

unsigned RetypdGenerator::getPointerElemSize(Type *ty) {
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

void RetypdGenerator::RetypdGeneratorVisitor::visitStoreInst(StoreInst &I) {
  auto DstVar =
      cg.deref(I.getPointerOperand(),
               cg.getPointerElemSize(I.getPointerOperandType()), false);
  cg.addSubtype(cg.getTypeVar(I.getValueOperand()), DstVar);
}

void RetypdGenerator::RetypdGeneratorVisitor::visitLoadInst(LoadInst &I) {
  auto SrcVar =
      cg.deref(I.getPointerOperand(),
               cg.getPointerElemSize(I.getPointerOperandType()), true);
  cg.setTypeVar(&I, SrcVar);
}

std::string RetypdGenerator::offset(APInt Offset, int Count) {
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

void RetypdGenerator::setOffset(DerivedTypeVariable &dtv, OffsetRange Offset) {
  OffsetLabel *current;
  if (!dtv.Labels.empty() &&
      std::holds_alternative<OffsetLabel>(dtv.Labels.back())) {
    // current = &std::get<OffsetLabel>(dtv.Labels.back());
    assert(false && "not implemented merging of offset labels");
  } else {
    current = &std::get<OffsetLabel>(
        dtv.Labels.emplace_back(OffsetLabel{.range = Offset}));
  }
}

// Special logics for load and store when generating type variables.
DerivedTypeVariable RetypdGenerator::deref(Value *Val, long BitSize,
                                           bool isLoad) {
  assert(BitSize != 0 && "RetypdGenerator::deref: zero size!?");
  // from the offset, generate a loaded type variable.
  auto DstVar = getTypeVar(Val);
  assert(BitSize % 8 == 0 && "size must be byte aligned");
  uint32_t Size = BitSize = BitSize / 8;
  if (isLoad) {
    DstVar.Labels.push_back(retypd::LoadLabel{.Size = Size});
  } else {
    DstVar.Labels.push_back(retypd::StoreLabel{.Size = Size});
  }
  return DstVar;
}

// =========== end: load/store insts ===========

// #endregion LoadStore

// =========== begin: other insts ===========

const std::map<unsigned, RetypdGenerator::PcodeOpType>
    RetypdGenerator::opTypes = {
        // for Trunc, ZExt, SExt
        {Instruction::Trunc, {"int", (const char *[1]){"int"}, 1}},
        {Instruction::ZExt, {"uint", (const char *[1]){"int"}, 1}},
        {Instruction::SExt, {"int", (const char *[1]){"int"}, 1}},

        // other cast insts: FPToUIInst, FPToSIInst, UIToFPInst, SIToFPInst
        {Instruction::FPToUI, {"uint", (const char *[1]){"float"}, 1}},
        {Instruction::FPToSI, {"int", (const char *[1]){"float"}, 1}},
        {Instruction::UIToFP, {"float", (const char *[1]){"uint"}, 1}},
        {Instruction::SIToFP, {"float", (const char *[1]){"int"}, 1}},
        {Instruction::FPTrunc, {"float", (const char *[1]){"float"}, 1}},
        {Instruction::FPExt, {"float", (const char *[1]){"float"}, 1}},

        {Instruction::UDiv, {"uint", (const char *[2]){"uint", "uint"}, 2}},
        {Instruction::SDiv, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::URem, {"uint", (const char *[2]){"uint", "uint"}, 2}},
        {Instruction::SRem, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::Xor, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::FNeg, {"float", (const char *[2]){"float"}, 1}},
        {Instruction::FAdd, {"float", (const char *[2]){"float", "float"}, 2}},
        {Instruction::FSub, {"float", (const char *[2]){"float", "float"}, 2}},
        {Instruction::FMul, {"float", (const char *[2]){"float", "float"}, 2}},
        {Instruction::FDiv, {"float", (const char *[2]){"float", "float"}, 2}},

        {Instruction::Add, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::Sub, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::Mul, {"int", (const char *[2]){"int", "int"}, 2}},

        {Instruction::Shl, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::LShr, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::AShr, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::And, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::Or, {"int", (const char *[2]){"int", "int"}, 2}},
};

void RetypdGenerator::RetypdGeneratorVisitor::visitInstruction(Instruction &I) {
  if (opTypes.count(I.getOpcode())) {
    opTypes.at(I.getOpcode()).addConstrains(&I, cg);
  }
  llvm::errs() << "WARN: RetypdGeneratorVisitor unhandled instruction: " << I
               << "\n";
}

void RetypdGenerator::RetypdGeneratorVisitor::visitAdd(BinaryOperator &I) {
  llvm::errs() << "Visiting " << __FUNCTION__ << " \n";
}
void RetypdGenerator::RetypdGeneratorVisitor::visitSub(BinaryOperator &I) {
  llvm::errs() << "Visiting " << __FUNCTION__ << " \n";
}
void RetypdGenerator::RetypdGeneratorVisitor::visitMul(BinaryOperator &I) {
  llvm::errs() << "Visiting " << __FUNCTION__ << " \n";
}

void RetypdGenerator::RetypdGeneratorVisitor::visitShl(BinaryOperator &I) {
  llvm::errs() << "visiting " << __FUNCTION__ << " \n";
}
void RetypdGenerator::RetypdGeneratorVisitor::visitLShr(BinaryOperator &I) {
  llvm::errs() << "visiting " << __FUNCTION__ << " \n";
}
void RetypdGenerator::RetypdGeneratorVisitor::visitAShr(BinaryOperator &I) {
  llvm::errs() << "visiting " << __FUNCTION__ << " \n";
}
void RetypdGenerator::RetypdGeneratorVisitor::visitAnd(BinaryOperator &I) {
  llvm::errs() << "visiting " << __FUNCTION__ << " \n";
}
void RetypdGenerator::RetypdGeneratorVisitor::visitOr(BinaryOperator &I) {
  llvm::errs() << "Visiting " << __FUNCTION__ << " \n";
}

const std::map<std::string_view, std::map<int, std::string_view>>
    RetypdGenerator::typeSize = {
        {"int", {{8, "int8"}, {16, "int16"}, {32, "int32"}, {64, "int64"}}},
        {"uint",
         {{8, "uint8"}, {16, "uint16"}, {32, "uint32"}, {64, "uint64"}}},
        {"float", {{32, "float"}, {64, "double"}}},
};

void RetypdGenerator::PcodeOpType::addConstrains(Instruction *I,
                                                 RetypdGenerator &cg) const {
  if (!I->getType()->isVoidTy()) {
    const char *ty = output;
    // for this opcode, change type according to the size.
    if (typeSize.count(output)) {
      ty =
          typeSize.at(output).at(I->getType()->getPrimitiveSizeInBits()).data();
    }
    cg.addSubtype(makeTv(std::string("#") + ty), cg.getTypeVar(I));
  }
  assert(size == I->getNumOperands() && "input size not match");

  for (int i = 0; i < size; i++) {
    auto *Src = I->getOperand(i);
    auto SrcVar = cg.getTypeVar(Src);
    // hack:https://stackoverflow.com/questions/17787394/why-doesnt-stdinitializer-list-provide-a-subscript-operator
    cg.addSubtype(SrcVar, makeTv(std::string("#") + inputs[i]));
  }
}

// =========== end: other insts ===========

void Retypd::gen_json(std::string OutputFilename) {
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
