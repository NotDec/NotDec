#ifndef _NOTDEC_OPTIMIZERS_POINTER_TYPE_RECOVERY_H_
#define _NOTDEC_OPTIMIZERS_POINTER_TYPE_RECOVERY_H_

#include <llvm/IR/Argument.h>
#include <llvm/IR/Instruction.h>
#include <map>
#include <set>

#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <souffle/SouffleInterface.h>

#include "datalog/fact-generator.h"

namespace notdec::optimizers {

using namespace llvm;

struct PointerTypeRecovery : PassInfoMixin<PointerTypeRecovery> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding  pass manager (to be queried if need be)
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &mam);

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }

  const char *MEM_NAME = "__notdec_mem0";
  std::map<Argument *, long> arg2type;
  std::map<Instruction *, long> inst2type;
  std::map<Function *, long> func_ret2type;
  std::map<GlobalVariable *, long> gv2type;
  std::map<Value *, Value *> replace_map;

  void fetch_result(datalog::FactGenerator &fg, souffle::SouffleProgram *prog);
  static Type *get_pointer_type(Module &M);
};

} // namespace notdec::optimizers

#endif
