#include <cassert>
#include <iostream>
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

#include "optimizers/retypd-generate.h"
#include "optimizers/stack-pointer-finder.h"

#define DEBUG_TYPE "retypd"

namespace notdec {

const char *RetypdGenerator::Memory = "MEMORY";

/// Uses ptrtoint to ensure type correctness.
PreservedAnalyses RetypdRunner::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(errs() << " ============== RetypdGenerator  ===============\n");

  auto SP = MAM.getResult<StackPointerFinderAnalysis>(M);
  RetypdGenerator Generator;
  Generator.run(M, SP.result);

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

  Generator.gen_json(TempPath);

  errs() << "constrains path: " << TempPath << "\n";

  // clean up
  // delete directory if not debug
  LLVM_DEBUG(if (!Path.empty()) {
    assert(llvm::sys::fs::remove_directories(Path.str()) == std::errc());
  });

  LLVM_DEBUG(errs() << " ============== RetypdGenerator End ===============\n");
  return PreservedAnalyses::none();
}

void RetypdGenerator::run(Module &M, llvm::Value *StackPointer) {
  this->StackPointer = StackPointer;
  data_layout = std::move(M.getDataLayoutStr());
  pointer_size = M.getDataLayout().getPointerSizeInBits();
  for (auto &F : M) {
    run(F);
  }
  gen_call_graph(M);
}

void RetypdGenerator::run(Function &F) {
  current = &func_constrains[&F];
  RetypdGeneratorVisitor Visitor(*this, func_constrains[&F]);
  Visitor.visit(F);
}

// 都用这个函数，获取或插入
size_t RetypdGenerator::get_or_insert_value(Value *Val) {
  if (val2type_var.count(Val)) {
    return val2type_var.at(Val);
  } else {
    auto Id = type_val_id++;
    assert(val2type_var.emplace(Val, Id).second);
    assert(type_var2val.emplace(Id, Val).second);
    return Id;
  }
}

std::string llvm_type_var(Type *Ty) {
  if (Ty->isPointerTy()) {
    // 20231122 why there is pointer constant(not inttoptr constant expr)
    assert(false && "TODO can this be pointer?");
    return llvm_type_var(Ty->getPointerElementType()) + "*";
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

std::string RetypdGenerator::get_type_var(Value *Val) {
  // ignore irrelevant cast instructions
  if (is_cast(Val)) {
    return get_type_var(cast<CastInst>(Val)->getOperand(0));
  }
  if (Constant *C = dyn_cast<Constant>(Val)) {
    // check for constantExpr
    if (auto CE = dyn_cast<ConstantExpr>(C)) {
      // ignore bitcast
      if (CE->getOpcode() == Instruction::BitCast) {
        return get_type_var(CE->getOperand(0));
      }
      if (CE->getOpcode() == Instruction::IntToPtr) {
        if (auto Addr = dyn_cast<ConstantInt>(CE->getOperand(0))) {
          return deref(Memory, "int", getPointerElemSize(CE->getType()),
                       toString(Addr->getValue(), 10, false).c_str());
        }
      }
    }
    // global variable
    if (isa<GlobalValue>(C)) {
      return get_named_type_var(Val);
    }
    if (isa<ConstantInt>(C) || isa<ConstantFP>(C)) {
      auto Ty = C->getType();
      return llvm_type_var(Ty);
    }
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "ERROR: unhandled type of constant: " << *C << "\n";
    std::abort();
  }
  return get_named_type_var(Val);
}

// get id, and return string name.
std::string RetypdGenerator::get_named_type_var(Value *val) {
  bool IsNew = false;
  size_t Id;
  std::string Ret;
  if (val2type_var.count(val)) {
    // exists
    Id = val2type_var.at(val);
  } else {
    // insert new
    Id = type_val_id++;
    assert(val2type_var.emplace(val, Id).second);
    assert(type_var2val.emplace(Id, val).second);

    IsNew = true;
  }
  if (auto arg = dyn_cast<Argument>(val)) {
    Ret = arg2name(get_func_name(*arg->getParent()), arg->getArgNo());
  } else {
    Ret = id2name(Id);
  }

  if (IsNew) {
    if (auto gv = dyn_cast<GlobalVariable>(val)) {
    }
  }
  return Ret;
}

std::string RetypdGenerator::sanitize_name(std::string S) {
  std::replace(S.begin(), S.end(), '.', '_');
  std::replace(S.begin(), S.end(), '@', '_');
  return S;
}

std::string RetypdGenerator::get_func_name(Function &Func) {
  if (Func.hasName()) {
    return sanitize_name(Func.getName().str());
  } else {
    // only enable this if assert_not_exist is false
    if (val2type_var.count(&Func)) {
      return "Fun_" + std::to_string(val2type_var.at(&Func));
    } else {
      auto Id = type_val_id++;
      assert(val2type_var.emplace(&Func, Id).second);
      assert(type_var2val.emplace(Id, &Func).second);
      return "Fun_" + std::to_string(Id);
    }
  }
}

std::string RetypdGenerator::id2name(size_t Id) {
  return "v_" + std::to_string(Id);
}

std::string RetypdGenerator::arg2name(std::string FuncName, unsigned Ind) {
  // TODO sanitize funcName
  return FuncName + ".in_" + std::to_string(Ind);
}

std::string RetypdGenerator::ret2name(std::string FuncName) {
  // TODO sanitize funcName
  return FuncName + ".out";
}

void RetypdGenerator::RetypdGeneratorVisitor::visitReturnInst(ReturnInst &I) {
  auto *Src = I.getReturnValue();
  if (Src == nullptr) { // ret void.
    return;
  }
  auto SrcVar = cg.get_type_var(Src);
  // src is a subtype of dest
  constrains.push_back(SrcVar +
                       " <= " + ret2name(cg.get_func_name(*I.getFunction())));
}

void RetypdGenerator::RetypdGeneratorVisitor::visitCallBase(CallBase &I) {
  auto *Dst = &I;
  auto DstVar = cg.get_type_var(Dst);
  auto Target = I.getCalledFunction();
  if (Target == nullptr) {
    // TODO indirect call
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Warn: indirect call not supported yet\n";
    return;
  }
  for (int i = 0; i < I.arg_size(); i++) {
    auto ParamVar = cg.get_type_var(Target->getArg(i));
    auto ArgumentVar = cg.get_type_var(I.getArgOperand(i));
    // argument is a subtype of param
    constrains.push_back(ArgumentVar + " <= " + ParamVar);
  }
}

void RetypdGenerator::RetypdGeneratorVisitor::visitSelectInst(SelectInst &I) {
  auto DstVar = cg.get_type_var(&I);
  auto *Src1 = I.getTrueValue();
  auto *Src2 = I.getFalseValue();
  auto Src1Var = cg.get_type_var(Src1);
  auto Src2Var = cg.get_type_var(Src2);
  constrains.push_back("bool <= " + cg.get_type_var(I.getCondition()));
  constrains.push_back(Src1Var + " <= " + DstVar);
  constrains.push_back(Src2Var + " <= " + DstVar);
}

void RetypdGenerator::RetypdGeneratorVisitor::visitPHINode(PHINode &I) {
  auto DstVar = cg.get_type_var(&I);
  for (auto &Op : I.incoming_values()) {
    auto *Src = Op.get();
    auto SrcVar = cg.get_type_var(Src);
    // src is a subtype of dest
    constrains.push_back(SrcVar + " <= " + DstVar);
  }
}

void RetypdGenerator::RetypdGeneratorVisitor::visitCastInst(CastInst &I) {
  if (isa<PtrToIntInst>(I) || isa<IntToPtrInst>(I) || isa<BitCastInst>(I) ||
      isa<AddrSpaceCastInst>(I)) {
    // ignore cast, view as assignment.
    auto *Src = I.getOperand(0);
    auto *Dst = &I;
    auto SrcVar = cg.get_type_var(Src);
    auto DstVar = cg.get_type_var(Dst);
    constrains.push_back(SrcVar + " <= " + DstVar);
  }
}

// FCMP implies float?
void RetypdGenerator::RetypdGeneratorVisitor::visitCmpInst(CmpInst &I) {
  auto *Src1 = I.getOperand(0);
  // constant usually at the right side.
  auto *Src2 = I.getOperand(1);
  if (isa<Constant>(Src1) && !isa<Constant>(Src2)) {
    // 2023.11.22 because of InstCombine, this should not happen?
    assert(false && "constant cannot be at the left side of cmp");
    std::swap(Src1, Src2);
  }
  auto ResultVar = cg.get_type_var(&I);
  auto Src2Var = cg.get_type_var(Src2);
  auto Src1Var = cg.get_type_var(Src1);
  constrains.push_back(Src2Var + " <= " + Src1Var);
  constrains.push_back("bool <= " + ResultVar);
}

// =========== begin: load/store insts and deref analysis ===========

unsigned RetypdGenerator::getPointerElemSize(Type *ty) {
  Type *Elem = ty->getPointerElementType();
  unsigned Size = Elem->getPrimitiveSizeInBits();
  if (Size != 0) {
    return Size;
  }
  if (Elem->isPointerTy()) {
    assert(pointer_size != 0 &&
           "RetypdGenerator: pointer size not initialized");
    return pointer_size;
  }
  assert(false && "unknown pointer type");
}

void RetypdGenerator::RetypdGeneratorVisitor::visitStoreInst(StoreInst &I) {
  auto DstVar =
      cg.deref_label(I.getPointerOperand(),
                     cg.getPointerElemSize(I.getPointerOperandType()), "store");
  constrains.push_back(cg.get_type_var(I.getValueOperand()) + " <= " + DstVar);
}

void RetypdGenerator::RetypdGeneratorVisitor::visitLoadInst(LoadInst &I) {
  auto SrcVar =
      cg.deref_label(I.getPointerOperand(),
                     cg.getPointerElemSize(I.getPointerOperandType()), "load");
  constrains.push_back(SrcVar + " <= " + cg.get_type_var(&I));
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

std::string RetypdGenerator::deref(std::string Label, const char *Mode,
                                   long BitSize, const char *OffsetStr) {
  assert(BitSize % 8 == 0 && "size must be byte aligned");
  long Size = BitSize = BitSize / 8;
  return Label + "." + Mode + ".σ" + std::to_string(Size) + "@" + OffsetStr;
}

// Special logics for load and store when generating type variables.
std::string RetypdGenerator::deref_label(Value *Val, long BitSize,
                                         const char *Mode) {
  assert(BitSize != 0 && "zero size!?");
  if (auto *Inst = dyn_cast<Instruction>(Val)) {
    // ignore cast
    if (is_cast(Inst)) {
      return deref_label(Inst->getOperand(0), BitSize, Mode);
    }
    // if instruction, deref according to the type of the instruction.
    switch (Inst->getOpcode()) {
    case Instruction::GetElementPtr: {
      // current consider only i8* or i8 array
      auto *Gep = dyn_cast<GetElementPtrInst>(Inst);
      auto *ETy = Gep->getSourceElementType();
      std::string Off;
      // gep i8, i8*
      if (ETy->isIntegerTy(8)) {
        auto v1 = Gep->getOperand(1);
        if (auto c = dyn_cast<ConstantInt>(v1)) {
          Off = c->getSExtValue();
        } else {
          std::cerr << __FILE__ << ":" << __LINE__ << ": "
                    << "Warn: gep offset not constant\n";
          Off = "0*[nobound]";
        }
      } else if (ETy->isArrayTy() &&
                 ETy->getArrayElementType()->isIntegerTy(8)) {
        // gep [n x i8], [n x i8]*
        assert(cast<ConstantInt>(Gep->getOperand(1))->getSExtValue() == 0);
        auto V1 = Gep->getOperand(1);
        if (auto c = dyn_cast<ConstantInt>(V1)) {
          Off = c->getSExtValue();
        } else {
          std::cerr << __FILE__ << ":" << __LINE__ << ": "
                    << "Warn: gep offset not constant\n";
          Off = "0*[nobound]";
        }
      } else {
        llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                     << "Warn: unknown gep: " << *Inst << "\n";
        Off = "0*[nobound]";
      }
      // off is initialized in each branch
      return deref(get_type_var(Gep->getOperand(0)), Mode, BitSize,
                   Off.c_str());
    }
    default:
      break;
    }
  } else if (auto CExp = dyn_cast<ConstantExpr>(Val)) {
    auto Op = CExp->getOpcode();
    if (Op == Instruction::IntToPtr) {
      if (auto Addr = dyn_cast<ConstantInt>(CExp->getOperand(0))) {
        // e.g. i32* inttoptr (i32 1024 to i32*)
        // the offset should be unsigned.
        return deref(Memory, Mode, getPointerElemSize(CExp->getType()),
                     toString(Addr->getValue(), 10, false).c_str());
      }
    }
  } else {
    return deref(get_type_var(Val), Mode, BitSize, 0);
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "Warn: Unhandled value in deref_label: " << *Val << " \n";
  std::abort();
}

// =========== end: load/store insts ===========

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

        // {Instruction::Add, {"int", {"int", "int"}}},
        // {Instruction::Sub, {"int", {"int", "int"}}},
        // {Instruction::Mul, {"int", {"int", "int"}}},

        // {Instruction::Shl, {"int", {"int", "int"}}},
        // {Instruction::LShr, {"int", {"int", "int"}}},
        // {Instruction::AShr, {"int", {"int", "int"}}},
        // {Instruction::And, {"int", {"int", "int"}}},
        // {Instruction::Or, {"int", {"int", "int"}}},
};

void RetypdGenerator::RetypdGeneratorVisitor::visitInstruction(Instruction &I) {
  if (opTypes.count(I.getOpcode())) {
    opTypes.at(I.getOpcode()).addConstrains(&I, constrains, cg);
  }
}

const std::map<std::string_view, std::map<int, std::string_view>>
    RetypdGenerator::typeSize = {
        {"int", {{8, "int8"}, {16, "int16"}, {32, "int32"}, {64, "int64"}}},
        {"uint",
         {{8, "uint8"}, {16, "uint16"}, {32, "uint32"}, {64, "uint64"}}},
        {"float", {{32, "float"}, {64, "double"}}},
};

void RetypdGenerator::PcodeOpType::addConstrains(
    Instruction *I, std::vector<std::string> &constrains,
    RetypdGenerator &cg) const {
  if (!I->getType()->isVoidTy()) {
    const char *ty = output;
    // for this opcode, change type according to the size.
    if (typeSize.count(output)) {
      ty =
          typeSize.at(output).at(I->getType()->getPrimitiveSizeInBits()).data();
    }
    constrains.push_back(std::string(ty) + " <= " + cg.get_type_var(I));
  }
  assert(size == I->getNumOperands() && "input size not match");

  for (int i = 0; i < size; i++) {
    auto *Src = I->getOperand(i);
    auto SrcVar = cg.get_type_var(Src);
    // hack:https://stackoverflow.com/questions/17787394/why-doesnt-stdinitializer-list-provide-a-subscript-operator
    constrains.push_back(SrcVar + " <= " + inputs[i]);
  }
}

// =========== end: other insts ===========

void RetypdGenerator::gen_json(std::string OutputFilename) {
  json::Object Root({{"data_layout", data_layout}});

  json::Object Constraints;
  // iterate func_constrains
  for (auto &kv : func_constrains) {
    auto funcName = get_func_name(*kv.first);
    json::Array FuncConstrainsJson;
    for (auto &c : kv.second) {
      FuncConstrainsJson.push_back(c);
    }
    Constraints[funcName] = json::Value(std::move(FuncConstrainsJson));
  }
  Root["constraints"] = json::Value(std::move(Constraints));

  // iterate call_graphs
  json::Object CallGraph;
  for (auto &kv : call_graphs) {
    auto funcName = get_func_name(*kv.first);
    json::Array CallGraphJson;
    for (auto &c : kv.second) {
      CallGraphJson.push_back(get_func_name(*c));
    }
    CallGraph[funcName] = std::move(CallGraphJson);
  }
  Root["callgraph"] = std::move(CallGraph);

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

void RetypdGenerator::gen_call_graph(Module &M) {
  CallGraph CG(M);
  for (auto &Node : CG) {
    auto func = const_cast<Function *>(Node.first);
    if (func == nullptr) {
      continue;
    }
    std::set<Function *> Callees;
    for (auto &Edge : *Node.second) {
      auto Callee = const_cast<Function *>(Edge.second->getFunction());
      if (Callee == nullptr) {
        continue;
      }
      Callees.emplace(Callee);
    }
    call_graphs.emplace(func, std::move(Callees));
  }
}

} // namespace notdec
