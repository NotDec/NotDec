#ifndef _NOTDEC_DATALOG_FACT_GENERATOR_H_
#define _NOTDEC_DATALOG_FACT_GENERATOR_H_

#include "llvm/IR/Module.h"
#include <cstddef>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Type.h>
#include <map>
#include <sstream>
#include <string>

using namespace llvm;

#include "datalog/fact-names.h"

namespace notdec::datalog {

class InstructionVisitor;

// 提取顺序：先指令，后operand，防止引用失效
// 先所有函数，后指令。因为指令可能引用函数
class FactGenerator {
  friend class InstructionVisitor;

protected:
  size_t typeID = 0;
  size_t valueID = 0;

  std::map<std::string, size_t> type_ids;
  std::pair<size_t, bool> get_or_insert_type(std::string key);

  std::map<const llvm::Value *, size_t> value2id;
  std::map<const llvm::BasicBlock *, size_t> id2value;
  size_t get_or_insert_value(const llvm::Value *val, bool assert_not_exist);
  size_t get_or_insert_value(const llvm::Value *val);

public:
  llvm::Module &mod;
  FactGenerator(llvm::Module &mod) : mod(mod) {}

  std::map<const char *, std::string> facts;
  void append_fact(const char *key, std::string to_append);
  size_t get_value_id(const llvm::Value *val);

  static void generate(llvm::Module &mod, const char *outputDirname);

  size_t visit_constant(const llvm::Constant &c);
  size_t visit_operand(llvm::Value &val);
  size_t visit_type(llvm::Type *ty);

  void visit_module();
  void visit_gvs();
  void visit_functions();

  void output_files(std::string outputDirname);
};

class InstructionVisitor : public llvm::InstVisitor<InstructionVisitor> {
  friend class FactGenerator;

public:
  InstructionVisitor(FactGenerator &generator, const llvm::Module &M)
      : gen(generator), module(M) {}

private:
  /* Instance of outer fact-generator */
  FactGenerator &gen;

  /* Associated LLVM module */
  const llvm::Module &module;
};

const char *getLinkageName(GlobalValue::LinkageTypes LT);
const char *getVisibilityName(GlobalValue::VisibilityTypes VT);
std::string printType(Type *ty);
std::string getNameOrAsOperand(const Value &val);
std::string printValue(const Value &val);
std::string printSafeValue(const Value &val);

template <typename Value, typename... Values>
std::string to_fact_str(Value v, Values... vs) {
  std::ostringstream oss;
  using expander = int[];
  oss << v; // first
  (void)expander{0, (oss << '\t' << vs, void(), 0)...};
  return oss.str() + '\n';
}

} // namespace notdec::datalog

#endif