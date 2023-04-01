#include "frontend/optimizers/opt-manager.h"



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

void run_passes(llvm::Module& mod) {
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
    // This one corresponds to a typical -O2 optimization pipeline.
    ModulePassManager MPM; // = PB.buildPerModuleDefaultPipeline(llvm::PassBuilder::OptimizationLevel::O2);
    //MPM.addPass(createModuleToFunctionPassAdaptor(HelloWorld()));
    //MPM.addPass(HelloModule());
    

    //mem2reg
    MPM.addPass(createModuleToFunctionPassAdaptor(llvm::PromotePass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(stack()));
    // Optimize the IR!
    MPM.run(mod, MAM);
}

}

