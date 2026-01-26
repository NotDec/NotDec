#ifndef _NOTDEC_PASSES_OPTMANAGER_H_
#define _NOTDEC_PASSES_OPTMANAGER_H_

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
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>

#include "DecompilerContext.h"
#include "Passes/ConstraintGenerator.h"
#include "Passes/StackPointerFinder.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"
#include "notdec-llvm2c/Interface.h"

namespace notdec::passes {

struct PassEnv {
  // 目前和module耦合。因为类型恢复和模块也耦合
  llvm::Module &Mod;
  // Create the analysis managers.
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager SCCAM;
  llvm::ModuleAnalysisManager MAM;
  // add instrumentations.
  llvm::PassInstrumentationCallbacks PIC;
  llvm::StandardInstrumentations SI;
  llvm::PassBuilder PB;
  llvm::ModulePassManager MPM;

  PassEnv(llvm::Module &Mod)
      : Mod(Mod), SI(::llvm::DebugFlag, false,
                     llvm::PrintPassOptions{.SkipAnalyses = true}),
        PB(nullptr, llvm::PipelineTuningOptions(), llvm::None, &PIC) {
    SI.registerCallbacks(PIC, &FAM);
    PIC.addClassToPassName("notdec::LinearAllocationRecovery",
                           "linear-allocation-recovery");
    PIC.addClassToPassName("notdec::PointerTypeRecovery",
                           "pointer-type-recovery");

    // llvm2c passes
    PIC.addClassToPassName("notdec::llvm2c::AdjustCFGPass", "adjustcfg");
    PIC.addClassToPassName("notdec::llvm2c::RetDupPass", "retdup");
    PIC.addClassToPassName("notdec::llvm2c::DemotePhiPass", "demote-phi");

    // Register all the basic analyses with the managers.
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(SCCAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, SCCAM, MAM);
    MAM.registerPass([&]() { return StackPointerFinderAnalysis(); });
  }

  std::shared_ptr<retypd::TRContext> TRCtx;
  std::shared_ptr<ast::HTypeContext> HTCtx;
  std::shared_ptr<mlsub::MLsubRecovery> TR;

  void prepareTypeRecoveryContext() {
    if (TRCtx == nullptr && HTCtx == nullptr && TR == nullptr) {
      TRCtx = std::make_shared<retypd::TRContext>();
      HTCtx = std::make_shared<ast::HTypeContext>();
      TR = std::make_shared<mlsub::MLsubRecovery>(Mod, MAM);
    }
  }

  void build_passes(int level);
  void add_llvm2c(std::string OutFilePath, ::notdec::llvm2c::Options llvm2cOpt,
                  bool disableTypeRecovery);
  void run_passes();
};

struct DecompileConfig {

  DecompileConfig(llvm::Module &Mod, std::string OutFilePath,
                  notdec::Options opt, ::notdec::llvm2c::Options llvm2cOpt)
      : Mod(Mod), OutFilePath(OutFilePath), Opts(opt), llvm2cOpt(llvm2cOpt),
        PE(Mod) {}

  llvm::Module &Mod;
  std::string OutFilePath;
  notdec::Options Opts;
  ::notdec::llvm2c::Options llvm2cOpt;
  llvm::GlobalVariable *SP = nullptr;
  llvm::GlobalVariable *Mem = nullptr;

  PassEnv PE;

  void find_special_gv();
  void build_passes(int level) {
    PE.build_passes(level);
    bool isC = getSuffix(OutFilePath) == ".c";
    if (isC) {
      PE.add_llvm2c(OutFilePath, llvm2cOpt, Opts.trLevel < 2);
    }
  }
  void run_passes() { PE.run_passes(); }
};

llvm::FunctionPassManager buildFunctionOptimizations();

} // namespace notdec::passes

#endif
