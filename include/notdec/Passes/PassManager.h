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
#include <optional>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Plugins/PassPlugin.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>

#include "DecompilerContext.h"
#include "Passes/StackPointerFinder.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"
#include "Utils/Utils.h"
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
  std::string CachedHTypeSnapshot;
  bool HasCachedHTypeSnapshot = false;

  PassEnv(llvm::Module &Mod)
      : Mod(Mod), SI(Mod.getContext(), ::llvm::DebugFlag, false,
                     llvm::PrintPassOptions{.SkipAnalyses = true}),
        PB(nullptr, llvm::PipelineTuningOptions(), std::nullopt, &PIC) {
    SI.registerCallbacks(PIC, &MAM);
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

  std::shared_ptr<mlsub::MLsubRecovery> TR;

  void prepareTypeRecoveryContext() {
    if (TR == nullptr) {
      TR = std::make_shared<mlsub::MLsubRecovery>(Mod, MAM);
    }
  }

  void add_pre_type_recovery_passes();
  void add_type_recovery_passes(int level);
  void build_passes(int level, bool stopBeforeTypeRecovery = false,
                    bool frozenTRInputIR = false);
  void add_llvm2c(std::string OutFilePath, ::notdec::llvm2c::Options llvm2cOpt,
                  bool disableTypeRecovery,
                  bool captureHTypeSnapshot = false);
  void run_passes();
  void dump_htypes(const std::string &OutputPath);
  void emit_tr_input_ir(const std::string &OutputPath);
};

struct DecompileConfig {

  DecompileConfig(llvm::Module &Mod, std::string OutFilePath,
                  std::string HTypeDumpPath, notdec::Options opt,
                  ::notdec::llvm2c::Options llvm2cOpt)
      : Mod(Mod), OutFilePath(OutFilePath), HTypeDumpPath(HTypeDumpPath),
        Opts(opt), llvm2cOpt(llvm2cOpt), PE(Mod) {}

  llvm::Module &Mod;
  std::string OutFilePath;
  std::string HTypeDumpPath;
  notdec::Options Opts;
  ::notdec::llvm2c::Options llvm2cOpt;
  llvm::GlobalVariable *SP = nullptr;
  llvm::GlobalVariable *Mem = nullptr;

  PassEnv PE;

  void find_special_gv();
  void build_passes(int level) {
    bool EmitTRInputIR = !Opts.emitTRInputIR.empty();
    bool FrozenTRInputIR = Opts.frozenTRInputIR;
    int EffectiveLevel = EmitTRInputIR ? std::max(level, 2) : level;
    PE.build_passes(EffectiveLevel, EmitTRInputIR, FrozenTRInputIR);
    if (EmitTRInputIR) {
      return;
    }
    bool isC = getSuffix(OutFilePath) == ".c";
    if (isC) {
      PE.add_llvm2c(OutFilePath, llvm2cOpt, EffectiveLevel < 2,
                    !HTypeDumpPath.empty());
    }
  }
  void run_passes() {
    PE.run_passes();
    if (!HTypeDumpPath.empty()) {
      PE.dump_htypes(HTypeDumpPath);
    }
  }
  void emit_tr_input_ir() { PE.emit_tr_input_ir(Opts.emitTRInputIR); }
};

llvm::FunctionPassManager buildFunctionOptimizations();

} // namespace notdec::passes

#endif
