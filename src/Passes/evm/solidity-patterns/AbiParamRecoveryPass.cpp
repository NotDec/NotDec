#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <notdec-llvm2c/Interface.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

using namespace llvm;

#define DEBUG_TYPE "evm-abi-param-recovery"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumAbiParamEntries,
          "Number of public entries analyzed for ABI parameters");
STATISTIC(NumAbiParamRecordMismatches,
          "Number of entries whose recovered calldata record disagrees with "
          "the per-function calldata.index annotations");
STATISTIC(NumAbiParamRecordOnlyEntries,
          "Number of entries with a recovered calldata record but no "
          "per-function annotation");

namespace {

constexpr llvm::StringLiteral KIND_ABI_PARAM_RECORD(
    "notdec.solidity.abi_param.record");
constexpr llvm::StringLiteral KIND_ABI_PARAM_ANNOTATED(
    "notdec.solidity.abi_param.annotated");
constexpr llvm::StringLiteral KIND_ABI_PARAM_MISMATCH(
    "notdec.solidity.abi_param.mismatch");

// Backend-interface annotations only: this pass measures and reports, it does
// not rewrite, so it deliberately avoids addStringMetadata() and its rewrite
// marker calls.
void setStringMetadata(LLVMContext &Ctx, Function &F, llvm::StringRef Kind,
                       llvm::StringRef Value) {
  F.setMetadata(Kind, MDNode::get(Ctx, MDString::get(Ctx, Value)));
}

std::optional<std::string> getStringMetadataValue(const Instruction &I,
                                                  llvm::StringRef Kind) {
  const MDNode *MD = I.getMetadata(Kind);
  if (MD == nullptr || MD->getNumOperands() != 1) {
    return std::nullopt;
  }
  const auto *Value = dyn_cast<MDString>(MD->getOperand(0));
  if (Value == nullptr) {
    return std::nullopt;
  }
  return Value->getString().str();
}

std::optional<unsigned> parseUnsignedDecimal(llvm::StringRef Text) {
  if (Text.empty()) {
    return std::nullopt;
  }
  unsigned Value = 0;
  for (char C : Text) {
    if (C < '0' || C > '9') {
      return std::nullopt;
    }
    Value = Value * 10 + static_cast<unsigned>(C - '0');
  }
  return Value;
}

// The calldata runtime pointer is the second formal of an EVM entry.
Argument *getCalldataArgument(Function &F) {
  if (F.arg_size() < 2) {
    return nullptr;
  }
  auto It = F.arg_begin();
  ++It;
  Argument &Arg = *It;
  return Arg.getType()->isPointerTy() ? &Arg : nullptr;
}

// ABI head field count of the recovered calldata record: the fields at byte
// offset 4 + 32*i.  MLsub currently unifies the calldata arguments of all
// entries (they flow into the same decoder helpers), so this is a module-wide
// upper bound, not yet a per-function parameter count; a missing record is
// reported as 0 instead of being guessed.
unsigned abiHeadFieldCount(const HTypeBufferView &View) {
  if (View.Record == nullptr) {
    return 0;
  }
  unsigned Count = 0;
  for (const ast::FieldDecl &Field : View.Record->getFields()) {
    if (Field.isPadding) {
      continue;
    }
    const OffsetTy Start = Field.R.Start;
    if (Start < 4 || (Start - 4) % 32 != 0) {
      continue;
    }
    Count = std::max(Count, static_cast<unsigned>((Start - 4) / 32) + 1);
  }
  return Count;
}

// Per-function parameter count derived from the pre-TR backend-interface
// annotations: constant-offset calldata loads and decoder helper call sites
// both record the ABI argument index they feed.
unsigned annotatedParamCount(Function &F) {
  unsigned Count = 0;
  for (Instruction &I : instructions(F)) {
    std::optional<std::string> Text =
        getStringMetadataValue(I, KIND_SOLIDITY_CALLDATA_ARG_INDEX);
    if (!Text.has_value()) {
      continue;
    }
    std::optional<unsigned> Index = parseUnsignedDecimal(*Text);
    if (!Index.has_value()) {
      continue;
    }
    Count = std::max(Count, *Index + 1);
  }
  return Count;
}

} // namespace

// Post-type-recovery ABI parameter identification.  The calldata pointer's
// recovered record describes the module's ABI head layout; this pass compares it
// with the per-function annotations and records both numbers on the function so
// the pattern suite can track the gap.  Identification that is usable for
// parameter signatures has to be per-function (record layout + the offsets a
// function actually uses); that refinement is measured against this metadata.
PreservedAnalyses AbiParamRecoveryPass::run(Module &M, ModuleAnalysisManager &MAM) {
  std::unique_ptr<mlsub::MLsubRecovery::Result> &HighTypes = TR.getResult(M, MAM);
  if (HighTypes == nullptr) {
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  for (Function &F : M) {
    if (F.isDeclaration() || !isPublicEntryFunction(F)) {
      continue;
    }
    Argument *Calldata = getCalldataArgument(F);
    if (Calldata == nullptr) {
      continue;
    }

    const HTypeBufferView View = getHTypeValueBufferView(*HighTypes, Calldata);
    const unsigned RecordCount = abiHeadFieldCount(View);
    const unsigned AnnotatedCount = annotatedParamCount(F);

    LLVMContext &Ctx = F.getContext();
    setStringMetadata(Ctx, F, KIND_ABI_PARAM_RECORD,
                      std::to_string(RecordCount));
    setStringMetadata(Ctx, F, KIND_ABI_PARAM_ANNOTATED,
                      std::to_string(AnnotatedCount));
    ++NumAbiParamEntries;
    if (RecordCount != AnnotatedCount) {
      const std::string Diagnostic = "record=" + std::to_string(RecordCount) +
                                     " annotated=" + std::to_string(AnnotatedCount);
      setStringMetadata(Ctx, F, KIND_ABI_PARAM_MISMATCH, Diagnostic);
      ++NumAbiParamRecordMismatches;
      if (AnnotatedCount == 0 && RecordCount > 0) {
        ++NumAbiParamRecordOnlyEntries;
      }
    }
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
