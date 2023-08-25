#ifndef _NOTDEC_OPTIMIZERS_FACT_GENERATOR_H_
#define _NOTDEC_OPTIMIZERS_FACT_GENERATOR_H_

#include "llvm/IR/Module.h"
#include <map>
#include <string>

using namespace llvm;

namespace notdec::frontend::optimizers {

struct FactGenerator {
  FactGenerator(llvm::Module &mod) : mod(mod) {}
  llvm::Module &mod;
  std::map<const char*, std::string> facts;
  static void generate(llvm::Module &mod);
  void append_fact(const char* key, std::string to_append);
  void emit_gvs();
  void emit_functions();
};

std::string getLinkageName(GlobalValue::LinkageTypes LT);
std::string printType(Type* ty);

} // namespace notdec::frontend::optimizers

#endif