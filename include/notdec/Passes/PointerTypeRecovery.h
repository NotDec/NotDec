#ifndef _NOTDEC_PASSES_POINTER_TYPE_RECOVERY_H_
#define _NOTDEC_PASSES_POINTER_TYPE_RECOVERY_H_

#include <map>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <souffle/SouffleInterface.h>

#include "datalog/fact-generator.h"

namespace notdec {

using namespace llvm;

struct PointerTypeRecovery : PassInfoMixin<PointerTypeRecovery> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding  pass manager (to be queried if need be)
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  // static bool isRequired() { return true; }
  PointerTypeRecovery() = default;
  PointerTypeRecovery(bool in_memory) : in_memory(in_memory) {}

  class PTRAnnotationWriter;
  void print(Module &M, std::string path) const;

  bool in_memory = false;
  const char *MEM_NAME = "__notdec_mem0";
  using aval = datalog::FactGenerator::aval;
  using val_t = datalog::FactGenerator::val_t;
  // value to high level type
  std::map<val_t, long> val2hty;
  std::map<Value *, Value *> replace_map;

  void fetch_result(datalog::FactGenerator &fg, souffle::SouffleProgram *prog);
  long get_ty_or_negative1(llvm::Value *val);
  long get_ty_or_negative1(aval val);
  Value *castBack(llvm::IRBuilder<> *builder, Instruction *inst, Type *old_ty,
                  long hty = -1);
  static Type *get_pointer_type(Module &M);
};

} // namespace notdec

#endif
