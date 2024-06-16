#ifndef _NOTDEC_DATALOG_FACT_GENERATOR_H_
#define _NOTDEC_DATALOG_FACT_GENERATOR_H_

#include <cstddef>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <map>
#include <souffle/RamTypes.h>
#include <souffle/SouffleInterface.h>
#include <sstream>
#include <string>
#include <variant>

using namespace llvm;

#include "datalog/fact-names.h"

namespace notdec::datalog {

class InstructionVisitor;

template <typename Value, typename... Values>
std::string inline to_fact_str(Value v, Values... vs) {
  std::ostringstream oss;
  using expander = int[];
  oss << v; // first
  (void)expander{0, (oss << '\t' << vs, void(), 0)...};
  return oss.str() + '\n';
}

// 提取顺序：先指令，后operand，防止引用失效
// 先所有函数，后指令。因为指令可能引用函数
/// Generate facts from llvm::Module
/// When \p SouffleProgram* is provided, the facts are directly inserted into
/// the SouffleProgram. Otherwise, the facts are stored in a map, and can be
/// output to csv files. The constructor without \p SouffleProgram* is
/// deprecated, and may be removed in the future.
class FactGenerator {
  friend class InstructionVisitor;

public:
  // Abstract Values attach to a llvm value.
  using aval = std::pair<const llvm::Value *, const char *>;
  using val_t = std::variant<const llvm::Value *, aval>;
  using number_t = souffle::RamSigned;
  using unsigned_t = souffle::RamUnsigned;
  using id_t = souffle::RamUnsigned;

protected:
  id_t typeID = 0;
  id_t valueID = 0;

  std::map<std::string, id_t> type_ids;
  std::pair<id_t, bool> get_or_insert_type(std::string key);

protected:
  std::map<val_t, id_t> value2id;
  std::map<id_t, val_t> id2value;
  id_t get_or_insert_value(val_t val, bool assert_not_exist);
  id_t get_or_insert_value(val_t val);
  /// insert to facts map
  void append_fact_str(const char *key, std::string to_append);

public:
  llvm::Module &mod;
  souffle::SouffleProgram *prog = nullptr;
  /// Construct an instance that uses std::map to hold csv values
  FactGenerator(llvm::Module &mod) : mod(mod) {}
  /// Construct an instance that directly insert to SouffleProgram
  FactGenerator(llvm::Module &mod, souffle::SouffleProgram *prog)
      : mod(mod), prog(prog) {}
  void set_program(souffle::SouffleProgram *prog) { this->prog = prog; }

  std::map<const char *, std::string> facts;

  std::set<const char *> ignored_relations;
  /// main interface to append facts. At least one Value is required.
  template <typename Value, typename... Values>
  void append_fact(const char *key, Value v, Values... vs) {
    // legacy mode: convert to csv string and insert into facts map.
    if (prog == nullptr) {
      append_fact(key, to_fact_str(v, vs...));
      return;
    }
    if (souffle::Relation *rel = prog->getRelation(key)) {
      souffle::tuple tup(rel);
      using expander = int[];
      tup << v; // first
      (void)expander{0, (tup << vs, void(), 0)...};
      rel->insert(tup); // Add the new tuple to the relation
    } else {
      ignored_relations.insert(key);
      // std::cerr << __FILE__ << ":" << __LINE__ << ": "
      //           << "Failed to get input relation: " << key << std::endl;
      // std::abort();
    }
  }
  id_t get_value_id(val_t val);
  val_t get_value_by_id(id_t vid);

  static void generate(llvm::Module &mod, const char *outputDirname);

  id_t visit_constant(const llvm::Constant &c);
  id_t visit_operand(llvm::Value &val);
  id_t visit_type(llvm::Type *ty);

  void visit_module();
  void visit_gvs();
  void visit_functions();

  /// output to csv files in the temporary directory \p outputDirname
  void output_files(std::string outputDirname);
};

/// For functions in InstVisitor to override, see:
/// https://llvm.org/doxygen/classllvm_1_1InstVisitor.html
class InstructionVisitor : public llvm::InstVisitor<InstructionVisitor> {

public:
  InstructionVisitor(FactGenerator &generator, const llvm::Module &M)
      : fg(generator), module(M) {}

  void visitReturnInst(ReturnInst &I);
  void visitCallInst(CallInst &I);

private:
  /* Instance of outer fact-generator */
  FactGenerator &fg;

  /* Associated LLVM module */
  const llvm::Module &module;
};

const char *getLinkageName(GlobalValue::LinkageTypes LT);
const char *getVisibilityName(GlobalValue::VisibilityTypes VT);
std::string printType(Type *ty);
std::string getNameOrAsOperand(const Value &val);
std::string printValue(const Value &val);
std::string printSafeValue(const Value &val);

} // namespace notdec::datalog

#endif
