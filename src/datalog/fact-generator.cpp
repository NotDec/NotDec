#include "datalog/fact-generator.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include "datalog/fact-names.def"

namespace notdec::datalog {

using val_t = FactGenerator::val_t;

val_t FactGenerator::get_value_by_id(size_t vid) {
  if (id2value.count(vid) > 0) {
    return id2value.at(vid);
  }
  return nullptr;
}

size_t FactGenerator::get_value_id(val_t val) { return value2id.at(val); }

void FactGenerator::visit_module() {
  visit_gvs();
  visit_functions();
  auto sizet =
      visit_type(mod.getDataLayout().getIntPtrType(mod.getContext(), 0));
  append_fact(FACT_SizeType, to_fact_str(sizet));
}

void FactGenerator::generate(llvm::Module &mod, const char *outputDirname) {
  FactGenerator fg(mod);
  fg.visit_module();
  if (outputDirname != nullptr) {
    fg.output_files(outputDirname);
  }
}

void FactGenerator::output_files(std::string outputDirname) {
  // if dir not exist, create it
  mkdir(outputDirname.c_str(), 0755);
  for (auto &kv : facts) {
    std::string fact_filename = outputDirname + "/" + kv.first + ".facts";
    std::ofstream fact_file(fact_filename);
    fact_file << kv.second;
    fact_file.close();
    if (!fact_file) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Error: " << strerror(errno);
      std::abort();
    }
  }
}

// https://github.com/llvm/llvm-project/blob/7cbf1a2591520c2491aa35339f227775f4d3adf6/llvm/lib/IR/AsmWriter.cpp#L536
size_t FactGenerator::visit_type(llvm::Type *ty) {
  size_t id = -1;
  std::pair<size_t, bool> ins;
  size_t id_dep = -1;
  std::string type_str;
  switch (ty->getTypeID()) {
  case Type::VoidTyID:
    type_str = "void";
    break;
  case Type::HalfTyID:
    type_str = "half";
    break;
  case Type::BFloatTyID:
    type_str = "bfloat";
    break;
  case Type::FloatTyID:
    type_str = "float";
    break;
  case Type::DoubleTyID:
    type_str = "double";
    break;
  case Type::X86_FP80TyID:
    type_str = "x86_fp80";
    break;
  case Type::FP128TyID:
    type_str = "fp128";
    break;
  case Type::PPC_FP128TyID:
    type_str = "ppc_fp128";
    break;
  case Type::LabelTyID:
    type_str = "label";
    break;
  case Type::MetadataTyID:
    type_str = "metadata";
    break;
  case Type::X86_MMXTyID:
    type_str = "x86_mmx";
    break;
  case Type::X86_AMXTyID:
    type_str = "x86_amx";
    break;
  case Type::TokenTyID:
    type_str = "token";
    break;
  case llvm::Type::FunctionTyID:
    type_str = "function";
    break;
  case llvm::Type::IntegerTyID:
    type_str = printType(ty);
    break;
  case llvm::Type::PointerTyID:
    id_dep = visit_type(ty->getPointerElementType());
    type_str = printType(ty);
    ins = get_or_insert_type(type_str);
    id = ins.first;
    if (ins.second) {
      append_fact(FACT_PointerType, to_fact_str(id, id_dep));
    }
    return id;
  case llvm::Type::StructTyID:
    type_str = printType(ty);
    ins = get_or_insert_type(type_str);
    id = ins.first;
    if (ins.second) {
      append_fact(FACT_StructType, to_fact_str(id, type_str));
    }
    return id;
  case llvm::Type::ArrayTyID:
    id_dep = visit_type(ty->getArrayElementType());
    type_str = printType(ty);
    ins = get_or_insert_type(type_str);
    id = ins.first;
    if (ins.second) {
      append_fact(FACT_ArrayType,
                  to_fact_str(id, id_dep, ty->getArrayNumElements()));
    }
    return id;
  case llvm::Type::FixedVectorTyID:
    id_dep = visit_type(ty->getArrayElementType());
    type_str = printType(ty);
    ins = get_or_insert_type(type_str);
    id = ins.first;
    if (ins.second) {
      append_fact(FACT_VectorType,
                  to_fact_str(id, id_dep, ty->getArrayNumElements()));
    }
    return id;
  case llvm::Type::ScalableVectorTyID:
    // default:
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Unsupported type: " << printType(ty) << std::endl;
    std::abort();
  }
  ins = get_or_insert_type(type_str);
  id = ins.first;
  if (ins.second) {
    append_fact(FACT_BaseType, to_fact_str(id, type_str));
  }
  return id;
}

void FactGenerator::visit_gvs() {
  for (auto &gv : mod.globals()) {
    auto name = getNameOrAsOperand(gv);
    auto id = get_or_insert_value(&gv, true);
    append_fact(FACT_GlobalVariable, to_fact_str(id, name));

    // auto type = printType(gv.getValueType());
    auto tid = visit_type(gv.getValueType());
    append_fact(FACT_ValueType, to_fact_str(id, tid));

    size_t isConst = gv.isConstant() ? 1 : 0;
    auto linkage = getLinkageName(gv.getLinkage());
    auto visibility = getVisibilityName(gv.getVisibility());
    append_fact(FACT_GlobalVarAttr,
                to_fact_str(id, isConst, linkage, visibility));
  }
}

// 2nd arg: is_inserted
std::pair<size_t, bool> FactGenerator::get_or_insert_type(std::string key) {
  // https://stackoverflow.com/questions/1409454/c-map-find-to-possibly-insert-how-to-optimize-operations
  typedef std::map<std::string, size_t> M;
  typedef M::iterator I;
  std::pair<I, bool> const &r = type_ids.insert(M::value_type(key, typeID));
  if (r.second) {
    // value was inserted; now my_map[foo_obj]="some value"
    return std::pair<size_t, bool>(typeID++, true);
  } else {
    // value wasn't inserted because my_map[foo_obj] already existed.
    // note: the old value is available through r.first->second
    // and may not be "some value"
    return std::pair<size_t, bool>(r.first->second, false);
  }
}

void FactGenerator::append_fact(const char *key, std::string to_append) {
  // https://stackoverflow.com/questions/1409454/c-map-find-to-possibly-insert-how-to-optimize-operations
  typedef std::map<const char *, std::string> M;
  typedef M::iterator I;
  std::pair<I, bool> const &r = facts.insert(M::value_type(key, to_append));
  if (r.second) {
    // value was inserted; now my_map[foo_obj]="some value"
  } else {
    // value wasn't inserted because my_map[foo_obj] already existed.
    // note: the old value is available through r.first->second
    // and may not be "some value"
    r.first->second.append(to_append);
  }
}

size_t FactGenerator::visit_operand(llvm::Value &val) {
  // 1. operand is other value(e.g. instruction, gv...)
  // 2. operand is constant
  if (value2id.count(&val)) {
    return value2id.at(&val);
  } else if (isa<Constant>(val)) {
    assert(!isa<Function>(val));
    Constant *con = static_cast<Constant *>(&val);
    return visit_constant(*con);
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Unsupported inst operand: " << val << "\n";
    std::abort();
  }
}

size_t FactGenerator::visit_constant(const llvm::Constant &c) {
  auto tid = visit_type(c.getType());

  auto id = get_or_insert_value(&c);
  if (isa<ConstantInt>(&c)) {
    // Signed or not?
    append_fact(
        FACT_IntConstant,
        to_fact_str(id, static_cast<const ConstantInt &>(c).getSExtValue()));
  } else {
    append_fact(FACT_Constant, to_fact_str(id, printSafeValue(c)));
  }
  append_fact(FACT_ValueType, to_fact_str(id, tid));
  return id;
}

size_t FactGenerator::get_or_insert_value(val_t val) {
  return get_or_insert_value(val, false);
}

// 可以都改用这个函数插入
size_t FactGenerator::get_or_insert_value(val_t val, bool assert_not_exist) {
  // only enable this if assert_not_exist is false
  if (!assert_not_exist && value2id.count(val)) {
    return value2id.at(val);
  } else {
    auto id = valueID++;
    assert(value2id.emplace(val, id).second);
    assert(id2value.emplace(id, val).second);
    return id;
  }
}

void FactGenerator::visit_functions() {
  InstructionVisitor IV(*this, mod);

  // export all function first
  for (auto &f : mod.functions()) {
    auto name = getNameOrAsOperand(f);
    auto id = get_or_insert_value(&f);
    append_fact(FACT_Func, to_fact_str(id, name));

    auto linkage = getLinkageName(f.getLinkage());
    auto visibility = getVisibilityName(f.getVisibility());
    append_fact(FACT_FuncAttr, to_fact_str(id, linkage, visibility));

    // return as a abstract value.
    auto ret_id =
        get_or_insert_value(FactGenerator::aval{&f, FACT_FuncRet}, true);
    append_fact(FACT_FuncRet, to_fact_str(ret_id, id));
    auto type_id = visit_type(f.getReturnType());
    append_fact(FACT_ValueType, to_fact_str(ret_id, type_id));

    size_t ind = 0;
    for (auto &arg : f.args()) {
      auto arg_name = getNameOrAsOperand(arg);
      auto arg_id = get_or_insert_value(&arg);
      append_fact(FACT_FuncArg, to_fact_str(arg_id, id, ind, arg_name));

      auto arg_type_id = visit_type(arg.getType());
      append_fact(FACT_ValueType, to_fact_str(arg_id, arg_type_id));
      ind += 1;
    }
  }

  for (auto &f : mod.functions()) {
    auto fid = value2id.at(&f);

    if (f.isDeclaration()) {
      continue;
    }

    for (const llvm::BasicBlock &bb : f) {
      auto name = getNameOrAsOperand(bb);
      auto id = get_or_insert_value(&bb);
      append_fact(FACT_BasicBlock, to_fact_str(id, fid, name));

      auto bb_type_id = visit_type(bb.getType());
      append_fact(FACT_ValueType, to_fact_str(id, bb_type_id));

      size_t prev_instr_id = 0;
      for (const llvm::Instruction &instr : bb) {
        auto inst_id = get_or_insert_value(&instr);
        auto inst_name = getNameOrAsOperand(instr);
        append_fact(FACT_Instruction,
                    to_fact_str(inst_id, instr.getOpcodeName(), inst_name, id));

        auto inst_type_id = visit_type(instr.getType());
        append_fact(FACT_ValueType, to_fact_str(inst_id, inst_type_id));

        // Record instruction flows
        if (prev_instr_id) {
          append_fact(FACT_InstNext, to_fact_str(prev_instr_id, id));
        }
        prev_instr_id = inst_id;
      }

      // block entry
      auto block_entry_id = value2id.at(&bb.getInstList().front());
      append_fact(FACT_BlockEntryInst, to_fact_str(id, block_entry_id));
    }

    // entry 关系
    auto entry_id = value2id.at(&f.getEntryBlock());
    append_fact(FACT_FuncEntryBlock, to_fact_str(fid, entry_id));

    // Traverse again to export CFG relation/operand relation.
    // e.g., call inst cannot find target function id during the first pass.
    for (const llvm::BasicBlock &bb : f) {
      auto id = value2id.at(&bb);
      // Record basic block predecessors
      for (llvm::const_pred_iterator pi = pred_begin(&bb),
                                     pi_end = pred_end(&bb);
           pi != pi_end; ++pi) {
        auto pred_id = value2id.at(*pi);
        append_fact(FACT_CFGEdge, to_fact_str(pred_id, id));

        auto pred_inst_id = value2id.at((*pi)->getTerminator());
        auto succ_inst_id = value2id.at(&bb.getInstList().front());
        append_fact(FACT_InstNext, to_fact_str(pred_inst_id, succ_inst_id));
      }

      for (const llvm::Instruction &instr : bb) {
        auto inst_id = value2id.at(&instr);

        // skip call inst. visitCallInst will handle it.
        if (isa<CallInst>(instr)) {
          IV.visit(const_cast<llvm::Instruction &>(instr));
          continue;
        }

        // visit operands in second pass
        size_t ind = 0;
        for (auto &op : instr.operands()) {
          auto op_vid = visit_operand(*op.get());
          append_fact(FACT_Operand, to_fact_str(inst_id, ind, op_vid));

          ind += 1;
        }

        IV.visit(const_cast<llvm::Instruction &>(instr));
      }
    }
  }
}

void InstructionVisitor::visitReturnInst(ReturnInst &I) {
  fg.append_fact(FACT_RetInstVal,
                 to_fact_str(fg.value2id.at(&I),
                             fg.value2id.at(FactGenerator::aval{
                                 I.getParent()->getParent(), FACT_FuncRet})));
}

void InstructionVisitor::visitCallInst(CallInst &I) {
  // for each arg (instead of operand)
  size_t ind = 0;
  for (auto &arg : I.args()) {
    auto op_vid = fg.visit_operand(*arg.get());
    fg.append_fact(FACT_Operand, to_fact_str(fg.value2id.at(&I), ind, op_vid));
    ind += 1;
  }

  // TODO call indirect
  auto target = I.getCalledFunction();
  if (target == nullptr) {
    return;
  }
  fg.append_fact(FACT_CallTarget,
                 to_fact_str(fg.value2id.at(&I), fg.value2id.at(target)));
}

std::string printType(Type *ty) {
  std::string type_str;
  llvm::raw_string_ostream rso(type_str);
  ty->print(rso);
  return rso.str();
}

const char *getVisibilityName(GlobalValue::VisibilityTypes VT) {
  switch (VT) {
  case GlobalValue::DefaultVisibility:
    return "default";
  case GlobalValue::HiddenVisibility:
    return "hidden";
  case GlobalValue::ProtectedVisibility:
    return "protected";
  }
  llvm_unreachable("invalid visibility");
}

const char *getLinkageName(GlobalValue::LinkageTypes LT) {
  switch (LT) {
  case GlobalValue::ExternalLinkage:
    return "external";
  case GlobalValue::PrivateLinkage:
    return "private";
  case GlobalValue::InternalLinkage:
    return "internal";
  case GlobalValue::LinkOnceAnyLinkage:
    return "linkonce";
  case GlobalValue::LinkOnceODRLinkage:
    return "linkonce_odr";
  case GlobalValue::WeakAnyLinkage:
    return "weak";
  case GlobalValue::WeakODRLinkage:
    return "weak_odr";
  case GlobalValue::CommonLinkage:
    return "common";
  case GlobalValue::AppendingLinkage:
    return "appending";
  case GlobalValue::ExternalWeakLinkage:
    return "extern_weak";
  case GlobalValue::AvailableExternallyLinkage:
    return "available_externally";
  }
  llvm_unreachable("invalid linkage");
}

std::string printSafeValue(const Value &val) {
  auto str = printValue(val);
  std::replace(str.begin(), str.end(), '\n', ' ');
  if (str.size() > 30) {
    str = str.substr(0, 30) + "...";
  }
  return str;
}

std::string printValue(const Value &val) {
  std::string Name;
  raw_string_ostream OS(Name);
  val.print(OS);
  return OS.str();
}

std::string getNameOrAsOperand(const Value &val) {
  if (!val.getName().empty())
    return std::string(val.getName());

  std::string Name;
  raw_string_ostream OS(Name);
  val.printAsOperand(OS, false);
  // void type inst
  if (OS.str() == "<badref>") {
    return printSafeValue(val);
  }
  return OS.str();
}

} // namespace notdec::datalog
