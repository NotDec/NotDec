#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <notdec-llvm2c/Interface.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
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
// Per-function parameter table built from the post-TR results: the ABI head
// words the entry consumes (from the calldata.index annotations) and how many
// of them already carry an informative recovered type.
constexpr llvm::StringLiteral KIND_ABI_PARAM_PARAMS(
    "notdec.solidity.abi_param.params");
constexpr llvm::StringLiteral KIND_ABI_PARAM_TYPED(
    "notdec.solidity.abi_param.typed");
constexpr llvm::StringLiteral KIND_ABI_PARAM_MISSING(
    "notdec.solidity.abi_param.missing");
constexpr llvm::StringLiteral KIND_ABI_PARAM_EXTRA(
    "notdec.solidity.abi_param.extra");

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

// ABI head indices present in the recovered record (byte offset 4 + 32*i).
void collectAbiHeadIndices(const HTypeBufferView &View,
                           std::set<unsigned> &Out) {
  if (View.Record == nullptr) {
    return;
  }
  for (const ast::FieldDecl &Field : View.Record->getFields()) {
    if (Field.isPadding) {
      continue;
    }
    const OffsetTy Start = Field.R.Start;
    if (Start < 4 || (Start - 4) % 32 != 0) {
      continue;
    }
    Out.insert(static_cast<unsigned>((Start - 4) / 32));
  }
}

std::string formatIndexList(const std::set<unsigned> &Indices) {
  std::string Text;
  for (unsigned Index : Indices) {
    if (!Text.empty()) {
      Text += ",";
    }
    Text += std::to_string(Index);
  }
  return Text;
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
    // Prefer the per-entry calldata view inserted by EvmCalldataAccessPass.
    // The raw %calldata argument's record is the module-wide join of every
    // entry's reads (the dispatcher passes one shared value to all of them).
    Value *CalldataRoot = detail::getEvmCalldataRoot(F);
    if (CalldataRoot == nullptr) {
      continue;
    }

    const HTypeBufferView View =
        getHTypeValueBufferView(*HighTypes, CalldataRoot);
    const unsigned RecordCount = abiHeadFieldCount(View);
    const unsigned AnnotatedCount = annotatedParamCount(F);

    LLVMContext &Ctx = F.getContext();
    setStringMetadata(Ctx, F, KIND_ABI_PARAM_RECORD,
                      std::to_string(RecordCount));
    setStringMetadata(Ctx, F, KIND_ABI_PARAM_ANNOTATED,
                      std::to_string(AnnotatedCount));

    // Per-function parameter table from the TR results: every calldata.index
    // annotation marks one ABI head word this entry consumes; look up its
    // recovered type and report coverage.  params is the number of distinct
    // words the entry consumes (the post-TR replacement for annotated),
    // typed how many of them have an informative type, missing those that
    // do not, and extra the recovered layout words the entry does not
    // consume (a cross-check of the annotation completeness).
    std::map<unsigned, bool> UsedIndices; // ABI index -> informative type?
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
      ast::HType *Ty =
          HighTypes->getDefaultValueType(getExtValuePtr(&I, nullptr));
      const bool Informative =
          Ty != nullptr && !Ty->isTopType() && !Ty->isBottomType();
      auto [It, Inserted] = UsedIndices.emplace(*Index, Informative);
      if (!Inserted && Informative) {
        It->second = true;
      }
    }

    // The recovered layout is the second type source: when the per-entry
    // record has a field at 4+32*i, its field type also types the word (the
    // consumed value itself can stay top/bottom, e.g. for a decoded word).
    if (View.Record != nullptr) {
      for (const ast::FieldDecl &Field : View.Record->getFields()) {
        if (Field.isPadding || Field.Type == nullptr) {
          continue;
        }
        const OffsetTy Start = Field.R.Start;
        if (Start < 4 || (Start - 4) % 32 != 0) {
          continue;
        }
        const auto Index = static_cast<unsigned>((Start - 4) / 32);
        auto It = UsedIndices.find(Index);
        if (It == UsedIndices.end()) {
          continue;
        }
        const bool Informative =
            !Field.Type->isTopType() && !Field.Type->isBottomType();
        It->second = It->second || Informative;
      }
    }

    std::set<unsigned> MissingIndices;
    unsigned TypedCount = 0;
    for (const auto &[Index, Informative] : UsedIndices) {
      if (Informative) {
        ++TypedCount;
      } else {
        MissingIndices.insert(Index);
      }
    }

    std::set<unsigned> RecordIndices;
    collectAbiHeadIndices(View, RecordIndices);
    std::set<unsigned> ExtraIndices;
    for (unsigned Index : RecordIndices) {
      if (UsedIndices.count(Index) == 0) {
        ExtraIndices.insert(Index);
      }
    }

    setStringMetadata(Ctx, F, KIND_ABI_PARAM_PARAMS,
                      std::to_string(UsedIndices.size()));
    setStringMetadata(Ctx, F, KIND_ABI_PARAM_TYPED,
                      std::to_string(TypedCount));
    if (!MissingIndices.empty()) {
      setStringMetadata(Ctx, F, KIND_ABI_PARAM_MISSING,
                        formatIndexList(MissingIndices));
    }
    if (!ExtraIndices.empty()) {
      setStringMetadata(Ctx, F, KIND_ABI_PARAM_EXTRA,
                        formatIndexList(ExtraIndices));
    }

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
