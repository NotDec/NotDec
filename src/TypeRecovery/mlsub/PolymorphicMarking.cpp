#include "notdec/TypeRecovery/mlsub/PolymorphicMarking.h"

#include <cstdlib>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include "binarysub/LogDir.h"
#include "notdec/TypeRecovery/mlsub/AllocatorBoundaryAnalysis.h"
#include "notdec/TypeRecovery/mlsub/MLsubGenerator.h"
#include "notdec/TypeRecovery/mlsub/Metadata.h"
#include "notdec/TypeRecovery/mlsub/PolymorphicPositions.h"

namespace notdec::mlsub {

namespace {

void writeOffsetList(llvm::raw_ostream &Out, const char *Key,
                     const std::vector<std::int64_t> &Values) {
  Out << ",\"" << Key << "\":[";
  for (std::size_t I = 0; I < Values.size(); ++I) {
    Out << (I == 0 ? "" : ",") << Values[I];
  }
  Out << "]";
}

} // namespace

std::string polymorphicMarkingReportPath() {
  if (auto Path = binarysub::log_file_path("poly-marking.json")) {
    return *Path;
  }
  return {};
}

bool polymorphicMarkingRecognitionOnly() {
  return binarysub::env_flag("NOTDEC_POLY_MARKING_ONLY");
}

bool polymorphicMarkingMarksAllocatorBoundaries() {
  return binarysub::env_flag("NOTDEC_POLY_MARK_ALLOCATOR_BOUNDARIES");
}

llvm::PreservedAnalyses
PolymorphicMarkingPass::run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM) {
  (void)MAM;

  // 1. The sources that used to run at the top of MLsubRecovery::run: the
  // summary/signature override documents, the structural malloc wrappers and
  // the builtin buffer/allocator name list.
  TR.loadOverridesAndPrimitives(M);
  TR.detectMallocWrappers(M);
  TR.markBuiltinPolymorphicBufferFunctions(M);
  TR.setPolymorphicMarkingDone(true);

  // 2. Allocator / raw-buffer boundaries, computed on the lifted IR.  The
  // analysis is name-independent; marking from it stays opt-in until its
  // thresholds are validated against the annotation.
  auto Evidence = analyzeAllocatorBoundaries(M);

  std::set<const llvm::Function *> BoundaryMarked;
  if (polymorphicMarkingMarksAllocatorBoundaries()) {
    for (const AllocatorBoundaryEvidence &Entry : Evidence) {
      if (Entry.Function != nullptr && Entry.AllocatorConfidence == "high") {
        BoundaryMarked.insert(Entry.Function);
      }
    }
    for (const llvm::Function *F : BoundaryMarked) {
      const_cast<llvm::Function *>(F)->setMetadata(
          KIND_MLSUB_POLYMORPHIC_FUNCTION,
          llvm::MDNode::get(F->getContext(), {}));
    }
    llvm::errs() << "Info: allocator-boundary marking: " << BoundaryMarked.size()
                 << " functions marked\n";
  }

  // 2c. Generic polymorphic wrappers.  A defined function that forwards one of
  // its own arguments through a call to a polymorphic function and returns that
  // call's result inherits the marking: its type has to be instantiated per
  // call site as well.  Separately, the wrapper may return the very object it
  // was handed, a type equality ("ret == arg k") applied inside the wrapper.
  // That identity is NOT inferred from the shape of the call: read/write and
  // fread return a count, getline a count, allocators a fresh pointer.  The
  // position is looked up from the callee (builtin copy semantics,
  // NOTDEC_POLY_FUNCS ret_alias_of_arg, or the callee's own
  // local_ret_alias_of_arg for wrapper-of-wrapper chains), the same way the
  // recovery treats an imported declaration.  Iterated to a fixpoint: wrappers
  // wrap wrappers.
  std::size_t WrapperMarked = 0;
  // ret==arg relations recorded on wrappers.  One per wrapper: the first
  // returned call whose aliased operand is one of the wrapper's arguments wins.
  std::size_t AliasRecorded = 0;
  // Relations the width/pointer invariant rejected.  With the lookup in place
  // this only fires for inconsistent input, never for read/write.
  std::size_t AliasRefused = 0;
  // Debug switch: NOTDEC_POLY_WRAPPER_DETECT=0 disables the generic wrapper
  // recognition (the marking itself is unchanged), so its cost can be isolated.
  const bool WrapperDetect =
      binarysub::env_flag("NOTDEC_POLY_WRAPPER_DETECT", true);
  if (WrapperDetect) {
    auto Strip = [](llvm::Value *V) -> llvm::Value * {
      while (V != nullptr) {
        if (auto *C = llvm::dyn_cast<llvm::CastInst>(V)) {
          if (C->getOpcode() == llvm::Instruction::BitCast ||
              C->getOpcode() == llvm::Instruction::AddrSpaceCast) {
            V = C->getOperand(0);
            continue;
          }
        }
        break;
      }
      return V;
    };
    // Two independent questions are asked about the callee:
    //   - is it polymorphic (so the wrapper also needs per-call-site types)?
    //   - is one of its arguments what it returns (object identity)?
    // Only the second one can justify a ret==arg relation, and the callee is
    // the only authority for it.
    auto CalleeReturnsArg = [&](const llvm::Function &Callee)
        -> std::optional<unsigned> {
      if (auto Local = getLocalRetAliasOfArg(Callee)) {
        return Local;
      }
      return TR.retAliasArgOf(Callee);
    };
    bool Changed = true;
    while (Changed) {
      Changed = false;
      for (llvm::Function &F : M) {
        if (F.isDeclaration() ||
            F.hasMetadata(KIND_MLSUB_POLYMORPHIC_FUNCTION)) {
          continue;
        }
        bool Marked = false;
        for (llvm::BasicBlock &BB : F) {
          for (llvm::Instruction &I : BB) {
            auto *Call = llvm::dyn_cast<llvm::CallBase>(&I);
            if (Call == nullptr || Call->getType()->isVoidTy()) {
              continue;
            }
            auto *Callee = Call->getCalledFunction();
            if (Callee == nullptr) {
              continue;
            }
            // The inner function either is already marked, or is a recognized
            // copy primitive by name: marking memcpy itself is deliberately
            // avoided (it is called from hundreds of sites and instantiating
            // its signature per call site dominates the runtime), so the name
            // check is what lets the xmemdup/xstrdup style wrappers be found.
            static const char *const CopyPrimitives[] = {
                "memcpy",  "memmove",  "mempcpy",  "strcpy",  "strncpy",
                "bcopy",   "__memcpy_chk", "__memmove_chk", "__strcpy_chk",
                "__strncpy_chk"};
            bool InnerIsPolymorphic =
                Callee->hasMetadata(KIND_MLSUB_POLYMORPHIC_FUNCTION);
            if (!InnerIsPolymorphic) {
              llvm::StringRef Name = Callee->getName();
              while (!Name.empty() && Name.front() == '_') {
                Name = Name.drop_front();
              }
              for (const char *Prim : CopyPrimitives) {
                if (Name == Prim) {
                  InnerIsPolymorphic = true;
                  break;
                }
              }
            }
            if (!InnerIsPolymorphic) {
              continue;
            }
            auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(Call->getNextNode());
            if (Ret == nullptr || Strip(Ret->getReturnValue()) != Call) {
              continue;
            }
            // 1) Wrapper marking: F forwards one of its own arguments into a
            //    polymorphic callee and returns that call's result.
            bool ForwardsOwnArgument = false;
            for (unsigned J = 0; J < Call->arg_size(); ++J) {
              auto *Arg = llvm::dyn_cast<llvm::Argument>(
                  Strip(Call->getArgOperand(J)));
              if (Arg != nullptr && Arg->getParent() == &F) {
                ForwardsOwnArgument = true;
                break;
              }
            }
            if (ForwardsOwnArgument) {
              F.setMetadata(KIND_MLSUB_POLYMORPHIC_FUNCTION,
                            llvm::MDNode::get(F.getContext(), {}));
              ++WrapperMarked;
              Changed = true;
              Marked = true;
            }
            // 2) Identity: only a callee that returns argument k can make the
            //    wrapper's return value the same object as one of the
            //    wrapper's arguments.  Take k from the callee; the value
            //    passed there must be one of F's own arguments (otherwise F
            //    returns a local object and nothing is aliased).
            if (auto AliasArg = CalleeReturnsArg(*Callee)) {
              if (*AliasArg < Call->arg_size() &&
                  !getLocalRetAliasOfArg(F).has_value()) {
                auto *Aliased = llvm::dyn_cast<llvm::Argument>(
                    Strip(Call->getArgOperand(*AliasArg)));
                llvm::Type *ReturnedTy = Ret->getReturnValue()->getType();
                if (Aliased != nullptr && Aliased->getParent() == &F &&
                    ReturnedTy->isPointerTy() &&
                    Aliased->getType()->isPointerTy()) {
                  if (setLocalRetAliasOfArg(F, Aliased->getArgNo(),
                                           ReturnedTy)) {
                    ++AliasRecorded;
                    Changed = true;
                  } else {
                    ++AliasRefused;
                  }
                }
              }
            }
            if (Marked) {
              break;
            }
          }
          if (Marked) {
            break;
          }
        }
      }
    }
    if (WrapperMarked > 0 || AliasRecorded > 0) {
      for (llvm::Function &F : M) {
        if (auto ArgNo = getLocalRetAliasOfArg(F)) {
          llvm::errs() << "  [wrapper] " << F.getName() << " ret=arg" << *ArgNo
                       << "\n";
        }
      }
      llvm::errs() << "Info: polymorphic-wrapper marking: " << WrapperMarked
                   << " functions marked";
      if (AliasRecorded > 0) {
        llvm::errs() << ", " << AliasRecorded << " ret==arg relation(s)";
      }
      if (AliasRefused > 0) {
        llvm::errs() << ", " << AliasRefused
                     << " ret==arg relation(s) refused (width/type mismatch)";
      }
      llvm::errs() << "\n";
    }
  }

  // 3. The report, which is also the recognition-only switch.
  const std::string ReportPath = polymorphicMarkingReportPath();
  if (ReportPath.empty()) {
    return llvm::PreservedAnalyses::all();
  }
  std::error_code EC;
  llvm::raw_fd_ostream Out(ReportPath, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Error: cannot write polymorphic marking report to "
                 << ReportPath << ": " << EC.message() << "\n";
    return llvm::PreservedAnalyses::all();
  }

  std::map<const llvm::Function *, const AllocatorBoundaryEvidence *> EvidenceByFn;
  for (const AllocatorBoundaryEvidence &Entry : Evidence) {
    if (Entry.Function != nullptr) {
      EvidenceByFn.emplace(Entry.Function, &Entry);
    }
  }
  const std::set<std::string> PolicyNames = loadPolyPolicyFunctionNames();
  const auto &MallocWrappers = TR.getCheckpointMallocWrappers();
  const auto &BuiltinMarked = TR.getCheckpointPolymorphicBufferFunctions();

  for (llvm::Function &F : M) {
    if (!F.hasName()) {
      continue;
    }
    std::vector<std::string> Reasons;
    if (BuiltinMarked.count(&F) != 0) {
      Reasons.push_back("builtin-name");
    }
    if (MallocWrappers.count(&F) != 0) {
      Reasons.push_back("malloc-wrapper");
    }
    if (BoundaryMarked.count(&F) != 0) {
      Reasons.push_back("allocator-boundary");
    }
    if (PolicyNames.count(F.getName().str()) != 0) {
      Reasons.push_back("policy-file");
    }
    if (TR.isSummaryOverridePolymorphic(F)) {
      Reasons.push_back("summary-override");
    }
    if (TR.isLevelPolymorphic(&F)) {
      Reasons.push_back("level-hook");
    }
    // Partial parameter polymorphism: record which positions of this
    // polymorphic function must stay monomorphic (shared across call sites).
    // The spec comes from the function metadata, the policy file field
    // "monomorphic_positions", or the NOTDEC_POLY_MONOMORPHIC override.
    const PolymorphicPositionSpec Positions = resolvePolymorphicPositionSpec(F);
    if (!Reasons.empty() && !Positions.empty()) {
      setPolymorphicPositionSpec(F, Positions);
    }
    Out << "{\"function\":\"" << F.getName() << "\",\"defined\":"
        << (F.isDeclaration() ? "false" : "true");
    // The lift address from the !microsub2.func metadata: the annotation is
    // keyed by address, and on a stripped binary the lifted name of the same
    // function can differ in case or numbering (php8.3), so scoring must be
    // able to join by address.
    if (const llvm::MDNode *MD = F.getMetadata("microsub2.func")) {
      if (MD->getNumOperands() == 1) {
        if (auto *C = llvm::dyn_cast<llvm::ConstantAsMetadata>(MD->getOperand(0))) {
          if (auto *CI = llvm::dyn_cast<llvm::ConstantInt>(C->getValue())) {
            if (CI->getType()->getIntegerBitWidth() <= 64) {
              Out << ",\"address\":" << CI->getZExtValue();
            }
          }
        }
      }
    }
    Out << ",\"poly\":"
        << (Reasons.empty() ? "false" : "true") << ",\"reasons\":[";
    for (std::size_t I = 0; I < Reasons.size(); ++I) {
      Out << (I == 0 ? "" : ",") << "\"" << Reasons[I] << "\"";
    }
    Out << "]";
    if (!Positions.empty()) {
      Out << ",\"monomorphic_positions\":\"" << Positions.str() << "\"";
    }
        auto EvIt = EvidenceByFn.find(&F);
    if (EvIt != EvidenceByFn.end()) {
      const AllocatorBoundaryEvidence &E = *EvIt->second;
      Out << ",\"allocator_boundary\":{\"score\":" << E.AllocatorScore
          << ",\"confidence\":\"" << E.AllocatorConfidence << "\""
          << ",\"direct_call_sites\":" << E.DirectCallSites
          << ",\"distinct_callers\":" << E.DistinctCallers << "}";
    }
    Out << "}\n";
  }
  Out.close();
  llvm::errs() << "Info: polymorphic marking report written to " << ReportPath
               << " (" << Evidence.size() << " allocator-boundary rows)\n";
  return llvm::PreservedAnalyses::all();
}

} // namespace notdec::mlsub
