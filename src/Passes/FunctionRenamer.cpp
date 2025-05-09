#include <iostream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/raw_ostream.h>
using namespace llvm;

namespace notdec::frontend::passes {

/// Find some stub function calls. rename jmp_XXX.
class FunctionRenamer : public PassInfoMixin<FunctionRenamer> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
    for (Function &F : M.functions()) {
      unsigned int instCount = F.size();
      unsigned short callInstCount = 0;
      Function *callee;
      if (instCount <= 6) {
        for (auto &BB : F) {
          for (auto &I : BB) {
            if (isa<CallInst>(I)) {
              callInstCount++;
              callee = dyn_cast<CallInst>(&I)->getCalledFunction();
            }
          }
        }

        if (callInstCount == 1) {
          if (callee->getName().str().find("func_") == 0) {
            std::string funcName = F.getName().str();
            F.setName("jmp_" + funcName);
            callee->setName(funcName);
          }
        }
      }
    }

    return PreservedAnalyses::all();
  }
};

} // namespace notdec::frontend::passes
