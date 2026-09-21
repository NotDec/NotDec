// Reconstructs the source-level shape of the Yul functions that solc's
// full inliner would have inlined.  The decompiled internal helpers are
// subroutines only because the forward compiler's FullInliner kept them, and
// the calldata offsets they read are relative to their own formal arguments;
// once the calls are expanded the offsets fold to the absolute per-entry
// constants the original Solidity had, which is what the recovered calldata
// record and the ABI parameter table need.
//
// The pass deliberately uses LLVM's own inliner and its default cost model
// instead of expanding every call: the forward compiler also inlined by
// judgement (single use, small bodies, constant arguments), and an exhaustive
// expansion floods the structurer with duplicated bounds/revert code.  Running
// it first -- before entry outlining, the per-entry calldata view and the
// calldata.index annotations -- means every later EVM pass sees the flattened
// shape, so the annotations land on the absolute offsets.
#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/IPO/Inliner.h>

#include <cstdlib>
#include <cstring>

#define DEBUG_TYPE "evm-helper-inlining"

STATISTIC(NumHelperFunctionsRemoved,
          "Number of unreferenced helper functions removed after inlining");

using namespace llvm;

namespace notdec::passes::evm {
namespace {

bool helperInliningDiagnosticsEnabled() {
  const char *Value = std::getenv("NOTDEC_EVM_INLINE_DIAG");
  return Value != nullptr && Value[0] != '\0' && std::strcmp(Value, "0") != 0;
}

unsigned countCalls(const Module &M) {
  unsigned Count = 0;
  for (const Function &F : M) {
    for (const Instruction &I : instructions(F)) {
      if (isa<CallBase>(&I)) {
        ++Count;
      }
    }
  }
  return Count;
}

unsigned countDefinitions(const Module &M) {
  unsigned Count = 0;
  for (const Function &F : M) {
    if (!F.isDeclaration()) {
      ++Count;
    }
  }
  return Count;
}

// Public entries are the contract's outward ABI boundary; a helper is any other
// function that still has a body.  Unreferenced local definitions can never be
// called again and would only make the backend render bodies nobody uses.
bool isRemovableHelper(const Function &F) {
  if (F.isDeclaration() || detail::isPublicEntryFunction(F)) {
    return false;
  }
  return F.hasLocalLinkage() && F.use_empty() && !F.hasAddressTaken();
}

} // namespace

// The phase is behind an environment switch until the pattern oracles that
// were calibrated on the outlined shape are re-baselined.
bool isHelperInliningEnabled() {
  const char *Value = std::getenv("NOTDEC_EVM_INLINE");
  return Value != nullptr && Value[0] != '\0' && std::strcmp(Value, "0") != 0;
}

PreservedAnalyses EvmHelperInliningPass::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  if (!isHelperInliningEnabled()) {
    return PreservedAnalyses::all();
  }

  const unsigned CallsBefore = countCalls(M);
  const unsigned DefinitionsBefore = countDefinitions(M);

  ModulePassManager InlinerPipeline;
  InlinerPipeline.addPass(ModuleInlinerWrapperPass());
  InlinerPipeline.run(M, MAM);

  SmallVector<Function *, 32> Dead;
  for (Function &F : M) {
    if (isRemovableHelper(F)) {
      Dead.push_back(&F);
    }
  }
  for (Function *F : Dead) {
    F->eraseFromParent();
    ++NumHelperFunctionsRemoved;
  }

  if (helperInliningDiagnosticsEnabled()) {
    errs() << "[evm-inline-helpers] calls " << CallsBefore << " -> "
           << countCalls(M) << ", definitions " << DefinitionsBefore << " -> "
           << countDefinitions(M) << ", removed " << Dead.size() << "\n";
  }
  // The inliner rewrote bodies across the whole call graph; nothing about the
  // module-level analyses survives a shape change of this size.
  return PreservedAnalyses::none();
}

} // namespace notdec::passes::evm
