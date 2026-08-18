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

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
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
#include <llvm/IR/IntrinsicInst.h>
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
#include <llvm/Transforms/Scalar/Scalarizer.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/SimplifyCFGOptions.h>

#include "Passes/AllocAnnotator.h"
#include "Passes/DangerousTypePatternScan.h"
#include "Passes/MemOpMatcher.h"
#include "Passes/evm/MemoryBufferAnalysis.h"
#include "Passes/PassManager.h"
#include "Passes/ReorderBasicblock.h"
#include "Passes/StackAlloca.h"
#include "Passes/evm/SolidityPatterns.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"
#include "Utils/Utils.h"
#include "notdec-wasm2llvm/utils.h"

#ifdef NOTDEC_ENABLE_LLVM2C
#include "notdec-backends/Solidity/Backend.h"
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/StructuralAnalysis.h"
#endif

namespace notdec::passes {

using notdec::frontend::wasm::MEM_NAME;

using namespace llvm;

namespace {

// The middle-end passes currently encode Wasm lowering assumptions. Keep the
// target split explicit so native source IR can run type recovery without
// accidentally reusing Wasm stack/memory recovery passes.
enum class TargetArch { Wasm, Evm, X64, Other };

TargetArch classifyTargetArch(StringRef Triple) {
  if (Triple.starts_with("wasm32") || Triple.starts_with("wasm64")) {
    return TargetArch::Wasm;
  }
  if (Triple.starts_with("evm")) {
    return TargetArch::Evm;
  }
  if (Triple.starts_with("x86_64") || Triple.starts_with("amd64")) {
    return TargetArch::X64;
  }
  return TargetArch::Other;
}

bool isDirectFreeCall(const CallInst &Call) {
  auto *Callee = Call.getCalledFunction();
  return Callee != nullptr && Callee->getName() == "free" &&
         Call.arg_size() == 1;
}

bool isFirstRealInstructionInBlock(const Instruction &Inst) {
  for (const Instruction &Cur : *Inst.getParent()) {
    if (&Cur == &Inst) {
      return true;
    }
    if (isa<PHINode>(Cur) || isa<DbgInfoIntrinsic>(Cur)) {
      continue;
    }
    return false;
  }
  return false;
}

SmallVector<unsigned, 2> successorIndicesTo(const BasicBlock &Pred,
                                            const BasicBlock &Succ) {
  SmallVector<unsigned, 2> Indices;
  const Instruction *Term = Pred.getTerminator();
  for (unsigned I = 0, E = Term->getNumSuccessors(); I != E; ++I) {
    if (Term->getSuccessor(I) == &Succ) {
      Indices.push_back(I);
    }
  }
  return Indices;
}

// Split a `free(phi)` sink into edge-local free calls.  `free` takes `void *`,
// so optimized IR may legally merge unrelated pointer values into one PHI only
// to pass it to `free`.  Keeping that PHI before MLsub makes type recovery see
// false value flow between unrelated pointee types.
struct FreePhiSplitPass : PassInfoMixin<FreePhiSplitPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    SmallVector<CallInst *, 8> Worklist;
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        auto *Call = dyn_cast<CallInst>(&I);
        if (Call == nullptr || !isDirectFreeCall(*Call)) {
          continue;
        }
        auto *Phi = dyn_cast<PHINode>(Call->getArgOperand(0));
        if (Phi == nullptr || Phi->getParent() != &BB || !Phi->hasOneUse() ||
            !isFirstRealInstructionInBlock(*Call)) {
          continue;
        }
        Worklist.push_back(Call);
      }
    }

    bool Changed = false;
    unsigned SplitCalls = 0;
    unsigned InsertedCalls = 0;
    for (CallInst *Call : Worklist) {
      auto *Phi = cast<PHINode>(Call->getArgOperand(0));
      if (!splitFreePhiCall(*Call, *Phi, InsertedCalls)) {
        continue;
      }
      ++SplitCalls;
      Changed = true;
    }

    if (SplitCalls != 0) {
      errs() << "Info: split free(phi) in " << F.getName() << ": " << SplitCalls
             << " call(s), " << InsertedCalls << " edge-local free call(s)\n";
    }
    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }

private:
  static bool splitFreePhiCall(CallInst &Call, PHINode &Phi,
                               unsigned &InsertedCalls) {
    BasicBlock *Join = Call.getParent();
    DenseMap<BasicBlock *, Value *> IncomingByPred;
    SmallVector<std::pair<BasicBlock *, Value *>, 8> Incoming;

    for (unsigned I = 0, E = Phi.getNumIncomingValues(); I != E; ++I) {
      auto *Pred = Phi.getIncomingBlock(I);
      auto *Value = Phi.getIncomingValue(I);
      if (Pred == Join) {
        return false;
      }
      auto It = IncomingByPred.find(Pred);
      if (It != IncomingByPred.end()) {
        if (It->second != Value) {
          return false;
        }
        continue;
      }
      IncomingByPred.insert({Pred, Value});
      Incoming.push_back({Pred, Value});
    }

    SmallVector<std::pair<BasicBlock *, Value *>, 8> EdgeBlocks;
    for (auto [Pred, Value] : Incoming) {
      auto SuccIndices = successorIndicesTo(*Pred, *Join);
      if (SuccIndices.empty()) {
        return false;
      }

      auto *Term = Pred->getTerminator();
      if (Term->getNumSuccessors() == 1) {
        EdgeBlocks.push_back({Pred, Value});
        continue;
      }

      for (unsigned SuccIndex : SuccIndices) {
        BasicBlock *Split = SplitCriticalEdge(Term, SuccIndex);
        if (Split == nullptr) {
          return false;
        }
        EdgeBlocks.push_back({Split, Value});
      }
    }

    for (auto [EdgeBlock, Value] : EdgeBlocks) {
      Instruction *Term = EdgeBlock->getTerminator();
      auto *NewCall = cast<CallInst>(Call.clone());
      NewCall->setArgOperand(0, Value);
      NewCall->insertBefore(Term->getIterator());
      ++InsertedCalls;
    }

    Call.eraseFromParent();
    if (Phi.use_empty()) {
      Phi.eraseFromParent();
    }
    return true;
  }
};

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

  mlsub::MLsubRecovery *TR = nullptr;
  std::string OutFilePath;
  ::notdec::llvm2c::Options llvm2cOpt;
  bool disableTypeRecovery = false;

  MLsubNotdecLLVM2C(mlsub::MLsubRecovery *TR, std::string outFilePath,
                    ::notdec::llvm2c::Options &llvm2cOpt,
                    bool disableTypeRecovery)
      : TR(TR), OutFilePath(outFilePath), llvm2cOpt(std::move(llvm2cOpt)),
        disableTypeRecovery(disableTypeRecovery) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
    std::string outsuffix = getSuffix(OutFilePath);
    assert(outsuffix == ".c");

    // Run type recovery.
    std::unique_ptr<mlsub::MLsubRecovery::Result> HighTypes;
    if (!disableTypeRecovery) {
      assert(TR != nullptr && "type recovery context is required");
      HighTypes = std::move(TR->getResult(M, MAM));
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

// A Pass that converts a module to Solidity-like source.
struct MLsubNotdecSolidity : PassInfoMixin<MLsubNotdecSolidity> {

  mlsub::MLsubRecovery &TR;
  std::string OutFilePath;

  MLsubNotdecSolidity(mlsub::MLsubRecovery &TR, std::string outFilePath)
      : TR(TR), OutFilePath(std::move(outFilePath)) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
    std::string outsuffix = getSuffix(OutFilePath);
    assert(outsuffix == ".sol");

    std::unique_ptr<mlsub::MLsubRecovery::Result> HighTypes =
        std::move(TR.getResult(M, MAM));

    std::error_code EC;
    llvm::raw_fd_ostream os(OutFilePath, EC);
    if (EC) {
      std::cerr << "Cannot open output file: " << OutFilePath << std::endl;
      std::cerr << EC.message() << std::endl;
      std::abort();
    }

    notdec::backend::solidity::Options Opts;
    if (auto DebugDir = notdec::getWorkDirOpt()) {
      Opts.workDir = *DebugDir;
    }

    const char *WorkDir = Opts.workDir.empty() ? nullptr : Opts.workDir.c_str();
    if (HighTypes) {
      notdec::llvm2c::demoteSSAFixHT(M, MAM, *HighTypes, WorkDir);
    } else {
      notdec::llvm2c::demoteSSA(M, MAM);
    }

    notdec::backend::solidity::decompileModule(M, MAM, os, Opts,
                                               std::move(HighTypes));
    std::cout << "Decompile result: " << OutFilePath << std::endl;

    return PreservedAnalyses::all();
  }
  static bool isRequired() { return true; }
};

struct HTypeDumpPass : PassInfoMixin<HTypeDumpPass> {
  mlsub::MLsubRecovery &TR;
  std::string OutputPath;

  HTypeDumpPass(mlsub::MLsubRecovery &TR, StringRef OutputPath)
      : TR(TR), OutputPath(OutputPath.str()) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
    std::unique_ptr<mlsub::MLsubRecovery::Result> &HighTypes =
        TR.getResult(M, MAM);
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

void PassEnv::add_type_recovery_passes(int level, bool SplitFreePhi) {
  ScalarizerPassOptions ScalarizerOptions;
  ScalarizerOptions.ScalarizeLoadStore = true;
  // MLsub reasons about scalar memory fields.  Source/native IR may still carry
  // SLP-created vector loads and stores, which otherwise look like one wide
  // field access and block struct-pointer merge policies.
  MPM.addPass(
      createModuleToFunctionPassAdaptor(ScalarizerPass(ScalarizerOptions)));
  if (SplitFreePhi) {
    MPM.addPass(createModuleToFunctionPassAdaptor(FreePhiSplitPass()));
  }
  // 类型推理前扫描危险模式：对可能让推理爆炸的函数报 warning。
  MPM.addPass(DangerousTypePatternScan());
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
                           bool frozenTRInputIR, StringRef HTypeDumpPath,
                           StringRef MergeEvalDirArg,
                           bool MergeStructPtrLoadStoreArg,
                           StringRef EmitPostConstraintStateArg,
                           StringRef LoadPostConstraintStateArg) {
  MergeEvalDir = MergeEvalDirArg.str();
  EmitPostConstraintState = EmitPostConstraintStateArg.str();
  LoadPostConstraintState = LoadPostConstraintStateArg.str();
  TypeRecoveryLevel = level;
  MergeStructPtrLoadStore = MergeStructPtrLoadStoreArg;
  TargetArch Arch = classifyTargetArch(Mod.getTargetTriple().getTriple());
  switch (Arch) {
  case TargetArch::Wasm:
    break;
  case TargetArch::Evm:
    if (!frozenTRInputIR) {
      // EVM inputs still benefit from LLVM's local canonicalization before
      // Solidity/EVM-specific matchers inspect the IR. Keep this target-local
      // pipeline separate from the Wasm pre-TR passes because those still
      // assume Wasm memory and stack shapes.
      MPM.addPass(
          createModuleToFunctionPassAdaptor(buildFunctionOptimizations()));
      MPM.addPass(createModuleToFunctionPassAdaptor(
          evm::SelectorEntryOutliningPass()));
      MPM.addPass(
          createModuleToFunctionPassAdaptor(evm::PayabilityGuardPass()));
      MPM.addPass(
          createModuleToFunctionPassAdaptor(evm::MemoryBufferRewritePass()));
      MPM.addPass(evm::EvmCalldataAccessPass());
      MPM.addPass(evm::AbiDecoderHelperRenamePass());
      MPM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
    }
    MPM.addPass(VerifierPass(false));
    if (level >= 2) {
      prepareTypeRecoveryContext();
      if (stopBeforeTypeRecovery) {
        return;
      }
      add_type_recovery_passes(level, /*SplitFreePhi=*/false);
      if (!HTypeDumpPath.empty()) {
        MPM.addPass(HTypeDumpPass(*TR, HTypeDumpPath));
      }
      MPM.addPass(evm::AbiReturnPass(*TR));
      MPM.addPass(evm::SolidityRevertPass(*TR));
      MPM.addPass(createModuleToFunctionPassAdaptor(evm::CheckedBoundsPass()));
      MPM.addPass(evm::EventLogPass(*TR));
      MPM.addPass(evm::EvmStorageHighLevelRewritePass(*TR));
    }
    return;
  case TargetArch::X64:
    if (level >= 2) {
      // Source-built native IR is already normal LLVM IR. Do not run the Wasm
      // pre-TR recovery passes here; they assume Wasm-specific stack/memory
      // shapes and can rewrite x64 source IR in the wrong direction.
      prepareTypeRecoveryContext();
      if (stopBeforeTypeRecovery) {
        return;
      }
      add_type_recovery_passes(level);
      if (!HTypeDumpPath.empty()) {
        MPM.addPass(HTypeDumpPass(*TR, HTypeDumpPath));
      }
    }
    return;
  case TargetArch::Other:
    if (!HTypeDumpPath.empty() || !MergeEvalDir.empty()) {
      llvm::errs() << "Error: HType dump / merge eval requires "
                      "a target with type recovery pipeline support.\n";
      std::abort();
    }
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
    if (!HTypeDumpPath.empty()) {
      MPM.addPass(HTypeDumpPass(*TR, HTypeDumpPath));
    }
    return;
  }

  FunctionPassManager FPM = buildFunctionOptimizations();
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
}

void PassEnv::add_llvm2c(std::string OutFilePath,
                         ::notdec::llvm2c::Options llvm2cOpt,
                         bool disableTypeRecovery) {
  MPM.addPass(MLsubNotdecLLVM2C(disableTypeRecovery ? nullptr : TR.get(),
                                OutFilePath, llvm2cOpt,
                                disableTypeRecovery));
}

void PassEnv::add_solidity(std::string OutFilePath) {
  MPM.addPass(MLsubNotdecSolidity(*TR, OutFilePath));
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
