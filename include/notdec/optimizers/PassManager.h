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
#include "notdec-llvm2c/Interface.h"

namespace notdec::optimizers {

struct DecompileConfig {

  DecompileConfig(llvm::Module &mod, std::string OutFilePath,
                  notdec::Options opt, ::notdec::llvm2c::Options llvm2cOpt)
      : Mod(mod), OutFilePath(OutFilePath), Opts(opt), llvm2cOpt(llvm2cOpt) {}

  llvm::Module &Mod;
  std::string OutFilePath;
  notdec::Options Opts;
  ::notdec::llvm2c::Options llvm2cOpt;
  llvm::GlobalVariable *SP = nullptr;
  llvm::GlobalVariable *Mem = nullptr;

  void find_special_gv();
  void run_passes();
};

llvm::FunctionPassManager buildFunctionOptimizations();

} // namespace notdec::optimizers

#endif
