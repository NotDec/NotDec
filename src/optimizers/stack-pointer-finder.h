#ifndef _NOTDEC_OPTIMIZERS_STACK_POINTER_FINDER_H_
#define _NOTDEC_OPTIMIZERS_STACK_POINTER_FINDER_H_

#include <iostream>
#include <map>

#include "llvm/Pass.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>

namespace notdec::optimizers {

using namespace llvm;

GlobalVariable *find_stack_ptr(Module &mod);
GlobalVariable *find_stack_ptr(Function &f);
GlobalVariable *find_stack_ptr(BasicBlock &entryBlock);

struct StackPointerFinderResult {
  GlobalVariable *result;
};

class StackPointerFinderAnalysis
    : public AnalysisInfoMixin<StackPointerFinderAnalysis> {
public:
  explicit StackPointerFinderAnalysis() = default;
  ~StackPointerFinderAnalysis() = default;
  // Provide a unique key, i.e., memory address to be used by the LLVM's pass
  // infrastructure.
  static inline llvm::AnalysisKey Key; // NOLINT
  friend llvm::AnalysisInfoMixin<StackPointerFinderAnalysis>;

  // Specify the result type of this analysis pass.
  using Result = StackPointerFinderResult;

  // Analyze the bitcode/IR in the given LLVM module.
  static Result run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM) {
    // The demangled(!) function name that we wish to find.
    StackPointerFinderResult result;
    result.result = find_stack_ptr(M);
    return result;
  }
};
} // namespace notdec::optimizers
#endif
