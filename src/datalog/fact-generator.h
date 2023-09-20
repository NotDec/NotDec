#ifndef _NOTDEC_DATALOG_FACT_GENERATOR_H_
#define _NOTDEC_DATALOG_FACT_GENERATOR_H_

#include "llvm/IR/Module.h"
#include <llvm/IR/Type.h>
#include <llvm/IR/InstVisitor.h>
#include <map>
#include <string>

using namespace llvm;

#include "datalog/fact-names.h"

namespace notdec::datalog {

class InstructionVisitor;

class FactGenerator {
  friend class InstructionVisitor;

protected:
  size_t typeID = 0;
  size_t valueID = 0;

  std::map<std::string, size_t> type_ids;
  size_t get_or_insert_type(std::string key);

public:
  llvm::Module &mod;
  FactGenerator(llvm::Module &mod) : mod(mod) {}

  std::map<const char*, std::string> facts;

  static void generate(llvm::Module &mod, const char* outputDirname);
  void append_fact(const char* key, std::string to_append);
  size_t emit_type(llvm::Type* ty);
  void emit_gvs();
  void emit_functions();

  void output_files(std::string outputDirname);
};

class InstructionVisitor
    : public llvm::InstVisitor<InstructionVisitor>
{
  friend class FactGenerator;
  public:

  InstructionVisitor(FactGenerator &generator, const llvm::Module &M)
        : gen(generator) , module(M)
  {}

  private:
    /* Instance of outer fact-generator */
    FactGenerator &gen;

    /* Associated LLVM module */
    const llvm::Module &module;
};


const char* getLinkageName(GlobalValue::LinkageTypes LT);
const char* getVisibilityName(GlobalValue::VisibilityTypes VT);
std::string printType(Type* ty);
std::string getNameOrAsOperand(Value& val);

} // namespace notdec::datalog

#endif