#ifndef _NOTDEC_OPTIMIZERS_OPTMANAGER_H_
#define _NOTDEC_OPTIMIZERS_OPTMANAGER_H_

#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>

#include "context.h"

namespace notdec::optimizers {

struct DecompileConfig {

  DecompileConfig(llvm::Module &mod, notdec::Options opt)
      : mod(mod), opts(opt) {}

  llvm::Module &mod;
  notdec::Options opts;
  llvm::GlobalVariable *sp = nullptr;
  llvm::GlobalVariable *mem = nullptr;

  void find_special_gv();
  void run_passes();
};

llvm::FunctionPassManager buildFunctionOptimizations();

} // namespace notdec::optimizers

#endif
