#ifndef _NOTDEC_FRONTEND_OPTIMIZERS_OPTMANAGER_H_
#define _NOTDEC_FRONTEND_OPTIMIZERS_OPTMANAGER_H_


#include "llvm/ADT/StringMap.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/OptimizationLevel.h"

#include "llvm/Transforms/Utils/Mem2Reg.h"

#include "frontend/context.h"

using namespace llvm;

namespace notdec::frontend::optimizers {

void run_passes(llvm::Module& mod, notdec::frontend::options opts);
llvm::FunctionPassManager buildFunctionOptimizations();

}



#endif