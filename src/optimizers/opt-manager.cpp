#include "optimizers/opt-manager.h"
#include "optimizers/retdec-stack/retdec-stack-pointer-op-remove.h"
#include "optimizers/retdec-stack/retdec-stack.h"
#include "llvm/Transforms/Scalar/DCE.h"
#include <algorithm>

#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/InstSimplifyPass.h>
#include "llvm/Transforms/Scalar/SCCP.h"
#include "llvm/Transforms/Utils/SimplifyCFGOptions.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/MemCpyOptimizer.h"
#include "llvm/Transforms/Scalar/BDCE.h"
#include "llvm/Transforms/Scalar/ADCE.h"

namespace notdec::frontend::optimizers {


// Function Pass example
struct HelloWorld : PassInfoMixin<HelloWorld> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    errs() << "(llvm-tutor) Hello from: "<< F.getName() << "\n";
    errs() << "(llvm-tutor)   number of arguments: " << F.arg_size() << "\n";
    return PreservedAnalyses::all();
  }

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};

// Module Pass example
struct HelloModule : PassInfoMixin<HelloWorld> {
  PreservedAnalyses run(Module &F, ModuleAnalysisManager &) {
    errs() << "(llvm-tutor) Hello Module: "<< F.getName() << "\n";
    return PreservedAnalyses::all();
  }
  static bool isRequired() { return true; }
};

void run_passes(llvm::Module& mod, notdec::frontend::options opts) {
    // Create the analysis managers.
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    // Create the new pass manager builder.
    // Take a look at the PassBuilder constructor parameters for more
    // customization, e.g. specifying a TargetMachine or various debugging
    // options.
    PassBuilder PB;

    // Register all the basic analyses with the managers.
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // Create the pass manager.
    // Optimize the IR!
    // This one corresponds to a typical -O2 optimization pipeline.
    //ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::PassBuilder::OptimizationLevel::O2);
    ModulePassManager MPM;
    FunctionPassManager FPM = buildFunctionOptimizations();

    // MPM.addPass(FunctionRenamer());
    //MPM.addPass(createModuleToFunctionPassAdaptor(stack()));
    //MPM.addPass(createModuleToFunctionPassAdaptor(llvm::DCEPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

    // if not recompile then decompile.
    if (!opts.recompile) {
      MPM.addPass(retdec::bin2llvmir::StackAnalysis());
      MPM.addPass(retdec::bin2llvmir::StackPointerOpsRemove());
    }
    MPM.run(mod, MAM);
}

// 需要去掉尾递归等优化，因此需要构建自己的Pass。
// TODO 未来根据是数据流分析还是反编译，选择是否执行，类似结构体解构这样的Pass。
llvm::FunctionPassManager buildFunctionOptimizations() {
  FunctionPassManager FPM; // = PB.buildFunctionSimplificationPipeline(OptimizationLevel::O1, ThinOrFullLTOPhase::None);

  FPM.addPass(
    SimplifyCFGPass(SimplifyCFGOptions().convertSwitchRangeToICmp(false)));
  FPM.addPass(InstCombinePass());
  // MPM.addPass(createModuleToFunctionPassAdaptor(HelloWorld()));
  // MPM.addPass(HelloModule());
  FPM.addPass(llvm::PromotePass());
  FPM.addPass(llvm::GVNPass());

  // Specially optimize memory movement as it doesn't look like dataflow in SSA.
  FPM.addPass(MemCpyOptPass());

  // Sparse conditional constant propagation.
  // FIXME: It isn't clear why we do this *after* loop passes rather than
  // before...
  FPM.addPass(SCCPPass());

  // Delete dead bit computations (instcombine runs after to fold away the dead
  // computations, and then ADCE will run later to exploit any new DCE
  // opportunities that creates).
  FPM.addPass(BDCEPass());

  // Run instcombine after redundancy and dead bit elimination to exploit
  // opportunities opened up by them.
  FPM.addPass(InstCombinePass());
  // Finally, do an expensive DCE pass to catch all the dead code exposed by
  // the simplifications and basic cleanup after all the simplifications.
  // TODO: Investigate if this is too expensive.
  FPM.addPass(ADCEPass());
  FPM.addPass(InstCombinePass());
  return FPM;
}

}

