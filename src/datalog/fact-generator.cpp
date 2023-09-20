#include "datalog/fact-generator.h"
#include <cstddef>
#include <iostream>
#include <llvm/IR/Value.h>
#include <string>
#include <sstream>
#include <fstream>

#include "datalog/fact-names.def"

namespace notdec::datalog {

template<typename Value, typename... Values>
std::string to_fact_str ( Value v, Values... vs )
{
  std::ostringstream oss;
  using expander = int[];
  oss << v; // first
  (void) expander{ 0, (oss << '\t' << vs, void(), 0)... };
  return oss.str() + '\n';
}

void FactGenerator::generate(llvm::Module &mod, const char* outputDirname) {
  FactGenerator fg(mod);
  fg.emit_gvs();
  fg.emit_functions();
  if (outputDirname != nullptr) {
    fg.output_files(outputDirname);
  }
}

void FactGenerator::output_files(std::string outputDirname) {
  // if dir not exist, create it
  // std::string cmd = "mkdir -p " + outputDirname;
  // system(cmd.c_str());
  for (auto &kv : facts) {
    std::string fact_filename = outputDirname + "/" + kv.first + ".csv";
    std::ofstream fact_file(fact_filename);
    fact_file << kv.second;
    fact_file.close();
  }
}

// https://github.com/llvm/llvm-project/blob/7cbf1a2591520c2491aa35339f227775f4d3adf6/llvm/lib/IR/AsmWriter.cpp#L536
size_t FactGenerator::emit_type(llvm::Type* ty) {
  size_t id = -1;
  size_t id_dep = -1;
  std::string type_str;
  switch (ty->getTypeID()) {
  case Type::VoidTyID:      type_str = "void"; break;
  case Type::HalfTyID:      type_str = "half"; break;
  case Type::BFloatTyID:    type_str = "bfloat"; break;
  case Type::FloatTyID:     type_str = "float"; break;
  case Type::DoubleTyID:    type_str = "double"; break;
  case Type::X86_FP80TyID:  type_str = "x86_fp80"; break;
  case Type::FP128TyID:     type_str = "fp128"; break;
  case Type::PPC_FP128TyID: type_str = "ppc_fp128"; break;
  case Type::LabelTyID:     type_str = "label"; break;
  case Type::MetadataTyID:  type_str = "metadata"; break;
  case Type::X86_MMXTyID:   type_str = "x86_mmx"; break;
  case Type::X86_AMXTyID:   type_str = "x86_amx"; break;
  case Type::TokenTyID:     type_str = "token"; break;
  case llvm::Type::IntegerTyID: type_str = printType(ty); break;
  case llvm::Type::PointerTyID:
    id_dep = emit_type(ty->getPointerElementType());
    type_str = printType(ty);
    id = get_or_insert_type(type_str);
    append_fact(FACT_PointerType, to_fact_str(id, id_dep));
    return id;
  case llvm::Type::StructTyID:
    type_str = printType(ty);
    id = get_or_insert_type(type_str);
    append_fact(FACT_StructType, to_fact_str(id, type_str));
    return id;
  case llvm::Type::ArrayTyID:
    id_dep = emit_type(ty->getArrayElementType());
    type_str = printType(ty);
    id = get_or_insert_type(type_str);
    append_fact(FACT_ArrayType, to_fact_str(id, id_dep, ty->getArrayNumElements()));
    return id;
  case llvm::Type::FixedVectorTyID:
    id_dep = emit_type(ty->getArrayElementType());
    type_str = printType(ty);
    id = get_or_insert_type(type_str);
    append_fact(FACT_VectorType, to_fact_str(id, id_dep, ty->getArrayNumElements()));
    return id;
  case llvm::Type::FunctionTyID:
  case llvm::Type::ScalableVectorTyID:
  // default:
    std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Unsupported type: " << printType(ty) << std::endl;
    std::abort();
  }
    id = get_or_insert_type(type_str);
    append_fact(FACT_BaseType, to_fact_str(id, type_str));
    return id;
}

void FactGenerator::emit_gvs() {
  for (auto &gv : mod.globals()) {
    auto name = getNameOrAsOperand(gv);
    auto id = valueID++;
    append_fact(FACT_GlobalVariable, to_fact_str(id, name));

    // auto type = printType(gv.getValueType());
    auto tid = emit_type(gv.getValueType());
    append_fact(FACT_ValueType, to_fact_str(id, tid));

    unsigned long isConst = gv.isConstant() ? 1 : 0;
    auto linkage = getLinkageName(gv.getLinkage());
    auto visibility = getVisibilityName(gv.getVisibility());
    append_fact(FACT_GlobalVarAttr, to_fact_str(id, isConst, linkage, visibility));
  }
}

unsigned long FactGenerator::get_or_insert_type(std::string key) {
  // https://stackoverflow.com/questions/1409454/c-map-find-to-possibly-insert-how-to-optimize-operations
  typedef std::map<std::string, unsigned long> M;
  typedef M::iterator I;
  std::pair<I, bool> const& r=type_ids.insert(M::value_type(key, typeID));
  if (r.second) {
    // value was inserted; now my_map[foo_obj]="some value"
    return typeID++;
  } else {
    // value wasn't inserted because my_map[foo_obj] already existed.
    // note: the old value is available through r.first->second
    // and may not be "some value"
    return r.first->second;
  }
}

void FactGenerator::append_fact(const char* key, std::string to_append) {
  // https://stackoverflow.com/questions/1409454/c-map-find-to-possibly-insert-how-to-optimize-operations
  typedef std::map<const char*,std::string> M;
  typedef M::iterator I;
  std::pair<I, bool> const& r=facts.insert(M::value_type(key, to_append));
  if (r.second) {
    // value was inserted; now my_map[foo_obj]="some value"
  } else {
    // value wasn't inserted because my_map[foo_obj] already existed.
    // note: the old value is available through r.first->second
    // and may not be "some value"
    r.first->second.append(to_append);
  }
}

void FactGenerator::emit_functions() {
  for(auto &f: mod.functions()) {
    auto name = getNameOrAsOperand(f);
    auto id = valueID++;
    append_fact(FACT_Func, to_fact_str(id, name));

    auto linkage = getLinkageName(f.getLinkage());
    auto visibility = getVisibilityName(f.getVisibility());
    append_fact(FACT_FuncAttr, to_fact_str(id, linkage, visibility));

    // return as value type
    auto type_id = emit_type(f.getReturnType());
    append_fact(FACT_ValueType, to_fact_str(id, type_id));

    for(auto& arg: f.args()) {
      auto arg_name = getNameOrAsOperand(arg);
      auto arg_id = valueID++;
      append_fact(FACT_FuncArgument, to_fact_str(arg_id, id, arg_name));

      auto arg_type_id = emit_type(arg.getType());
      append_fact(FACT_ValueType, to_fact_str(arg_id, arg_type_id));
    }
  }
}

std::string printType(Type* ty) {
  std::string type_str;
  llvm::raw_string_ostream rso(type_str);
  ty->print(rso);
  return rso.str();
}

const char* getVisibilityName(GlobalValue::VisibilityTypes VT) {
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


const char* getLinkageName(GlobalValue::LinkageTypes LT) {
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

std::string getNameOrAsOperand(Value& val) {
  if (!val.getName().empty())
    return std::string(val.getName());

  std::string BBName;
  raw_string_ostream OS(BBName);
  val.printAsOperand(OS, false);
  return OS.str();
}

}

