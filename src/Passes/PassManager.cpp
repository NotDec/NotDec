#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FileSystem.h>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <llvm/ADT/StringRef.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Analysis/SimplifyQuery.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Pass.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/ADCE.h>
#include <llvm/Transforms/Scalar/BDCE.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/IndVarSimplify.h>
#include <llvm/Transforms/Scalar/InstSimplifyPass.h>
#include <llvm/Transforms/Scalar/LoopRotation.h>
#include <llvm/Transforms/Scalar/MemCpyOptimizer.h>
#include <llvm/Transforms/Scalar/SCCP.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/SimplifyCFGOptions.h>

#include "Passes/AllocAnnotator.h"
#include "Passes/MemOpMatcher.h"
#include "Passes/PassManager.h"
#include "Passes/ReorderBasicblock.h"
#include "Passes/StackAlloca.h"
#include "Passes/evm/SolidityPatterns.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"
#include "Utils/Utils.h"
#include "notdec-wasm2llvm/utils.h"

#ifdef NOTDEC_ENABLE_LLVM2C
#include "notdec-llvm2c/Interface.h"
#endif

namespace notdec::passes {

using notdec::frontend::wasm::MEM_NAME;

using namespace llvm;

namespace {

// The middle-end passes currently encode Wasm lowering assumptions. Keep the
// target split explicit so EVM IR can enter the main driver without reusing the
// Wasm recovery pipeline by accident.
enum class TargetArch { Wasm, Evm, Other };

TargetArch classifyTargetArch(StringRef Triple) {
  if (Triple.starts_with("wasm32") || Triple.starts_with("wasm64")) {
    return TargetArch::Wasm;
  }
  if (Triple.starts_with("evm")) {
    return TargetArch::Evm;
  }
  return TargetArch::Other;
}

} // namespace

// A Pass that undo some optimizations of the InstCombinePass.
struct UndoInstCombine : PassInfoMixin<UndoInstCombine> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    auto &AC = AM.getResult<AssumptionAnalysis>(F);
    auto &DT = AM.getResult<DominatorTreeAnalysis>(F);
    const DataLayout &DL = F.getParent()->getDataLayout();
    llvm::IRBuilder<> Builder(F.getParent()->getContext());
    std::vector<Instruction *> toRemove;

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        // revert: A+B --> A|B iff A and B have no bits set in common.
        if (BinaryOperator *BO = dyn_cast<BinaryOperator>(&I)) {
          if (BO->getOpcode() == Instruction::Or) {
            Value *LHS = BO->getOperand(0);
            Value *RHS = BO->getOperand(1);
            llvm::SimplifyQuery SQ(DL, &DT, &AC, &I);
            if (llvm::haveNoCommonBitsSet(LHS, RHS, SQ)) {
              // https://github.com/llvm/llvm-project/blob/e188aae406f3fecaed65a1f7e6562205f0de937e/llvm/lib/Transforms/InstCombine/InstructionCombining.cpp#L4095
              Builder.SetInsertPoint(BO);
              Instruction *Result =
                  llvm::cast<Instruction>(Builder.CreateAdd(LHS, RHS));
              Result->copyMetadata(
                  I, {LLVMContext::MD_dbg, LLVMContext::MD_annotation});
              BO->replaceAllUsesWith(Result);
              Result->takeName(BO);
              toRemove.push_back(BO);
            }
          }
        }
      }
    }

    for (Instruction *V : toRemove) {
      V->eraseFromParent();
    }

    return PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};


// A Pass that convert module to C.
struct MLsubNotdecLLVM2C : PassInfoMixin<MLsubNotdecLLVM2C> {

  mlsub::MLsubRecovery &TR;
  std::string OutFilePath;
  ::notdec::llvm2c::Options llvm2cOpt;
  bool disableTypeRecovery = false;
  std::string *CachedHTypeSnapshot = nullptr;
  bool *HasCachedHTypeSnapshot = nullptr;

  MLsubNotdecLLVM2C(mlsub::MLsubRecovery &TR, std::string outFilePath,
               ::notdec::llvm2c::Options &llvm2cOpt, bool disableTypeRecovery,
               std::string *CachedHTypeSnapshot = nullptr,
               bool *HasCachedHTypeSnapshot = nullptr)
      : TR(TR), OutFilePath(outFilePath), llvm2cOpt(std::move(llvm2cOpt)),
        disableTypeRecovery(disableTypeRecovery),
        CachedHTypeSnapshot(CachedHTypeSnapshot),
        HasCachedHTypeSnapshot(HasCachedHTypeSnapshot) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
    std::string outsuffix = getSuffix(OutFilePath);
    assert(outsuffix == ".c");

    // Run type recovery.
    std::unique_ptr<mlsub::MLsubRecovery::Result> HighTypes;
    if (!disableTypeRecovery) {
      HighTypes = std::move(TR.getResult(M, MAM));
      if (HighTypes != nullptr && CachedHTypeSnapshot != nullptr &&
          HasCachedHTypeSnapshot != nullptr) {
        CachedHTypeSnapshot->clear();
        llvm::raw_string_ostream SnapshotOS(*CachedHTypeSnapshot);
        HighTypes->print(SnapshotOS);
        SnapshotOS.flush();
        *HasCachedHTypeSnapshot = true;
      }
    }

    std::error_code EC;
    llvm::raw_fd_ostream os(OutFilePath, EC);
    if (EC) {
      std::cerr << "Cannot open output file: " << OutFilePath << std::endl;
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    // llvm2cOpt.noDemoteSSA = true;
    notdec::llvm2c::decompileModule(M, MAM, os, llvm2cOpt,
                                    std::move(HighTypes));
    std::cout << "Decompile result: " << OutFilePath << std::endl;

    return PreservedAnalyses::all();
  }
  static bool isRequired() { return true; }
};

// Function Pass example
struct HelloWorld : PassInfoMixin<HelloWorld> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding  pass manager (to be queried if need be)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    errs() << "(llvm-tutor) Hello from: " << F.getName() << "\n";
    errs() << "(llvm-tutor)   number of arguments: " << F.arg_size() << "\n";
    for (BasicBlock &bv : F) {
      bv.printAsOperand(errs());
      errs() << '\n';
      for (Instruction &i : bv) {
        i.printAsOperand(errs());
        errs() << '\n';
      }
    }
    return PreservedAnalyses::all();
  }

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};

// Module Pass example
struct HelloModule : PassInfoMixin<HelloModule> {
  PreservedAnalyses run(Module &F, ModuleAnalysisManager &) {
    errs() << "(llvm-tutor) Hello Module: " << F.getName() << "\n";
    return PreservedAnalyses::all();
  }
  static bool isRequired() { return true; }
};

void DecompileConfig::find_special_gv() {
  for (GlobalVariable &gv : Mod.globals()) {
    if (gv.getName() == MEM_NAME) {
      Mem = &gv;
    }
  }
  SP = StackPointerFinderAnalysis::find_stack_ptr(Mod);
}

void PassEnv::add_pre_type_recovery_passes() {
  FunctionPassManager FPM = buildFunctionOptimizations();
  // MPM.addPass(FunctionRenamer());
  // MPM.addPass(createModuleToFunctionPassAdaptor(stack()));
  // MPM.addPass(createModuleToFunctionPassAdaptor(llvm::DCEPass()));
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

  MPM.addPass(VerifierPass(false));
  MPM.addPass(LinearAllocationRecovery());
  // MPM.addPass(PointerTypeRecovery(
  //     llvm::DebugFlag &&
  //     llvm::isCurrentDebugType("pointer-type-recovery")));
  MPM.addPass(VerifierPass(false));
  MPM.addPass(createModuleToFunctionPassAdaptor(MemsetMatcher()));
  MPM.addPass(createModuleToFunctionPassAdaptor(MemcpyMatcher()));
  // instcombine will revert matched memset and memcpy!!!
  // MPM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
  MPM.addPass(createModuleToFunctionPassAdaptor(UndoInstCombine()));
  MPM.addPass(createModuleToFunctionPassAdaptor(BDCEPass()));
  // MPM.addPass(createModuleToFunctionPassAdaptor(
  //     createFunctionToLoopPassAdaptor(LoopRotatePass())));
  // MPM.addPass(createModuleToFunctionPassAdaptor(
  //     createFunctionToLoopPassAdaptor(IndVarSimplifyPass())));
  MPM.addPass(createModuleToFunctionPassAdaptor(ReorderBlocksPass()));
}

void PassEnv::add_type_recovery_passes(int level) {
  MPM.addPass(mlsub::MLsubRecoveryMain(*TR));

  // level 3 with additional optimization and cleanup.
  if (level >= 3) {
    MPM.addPass(createModuleToFunctionPassAdaptor(ReorderBlocksPass()));
    MPM.addPass(mlsub::MLsubRecoveryOpt(*TR));
    MPM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(PromotePass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(GVNPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(BDCEPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(
        SimplifyCFGPass(SimplifyCFGOptions())));
    MPM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(UndoInstCombine()));
    MPM.addPass(createModuleToFunctionPassAdaptor(AllocAnnotator()));

    if (const char *val = std::getenv("NOTDEC_KEEP_DEAD_STACK")) {
      if ((std::strcmp(val, "1") == 0)) {
        // TODO MLSUB: support dead stack recovery in the new pipeline.
      }
    }
    MPM.addPass(createModuleToFunctionPassAdaptor(ReorderBlocksPass()));
  }
}

void PassEnv::build_passes(int level, bool stopBeforeTypeRecovery,
                           bool frozenTRInputIR) {
  TargetArch Arch = classifyTargetArch(Mod.getTargetTriple().getTriple());
  switch (Arch) {
  case TargetArch::Wasm:
    break;
  case TargetArch::Evm:
    // EVM inputs still benefit from LLVM's local canonicalization before
    // Solidity/EVM-specific matchers inspect the IR.
    MPM.addPass(
        createModuleToFunctionPassAdaptor(buildFunctionOptimizations()));
    MPM.addPass(createModuleToFunctionPassAdaptor(
        evm::SelectorEntryOutliningPass()));
    MPM.addPass(evm::SolidityPatternAnnotationPass());
    MPM.addPass(createModuleToFunctionPassAdaptor(
        evm::SelectorInlinedLogicExtractionPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(evm::PayabilityGuardPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(evm::AbiDecodePass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(evm::AbiReturnPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(evm::SolidityRevertPass()));
    MPM.addPass(
        createModuleToFunctionPassAdaptor(evm::AbiRevertEncodingPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(evm::CheckedBoundsPass()));
    MPM.addPass(
        createModuleToFunctionPassAdaptor(evm::ValueCleanupTypeHintPass()));
    MPM.addPass(
        createModuleToFunctionPassAdaptor(evm::StorageAddressingPass()));
    MPM.addPass(
        createModuleToFunctionPassAdaptor(evm::PackedStorageFieldPass()));
    MPM.addPass(
        createModuleToFunctionPassAdaptor(evm::StorageBytesStringPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(evm::MemoryObjectPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(evm::EventLogPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(evm::ExternalCallPass()));
    MPM.addPass(VerifierPass(false));
    return;
  case TargetArch::Other:
    return;
  }

  if (level < 1) {
    return;
  }

  if (level >= 2) {
    prepareTypeRecoveryContext();
    if (!frozenTRInputIR) {
      add_pre_type_recovery_passes();
      if (stopBeforeTypeRecovery) {
        return;
      }
    }
    add_type_recovery_passes(level);
    return;
  }

  FunctionPassManager FPM = buildFunctionOptimizations();
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
}

void PassEnv::add_llvm2c(std::string OutFilePath,
                         ::notdec::llvm2c::Options llvm2cOpt,
                         bool disableTypeRecovery,
                         bool captureHTypeSnapshot) {
  CachedHTypeSnapshot.clear();
  HasCachedHTypeSnapshot = false;
  MPM.addPass(MLsubNotdecLLVM2C(
      *TR, OutFilePath, llvm2cOpt, disableTypeRecovery,
      captureHTypeSnapshot ? &CachedHTypeSnapshot : nullptr,
      captureHTypeSnapshot ? &HasCachedHTypeSnapshot : nullptr));
}

void PassEnv::run_passes() {
  if (auto DebugDir = notdec::getWorkDirOpt()) {
    llvm::sys::fs::create_directories(*DebugDir);
    printModule(Mod, join(*DebugDir, "00-lifted.ll").c_str());
  }

  MPM.run(Mod, MAM);
}

void PassEnv::emit_tr_input_ir(const std::string &OutputPath) {
  if (TR == nullptr) {
    llvm::errs() << "Error: --emit-tr-input-ir requires the pre-type-recovery "
                    "pipeline to be initialized.\n";
    std::abort();
  }
  TR->emitTRInputArtifacts(Mod, OutputPath);
}

void PassEnv::dump_htypes(const std::string &OutputPath) {
  if (OutputPath.empty()) {
    return;
  }

  if (HasCachedHTypeSnapshot) {
    std::error_code EC;
    llvm::raw_fd_ostream OS(OutputPath, EC, llvm::sys::fs::OF_Text);
    if (EC) {
      llvm::errs() << "Cannot open HType dump output file " << OutputPath
                   << ": " << EC.message() << "\n";
      std::abort();
    }
    OS << CachedHTypeSnapshot;
    return;
  }

  if (TR == nullptr) {
    llvm::errs() << "Error: --dump-htypes requires type recovery to be "
                    "initialized (tr-level >= 2).\n";
    std::abort();
  }

  std::unique_ptr<mlsub::MLsubRecovery::Result> &HighTypes =
      TR->getResult(Mod, MAM);
  if (HighTypes == nullptr) {
    llvm::errs() << "Error: failed to materialize HType results for dump.\n";
    std::abort();
  }

  std::error_code EC;
  llvm::raw_fd_ostream OS(OutputPath, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Cannot open HType dump output file " << OutputPath
                 << ": " << EC.message() << "\n";
    std::abort();
  }

  HighTypes->print(OS);
}

// 需要去掉尾递归等优化，因此需要构建自己的Pass。
llvm::FunctionPassManager buildFunctionOptimizations() {
  FunctionPassManager
      FPM; // = PB.buildFunctionSimplificationPipeline(OptimizationLevel::O1,
           // ThinOrFullLTOPhase::None);

  FPM.addPass(VerifierPass());
  FPM.addPass(InstCombinePass());
  FPM.addPass(SimplifyCFGPass(SimplifyCFGOptions()));
  FPM.addPass(InstCombinePass());
  FPM.addPass(llvm::PromotePass());
  FPM.addPass(llvm::GVNPass());

  // Specially optimize memory movement as it doesn't look like dataflow in SSA.
  // FPM.addPass(MemCpyOptPass());

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
  // 简化Phi里面重复的基本块和值
  FPM.addPass(SimplifyCFGPass(SimplifyCFGOptions()));

  return FPM;
}

} // namespace notdec::passes
