#ifndef _NOTDEC_OPTIMIZERS_POINTER_TYPE_RECOVERY_H_
#define _NOTDEC_OPTIMIZERS_POINTER_TYPE_RECOVERY_H_

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
};

} // namespace notdec::optimizers

#endif
