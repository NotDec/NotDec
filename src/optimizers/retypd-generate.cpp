#include "optimizers/retypd-generate.h"
#include <cassert>
#include <iostream>
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

#include <string>
#include <utility>

namespace notdec::optimizers {

static const bool is_debug = true;

/// Uses ptrtoint to ensure type correctness.
PreservedAnalyses RetypdRunner::run(Module &M, ModuleAnalysisManager &MAM) {
  errs() << " ============== RetypdGenerator  ===============\n";

  RetypdGenerator generator;
  generator.run(M);

  // create a temporary directory
  SmallString<128> Path;
  std::error_code EC;
  EC = llvm::sys::fs::createUniqueDirectory("notdec_retypd", Path);
  if (!EC) {
    // Resolve any symlinks in the new directory.
    std::string UnresolvedPath(Path.str());
    EC = llvm::sys::fs::real_path(UnresolvedPath, Path);
  }
  auto temp_path = Path.str().str() + "/retypd-constrains.json";

  generator.gen_json(temp_path);

  errs() << "constrains path: " << temp_path << "\n";

  // clean up
  // delete directory if not debug
  if (!is_debug && !Path.empty()) {
    assert(llvm::sys::fs::remove_directories(Path.str()) == std::errc());
  }

  errs() << " ============== RetypdGenerator End ===============\n";
  return PreservedAnalyses::none();
}

void RetypdGenerator::run(Module &M) {
  data_layout = std::move(M.getDataLayoutStr());
  pointer_size = M.getDataLayout().getPointerSizeInBits();
  for (auto &F : M) {
    run(F);
  }
  gen_call_graph(M);
}

void RetypdGenerator::run(Function &F) {
  current = &func_constrains[&F];
  RetypdGeneratorVisitor visitor(*this, func_constrains[&F]);
  visitor.visit(F);
}

// 都用这个函数，获取或插入
size_t RetypdGenerator::get_or_insert_value(Value *val) {
  if (val2type_var.count(val)) {
    return val2type_var.at(val);
  } else {
    auto id = type_val_id++;
    assert(val2type_var.emplace(val, id).second);
    assert(type_var2val.emplace(id, val).second);
    return id;
  }
}

std::string llvm_type_var(Type *ty) {
  if (ty->isPointerTy()) {
    // 20231122 why there is pointer constant(not inttoptr constant expr)
    assert(false && "TODO can this be pointer?");
    return llvm_type_var(ty->getPointerElementType()) + "*";
  }
  if (ty->isFloatTy()) {
    return "float";
  } else if (ty->isDoubleTy()) {
    return "double";
  } else if (ty->isIntegerTy()) {
    if (ty->getIntegerBitWidth() == 1)
      return "bool";
    if (ty->getIntegerBitWidth() == 8)
      return "uint8";
    if (ty->getIntegerBitWidth() == 16)
      return "uint16";
    if (ty->getIntegerBitWidth() == 32)
      return "uint32";
    if (ty->getIntegerBitWidth() == 64)
      return "uint64";
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: unknown integer type: " << *ty << "\n";
    assert(false && "unknown integer type");
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: unknown constant type: " << *ty << "\n";
    assert(false && "unknown constant type");
  }
}

// get id, and return string name.
std::string RetypdGenerator::get_type_var(Value *val) {
  bool is_new = false;
  size_t id;
  std::string ret;
  if (val2type_var.count(val)) {
    // exists
    id = val2type_var.at(val);
  } else {
    // insert new
    id = type_val_id++;
    assert(val2type_var.emplace(val, id).second);
    assert(type_var2val.emplace(id, val).second);

    is_new = true;
  }
  if (auto arg = dyn_cast<Argument>(val)) {
    ret = arg2name(get_func_name(*arg->getParent()), arg->getArgNo());
  } else {
    ret = id2name(id);
  }

  if (is_new) {
    // if constant, insert additional constraint.
    if (auto cexp = dyn_cast<ConstantExpr>(val)) {

      // inttoptr constant <= void* ?
      current->push_back("┬* <= " + ret);
    } else if (auto gv = dyn_cast<GlobalVariable>(val)) {
    } else if (Constant *c = dyn_cast<Constant>(val)) {
      if (c->isNullValue()) {

      } else {
        auto ty = c->getType();
        current->push_back(llvm_type_var(ty) + " <= " + ret);
      }
    }
  }
  return ret;
}

std::string RetypdGenerator::sanitize_name(std::string s) {
  std::replace(s.begin(), s.end(), '.', '_');
  std::replace(s.begin(), s.end(), '@', '_');
  return s;
}

std::string RetypdGenerator::get_func_name(Function &func) {
  if (func.hasName()) {
    return sanitize_name(func.getName().str());
  } else {
    // only enable this if assert_not_exist is false
    if (val2type_var.count(&func)) {
      return "Fun_" + std::to_string(val2type_var.at(&func));
    } else {
      auto id = type_val_id++;
      assert(val2type_var.emplace(&func, id).second);
      assert(type_var2val.emplace(id, &func).second);
      return "Fun_" + std::to_string(id);
    }
  }
}

std::string RetypdGenerator::id2name(size_t id) {
  return "v_" + std::to_string(id);
}

std::string RetypdGenerator::arg2name(std::string funcName, unsigned ind) {
  // TODO sanitize funcName
  return funcName + ".in_" + std::to_string(ind);
}

std::string RetypdGenerator::ret2name(std::string funcName) {
  // TODO sanitize funcName
  return funcName + ".out";
}

void RetypdGenerator::RetypdGeneratorVisitor::visitReturnInst(ReturnInst &I) {
  auto *src = I.getReturnValue();
  if (src == nullptr) { // ret void.
    return;
  }
  auto src_var = cg.get_type_var(src);
  // src is a subtype of dest
  constrains.push_back(src_var +
                       " <= " + ret2name(cg.get_func_name(*I.getFunction())));
}

void RetypdGenerator::RetypdGeneratorVisitor::visitCallBase(CallBase &I) {
  auto *dst = &I;
  auto dst_var = cg.get_type_var(dst);
  auto target = I.getCalledFunction();
  if (target == nullptr) {
    // TODO indirect call
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Warn: indirect call not supported yet\n";
    return;
  }
  for (int i = 0; i < I.arg_size(); i++) {
    auto param_var = cg.get_type_var(target->getArg(i));
    auto argument_var = cg.get_type_var(I.getArgOperand(i));
    // argument is a subtype of param
    constrains.push_back(argument_var + " <= " + param_var);
  }
}

void RetypdGenerator::RetypdGeneratorVisitor::visitSelectInst(SelectInst &I) {
  auto dst_var = cg.get_type_var(&I);
  auto *src1 = I.getTrueValue();
  auto *src2 = I.getFalseValue();
  auto src1_var = cg.get_type_var(src1);
  auto src2_var = cg.get_type_var(src2);
  constrains.push_back("bool <= " + cg.get_type_var(I.getCondition()));
  constrains.push_back(src1_var + " <= " + dst_var);
  constrains.push_back(src2_var + " <= " + dst_var);
}

void RetypdGenerator::RetypdGeneratorVisitor::visitPHINode(PHINode &I) {
  auto dst_var = cg.get_type_var(&I);
  for (auto &op : I.incoming_values()) {
    auto *src = op.get();
    auto src_var = cg.get_type_var(src);
    // src is a subtype of dest
    constrains.push_back(src_var + " <= " + dst_var);
  }
}

void RetypdGenerator::RetypdGeneratorVisitor::visitCastInst(CastInst &I) {
  if (isa<PtrToIntInst>(I) || isa<IntToPtrInst>(I) || isa<BitCastInst>(I) ||
      isa<AddrSpaceCastInst>(I)) {
    // ignore cast, view as assignment.
    auto *src = I.getOperand(0);
    auto *dst = &I;
    auto src_var = cg.get_type_var(src);
    auto dst_var = cg.get_type_var(dst);
    constrains.push_back(src_var + " <= " + dst_var);
  }
}

// FCMP implies float?
void RetypdGenerator::RetypdGeneratorVisitor::visitCmpInst(CmpInst &I) {
  auto *src1 = I.getOperand(0);
  // constant usually at the right side.
  auto *src2 = I.getOperand(1);
  if (isa<Constant>(src1) && !isa<Constant>(src2)) {
    // 2023.11.22 because of InstCombine, this should not happen?
    assert(false && "constant cannot be at the left side of cmp");
    std::swap(src1, src2);
  }
  auto result_var = cg.get_type_var(&I);
  auto src2_var = cg.get_type_var(src2);
  auto src1_var = cg.get_type_var(src1);
  constrains.push_back(src2_var + " <= " + src1_var);
  constrains.push_back("bool <= " + result_var);
}

// =========== begin: load/store insts and deref analysis ===========

unsigned RetypdGenerator::getPointerElemSize(Type *ty) {
  Type *elem = ty->getPointerElementType();
  unsigned size = elem->getPrimitiveSizeInBits();
  if (size != 0) {
    return size;
  }
  if (elem->isPointerTy()) {
    assert(pointer_size != 0 &&
           "RetypdGenerator: pointer size not initialized");
    return pointer_size;
  }
  assert(false && "unknown pointer type");
}

void RetypdGenerator::RetypdGeneratorVisitor::visitStoreInst(StoreInst &I) {
  auto dst_var =
      cg.deref_label(I.getPointerOperand(),
                     cg.getPointerElemSize(I.getPointerOperandType()), "store");
  constrains.push_back(cg.get_type_var(I.getValueOperand()) + " <= " + dst_var);
}

void RetypdGenerator::RetypdGeneratorVisitor::visitLoadInst(LoadInst &I) {
  auto src_var =
      cg.deref_label(I.getPointerOperand(),
                     cg.getPointerElemSize(I.getPointerOperandType()), "load");
  constrains.push_back(src_var + " <= " + cg.get_type_var(&I));
}

std::string RetypdGenerator::deref(std::string label, const char *mode,
                                   long bit_size, int offset, int count) {
  assert(bit_size % 8 == 0 && "size must be byte aligned");
  long size = bit_size = bit_size / 8;
  std::string offsetStr;
  std::string countStr;

  switch (offset) {
  case -1:
    offsetStr = "0*[nobound]";
    break;
  case -2:
    offsetStr = "0*[nullterm]";
    break;
  default:
    offsetStr = std::to_string(offset);
    break;
  }
  countStr = (count > 1 ? "*" + std::to_string(count) : "");

  return label + "." + mode + ".σ" + std::to_string(size) + "@" + offsetStr +
         countStr;
}

std::string RetypdGenerator::deref_label(Value *val, long bit_size,
                                         const char *mode) {
  assert(bit_size != 0 && "zero size!?");
  // if not instruction, directly deref
  if (!isa<Instruction>(val)) {
    return deref(get_type_var(val), mode, bit_size, 0);
  } else {
    auto *inst = dyn_cast<Instruction>(val);
    // ignore cast
    if (is_cast(inst)) {
      return deref_label(inst->getOperand(0), bit_size, mode);
    }
    // if instruction, deref according to the type of the instruction.
    switch (inst->getOpcode()) {
    case Instruction::GetElementPtr: {
      // current consider only i8* or i8 array
      auto *gep = dyn_cast<GetElementPtrInst>(inst);
      auto *ety = gep->getSourceElementType();
      int64_t off;
      // gep i8, i8*
      if (ety->isIntegerTy(8)) {
        auto v1 = gep->getOperand(1);
        if (auto c = dyn_cast<ConstantInt>(v1)) {
          off = c->getSExtValue();
        } else {
          std::cerr << __FILE__ << ":" << __LINE__ << ": "
                    << "Warn: gep offset not constant\n";
          off = -1;
        }
      } else if (ety->isArrayTy() &&
                 ety->getArrayElementType()->isIntegerTy(8)) {
        // gep [n x i8], [n x i8]*
        assert(cast<ConstantInt>(gep->getOperand(1))->getSExtValue() == 0);
        auto v1 = gep->getOperand(1);
        if (auto c = dyn_cast<ConstantInt>(v1)) {
          off = c->getSExtValue();
        } else {
          std::cerr << __FILE__ << ":" << __LINE__ << ": "
                    << "Warn: gep offset not constant\n";
          off = -1;
        }
      } else {
        llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                     << "Warn: unknown gep: " << *inst << "\n";
        off = -1;
      }
      // off is initialized in each branch
      return deref(get_type_var(gep->getOperand(0)), mode, bit_size, off);
    }
    default:
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Warn: Unhandled inst in deref_label: "
                << inst->getOpcodeName() << " \n";
      return deref(get_type_var(val), mode, bit_size, 0);
    }
  }
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
    auto *src = I->getOperand(i);
    auto src_var = cg.get_type_var(src);
    // hack:https://stackoverflow.com/questions/17787394/why-doesnt-stdinitializer-list-provide-a-subscript-operator
    constrains.push_back(src_var + " <= " + inputs[i]);
  }
}

// =========== end: other insts ===========

void RetypdGenerator::gen_json(std::string outputFilename) {
  json::Object root({{"data_layout", data_layout}});

  json::Object constraints;
  // iterate func_constrains
  for (auto &kv : func_constrains) {
    auto funcName = get_func_name(*kv.first);
    json::Array func_constrains_json;
    for (auto &c : kv.second) {
      func_constrains_json.push_back(c);
    }
    constraints[funcName] = json::Value(std::move(func_constrains_json));
  }
  root["constraints"] = json::Value(std::move(constraints));

  // iterate call_graphs
  json::Object call_graph;
  for (auto &kv : call_graphs) {
    auto funcName = get_func_name(*kv.first);
    json::Array call_graph_json;
    for (auto &c : kv.second) {
      call_graph_json.push_back(get_func_name(*c));
    }
    call_graph[funcName] = std::move(call_graph_json);
  }
  root["callgraph"] = std::move(call_graph);

  // write to file
  std::error_code EC;
  llvm::raw_fd_ostream os(outputFilename, EC);
  if (EC) {
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Cannot open output json file." << std::endl;
    std::cerr << EC.message() << std::endl;
    std::abort();
  }
  json::OStream J(os, 2);
  J.value(std::move(root));
}

void RetypdGenerator::gen_call_graph(Module &M) {
  CallGraph CG(M);
  for (auto &node : CG) {
    auto func = const_cast<Function *>(node.first);
    if (func == nullptr) {
      continue;
    }
    std::set<Function *> callees;
    for (auto &edge : *node.second) {
      auto callee = const_cast<Function *>(edge.second->getFunction());
      if (callee == nullptr) {
        continue;
      }
      callees.emplace(callee);
    }
    call_graphs.emplace(func, std::move(callees));
  }
}

} // namespace notdec::optimizers
