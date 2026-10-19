// Reconstructs the source-level shape of the Yul functions that solc's
// full inliner would have inlined.  The decompiled internal helpers are
// subroutines only because the forward compiler's FullInliner kept them, and
// the calldata offsets they read are relative to their own formal arguments;
// once the calls are expanded the offsets fold to the absolute per-entry
// constants the original Solidity had, which is what the recovered calldata
// record and the ABI parameter table need.
//
// The scope stays on the calldata chain (the readers plus everything that can
// reach them) instead of expanding every call: an exhaustive expansion floods
// the structurer with duplicated bounds/revert code and destroys the helper
// coverage the Solidity output is meant to keep.  This flattened shape is the
// only one the EVM pipeline produces -- there is no outlined mode to keep
// alive.  Running the pass first -- before entry outlining, the per-entry
// calldata view and the calldata.index annotations -- means every later EVM
// pass sees the flattened shape, so the annotations land on the absolute
// offsets.
#include "binarysub/Env.h"
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
  return binarysub::env_flag("NOTDEC_EVM_HELPER_INLINE_DIAG");
}

// Public entries read calldata by definition, so they cannot be the seed of
// the scope: seeding from them would pull the whole module into the closure.
// A public entry is the contract's outward ABI boundary.  Inlining a small
// entry into the dispatcher leaves its original definition dead and lets the
// entry outlining clone the body (and its payability guard) a second time, so
// the entries are pinned even though nothing else calls them.
void pinPublicEntriesNoInline(Module &M) {
  for (Function &F : M) {
    if (!F.isDeclaration() && detail::isPublicEntryFunction(F)) {
      F.removeFnAttr(Attribute::AlwaysInline);
      F.addFnAttr(Attribute::NoInline);
    }
  }
}

bool isCalldataReader(const Function &F) {
  if (detail::isPublicEntryFunction(F)) {
    return false;
  }
  for (const Instruction &I : instructions(F)) {
    const auto *Call = dyn_cast<CallBase>(&I);
    if (Call == nullptr) {
      continue;
    }
    StringRef Name = detail::getCalleeName(Call);
    if (Name == "evm_calldataload" || Name == "evm_calldatacopy") {
      return true;
    }
  }
  return false;
}

// The calldata chain is the readers plus everything that can reach them (the
// entry-side wrappers).  Only those calls are forced open: the readers' callees
// are business logic and shared guard scaffolding, and the non-entry helpers
// nothing on a calldata path reaches are pinned with noinline so the LLVM cost
// model cannot flatten them either.
void restrictInliningToCalldataChains(Module &M) {
  SmallPtrSet<const Function *, 32> Scoped;
  for (Function &F : M) {
    if (!F.isDeclaration() && isCalldataReader(F)) {
      Scoped.insert(&F);
    }
  }
  // An empty reader set still has to pin every helper: the scope means "only
  // the calldata chains are expanded", so a module without a calldata read has
  // nothing to expand and must keep the outlined shape.

  // Only the upward closure is scoped.  A reader's callees are business logic
  // (balance math, storage updates) that does not need the entry's absolute
  // offsets, and pulling them in would flatten the shared guard helpers whose
  // checks the constant call sites prove dead.
  bool Changed = true;
  while (Changed) {
    Changed = false;
    for (Function &F : M) {
      if (F.isDeclaration() || Scoped.count(&F) != 0) {
        continue;
      }
      for (const Instruction &I : instructions(F)) {
        const auto *Call = dyn_cast<CallBase>(&I);
        const Function *Callee =
            Call == nullptr ? nullptr : Call->getCalledFunction();
        if (Callee != nullptr && Scoped.count(Callee) != 0) {
          Scoped.insert(&F);
          Changed = true;
          break;
        }
      }
    }
  }

  for (Function &F : M) {
    if (F.isDeclaration() || detail::isPublicEntryFunction(F)) {
      continue;
    }
    if (Scoped.count(&F) != 0) {
      F.removeFnAttr(Attribute::NoInline);
      F.addFnAttr(Attribute::AlwaysInline);
    } else {
      F.removeFnAttr(Attribute::AlwaysInline);
      F.addFnAttr(Attribute::NoInline);
    }
  }
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

PreservedAnalyses EvmHelperInliningPass::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  pinPublicEntriesNoInline(M);
  restrictInliningToCalldataChains(M);

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
    errs() << "[evm-inline-helpers] scope=calldata calls " << CallsBefore
           << " -> "
           << countCalls(M) << ", definitions " << DefinitionsBefore << " -> "
           << countDefinitions(M) << ", removed " << Dead.size() << "\n";
  }
  // The inliner rewrote bodies across the whole call graph; nothing about the
  // module-level analyses survives a shape change of this size.
  return PreservedAnalyses::none();
}

} // namespace notdec::passes::evm
