#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Debug.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <llvm/ADT/StringRef.h>
#include <llvm/Analysis/ValueTracking.h>
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
#include "Passes/PointerTypeRecovery.h"
#include "Passes/ReorderBasicblock.h"
#include "Passes/StackAlloca.h"
#include "Passes/retdec-stack/retdec-abi.h"
#include "Passes/retdec-stack/retdec-stack-pointer-op-remove.h"
#include "Passes/retdec-stack/retdec-stack.h"
#include "Passes/retdec-stack/retdec-symbolic-tree.h"
#include "TypeRecovery/TRContext.h"
#include "Utils/Utils.h"
#include "notdec-wasm2llvm/utils.h"

#ifdef NOTDEC_ENABLE_LLVM2C
#include "notdec-llvm2c/Interface.h"
#endif

namespace notdec::passes {

using notdec::frontend::wasm::MEM_NAME;

using namespace llvm;

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
            if (llvm::haveNoCommonBitsSet(LHS, RHS, DL, &AC, &I, &DT)) {
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
struct NotdecLLVM2C : PassInfoMixin<NotdecLLVM2C> {

  TypeRecovery &TR;
  std::string OutFilePath;
  ::notdec::llvm2c::Options llvm2cOpt;
  bool disableTypeRecovery = false;

  NotdecLLVM2C(TypeRecovery &TR, std::string outFilePath,
               ::notdec::llvm2c::Options &llvm2cOpt, bool disableTypeRecovery)
      : TR(TR), OutFilePath(outFilePath), llvm2cOpt(std::move(llvm2cOpt)),
        disableTypeRecovery(disableTypeRecovery) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
    std::string outsuffix = getSuffix(OutFilePath);
    assert(outsuffix == ".c");

    // Run type recovery.
    std::unique_ptr<TypeRecovery::Result> HighTypes;
    if (!disableTypeRecovery) {
      HighTypes = std::move(TR.getResult(M, MAM));
      HighTypes->dump();
    }

    std::error_code EC;
    llvm::raw_fd_ostream os(OutFilePath, EC);
    if (EC) {
      std::cerr << "Cannot open output file." << std::endl;
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

// 把malloc的函数签名改为返回指针，把free的函数签名改为接受指针。
// 因为LLVM不能直接修改函数的签名，导致需要创建一个新的函数替代旧的函数。
struct FuncSigModify : PassInfoMixin<FuncSigModify> {
  std::vector<std::pair<Function *, Function *>> toReplace;
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    llvm::IRBuilder<> builder(M.getContext());
    // 1 为每个要改变前面的函数创建新的函数
    Function *malloc = M.getFunction("malloc");
    if (malloc != nullptr && (!malloc->getReturnType()->isPointerTy())) {
      addChangeRetToPointer(malloc);
    }
    Function *calloc = M.getFunction("calloc");
    if (calloc != nullptr && (!calloc->getReturnType()->isPointerTy())) {
      addChangeRetToPointer(calloc);
    }
    Function *free = M.getFunction("free");
    if (free != nullptr && (!free->getReturnType()->isPointerTy())) {
      addChangeArgToPointer(free, 0);
    }
    for (Function &F : M) {
      run(F);
    }
    // 删除原来的malloc
    for (auto &p : toReplace) {
      std::string name = p.first->getName().str();
      p.first->eraseFromParent();
      p.second->setName(name);
    }
    return PreservedAnalyses::all();
  }

  void addChangeArgToPointer(Function *func, int index) {
    std::cerr << "Changing the definition of " << func->getName().str()
              << std::endl;
    // lib\Transforms\Utils\CloneFunction.cpp
    std::vector<Type *> ArgTypes;

    int i = 0;
    for (const Argument &I : func->args()) {
      if (i == index) {
        ArgTypes.push_back(I.getType()->getPointerTo());
      } else {
        ArgTypes.push_back(I.getType());
      }
      i++;
    }

    FunctionType *fty = FunctionType::get(func->getReturnType(), ArgTypes,
                                          func->getFunctionType()->isVarArg());
    Function *newFunc =
        Function::Create(fty, func->getLinkage(), func->getAddressSpace(),
                         func->getName(), func->getParent());
    toReplace.emplace_back(func, newFunc);
  }

  void addChangeRetToPointer(Function *func) {
    std::cerr << "Changing the definition of " << func->getName().str()
              << std::endl;
    // lib\Transforms\Utils\CloneFunction.cpp
    std::vector<Type *> ArgTypes;

    for (const Argument &I : func->args())
      ArgTypes.push_back(I.getType());
    FunctionType *fty =
        FunctionType::get(func->getReturnType()->getPointerTo(), ArgTypes,
                          func->getFunctionType()->isVarArg());
    Function *newFunc =
        Function::Create(fty, func->getLinkage(), func->getAddressSpace(),
                         func->getName(), func->getParent());
    toReplace.emplace_back(func, newFunc);
  }

  // 直接遍历新函数的每一个参数的类型，和旧的函数参数的类型对比，如果发现有不同，就创建bitcast指令转换
  // 返回值同理。最后替换原有的call指令。
  void changeCall(CallInst *call, Function *to) {
    IRBuilder<> builder(call->getContext());
    std::vector<Value *> args;
    builder.SetInsertPoint(call);
    Function *called = call->getCalledFunction();
    // 为参数创建bitcast
    for (int i = 0; i < called->arg_size(); i++) {
      const Argument *current = called->getArg(i);
      const Argument *target = to->getArg(i);
      if (current->getType() != target->getType()) {
        Value *arg = builder.CreateBitOrPointerCast(call->getArgOperand(i),
                                                    target->getType());
        args.push_back(arg);
        llvm::errs() << "bitcast arg " << i << ": " << current;
      } else {
        args.push_back(call->getArgOperand(i));
      }
    }
    // 创建调用
    Value *new_call = builder.CreateCall(to->getFunctionType(), to, args);
    // 为返回值创建bitcast，转换回原来的指令
    if (called->getReturnType() != to->getReturnType()) {
      // builder.SetInsertPoint(call->getNextNode());
      new_call =
          builder.CreateBitOrPointerCast(new_call, called->getReturnType());
    }
    // 替换指令返回值
    call->replaceAllUsesWith(new_call);
    call->eraseFromParent();
  }

  void run(Function &F) {
    for (BasicBlock &BB : F) {
      auto InstIter = BB.begin();
      while (InstIter != BB.end()) {
        CallInst *call = dyn_cast_or_null<CallInst>(&*InstIter);
        ++InstIter;
        if ((call != nullptr)) {
          for (auto p : toReplace) {
            if (call->getCalledFunction() == p.first) {
              changeCall(call, p.second);
              break;
            }
          }
        }
      }
    }
  }

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
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
  for (GlobalVariable &gv : Mod.getGlobalList()) {
    if (gv.getName().equals(MEM_NAME)) {
      Mem = &gv;
    }
  }
  SP = StackPointerFinderAnalysis::find_stack_ptr(Mod);
}

void PassEnv::build_passes(int level) {
  // level 1 only optimizations
  if (level >= 1) {
    MPM.addPass(createModuleToFunctionPassAdaptor(ReorderBlocksPass()));

    FunctionPassManager FPM = buildFunctionOptimizations();
    // MPM.addPass(FunctionRenamer());
    // MPM.addPass(createModuleToFunctionPassAdaptor(stack()));
    // MPM.addPass(createModuleToFunctionPassAdaptor(llvm::DCEPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

    // level 2 no stack breaking
    if (level >= 2) {
      prepareTypeRecoveryContext();
      FAM.registerPass([&]() { return FunctionTypeRecovery(*TR); });

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
      MPM.addPass(TypeRecoveryMain(*TR));

      // level 3 with TypeRecoveryOpt and stack breaking.
      if (level >= 3) {
        MPM.addPass(TypeRecoveryOpt(*TR));
        MPM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
        MPM.addPass(createModuleToFunctionPassAdaptor(PromotePass()));
        MPM.addPass(createModuleToFunctionPassAdaptor(GVNPass()));
        MPM.addPass(createModuleToFunctionPassAdaptor(BDCEPass()));
        MPM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
        MPM.addPass(createModuleToFunctionPassAdaptor(
            SimplifyCFGPass(SimplifyCFGOptions())));
        MPM.addPass(createModuleToFunctionPassAdaptor(UndoInstCombine()));
        MPM.addPass(createModuleToFunctionPassAdaptor(AllocAnnotator()));
        MPM.addPass(RecoverDeadAlloca(*TR));
        MPM.addPass(InvalidateAllTypes(*TR));
      }
    }
  }
}

void PassEnv::add_llvm2c(std::string OutFilePath,
                         ::notdec::llvm2c::Options llvm2cOpt,
                         bool disableTypeRecovery) {
  MPM.addPass(NotdecLLVM2C(*TR, OutFilePath, llvm2cOpt, disableTypeRecovery));
}

void PassEnv::run_passes() {
  const char *DebugDir = getTRDebugDir();
  if (DebugDir) {
    llvm::sys::fs::create_directories(DebugDir);
    printModule(Mod, join(DebugDir, "00-lifted.ll").c_str());
  }

  MPM.run(Mod, MAM);
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
