
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Analysis/ValueTracking.h>
#include "Passes/evm/SolidityPatternUtils.h"
#include "notdec/TypeRecovery/mlsub/MLsubGenerator.h"
#include "Utils/CallGraphDotInfo.h"
#include "binarysub/binarysub-core.h"
#include "binarysub/binarysub-primitive-semantics.h"
#include "binarysub/binarysub.h"
#include "binarysub/HType.h"
#include "notdec/TypeRecovery/mlsub/HTypeDebug.h"
#include "notdec/TypeRecovery/mlsub/HTypeNormalize.h"
#include "notdec/TypeRecovery/mlsub/AllocatorBoundaryAnalysis.h"
#include "notdec/TypeRecovery/mlsub/PolymorphicMarking.h"
#include "notdec/TypeRecovery/mlsub/Metadata.h"
#include "notdec/TypeRecovery/mlsub/PolymorphicPositions.h"
#include "notdec/TypeRecovery/mlsub/TypeBuilder.h"
#include "notdec/TypeRecovery/LowTy.h"
#include "notdec/Utils/SingleNodeSCCIterator.h"
#include "notdec/Utils/Utils.h"

#include <cassert>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Operator.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/SHA256.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <sys/resource.h>
#include <tuple>
#include <unistd.h>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "mlsub_generator"

#ifndef NOTDEC_DEFAULT_MLSUB_SUMMARY_OVERRIDE_PATH
#define NOTDEC_DEFAULT_MLSUB_SUMMARY_OVERRIDE_PATH ""
#endif

namespace notdec::mlsub {

static bool isEVMModule(const llvm::Module &M);

// Ablation switch NOTDEC_MEM_PTR_SINGLE=covariant|contravariant: keep only a
// single pointer parameter.  Every memory access then contributes the accessed
// type to that one slot instead of to a separate read or write half, so the
// pointer type carries a single variance (see ptrVariance in
// binarysub/src/binarysub-core.cpp, which propagates the halves accordingly).
// Unset keeps the read/write split, so the default configuration is untouched.
enum class PtrSingleMode { Off, Covariant, Contravariant };

static PtrSingleMode ptrSingleMode() {
  static const PtrSingleMode Mode = [] {
    const char *Value = std::getenv("NOTDEC_MEM_PTR_SINGLE");
    if (Value == nullptr) {
      return PtrSingleMode::Off;
    }
    const std::string Text(Value);
    if (Text == "covariant") {
      return PtrSingleMode::Covariant;
    }
    if (Text == "contravariant") {
      return PtrSingleMode::Contravariant;
    }
    return PtrSingleMode::Off;
  }();
  return Mode;
}

// Store=true for a write access: the split design emits a PtrStore capability,
// the single-parameter modes emit the one slot they keep for both directions.
static binarysub::SimpleType ptrAccessCapability(binarysub::SimpleType To,
                                                 unsigned BitSize,
                                                 bool Store) {
  switch (ptrSingleMode()) {
  case PtrSingleMode::Covariant:
    return binarysub::make_ptr_load(To, BitSize);
  case PtrSingleMode::Contravariant:
    return binarysub::make_ptr_store(To, BitSize);
  case PtrSingleMode::Off:
    break;
  }
  return Store ? binarysub::make_ptr_store(To, BitSize)
               : binarysub::make_ptr_load(To, BitSize);
}

namespace {

static_assert(sizeof(ExtValuePtr) > 2 * sizeof(void *),
              "ExtValuePtr became small enough to revisit inline value handles");

struct PolyPolicyConfig {
  std::set<std::string> PolyFuncs;
  std::map<std::string, unsigned int> LevelOverrides;
  // Partial parameter polymorphism: which positions of a polymorphic function
  // stay monomorphic (shared across call sites).  Populated from the policy
  // field "monomorphic_positions" as {name: "args=0,2;ret=1"}.
  std::map<std::string, PolymorphicPositionSpec> MonomorphicPositions;
  // Import semantics: "ret_alias_of_arg": {"memcpy": 0} means the value a call
  // returns IS the object passed as argument 0 (memcpy/memmove return dest).
  // Needed when the callee has no body: a real body encodes the same fact by
  // returning the result of the import call that receives argument k.
  std::map<std::string, unsigned int> RetAliasOfArg;
  // Import semantics continued: which arguments are pointers to caller-owned
  // objects, and whether the return value is a fresh pointer.  Declarations have
  // no body, so these are the only source of "used as a pointer" evidence.
  std::map<std::string, std::set<unsigned int>> PointerArgs;
  std::set<std::string> PointerRet;
  // Unified policy document (version 2): per-function PNDiff specs, kept in the
  // summary-override shape ({"pndiff": [...]}) so the existing application path
  // is reused unchanged.
  std::map<std::string, llvm::json::Value> PndiffSpecs;
  // "arg_alias": [[0,1]] -- the two argument positions denote objects of the
  // same type (copies).
  std::map<std::string, std::vector<std::pair<unsigned int, unsigned int>>>
      ArgAliases;
};

constexpr llvm::StringLiteral kValueTypesFile = "ValueTypes.txt";
constexpr llvm::StringLiteral kVarOriginsFile = "VarOrigins.txt";
constexpr llvm::StringLiteral kValueHTypesFile = "ValueHTypes.txt";
constexpr llvm::StringLiteral kImportantHTypesFile = "ImportantHTypes.txt";
constexpr llvm::StringLiteral kSelectableValuesFile = "SelectableValues.txt";
constexpr llvm::StringLiteral kPNDiffWarnFile = "PNDiff.warn.txt";
constexpr llvm::StringLiteral kMLsubInputIRFile = "02-mlsub-input.ll";
constexpr llvm::StringLiteral kMLsubInputAnchorFile =
    "02-mlsub-input.anchor.json";
constexpr llvm::StringLiteral kPNDiffAnnotatedFile = "03-pndiff-final.ll";
constexpr llvm::StringLiteral kBinarysubTraceFile = "binarysub-trace.log";
constexpr llvm::StringLiteral kBinarysubTraceEnv = "NOTDEC_BINARYSUB_TRACE";
constexpr llvm::StringLiteral kMallocWrappersFile = "MallocWrappers.txt";
constexpr llvm::StringLiteral kPolymorphicBufferFunctionsFile =
    "PolymorphicBufferFunctions.txt";

constexpr llvm::StringLiteral kCallSlotMergeDecisionsFile =
    "CallSlotMergeDecisions.txt";
constexpr llvm::StringLiteral kLocalSubtypeMergeStatsFile =
    "LocalSubtypeMergeStats.txt";
constexpr llvm::StringLiteral kSimplifyStopAfterGroupsEnv =
    "NOTDEC_SIMPLIFY_STOP_AFTER_GROUPS";

using GenTypesClock = std::chrono::steady_clock;

// Bottom-up is currently serialized, but keep diagnostic state outside
// ConstraintsGenerator so enabling diagnostics cannot perturb its layout.
// Thread-local storage also keeps the context independent if SCC processing
// is parallelized later.
thread_local BottomUpTiming *ActiveBottomUpTimingContext = nullptr;
thread_local bool ActiveBottomUpPostSummaryContext = false;

std::optional<std::size_t> getSimplifyStopAfterGroups() {
  const char *Value = std::getenv(kSimplifyStopAfterGroupsEnv.data());
  if (Value == nullptr || *Value == '\0') {
    return std::nullopt;
  }
  std::uint64_t Parsed = 0;
  if (llvm::StringRef(Value).getAsInteger(10, Parsed) || Parsed == 0 ||
      Parsed > std::numeric_limits<std::size_t>::max()) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(Parsed);
}

struct ProcessCpuSnapshot {
  std::uint64_t TotalUs = 0;
  bool Available = false;
};

std::uint64_t timevalToUs(const timeval &Time) {
  return static_cast<std::uint64_t>(Time.tv_sec) * 1000000 +
         static_cast<std::uint64_t>(Time.tv_usec);
}

ProcessCpuSnapshot currentProcessCpuTime() {
  rusage Usage{};
  if (getrusage(RUSAGE_SELF, &Usage) != 0) {
    return {};
  }
  return {.TotalUs = timevalToUs(Usage.ru_utime) +
                     timevalToUs(Usage.ru_stime),
          .Available = true};
}

GenTypesPhaseTiming finishGenTypesPhase(
    GenTypesClock::time_point WallStart, const ProcessCpuSnapshot &CpuStart) {
  const auto WallUs = std::chrono::duration_cast<std::chrono::microseconds>(
                          GenTypesClock::now() - WallStart)
                          .count();
  if (!CpuStart.Available) {
    return {.WallUs = static_cast<std::uint64_t>(WallUs)};
  }
  const auto CpuEnd = currentProcessCpuTime();
  const bool CpuAvailable =
      CpuEnd.Available && CpuEnd.TotalUs >= CpuStart.TotalUs;
  return {.WallUs = static_cast<std::uint64_t>(WallUs),
          .CpuUs = CpuAvailable ? CpuEnd.TotalUs - CpuStart.TotalUs : 0,
          .CpuAvailable = CpuAvailable};
}

std::string formatAverageCores(const GenTypesPhaseTiming &Timing) {
  if (!Timing.CpuAvailable || Timing.WallUs == 0) {
    return "unavailable";
  }
  std::ostringstream Out;
  Out << std::fixed << std::setprecision(2)
      << static_cast<double>(Timing.CpuUs) /
             static_cast<double>(Timing.WallUs);
  return Out.str();
}

void printGenTypesTimingFields(const GenTypesPhaseTiming &Timing) {
  llvm::errs() << " wall_ms=" << Timing.WallUs / 1000 << " cpu_ms=";
  if (!Timing.CpuAvailable) {
    llvm::errs() << "unavailable avg_cores=unavailable";
    return;
  }
  llvm::errs() << Timing.CpuUs / 1000
               << " avg_cores=" << formatAverageCores(Timing);
}

long currentResidentRssKb() {
  std::ifstream Statm("/proc/self/statm");
  long VirtualPages = 0;
  long ResidentPages = 0;
  if (!(Statm >> VirtualPages >> ResidentPages)) {
    return 0;
  }
  return ResidentPages * (sysconf(_SC_PAGESIZE) / 1024);
}

void addGenTypesPhaseTiming(GenTypesPhaseTiming &Total,
                            const GenTypesPhaseTiming &Current) {
  Total.WallUs += Current.WallUs;
  Total.CpuUs += Current.CpuUs;
  Total.CpuAvailable = Total.CpuAvailable && Current.CpuAvailable;
}

void addGenTypesTiming(GenTypesTiming &Total, const GenTypesTiming &Current) {
  addGenTypesPhaseTiming(Total.Prepare, Current.Prepare);
  addGenTypesPhaseTiming(Total.Bulk, Current.Bulk);
  addGenTypesPhaseTiming(Total.Lower, Current.Lower);
  addGenTypesPhaseTiming(Total.Upper, Current.Upper);
  addGenTypesPhaseTiming(Total.DebugOutput, Current.DebugOutput);
}

struct BottomUpPhaseStart {
  GenTypesClock::time_point Wall;
  ProcessCpuSnapshot Cpu;
};

BottomUpPhaseStart startBottomUpPhase(bool Enabled) {
  return {.Wall = GenTypesClock::now(),
          .Cpu = Enabled ? currentProcessCpuTime() : ProcessCpuSnapshot{}};
}

BottomUpPhaseTiming finishBottomUpPhase(
    const BottomUpPhaseStart &Start,
    const binarysub::ConstraintSolverStats &Stats, bool Enabled) {
  if (!Enabled) {
    return {};
  }
  const auto Basic = finishGenTypesPhase(Start.Wall, Start.Cpu);
  return {.WallUs = Basic.WallUs,
          .CpuUs = Basic.CpuUs,
          .CpuAvailable = Basic.CpuAvailable,
          .ResidentRssKb = currentResidentRssKb(),
          .ConstraintStats = Stats};
}

void recordBottomUpSubphase(BottomUpTiming *Timing, BottomUpSubstage Stage,
                            const BottomUpPhaseStart &Start,
                            std::uint64_t WorkItems, bool PostSummary) {
  if (Timing == nullptr) {
    return;
  }
  const auto Basic = finishGenTypesPhase(Start.Wall, Start.Cpu);
  auto &Dst = PostSummary ? Timing->atPostSummary(Stage) : Timing->at(Stage);
  Dst.WallUs += Basic.WallUs;
  Dst.CpuUs += Basic.CpuUs;
  Dst.CpuAvailable = Dst.CpuAvailable && Basic.CpuAvailable;
  Dst.WorkItems += WorkItems;
}

void addConstraintSolverStats(binarysub::ConstraintSolverStats &Total,
                              const binarysub::ConstraintSolverStats &Current) {
  Total.constrainCalls += Current.constrainCalls;
  Total.mergeVariableCalls += Current.mergeVariableCalls;
  Total.constraintTasksEnqueued += Current.constraintTasksEnqueued;
  Total.constraintTasksPopped += Current.constraintTasksPopped;
  Total.constraintTasksProcessed += Current.constraintTasksProcessed;
  Total.mergeTasksEnqueued += Current.mergeTasksEnqueued;
  Total.mergeTasksPopped += Current.mergeTasksPopped;
  Total.mergeTasksProcessed += Current.mergeTasksProcessed;
  Total.upperBoundsAdded += Current.upperBoundsAdded;
  Total.lowerBoundsAdded += Current.lowerBoundsAdded;
  Total.directVariableEdgeFastPathHits +=
      Current.directVariableEdgeFastPathHits;
  Total.maxConstraintWorklistDepth =
      std::max(Total.maxConstraintWorklistDepth,
               Current.maxConstraintWorklistDepth);
  Total.maxMergeWorklistDepth =
      std::max(Total.maxMergeWorklistDepth, Current.maxMergeWorklistDepth);
}

void addBottomUpTiming(BottomUpTiming &Total, const BottomUpTiming &Current) {
  for (std::size_t I = 0; I < Total.Phases.size(); ++I) {
    auto &Dst = Total.Phases[I];
    const auto &Src = Current.Phases[I];
    Dst.WallUs += Src.WallUs;
    Dst.CpuUs += Src.CpuUs;
    Dst.CpuAvailable = Dst.CpuAvailable && Src.CpuAvailable;
    Dst.ResidentRssKb = std::max(Dst.ResidentRssKb, Src.ResidentRssKb);
    addConstraintSolverStats(Dst.ConstraintStats, Src.ConstraintStats);
  }
  for (std::size_t I = 0; I < Total.Subphases.size(); ++I) {
    auto &Dst = Total.Subphases[I];
    const auto &Src = Current.Subphases[I];
    Dst.WallUs += Src.WallUs;
    Dst.CpuUs += Src.CpuUs;
    Dst.CpuAvailable = Dst.CpuAvailable && Src.CpuAvailable;
    Dst.WorkItems += Src.WorkItems;
  }
  for (std::size_t I = 0; I < Total.PostSummarySubphases.size(); ++I) {
    auto &Dst = Total.PostSummarySubphases[I];
    const auto &Src = Current.PostSummarySubphases[I];
    Dst.WallUs += Src.WallUs;
    Dst.CpuUs += Src.CpuUs;
    Dst.CpuAvailable = Dst.CpuAvailable && Src.CpuAvailable;
    Dst.WorkItems += Src.WorkItems;
  }
}

std::string formatBottomUpAverageCores(const BottomUpPhaseTiming &Timing) {
  if (!Timing.CpuAvailable || Timing.WallUs == 0) {
    return "unavailable";
  }
  std::ostringstream Out;
  Out << std::fixed << std::setprecision(2)
      << static_cast<double>(Timing.CpuUs) /
             static_cast<double>(Timing.WallUs);
  return Out.str();
}

void printBottomUpTimingFields(const BottomUpPhaseTiming &Timing) {
  const auto &Stats = Timing.ConstraintStats;
  llvm::errs() << " wall_ms=" << Timing.WallUs / 1000 << " cpu_ms=";
  if (Timing.CpuAvailable) {
    llvm::errs() << Timing.CpuUs / 1000
                 << " avg_cores=" << formatBottomUpAverageCores(Timing);
  } else {
    llvm::errs() << "unavailable avg_cores=unavailable";
  }
  llvm::errs() << " rss_kb=" << Timing.ResidentRssKb
               << " constrain_calls=" << Stats.constrainCalls
               << " merge_calls=" << Stats.mergeVariableCalls
               << " constraint_enqueued=" << Stats.constraintTasksEnqueued
               << " constraint_popped=" << Stats.constraintTasksPopped
               << " constraint_processed=" << Stats.constraintTasksProcessed
               << " merge_enqueued=" << Stats.mergeTasksEnqueued
               << " merge_popped=" << Stats.mergeTasksPopped
               << " merge_processed=" << Stats.mergeTasksProcessed
               << " upper_added=" << Stats.upperBoundsAdded
               << " lower_added=" << Stats.lowerBoundsAdded
               << " direct_edge_fastpath="
               << Stats.directVariableEdgeFastPathHits
               << " max_constraint_depth="
               << Stats.maxConstraintWorklistDepth
               << " max_merge_depth=" << Stats.maxMergeWorklistDepth;
}

void printBottomUpSubphaseTimingFields(
    const BottomUpSubphaseTiming &Timing) {
  llvm::errs() << " wall_ms=" << Timing.WallUs / 1000 << " cpu_ms=";
  if (Timing.CpuAvailable) {
    llvm::errs() << Timing.CpuUs / 1000 << " avg_cores=";
    if (Timing.WallUs == 0) {
      llvm::errs() << "unavailable";
    } else {
      std::ostringstream Average;
      Average << std::fixed << std::setprecision(2)
              << static_cast<double>(Timing.CpuUs) /
                     static_cast<double>(Timing.WallUs);
      llvm::errs() << Average.str();
    }
  } else {
    llvm::errs() << "unavailable avg_cores=unavailable";
  }
  llvm::errs() << " work_items=" << Timing.WorkItems;
}

bool hasBottomUpSubphaseTiming(const BottomUpSubphaseTiming &Timing) {
  return Timing.WallUs != 0 || Timing.CpuUs != 0 || Timing.WorkItems != 0;
}

// Master switch for every external polymorphic-function input: the builtin
// name list below and the NOTDEC_POLY_FUNCS policy file.  Off by default, so a
// benchmark run never depends on a hand-curated name list; the general
// recognition strategy is still being developed.  NOTDEC_POLYMORPHIC_FUNCTIONS=1
// turns the marking on.
bool polymorphicFunctionMarkingEnabled() {
  static const bool Enabled = [] {
    const char *Value = std::getenv("NOTDEC_POLYMORPHIC_FUNCTIONS");
    return Value != nullptr && std::string_view(Value) == "1";
  }();
  return Enabled;
}

bool isBuiltinPolymorphicBufferFunctionName(llvm::StringRef Name) {
  if (!polymorphicFunctionMarkingEnabled()) {
    return false;
  }
  // Benchmark knob: drop individual builtin names again (comma separated,
  // compared with leading underscores stripped like the list below).  The
  // libc buffer/IO entries (memcpy/memset/fwrite/read/...) are called from
  // hundreds of sites, so marking them polymorphic instantiates their
  // signature at every call site and dominates the run time; a container-only
  // name list is what "annotate by name" should mean for those runs.
  static const std::set<std::string> Excluded = [] {
    std::set<std::string> Out;
    const char *Raw = std::getenv("NOTDEC_POLY_NAME_EXCLUDE");
    if (Raw == nullptr || *Raw == '\0') {
      return Out;
    }
    llvm::SmallVector<llvm::StringRef, 16> Entries;
    llvm::StringRef(Raw).split(Entries, ',');
    for (llvm::StringRef Entry : Entries) {
      Entry = Entry.trim();
      while (!Entry.empty() && Entry.front() == '_') {
        Entry = Entry.drop_front();
      }
      if (!Entry.empty()) {
        Out.insert(Entry.str());
      }
    }
    return Out;
  }();
  static constexpr llvm::StringLiteral Names[] = {
      "read",          "write",          "pread",          "pwrite",
      "pread64",       "pwrite64",       "fread",          "fwrite",
      "fgets",         "fputs",          "gets",           "getline",
      "getdelim",      "recv",           "recvfrom",       "send",
      "sendto",        "memcpy",         "memmove",        "memset",
      "bzero",         "strcpy",         "strncpy",        "strcat",
      "strncat",       "sprintf",        "snprintf",       "vsprintf",
      "vsnprintf",     "__memcpy_chk",   "__memmove_chk",  "__memset_chk",
      "__strcpy_chk",  "__strncpy_chk",  "__strcat_chk",   "__strncat_chk",
      "__sprintf_chk", "__snprintf_chk", "__vsprintf_chk", "__vsnprintf_chk",
      // FFmpeg allocators, FIFO transfers, and qsort are generic over
      // caller-owned element types. The logging and option APIs likewise take
      // unrelated object layouts through one void* context parameter. One
      // declaration must not connect all callsites.
      "av_calloc", "av_mallocz", "av_malloc_array", "av_realloc_array",
      "av_fast_malloc", "av_fifo_read", "av_fifo_write", "av_log",
      "av_opt_set", "av_opt_set_int", "av_opt_set_bin", "qsort",
      // redis zmalloc family: generic allocators whose returned layout is
      // decided by each callsite. The automatic malloc-wrapper detection
      // rejects them because malloc results flow through malloc_usable_size
      // and the used_memory atomic counter. Marking them polymorphic
      // instantiates each callsite independently (same role as av_calloc).
      "zmalloc", "zcalloc", "zcalloc_num", "zrealloc", "zstrdup", "zfree",
      "ztrymalloc", "ztrycalloc", "ztryrealloc", "zmalloc_usable",
      "zcalloc_usable", "zrealloc_usable", "zfree_usable",
      // gnulib generic containers/allocators that are reused across element
      // types (see the poly-functions dataset): the hash-table family, the
      // obstack allocator, the scratch buffer, and the token reader.  The
      // hash functions take void* keys/entries supplied by each caller;
      // marking them instantiates each call site independently.
      "hash_initialize", "hash_insert", "hash_insert_if_absent",
      "hash_lookup", "hash_remove", "hash_delete", "hash_free",
      "hash_rehash", "hash_find_entry", "transfer_entries",
      "hash_clear", "hash_reset_tuning", "hash_get_first",
      "hash_get_next", "hash_get_entries", "hash_do_for_each",
      "obstack_init", "obstack_alloc", "obstack_free", "obstack_finish",
      "obstack_blank", "obstack_grow", "obstack_1grow", "obstack_room",
      "gl_scratch_buffer_grow", "gl_scratch_buffer_grow_preserve",
      "scratch_buffer_init", "scratch_buffer_free",
      "readtokens", "readtokens0", "readtokens0_init", "readtokens0_free",
      // ffmpeg option/tree/fifo APIs generic over the caller's context
      "av_opt_get", "av_opt_ptr", "av_opt_copy", "av_opt_find",
      "av_tree_insert", "av_tree_find", "av_tree_enumerate",
      "av_tree_destroy", "av_fifo_alloc", "av_fifo_alloc_array",
      // php/zend container API (the generic side lives in the php binary;
      // the loadable modules only import it)
      "zend_hash_init", "zend_hash_add", "zend_hash_update",
      "zend_hash_find", "zend_hash_del", "zend_hash_destroy",
      "zend_hash_apply", "zend_hash_clean"};
  // Recovered ELF symbol names keep their leading underscore (IDA reports
  // "_memcpy", "___snprintf_chk"), so compare with underscores stripped.
  auto StripUnderscores = [](llvm::StringRef S) {
    while (!S.empty() && S.front() == '_') {
      S = S.drop_front();
    }
    return S;
  };
  llvm::StringRef Normalized = StripUnderscores(Name);
  if (Excluded.count(Normalized.str()) != 0) {
    return false;
  }
  for (auto Candidate : Names) {
    if (Candidate == Name || StripUnderscores(Candidate) == Normalized) {
      return true;
    }
  }
  return false;
}

bool envFlagEnabled(llvm::StringRef Name) {
  auto *Value = std::getenv(Name.data());
  return Value != nullptr && llvm::StringRef(Value) == "1";
}

llvm::Value *getPNDiffLLVMValue(const ExtValuePtr &Val) {
  if (auto *V = std::get_if<llvm::Value *>(&Val)) {
    return *V;
  }
  if (auto *C = std::get_if<UConstant>(&Val)) {
    return C->Val;
  }
  return nullptr;
}

std::optional<int64_t> getPNDiffIntConstantValue(const ExtValuePtr &Val) {
  auto *Value = getPNDiffLLVMValue(Val);
  auto *CI = llvm::dyn_cast_or_null<llvm::ConstantInt>(Value);
  if (CI == nullptr || CI->getValue().getSignificantBits() > 64) {
    return std::nullopt;
  }
  return CI->getValue().getSExtValue();
}

OffsetRange getUnknownPNDiffOffsetRange() {
  return OffsetRange{.offset = 0, .access = {{1, 0}}};
}

bool isUnknownPNDiffOffsetRange(const OffsetRange &Range) {
  return Range == getUnknownPNDiffOffsetRange();
}

std::optional<uint64_t> parseConstantFieldOffset(llvm::StringRef FieldName) {
  if (FieldName.empty() || FieldName.contains("+") ||
      FieldName.contains("i")) {
    return std::nullopt;
  }
  if (FieldName.starts_with("@")) {
    FieldName = FieldName.drop_front();
  }
  uint64_t Offset = 0;
  if (FieldName.getAsInteger(10, Offset)) {
    return std::nullopt;
  }
  return Offset;
}

std::optional<OffsetRange> matchPNDiffOffsetRange(llvm::Value *I) {
  assert(I->getType()->isIntegerTy());
  if (auto *CI = llvm::dyn_cast<llvm::ConstantInt>(I)) {
    auto Constant = getPNDiffIntConstantValue(ExtValuePtr{CI});
    if (!Constant) {
      return std::nullopt;
    }
    return OffsetRange{.offset = *Constant};
  }
  if (!llvm::isa<llvm::BinaryOperator>(I)) {
    return std::nullopt;
  }

  auto *BinOp = llvm::cast<llvm::BinaryOperator>(I);
  auto Opcode = BinOp->getOpcode();
  if (Opcode != llvm::Instruction::Add && Opcode != llvm::Instruction::Mul &&
      Opcode != llvm::Instruction::Shl) {
    return std::nullopt;
  }

  auto *Src1 = BinOp->getOperand(0);
  auto *Src2 = BinOp->getOperand(1);
  if (llvm::isa<llvm::ConstantInt>(Src1) && llvm::isa<llvm::ConstantInt>(Src2)) {
    assert(false && "Constant at both sides. Run Optimization first!");
  }
  if (llvm::isa<llvm::ConstantInt>(Src1) && !llvm::isa<llvm::ConstantInt>(Src2) &&
      Opcode != llvm::Instruction::Shl) {
    // X64 源码级 IR 分支不跑 InstCombine，-O0 下 add/mul 的常量操作数
    // 保持源码顺序可能在左侧；add/mul 可交换，swap 语义不变（shl 不
    // 可交换，保持原样走 nullopt，不在这里 swap）。
    std::swap(Src1, Src2);
  }

  if (Opcode == llvm::Instruction::Add) {
    auto Result =
        matchPNDiffOffsetRange(Src1).value_or(getUnknownPNDiffOffsetRange()) +
        matchPNDiffOffsetRange(Src2).value_or(getUnknownPNDiffOffsetRange());
    if (isUnknownPNDiffOffsetRange(Result)) {
      return std::nullopt;
    }
    return Result;
  }
  if (Opcode == llvm::Instruction::Mul) {
    auto Result =
        matchPNDiffOffsetRange(Src1).value_or(getUnknownPNDiffOffsetRange()) *
        matchPNDiffOffsetRange(Src2).value_or(getUnknownPNDiffOffsetRange());
    if (isUnknownPNDiffOffsetRange(Result)) {
      return std::nullopt;
    }
    return Result;
  }
  if (Opcode == llvm::Instruction::Shl && llvm::isa<llvm::ConstantInt>(Src2)) {
    auto Shift = getPNDiffIntConstantValue(ExtValuePtr{Src2});
    if (!Shift || *Shift < 0 || *Shift >= 63) {
      return std::nullopt;
    }
    auto Result =
        matchPNDiffOffsetRange(Src1).value_or(getUnknownPNDiffOffsetRange()) *
        (int64_t{1} << *Shift);
    if (isUnknownPNDiffOffsetRange(Result)) {
      return std::nullopt;
    }
    return Result;
  }
  return std::nullopt;
}

std::optional<OffsetRange> matchPNDiffOffsetRangeNoNegativeAccess(
    const ExtValuePtr &Val) {
  auto *Value = getPNDiffLLVMValue(Val);
  if (Value == nullptr || !Value->getType()->isIntegerTy()) {
    return std::nullopt;
  }
  auto R = matchPNDiffOffsetRange(Value);
  if (!R) {
    return std::nullopt;
  }
  R->access.erase(std::remove_if(R->access.begin(), R->access.end(),
                                 [](ArrayOffset Val) { return Val.Size < 0; }),
                  R->access.end());
  return R;
}

std::string formatPNDiffInstruction(void *Inst) {
  auto *I = static_cast<llvm::Instruction *>(Inst);
  if (I == nullptr) {
    return "<null-inst>";
  }
  std::string S;
  llvm::raw_string_ostream OS(S);
  I->print(OS);
  return OS.str();
}

bool isFunctionPointerTable(const llvm::GlobalVariable &GV) {
  auto *TableTy = GV.getValueType();
  if (!TableTy->isArrayTy()) {
    return false;
  }

  auto *ElemTy = TableTy->getArrayElementType();
  if (!ElemTy->isPointerTy()) {
    return false;
  }

  if (!GV.hasInitializer()) {
    return false;
  }
  auto *Init = GV.getInitializer();
  if (Init == nullptr) {
    return false;
  }

  // Opaque pointer mode no longer exposes the pointee from `ptr`, so only
  // trust tables whose initializer entries are visibly functions or casts of
  // functions.
  for (const auto &Op : Init->operands()) {
    auto *Entry = dyn_cast<Constant>(Op.get());
    if (Entry == nullptr) {
      continue;
    }
    if (isa<Function>(Entry)) {
      return true;
    }
    auto *CE = dyn_cast<ConstantExpr>(Entry);
    if (CE == nullptr || !CE->isCast()) {
      continue;
    }
    if (isa<Function>(CE->getOperand(0))) {
      return true;
    }
  }

  return false;
}

std::optional<int64_t> getSigned64ConstantAddress(const llvm::ConstantInt &CI) {
  const llvm::APInt &Value = CI.getValue();
  if (!Value.isSignedIntN(64)) {
    llvm::errs() << "Warning: skip constant memory field outside int64 range: "
                 << CI << "\n";
    return std::nullopt;
  }
  return Value.getSExtValue();
}

std::optional<int64_t> getSigned64APInt(const llvm::APInt &Value) {
  if (!Value.isSignedIntN(64)) {
    return std::nullopt;
  }
  return Value.getSExtValue();
}

std::optional<OffsetRange> getGEPOffsetRange(const llvm::GEPOperator &GEP,
                                             const llvm::DataLayout &DL) {
  if (GEP.getPointerOperandType()->isVectorTy() || GEP.getType()->isVectorTy()) {
    return std::nullopt;
  }

  auto BitWidth = DL.getIndexTypeSizeInBits(GEP.getPointerOperandType());
  llvm::APInt ConstantOffset(BitWidth, 0, true);
  if (GEP.accumulateConstantOffset(DL, ConstantOffset)) {
    auto Offset = getSigned64APInt(ConstantOffset);
    if (!Offset) {
      return std::nullopt;
    }
    return OffsetRange{.offset = *Offset};
  }

  llvm::SmallMapVector<llvm::Value *, llvm::APInt, 4> VariableOffsets;
  ConstantOffset = llvm::APInt(BitWidth, 0, true);
  if (!GEP.collectOffset(DL, BitWidth, VariableOffsets, ConstantOffset)) {
    return std::nullopt;
  }

  auto Offset = getSigned64APInt(ConstantOffset);
  if (!Offset) {
    return std::nullopt;
  }

  OffsetRange Range{.offset = *Offset};
  for (const auto &Entry : VariableOffsets) {
    auto Scale = getSigned64APInt(Entry.second);
    if (!Scale) {
      return std::nullopt;
    }
    if (*Scale == 0) {
      continue;
    }
    // OffsetRange can model array-like dynamic steps, but later record sizing
    // assumes non-negative access sizes. Keep negative dynamic addressing out
    // of field constraints until the type builder has a representation for it.
    if (*Scale < 0) {
      return std::nullopt;
    }
    Range.access.emplace_back(*Scale);
  }
  return Range;
}

bool lowerGEPOperatorAsPtrAdd(ConstraintsGenerator &CG,
                              const llvm::GEPOperator &GEP,
                              llvm::User &GEPUser, const llvm::DataLayout &DL,
                              ExtValuePtr Result) {
  if (GEP.getPointerOperand()->getName().starts_with("table_")) {
    return false;
  }

  auto *BaseValue = const_cast<llvm::Value *>(GEP.getPointerOperand());
  auto Base = getExtValuePtr(BaseValue, &GEPUser,
                             llvm::GEPOperator::getPointerOperandIndex());
  if (GEP.hasAllZeroIndices()) {
    CG.getOrInsertNode(Base);
    CG.addRemapType(Result, Base);
    return true;
  }

  auto Offset = getGEPOffsetRange(GEP, DL);
  if (!Offset || Offset->hasNegativeBaseOffset()) {
    return false;
  }

  CG.setAsPtrAdd(Base, Result, std::move(*Offset),
                 "gep:" + toStableString(Base) + Offset->str() + "->" +
                     toStableString(Result));
  return true;
}

bool lowerConstantExprGEPAddress(ConstraintsGenerator &CG, llvm::Value *Ptr,
                                 llvm::User &User, long OpInd,
                                 const llvm::DataLayout &DL) {
  auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(Ptr);
  if (CE == nullptr) {
    return false;
  }

  if (CE->getOpcode() == llvm::Instruction::BitCast) {
    auto *InnerCE = llvm::dyn_cast<llvm::ConstantExpr>(CE->getOperand(0));
    if (InnerCE == nullptr ||
        InnerCE->getOpcode() != llvm::Instruction::GetElementPtr) {
      return false;
    }

    auto InnerResult = getExtValuePtr(InnerCE, CE, 0);
    if (!lowerGEPOperatorAsPtrAdd(CG, llvm::cast<llvm::GEPOperator>(*InnerCE),
                                  *InnerCE, DL, InnerResult)) {
      return false;
    }

    auto BitCastResult = getExtValuePtr(CE, &User, OpInd);
    CG.addRemapType(BitCastResult, InnerResult);
    return true;
  }

  if (CE->getOpcode() != llvm::Instruction::GetElementPtr) {
    return false;
  }

  auto Result = getExtValuePtr(CE, &User, OpInd);
  return lowerGEPOperatorAsPtrAdd(CG, llvm::cast<llvm::GEPOperator>(*CE), *CE,
                                  DL, Result);
}

[[noreturn]] void failSignatureOverride(llvm::StringRef Path,
                                        llvm::StringRef Message) {
  llvm::errs() << "Error: invalid MLsub override";
  if (!Path.empty()) {
    llvm::errs() << " at " << Path;
  }
  llvm::errs() << ": " << Message << "\n";
  std::abort();
}

[[noreturn]] void failExtraConstraints(llvm::StringRef Path,
                                       llvm::StringRef Message) {
  llvm::errs() << "Error: invalid MLsub extra constraints";
  if (!Path.empty()) {
    llvm::errs() << " at " << Path;
  }
  llvm::errs() << ": " << Message << "\n";
  std::abort();
}

[[noreturn]] void failPolyPolicy(llvm::StringRef Path,
                                 llvm::StringRef Message) {
  llvm::errs() << "Error: invalid NOTDEC_POLY_FUNCS policy";
  if (!Path.empty()) {
    llvm::errs() << " at " << Path;
  }
  llvm::errs() << ": " << Message << "\n";
  std::abort();
}

[[noreturn]] void failPNDiffPolicy(llvm::StringRef Path,
                                   llvm::StringRef Message) {
  llvm::errs() << "Error: invalid NOTDEC_PNDIFF_POLICY_OVERRIDE policy";
  if (!Path.empty()) {
    llvm::errs() << " at " << Path;
  }
  llvm::errs() << ": " << Message << "\n";
  std::abort();
}

const llvm::json::Object &requireObject(const llvm::json::Value &Value,
                                        llvm::StringRef Path) {
  if (const auto *Obj = Value.getAsObject()) {
    return *Obj;
  }
  failSignatureOverride(Path, "expected object");
}

const llvm::json::Array &requireArray(const llvm::json::Value &Value,
                                      llvm::StringRef Path) {
  if (const auto *Arr = Value.getAsArray()) {
    return *Arr;
  }
  failSignatureOverride(Path, "expected array");
}

llvm::StringRef requireString(const llvm::json::Object &Obj,
                              llvm::StringRef Key, llvm::StringRef Path) {
  if (auto Value = Obj.getString(Key)) {
    return *Value;
  }
  failSignatureOverride(
      Path,
      ("missing or invalid string field '" + Key.str() + "'").c_str());
}

int64_t requireInteger(const llvm::json::Object &Obj, llvm::StringRef Key,
                       llvm::StringRef Path) {
  if (auto Value = Obj.getInteger(Key)) {
    return *Value;
  }
  failSignatureOverride(
      Path,
      ("missing or invalid integer field '" + Key.str() + "'").c_str());
}

std::string appendJSONPath(llvm::StringRef Base, llvm::StringRef Suffix) {
  std::string Result = Base.str();
  if (!Result.empty()) {
    Result += ".";
  }
  Result += Suffix.str();
  return Result;
}

std::string appendJSONIndexPath(llvm::StringRef Base, size_t Index) {
  return Base.str() + "[" + std::to_string(Index) + "]";
}

PolyPolicyConfig loadPolyPolicyConfig() {
  PolyPolicyConfig Config;
  auto *PolicyFile = std::getenv("NOTDEC_POLY_FUNCS");
  if (PolicyFile == nullptr) {
    return Config;
  }
  if (!polymorphicFunctionMarkingEnabled()) {
    llvm::errs() << "Info: ignoring NOTDEC_POLY_FUNCS=" << PolicyFile
                 << " (polymorphic function marking is off; set "
                    "NOTDEC_POLYMORPHIC_FUNCTIONS=1 to enable)\n";
    return Config;
  }

  auto Content = readFileToString(PolicyFile);
  auto ValE = json::parse(Content);
  if (!ValE) {
    failPolyPolicy(PolicyFile, "JSON parse failed");
  }

  auto *Obj = ValE->getAsObject();
  if (Obj == nullptr) {
    failPolyPolicy(PolicyFile, "expected top-level object");
  }

  // Unified documents (version 2) list the polymorphic functions inside the
  // per-function "functions" object ("polymorphic": true) instead of the legacy
  // "poly_funcs" array, so that array is only required in the legacy shape.
  const bool IsUnifiedDoc = Obj->getObject("functions") != nullptr;
  auto *PolyArray = Obj->getArray("poly_funcs");
  // Unified documents derive the polymorphic set from the per-function
  // "polymorphic" flags; materialize it into a local array so the legacy loop
  // below stays untouched.
  llvm::json::Array UnifiedPoly;
  if (IsUnifiedDoc && PolyArray == nullptr) {
    if (auto *FuncsObj = Obj->getObject("functions")) {
      for (const auto &Ent : *FuncsObj) {
        const auto *Spec = Ent.second.getAsObject();
        if (Spec != nullptr && Spec->getBoolean("polymorphic").value_or(true)) {
          UnifiedPoly.emplace_back(Ent.first.str());
        }
      }
    }
    PolyArray = &UnifiedPoly;
  }
  if (PolyArray == nullptr && !IsUnifiedDoc) {
    failPolyPolicy(PolicyFile, "missing array field 'poly_funcs'");
  }
  for (size_t Index = 0; Index < PolyArray->size(); ++Index) {
    auto Name = (*PolyArray)[Index].getAsString();
    if (!Name) {
      failPolyPolicy(appendJSONIndexPath("poly_funcs", Index),
                     "expected string");
    }
    Config.PolyFuncs.insert(Name->str());
  }

  if (auto *MonoObj = Obj->getObject("monomorphic_positions")) {
    for (auto &Ent : *MonoObj) {
      std::optional<PolymorphicPositionSpec> Spec;
      if (auto Text = Ent.second.getAsString()) {
        Spec = PolymorphicPositionSpec::parse(*Text);
      } else if (auto *SpecObj = Ent.second.getAsObject()) {
        PolymorphicPositionSpec Parsed;
        if (auto *Args = SpecObj->getArray("args")) {
          for (size_t Index = 0; Index < Args->size(); ++Index) {
            auto Value = (*Args)[Index].getAsInteger();
            if (!Value || *Value < 0) {
              failPolyPolicy(appendJSONPath("monomorphic_positions", Ent.first.str()),
                             "expected a non-negative argument index");
            }
            Parsed.MonomorphicArgs.push_back(static_cast<unsigned>(*Value));
          }
        }
        if (auto Ret = SpecObj->getBoolean("return")) {
          Parsed.MonomorphicReturn = *Ret;
        }
        Spec = Parsed;
      }
      if (!Spec) {
        failPolyPolicy(appendJSONPath("monomorphic_positions", Ent.first.str()),
                       "expected an object {args:[...], return:bool} or a "
                       "string such as args=0,2;ret=1");
      }
      Config.MonomorphicPositions[Ent.first.str()] = *Spec;
    }
  }

  // Unified policy document (version 2): "functions" keyed by IR name, each
  // entry carrying polymorphic / monomorphic_positions / import_semantics /
  // pndiff / level_override.  The legacy forms above stay supported, so old
  // policy files keep working.
  if (auto *FuncsObj = Obj->getObject("functions")) {
    for (const auto &Ent : *FuncsObj) {
      const std::string Name = Ent.first.str();
      const auto *Spec = Ent.second.getAsObject();
      if (Spec == nullptr) {
        continue;
      }
      if (Spec->getBoolean("polymorphic").value_or(true)) {
        Config.PolyFuncs.insert(Name);
      }
      if (auto *Mono = Spec->getObject("monomorphic_positions")) {
        PolymorphicPositionSpec Parsed;
        if (const auto *Args = Mono->getArray("args")) {
          for (const auto &Arg : *Args) {
            if (auto Index = Arg.getAsInteger(); Index && *Index >= 0) {
              Parsed.MonomorphicArgs.push_back(
                  static_cast<unsigned>(*Index));
            }
          }
        }
        if (Mono->getBoolean("ret").value_or(false)) {
          Parsed.MonomorphicReturn = true;
        }
        if (!Parsed.empty()) {
          Config.MonomorphicPositions[Name] = Parsed;
        }
      }
      if (auto *Sem = Spec->getObject("import_semantics")) {
        if (const auto *PtrArgs = Sem->getArray("pointer_args")) {
          for (const auto &Arg : *PtrArgs) {
            if (auto Index = Arg.getAsInteger(); Index && *Index >= 0) {
              Config.PointerArgs[Name].insert(static_cast<unsigned>(*Index));
            }
          }
        }
        if (Sem->getBoolean("pointer_ret").value_or(false)) {
          Config.PointerRet.insert(Name);
        }
        if (auto Alias = Sem->getInteger("ret_alias_of_arg");
            Alias && *Alias >= 0) {
          Config.RetAliasOfArg[Name] = static_cast<unsigned>(*Alias);
        }
        if (const auto *ArgAlias = Sem->getArray("arg_alias")) {
          for (const auto &Pair : *ArgAlias) {
            const auto *Arr = Pair.getAsArray();
            if (Arr == nullptr || Arr->size() != 2) {
              continue;
            }
            auto L = (*Arr)[0].getAsInteger();
            auto R = (*Arr)[1].getAsInteger();
            if (L && R && *L >= 0 && *R >= 0) {
              Config.ArgAliases[Name].emplace_back(
                  static_cast<unsigned>(*L), static_cast<unsigned>(*R));
            }
          }
        }
      }
      if (auto Level = Spec->getInteger("level_override"); Level && *Level >= 0) {
        Config.LevelOverrides[Name] = static_cast<unsigned>(*Level);
      }
      // The v2 function entry already carries "pndiff" at its top level, which
      // is exactly the shape the summary-override consumer expects, so hand the
      // entry itself over (no synthesis needed).
      if (Spec->getArray("pndiff") != nullptr) {
        Config.PndiffSpecs.insert(std::make_pair(Name, Ent.second));
      }
    }
  }
  if (auto *SemObj = Obj->getObject("import_semantics")) {
    for (const auto &Ent : *SemObj) {
      const auto *Spec = Ent.second.getAsObject();
      if (Spec == nullptr) {
        failPolyPolicy(appendJSONPath("import_semantics", Ent.first.str()),
                       "expected an object");
      }
      if (auto Alias = Spec->getInteger("ret_alias_of_arg")) {
        Config.RetAliasOfArg[Ent.first.str()] =
            static_cast<unsigned int>(Alias.value());
      }
      if (const auto *PtrArgs = Spec->getArray("pointer_args")) {
        for (const auto &Arg : *PtrArgs) {
          if (auto Index = Arg.getAsInteger()) {
            Config.PointerArgs[Ent.first.str()].insert(
                static_cast<unsigned int>(*Index));
          }
        }
      }
      if (Spec->getBoolean("pointer_ret").value_or(false)) {
        Config.PointerRet.insert(Ent.first.str());
      }
    }
  }
  if (auto *AliasObj = Obj->getObject("ret_alias_of_arg")) {
    for (const auto &Ent : *AliasObj) {
      auto Index = Ent.second.getAsInteger();
      if (!Index || *Index < 0) {
        failPolyPolicy(appendJSONPath("ret_alias_of_arg", Ent.first.str()),
                       "expected a non-negative argument index");
      }
      Config.RetAliasOfArg[Ent.first.str()] =
          static_cast<unsigned int>(*Index);
    }
  }
  if (auto *LevelObj = Obj->getObject("level_override")) {
    for (auto &Ent : *LevelObj) {
      auto Level = Ent.second.getAsInteger();
      if (!Level || *Level < 0) {
        failPolyPolicy(appendJSONPath("level_override", Ent.first),
                       "expected non-negative integer");
      }
      Config.LevelOverrides[Ent.first.str()] =
          static_cast<unsigned int>(*Level);
    }
  }

  return Config;
}

PNDiffPolicyConfig loadPNDiffPolicyConfig() {
  PNDiffPolicyConfig Config;
  auto *PolicyFile = std::getenv("NOTDEC_PNDIFF_POLICY_OVERRIDE");
  if (PolicyFile == nullptr || std::strlen(PolicyFile) == 0) {
    return Config;
  }

  auto Parsed = llvm::json::parse(readFileToString(PolicyFile));
  if (!Parsed) {
    failPNDiffPolicy(PolicyFile, "JSON parse failed");
  }

  auto *Obj = Parsed->getAsObject();
  if (Obj == nullptr) {
    failPNDiffPolicy(PolicyFile, "expected top-level object");
  }

  if (auto Version = Obj->getInteger("version")) {
    if (*Version != 1) {
      failPNDiffPolicy(PolicyFile, "unsupported version");
    }
  }

  if (auto *IntConstantPolicy = Obj->getObject("int_constant_policy")) {
    if (auto Threshold =
            IntConstantPolicy->getInteger("non_pointer_abs_lt")) {
      Config.nonPointerAbsLt = *Threshold;
    }
    if (auto ExcludeZero = IntConstantPolicy->getBoolean("exclude_zero")) {
      Config.excludeZero = *ExcludeZero;
    }
  }

  return Config;
}

bool hasFunctionTypeOverride(const llvm::json::Object &Spec,
                             llvm::StringRef FuncPath,
                             bool RequireFunctionType) {
  bool HasArgs = Spec.get("args") != nullptr;
  bool HasRet = Spec.get("ret") != nullptr;
  if (HasArgs != HasRet) {
    failSignatureOverride(
        FuncPath, "override must contain both 'args' and 'ret' or neither");
  }
  if (RequireFunctionType && !HasArgs) {
    failSignatureOverride(FuncPath,
                          "override requires both 'args' and 'ret'");
  }
  return HasArgs;
}

constexpr llvm::StringLiteral kBuiltinEVMI256SemanticLattice = R"dot(
digraph evm_i256_semantics {
  graph [base="uint", bits="256", namespace="evm"];

  root [kind="root", display_name="evm_word"];
  integer;
  address;
  storage_key;
  bytes;
  string;

  integer -> root;
  address -> root;
  storage_key -> root;
  bytes -> root;
  string -> bytes;
}
)dot";

std::string getOverrideArchName(const llvm::Module &M) {
  StringRef Triple = M.getTargetTriple().getTriple();
  if (Triple.starts_with("evm")) {
    return "evm";
  }
  if (Triple.starts_with("wasm32") || Triple.starts_with("wasm64")) {
    return "wasm";
  }

  auto Arch = Triple.split('-').first;
  return Arch.empty() ? std::string("unknown") : Arch.str();
}

bool overrideAllowedForModule(const llvm::json::Object &Spec,
                              llvm::StringRef Path, const llvm::Module &M) {
  auto *AllowedValue = Spec.get("allowed_arch");
  if (AllowedValue == nullptr) {
    return true;
  }

  const auto *Allowed = AllowedValue->getAsArray();
  if (Allowed == nullptr) {
    failSignatureOverride(appendJSONPath(Path, "allowed_arch"),
                          "expected array of strings");
  }

  auto CurrentArch = getOverrideArchName(M);
  for (size_t Index = 0; Index < Allowed->size(); ++Index) {
    auto EntryPath =
        appendJSONIndexPath(appendJSONPath(Path, "allowed_arch"), Index);
    auto Arch = (*Allowed)[Index].getAsString();
    if (!Arch) {
      failSignatureOverride(EntryPath, "expected string");
    }
    if (*Arch == CurrentArch) {
      return true;
    }
  }
  return false;
}

binarysub::expected<void, binarysub::Error>
registerBuiltinPrimitiveSemanticLatticesForModule(const llvm::Module &M) {
  if (getOverrideArchName(M) != "evm") {
    return {};
  }

  auto &Registry = binarysub::globalPrimitiveSemanticRegistry();
  if (Registry.findFamily("uint", 256, "evm") != nullptr) {
    return {};
  }

  auto Family = Registry.registerFamilyFromDot(kBuiltinEVMI256SemanticLattice,
                                               "<builtin-evm-i256>");
  if (!Family) {
    return binarysub::make_unexpected(Family.error());
  }
  return {};
}

enum class OverridePNDiffState {
  Ptr,
  Number,
};

llvm::StringRef getOverridePNDiffStateName(OverridePNDiffState State) {
  switch (State) {
  case OverridePNDiffState::Ptr:
    return "ptr";
  case OverridePNDiffState::Number:
    return "number";
  }
  llvm_unreachable("unhandled OverridePNDiffState");
}

std::string formatLLVMType(llvm::Type *Ty) {
  std::string Buffer;
  llvm::raw_string_ostream OS(Buffer);
  if (Ty == nullptr) {
    OS << "<null>";
  } else {
    Ty->print(OS);
  }
  return Buffer;
}

void warnIgnoredPNDiffOverride(llvm::StringRef Kind, llvm::StringRef Path,
                               llvm::StringRef TargetKey,
                               OverridePNDiffState State,
                               const ExtValuePtr &Value,
                               bool *WroteHeader) {
  std::string Message;
  llvm::raw_string_ostream OS(Message);
  OS << "Warning: skip MLsub " << Kind << " pndiff";
  if (!Path.empty()) {
    OS << " at " << Path;
  }
  OS << ": target '" << TargetKey << "' has non-PNDiff LLVM type '"
     << formatLLVMType(getType(Value))
     << "'; requested state '"
     << getOverridePNDiffStateName(State)
     << "' will be ignored\n";
  OS.flush();

  if (WroteHeader != nullptr && notdec::hasWorkDir()) {
    if (!*WroteHeader) {
      notdec::appendWorkDirLog(
          kPNDiffWarnFile,
          "# Override / validation warnings\n\n");
      *WroteHeader = true;
    }
    notdec::appendWorkDirLog(kPNDiffWarnFile, Message);
    return;
  }

  llvm::errs() << Message;
}

bool isPointerSizedNumberProjection(const llvm::Value &Source,
                                    const llvm::Instruction &User,
                                    unsigned PointerSize) {
  auto *SourceTy = llvm::dyn_cast<llvm::IntegerType>(Source.getType());
  auto *UserTy = llvm::dyn_cast<llvm::IntegerType>(User.getType());
  if (SourceTy == nullptr || UserTy == nullptr) {
    return false;
  }

  auto SourceBits = SourceTy->getBitWidth();
  auto UserBits = UserTy->getBitWidth();
  if (UserBits != PointerSize || SourceBits == PointerSize) {
    return false;
  }

  if (SourceBits > PointerSize) {
    return llvm::isa<llvm::TruncInst>(User);
  }
  return llvm::isa<llvm::ZExtInst, llvm::SExtInst>(User);
}

llvm::SmallVector<llvm::Value *, 4>
collectNumberOverrideUserScanSeeds(const ExtValuePtr &Value) {
  llvm::SmallVector<llvm::Value *, 4> Seeds;
  if (auto *V = std::get_if<llvm::Value *>(&Value)) {
    if (*V != nullptr) {
      Seeds.push_back(*V);
    }
    return Seeds;
  }

  auto *Ret = std::get_if<ReturnValue>(&Value);
  if (Ret == nullptr || Ret->Func == nullptr) {
    return Seeds;
  }

  for (auto *User : Ret->Func->users()) {
    auto *Call = llvm::dyn_cast<llvm::CallBase>(User);
    if (Call == nullptr || Call->getCalledFunction() != Ret->Func ||
        Call->getType()->isVoidTy()) {
      continue;
    }
    Seeds.push_back(Call);
  }
  return Seeds;
}

bool applyNumberOverrideToCompatibleCastUsers(ConstraintsGenerator &G,
                                              const ExtValuePtr &Value) {
  bool Applied = false;
  llvm::SmallPtrSet<llvm::Instruction *, 8> Seen;
  auto Seeds = collectNumberOverrideUserScanSeeds(Value);
  for (auto *Seed : Seeds) {
    if (Seed == nullptr || !Seed->getType()->isIntegerTy()) {
      continue;
    }
    for (auto *User : Seed->users()) {
      auto *Inst = llvm::dyn_cast<llvm::Instruction>(User);
      if (Inst == nullptr || !Seen.insert(Inst).second ||
          !isPointerSizedNumberProjection(*Seed, *Inst, G.PointerSize)) {
        continue;
      }
      auto *Node = G.getPNINodeOrNull(Inst);
      if (Node == nullptr || !Node->isPNRelated()) {
        continue;
      }
      Node->setNonPtr();
      Applied = true;
    }
  }
  return Applied;
}

struct ResolvedPNDiffTarget {
  ExtValuePtr Value;
  std::string Key;
};

struct ResolvedConstraintTarget {
  ExtValuePtr Value;
  std::string Key;
};

ResolvedConstraintTarget resolveSummaryConstraintTarget(
    const llvm::json::Object &Obj, llvm::Function &Func,
    llvm::StringRef Path) {
  auto Kind = Obj.getString("kind");
  if (!Kind) {
    failSignatureOverride(Path, "missing or invalid string field 'kind'");
  }
  if (*Kind == "ret") {
    if (Func.getReturnType()->isVoidTy()) {
      failSignatureOverride(Path, "ret target requires non-void function");
    }
    return {
        .Value = ReturnValue{.Func = &Func},
        .Key = "ret",
    };
  }
  if (*Kind == "arg") {
    auto Index = Obj.getInteger("index");
    if (!Index) {
      failSignatureOverride(Path,
                            "missing or invalid integer field 'index'");
    }
    if (*Index < 0 || *Index >= static_cast<int64_t>(Func.arg_size())) {
      failSignatureOverride(Path, "arg target index out of range");
    }
    auto *Arg = Func.getArg(static_cast<unsigned>(*Index));
    assert(Arg != nullptr);
    return {
        .Value = Arg,
        .Key = "arg:" + std::to_string(*Index),
    };
  }
  failSignatureOverride(Path, "summary constraint target kind must be 'arg' or 'ret'");
}

bool isSummaryConstraintTargetOperand(const llvm::json::Value &Value) {
  const auto *Obj = Value.getAsObject();
  if (Obj == nullptr) {
    return false;
  }
  auto Kind = Obj->getString("kind");
  return Kind && (*Kind == "arg" || *Kind == "ret");
}

const llvm::json::Object *
getExtraConstraintBindings(const llvm::json::Object &Spec,
                           llvm::StringRef FuncPath) {
  auto *BindingsValue = Spec.get("bindings");
  if (BindingsValue == nullptr) {
    return nullptr;
  }
  const auto *Bindings = BindingsValue->getAsObject();
  if (Bindings == nullptr) {
    failExtraConstraints(appendJSONPath(FuncPath, "bindings"), "expected object");
  }
  for (const auto &Ent : *Bindings) {
    auto BindingPath =
        appendJSONPath(appendJSONPath(FuncPath, "bindings"), Ent.first);
    if (Ent.second.getAsObject() == nullptr) {
      failExtraConstraints(BindingPath, "expected object");
    }
  }
  return Bindings;
}

llvm::Instruction *findFirstNamedInstruction(llvm::Function &Func,
                                             llvm::StringRef Name) {
  for (llvm::BasicBlock &BB : Func) {
    for (llvm::Instruction &I : BB) {
      if (I.getType()->isVoidTy()) {
        continue;
      }
      if (!I.hasName()) {
        continue;
      }
      if (I.getName() == Name) {
        return &I;
      }
    }
  }
  return nullptr;
}

llvm::Instruction *findInstructionByStableId(llvm::Function &Func,
                                             llvm::StringRef StableId) {
  for (llvm::BasicBlock &BB : Func) {
    for (llvm::Instruction &I : BB) {
      if (toStableString(ExtValuePtr(&I)) == StableId) {
        return &I;
      }
    }
  }
  return nullptr;
}

llvm::Instruction *resolveOperandInstructionSelector(
    const llvm::json::Object &TargetObj, llvm::Function &Func,
    llvm::StringRef TargetPath, std::string &SelectorLabel) {
  bool HasInst = TargetObj.get("inst") != nullptr;
  bool HasName = TargetObj.get("name") != nullptr;
  if (HasInst == HasName) {
    failExtraConstraints(
        TargetPath,
        "operand target must contain exactly one of 'inst' or 'name'");
  }

  if (HasInst) {
    auto InstId = TargetObj.getString("inst");
    if (!InstId) {
      failExtraConstraints(TargetPath,
                           "missing or invalid string field 'inst'");
    }
    auto *Inst = findInstructionByStableId(Func, *InstId);
    if (Inst == nullptr) {
      failExtraConstraints(
          TargetPath,
          ("operand target instruction '" + InstId->str() +
           "' not found in function")
              .c_str());
    }
    SelectorLabel = InstId->str();
    return Inst;
  }

  auto Name = TargetObj.getString("name");
  if (!Name) {
    failExtraConstraints(TargetPath, "missing or invalid string field 'name'");
  }
  auto *Inst = findFirstNamedInstruction(Func, *Name);
  if (Inst == nullptr) {
    failExtraConstraints(
        TargetPath,
        ("operand target name '" + Name->str() +
         "' not found as a non-void instruction in function")
            .c_str());
  }
  SelectorLabel = "name:" + Name->str();
  return Inst;
}

void appendOverrideConstraints(
    notdec::mlsub::MLsubRecovery::OverrideTypeRecipe &Into,
    const notdec::mlsub::MLsubRecovery::OverrideTypeRecipe &From) {
  Into.Constraints.insert(Into.Constraints.end(),
                          From.Constraints.begin(), From.Constraints.end());
}

constexpr llvm::StringLiteral kDefaultMLsubBuiltinSummaryOverridePath(
    NOTDEC_DEFAULT_MLSUB_SUMMARY_OVERRIDE_PATH);

llvm::json::Value makeEmptyOverrideDoc() {
  return llvm::json::Object{{"version", int64_t(1)},
                            {"functions", llvm::json::Object{}}};
}

std::string computeSHA256Hex(llvm::StringRef Content) {
  llvm::SHA256 Hash;
  Hash.update(Content);
  return llvm::toHex(Hash.final(), true);
}

std::string renderModuleToString(const llvm::Module &M) {
  std::string Buffer;
  llvm::raw_string_ostream OS(Buffer);
  M.print(OS, nullptr);
  OS.flush();
  return Buffer;
}

void writeJSONFile(llvm::StringRef Path, const llvm::json::Value &Doc) {
  std::error_code EC;
  llvm::raw_fd_ostream OS(Path, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Cannot open output file " << Path << ": "
                 << EC.message() << "\n";
    std::abort();
  }
  OS << llvm::formatv("{0:2}", Doc) << "\n";
}

void writeMLsubInputAnchor(const llvm::Module &M, llvm::StringRef AnchorPath,
                           llvm::StringRef ModuleSHA256Hex) {
  llvm::json::Object Anchor;
  Anchor["version"] = int64_t(1);
  Anchor["stage"] = "mlsub-input";
  Anchor["ir_file"] = kMLsubInputIRFile.str();
  Anchor["sha256"] = ModuleSHA256Hex.str();
  Anchor["data_layout"] = M.getDataLayout().getStringRepresentation();
  Anchor["target_triple"] = M.getTargetTriple().str();
  writeJSONFile(AnchorPath, llvm::json::Value(std::move(Anchor)));
}

void writeTRInputModule(llvm::Module &M, llvm::StringRef OutputPath) {
  auto Suffix = getSuffix(OutputPath.str());
  if (Suffix == ".ll") {
    printModule(M, OutputPath.str().c_str());
    return;
  }
  if (Suffix == ".bc") {
    std::error_code EC;
    llvm::raw_fd_ostream OS(OutputPath, EC);
    if (EC) {
      llvm::errs() << "Cannot open TR input IR output file " << OutputPath
                   << ": " << EC.message() << "\n";
      std::abort();
    }
    llvm::WriteBitcodeToFile(M, OS);
    return;
  }
  llvm::errs() << "Error: --emit-tr-input-ir expects output suffix .ll or .bc, "
                  "got "
               << OutputPath << "\n";
  std::abort();
}

void writeSelectableValues(const llvm::Module &M, llvm::StringRef Path) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(Path, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Cannot open output file " << Path << ": "
                 << EC.message() << "\n";
    std::abort();
  }

  Out << "# Selectable MLsub extra-constraint targets\n\n";
  for (const llvm::Function &Func : M) {
    if (Func.isDeclaration()) {
      continue;
    }
    Out << "## Function: " << Func.getName() << "\n";
    for (const llvm::Argument &Arg : Func.args()) {
      Out << toStableString(ExtValuePtr(const_cast<llvm::Argument *>(&Arg)))
          << "\n";
    }
    if (!Func.getReturnType()->isVoidTy()) {
      Out << toStableString(ReturnValue{
                 .Func = const_cast<llvm::Function *>(&Func)})
          << "\n";
    }
    for (const llvm::BasicBlock &BB : Func) {
      for (const llvm::Instruction &I : BB) {
        auto InstId =
            toStableString(ExtValuePtr(const_cast<llvm::Instruction *>(&I)));
        Out << InstId << "\n";
        if (!I.getType()->isVoidTy() && I.hasName()) {
          Out << "named_value(" << I.getName() << ") -> " << InstId << "\n";
        }
        for (unsigned Index = 0; Index < I.getNumOperands(); ++Index) {
          auto OperandStable = toStableString(getExtValuePtr(
              I.getOperand(Index), const_cast<llvm::Instruction *>(&I), Index));
          Out << "operand(inst=\"" << InstId << "\", index=" << Index
              << ") -> " << OperandStable << "\n";
          if (!I.getType()->isVoidTy() && I.hasName()) {
            Out << "operand(name=\"" << I.getName() << "\", index=" << Index
                << ") -> " << OperandStable << "\n";
          }
        }
      }
    }
    Out << "\n";
  }
}

llvm::StringRef getOverrideKindLabel(bool RequireDefinition) {
  return RequireDefinition ? "signature" : "summary";
}

OverridePNDiffState parsePNDiffState(const llvm::json::Object &Obj,
                                     llvm::StringRef Path) {
  auto State = requireString(Obj, "state", Path);
  if (State == "ptr") {
    return OverridePNDiffState::Ptr;
  }
  if (State == "number" || State == "num") {
    return OverridePNDiffState::Number;
  }
  failSignatureOverride(Path, "pndiff state must be 'ptr' or 'number'");
}

OverridePNDiffState parseExtraConstraintPNDiffState(const llvm::json::Object &Obj,
                                                    llvm::StringRef Path) {
  auto State = Obj.getString("state");
  if (!State) {
    failExtraConstraints(Path, "missing or invalid string field 'state'");
  }
  if (*State == "ptr") {
    return OverridePNDiffState::Ptr;
  }
  if (*State == "number" || *State == "num") {
    return OverridePNDiffState::Number;
  }
  failExtraConstraints(Path, "pndiff state must be 'ptr' or 'number'");
}

ResolvedPNDiffTarget resolvePNDiffTarget(const llvm::json::Object &Obj,
                                        llvm::Function &Func,
                                        llvm::StringRef Path) {
  auto *TargetValue = Obj.get("target");
  if (TargetValue == nullptr) {
    failSignatureOverride(Path, "missing field 'target'");
  }
  const auto &TargetObj =
      requireObject(*TargetValue, appendJSONPath(Path, "target"));
  auto TargetPath = appendJSONPath(Path, "target");
  auto Kind = requireString(TargetObj, "kind", TargetPath);
  if (Kind == "ret") {
    if (Func.getReturnType()->isVoidTy()) {
      llvm::errs() << "Warning: MLsub override at " << TargetPath
                   << ": ret target on void function " << Func.getName()
                   << "; entry skipped\n";
      return {.Value = ExtValuePtr{}, .Key = ""};
    }
    return {
        .Value = ReturnValue{.Func = &Func},
        .Key = "ret",
    };
  }
  if (Kind == "arg") {
    auto Index = requireInteger(TargetObj, "index", TargetPath);
    if (Index < 0 || Index >= static_cast<int64_t>(Func.arg_size())) {
      llvm::errs() << "Warning: MLsub override at " << TargetPath
                   << ": arg target index " << Index << " out of range for "
                   << Func.getName() << " (args=" << Func.arg_size()
                   << "); entry skipped\n";
      return {.Value = ExtValuePtr{}, .Key = ""};
    }
    auto *Arg = Func.getArg(static_cast<unsigned>(Index));
    assert(Arg != nullptr);
    return {
        .Value = Arg,
        .Key = "arg:" + std::to_string(Index),
    };
  }
  failSignatureOverride(TargetPath, "pndiff target kind must be 'arg' or 'ret'");
}

ResolvedConstraintTarget resolveExtraConstraintTargetSelector(
    const llvm::json::Object &TargetObj, llvm::Function &Func,
    const llvm::json::Object *Bindings, llvm::StringRef TargetPath,
    bool ForPNDiff, std::set<std::string> &ResolvingBindings) {
  auto Kind = TargetObj.getString("kind");
  if (!Kind) {
    failExtraConstraints(TargetPath, "missing or invalid string field 'kind'");
  }
  if (*Kind == "ret") {
    if (Func.getReturnType()->isVoidTy()) {
      failExtraConstraints(TargetPath, "ret target requires non-void function");
    }
    return {
        .Value = ReturnValue{.Func = &Func},
        .Key = "ret",
    };
  }
  if (*Kind == "arg") {
    auto Index = TargetObj.getInteger("index");
    if (!Index) {
      failExtraConstraints(TargetPath,
                           "missing or invalid integer field 'index'");
    }
    if (*Index < 0 || *Index >= static_cast<int64_t>(Func.arg_size())) {
      failExtraConstraints(TargetPath, "arg target index out of range");
    }
    auto *Arg = Func.getArg(static_cast<unsigned>(*Index));
    assert(Arg != nullptr);
    return {
        .Value = Arg,
        .Key = "arg:" + std::to_string(*Index),
    };
  }
  if (*Kind == "named_value") {
    auto Name = TargetObj.getString("name");
    if (!Name) {
      failExtraConstraints(TargetPath,
                           "missing or invalid string field 'name'");
    }
    auto *Inst = findFirstNamedInstruction(Func, *Name);
    if (Inst == nullptr) {
      failExtraConstraints(
          TargetPath,
          ("named_value target '" + Name->str() +
           "' not found as a non-void instruction in function")
              .c_str());
    }
    return {
        .Value = Inst,
        .Key = "named_value:" + Name->str(),
    };
  }
  if (*Kind == "inst") {
    auto Id = TargetObj.getString("id");
    if (!Id) {
      failExtraConstraints(TargetPath, "missing or invalid string field 'id'");
    }
    auto *Inst = findInstructionByStableId(Func, *Id);
    if (Inst == nullptr) {
      failExtraConstraints(
          TargetPath,
          ("inst target '" + Id->str() + "' not found in function").c_str());
    }
    if (Inst->getType()->isVoidTy()) {
      failExtraConstraints(TargetPath,
                           "inst target requires non-void instruction");
    }
    return {
        .Value = Inst,
        .Key = "inst:" + Id->str(),
    };
  }
  if (*Kind == "operand") {
    auto Index = TargetObj.getInteger("index");
    if (!Index) {
      failExtraConstraints(TargetPath,
                           "missing or invalid integer field 'index'");
    }
    std::string SelectorLabel;
    auto *Inst = resolveOperandInstructionSelector(TargetObj, Func, TargetPath,
                                                   SelectorLabel);
    if (*Index < 0 ||
        *Index >= static_cast<int64_t>(Inst->getNumOperands())) {
      failExtraConstraints(TargetPath, "operand target index out of range");
    }
    auto OpIndex = static_cast<unsigned>(*Index);
    auto *Operand = Inst->getOperand(OpIndex);
    return {
        .Value = getExtValuePtr(Operand, Inst, *Index),
        .Key = "operand:" + SelectorLabel + ":" + std::to_string(OpIndex),
    };
  }
  if (*Kind == "binding") {
    auto Name = TargetObj.getString("name");
    if (!Name) {
      failExtraConstraints(TargetPath,
                           "missing or invalid string field 'name'");
    }
    if (Bindings == nullptr) {
      failExtraConstraints(TargetPath,
                           "binding target requires function-level 'bindings'");
    }
    std::string BindingName = Name->str();
    if (!ResolvingBindings.insert(BindingName).second) {
      failExtraConstraints(TargetPath,
                           ("binding cycle detected for '" + BindingName + "'")
                               .c_str());
    }
    auto *BindingValue = Bindings->get(*Name);
    if (BindingValue == nullptr) {
      failExtraConstraints(
          TargetPath, ("binding '" + BindingName + "' not found").c_str());
    }
    auto BindingPath = appendJSONPath("bindings", *Name);
    const auto *BindingObj = BindingValue->getAsObject();
    if (BindingObj == nullptr) {
      failExtraConstraints(BindingPath, "expected object");
    }
    auto Resolved = resolveExtraConstraintTargetSelector(
        *BindingObj, Func, Bindings, BindingPath, ForPNDiff, ResolvingBindings);
    ResolvingBindings.erase(BindingName);
    return Resolved;
  }
  if (ForPNDiff) {
    failExtraConstraints(
        TargetPath,
        "pndiff target kind must be 'arg', 'ret', 'named_value', 'inst', 'operand', or 'binding'");
  }
  failExtraConstraints(
      TargetPath,
      "target kind must be 'arg', 'ret', 'named_value', 'inst', 'operand', or 'binding' currently");
}

ResolvedPNDiffTarget resolveExtraConstraintPNDiffTarget(
    const llvm::json::Object &Obj, llvm::Function &Func,
    const llvm::json::Object *Bindings, llvm::StringRef Path) {
  auto *TargetValue = Obj.get("target");
  if (TargetValue == nullptr) {
    failExtraConstraints(Path, "missing field 'target'");
  }
  const auto *TargetObj = TargetValue->getAsObject();
  auto TargetPath = appendJSONPath(Path, "target");
  if (TargetObj == nullptr) {
    failExtraConstraints(TargetPath, "expected object");
  }
  std::set<std::string> ResolvingBindings;
  auto Resolved = resolveExtraConstraintTargetSelector(
      *TargetObj, Func, Bindings, TargetPath, true, ResolvingBindings);
  return {
      .Value = Resolved.Value,
      .Key = Resolved.Key,
  };
}

ResolvedConstraintTarget resolveExtraConstraintTarget(
    const llvm::json::Object &Obj, llvm::Function &Func,
    const llvm::json::Object *Bindings, llvm::StringRef Path) {
  auto *TargetValue = Obj.get("target");
  if (TargetValue == nullptr) {
    failExtraConstraints(Path, "missing field 'target'");
  }
  const auto *TargetObj = TargetValue->getAsObject();
  auto TargetPath = appendJSONPath(Path, "target");
  if (TargetObj == nullptr) {
    failExtraConstraints(TargetPath, "expected object");
  }
  std::set<std::string> ResolvingBindings;
  return resolveExtraConstraintTargetSelector(*TargetObj, Func, Bindings,
                                              TargetPath, false,
                                              ResolvingBindings);
}

void validateExtraConstraintOperand(const llvm::json::Value &Value,
                                    llvm::Function &Func,
                                    const llvm::json::Object *Bindings,
                                    llvm::StringRef Path) {
  const auto *Obj = Value.getAsObject();
  if (Obj == nullptr) {
    failExtraConstraints(Path, "expected object");
  }
  bool HasType = Obj->get("type") != nullptr;
  bool HasTarget = Obj->get("target") != nullptr;
  if (HasType == HasTarget) {
    failExtraConstraints(
        Path, "operand must contain exactly one of 'type' or 'target'");
  }
  if (HasTarget) {
    resolveExtraConstraintTarget(*Obj, Func, Bindings, Path);
    return;
  }
  if (Obj->get("type")->getAsObject() == nullptr &&
      !Obj->get("type")->getAsNull()) {
    failExtraConstraints(appendJSONPath(Path, "type"), "expected object");
  }
}

notdec::mlsub::MLsubRecovery::OverrideTypeRecipe buildExtraConstraintOperand(
    notdec::mlsub::MLsubRecovery &Recovery, ConstraintsGenerator &G,
    notdec::mlsub::MLsubRecovery::OverrideBuildContext &Ctx,
    llvm::Function &Func, const llvm::json::Object *Bindings,
    const llvm::json::Value &Value, llvm::StringRef Path) {
  validateExtraConstraintOperand(Value, Func, Bindings, Path);
  const auto &Obj = *Value.getAsObject();
  if (auto *TypeValue = Obj.get("type")) {
    return Recovery.buildOverrideType(*TypeValue, Ctx,
                                      appendJSONPath(Path, "type"), false);
  }

  auto Target = resolveExtraConstraintTarget(Obj, Func, Bindings, Path);
  auto Node = G.getNodeOrNull(Target.Value);
  if (Node == nullptr) {
    failExtraConstraints(
        Path, ("target '" + Target.Key + "' has no corresponding node").c_str());
  }
  notdec::mlsub::MLsubRecovery::OverrideTypeRecipe Recipe;
  Recipe.Root = Node;
  return Recipe;
}

void applyPNDiffOverrides(ConstraintsGenerator &G, llvm::Function &Func,
                          const llvm::json::Object &Spec,
                          llvm::StringRef FuncPath, bool *WroteHeader) {
  auto *PNDiffValue = Spec.get("pndiff");
  if (PNDiffValue == nullptr) {
    return;
  }

  const auto &PNDiffArray =
      requireArray(*PNDiffValue, appendJSONPath(FuncPath, "pndiff"));
  std::map<std::string, OverridePNDiffState> SeenTargets;
  for (size_t Index = 0; Index < PNDiffArray.size(); ++Index) {
    auto EntryPath =
        appendJSONIndexPath(appendJSONPath(FuncPath, "pndiff"), Index);
    const auto &EntryObj = requireObject(PNDiffArray[Index], EntryPath);
    auto Target = resolvePNDiffTarget(EntryObj, Func, EntryPath);
      if (Target.Key.empty()) {
        continue;
      }
    auto State = parsePNDiffState(EntryObj, EntryPath);

    auto [It, Inserted] = SeenTargets.insert({Target.Key, State});
    if (!Inserted && It->second != State) {
      failSignatureOverride(
          EntryPath,
          ("conflicting duplicate pndiff target '" + Target.Key + "'").c_str());
    }
    if (!Inserted) {
      continue;
    }

    auto *Node = G.getPNINodeOrNull(Target.Value);
    if (Node == nullptr) {
      failSignatureOverride(
          EntryPath,
          ("pndiff target '" + Target.Key + "' has no corresponding node")
              .c_str());
    }
    if (!Node->isPNRelated()) {
      if (State == OverridePNDiffState::Number) {
        if (applyNumberOverrideToCompatibleCastUsers(G, Target.Value)) {
          continue;
        }
        warnIgnoredPNDiffOverride("override", EntryPath, Target.Key, State,
                                  Target.Value, WroteHeader);
        continue;
      }
      failSignatureOverride(
          EntryPath,
          ("pndiff target '" + Target.Key +
           "' is not pointer-sized int or pointer")
              .c_str());
    }

    switch (State) {
    case OverridePNDiffState::Ptr:
      Node->setPtr();
      break;
    case OverridePNDiffState::Number:
      Node->setNonPtr();
      break;
    }
  }
}

void applyExtraConstraintPNDiffs(ConstraintsGenerator &G, llvm::Function &Func,
                                 const llvm::json::Object &Spec,
                                 llvm::StringRef FuncPath,
                                 bool *WroteHeader) {
  auto *Bindings = getExtraConstraintBindings(Spec, FuncPath);
  auto *ActionsValue = Spec.get("actions");
  if (ActionsValue == nullptr) {
    return;
  }
  const auto *Actions = ActionsValue->getAsArray();
  if (Actions == nullptr) {
    failExtraConstraints(appendJSONPath(FuncPath, "actions"), "expected array");
  }

  std::map<std::string, OverridePNDiffState> SeenTargets;
  for (size_t Index = 0; Index < Actions->size(); ++Index) {
    auto ActionPath =
        appendJSONIndexPath(appendJSONPath(FuncPath, "actions"), Index);
    const auto *ActionObj = (*Actions)[Index].getAsObject();
    if (ActionObj == nullptr) {
      failExtraConstraints(ActionPath, "expected object");
    }
    auto Kind = ActionObj->getString("kind");
    if (!Kind) {
      failExtraConstraints(ActionPath, "missing or invalid string field 'kind'");
    }
    if (*Kind != "pndiff") {
      continue;
    }

    auto Target = resolveExtraConstraintPNDiffTarget(*ActionObj, Func, Bindings,
                                                     ActionPath);
    auto State = parseExtraConstraintPNDiffState(*ActionObj, ActionPath);
    auto [It, Inserted] = SeenTargets.insert({Target.Key, State});
    if (!Inserted && It->second != State) {
      failExtraConstraints(
          ActionPath,
          ("conflicting duplicate pndiff target '" + Target.Key + "'").c_str());
    }
    if (!Inserted) {
      continue;
    }

    auto *Node = G.getPNINodeOrNull(Target.Value);
    if (Node == nullptr) {
      failExtraConstraints(
          ActionPath,
          ("pndiff target '" + Target.Key + "' has no corresponding node")
              .c_str());
    }
    if (!Node->isPNRelated()) {
      if (State == OverridePNDiffState::Number) {
        if (applyNumberOverrideToCompatibleCastUsers(G, Target.Value)) {
          continue;
        }
        warnIgnoredPNDiffOverride("extra constraints", ActionPath, Target.Key,
                                  State, Target.Value, WroteHeader);
        continue;
      }
      failExtraConstraints(
          ActionPath,
          ("pndiff target '" + Target.Key +
           "' is not pointer-sized int or pointer")
              .c_str());
    }

    switch (State) {
    case OverridePNDiffState::Ptr:
      Node->setPtr();
      break;
    case OverridePNDiffState::Number:
      Node->setNonPtr();
      break;
    }
  }
}

void loadOverrideFileImpl(llvm::Module &M, const char *Path,
                          llvm::json::Value &Doc,
                          std::set<llvm::Function *> &Funcs,
                          bool StrictValidation, bool RequireDefinition) {
  llvm::StringRef Kind = getOverrideKindLabel(RequireDefinition);
  llvm::errs() << "Loading MLsub " << Kind << " override from: " << Path
               << "\n";
  auto Parsed = llvm::json::parse(readFileToString(Path));
  if (!Parsed) {
    failSignatureOverride("",
                          ("JSON parse failed for " + std::string(Path)).c_str());
  }

  const auto &Root = requireObject(*Parsed, "<root>");
  auto Version = Root.getInteger("version");
  if (!Version || *Version != 1) {
    failSignatureOverride("<root>", "expected version = 1");
  }

  const auto *Functions = Root.getObject("functions");
  if (Functions == nullptr) {
    failSignatureOverride("<root>", "missing object field 'functions'");
  }

  for (const auto &Ent : *Functions) {
    std::string FuncPath = appendJSONPath("functions", Ent.first);
    const auto &Spec = requireObject(Ent.second, FuncPath);
    if (!overrideAllowedForModule(Spec, FuncPath, M)) {
      continue;
    }

    auto *Func = M.getFunction(Ent.first);
    if (Func == nullptr) {
      llvm::errs() << "Warning: MLsub " << Kind
                   << " override function not found: " << Ent.first << "\n";
      continue;
    }

    bool HasFunctionType =
        hasFunctionTypeOverride(Spec, FuncPath, RequireDefinition);
    if (HasFunctionType) {
      auto *Args = Spec.getArray("args");
      if (Args == nullptr) {
        failSignatureOverride(FuncPath, "field 'args' must be an array");
      }
      if (Func->isVarArg()) {
        if (StrictValidation) {
          failSignatureOverride(FuncPath,
                                "vararg functions are not supported yet");
        }
        llvm::errs() << "Warning: skip MLsub " << Kind
                     << " override for vararg " << Func->getName() << "\n";
        continue;
      }
      if (Args->size() != Func->arg_size()) {
        if (StrictValidation) {
          failSignatureOverride(
              FuncPath,
              ("override arg count does not match LLVM function '"
               + Func->getName().str() + "'")
                  .c_str());
        }
        llvm::errs() << "Warning: skip MLsub " << Kind << " override for "
                     << Func->getName() << " because arg count mismatched\n";
        continue;
      }
      if (RequireDefinition && Func->isDeclaration()) {
        if (StrictValidation) {
          failSignatureOverride(
              FuncPath, "signature override requires a function definition");
        }
        llvm::errs() << "Warning: skip MLsub signature override for declaration "
                     << Func->getName() << "\n";
        continue;
      }
    }
    if (auto *Constraints = Spec.get("constraints")) {
      const auto &ConstraintsArray =
          requireArray(*Constraints, appendJSONPath(FuncPath, "constraints"));
      for (size_t Index = 0; Index < ConstraintsArray.size(); ++Index) {
        auto ConstraintPath = appendJSONIndexPath(
            appendJSONPath(FuncPath, "constraints"), Index);
        const auto &ConstraintObj =
            requireObject(ConstraintsArray[Index], ConstraintPath);
        auto Kind = requireString(ConstraintObj, "kind", ConstraintPath);
        if (Kind != "subtype") {
          failSignatureOverride(ConstraintPath,
                                "only subtype constraints are supported");
        }
        if (ConstraintObj.get("lhs") == nullptr ||
            ConstraintObj.get("rhs") == nullptr) {
          failSignatureOverride(ConstraintPath,
                                "constraint requires lhs and rhs");
        }
      }
    }
    if (auto *PNDiff = Spec.get("pndiff")) {
      const auto &PNDiffArray =
          requireArray(*PNDiff, appendJSONPath(FuncPath, "pndiff"));
      std::map<std::string, OverridePNDiffState> SeenTargets;
      for (size_t Index = 0; Index < PNDiffArray.size(); ++Index) {
        auto EntryPath =
            appendJSONIndexPath(appendJSONPath(FuncPath, "pndiff"), Index);
        const auto &EntryObj = requireObject(PNDiffArray[Index], EntryPath);
        auto Target = resolvePNDiffTarget(EntryObj, *Func, EntryPath);
        if (Target.Key.empty()) {
          continue;
        }
        auto State = parsePNDiffState(EntryObj, EntryPath);
        auto [It, Inserted] = SeenTargets.insert({Target.Key, State});
        if (!Inserted && It->second != State) {
          failSignatureOverride(
              EntryPath,
              ("conflicting duplicate pndiff target '" + Target.Key + "'")
                  .c_str());
        }
      }
    }
    if (Spec.get("is_polymorphic") != nullptr &&
        !Spec.getBoolean("is_polymorphic")) {
      failSignatureOverride(appendJSONPath(FuncPath, "is_polymorphic"),
                            "expected boolean");
    }
    if (Spec.get("opaque_body") != nullptr &&
        !Spec.getBoolean("opaque_body")) {
      failSignatureOverride(appendJSONPath(FuncPath, "opaque_body"),
                            "expected boolean");
    }

    auto *DocRoot = Doc.getAsObject();
    assert(DocRoot != nullptr);
    auto *DocFunctions = DocRoot->getObject("functions");
    assert(DocFunctions != nullptr);
    (*DocFunctions)[Ent.first] = Ent.second;
    Funcs.insert(Func);
  }
}

const llvm::json::Value *getOverrideSpecImpl(
    const llvm::json::Value &Doc, const std::set<llvm::Function *> &Funcs,
    const llvm::Function &Func) {
  if (Funcs.count(const_cast<llvm::Function *>(&Func)) == 0) {
    return nullptr;
  }

  const auto *Root = Doc.getAsObject();
  if (Root == nullptr) {
    return nullptr;
  }
  const auto *Functions = Root->getObject("functions");
  if (Functions == nullptr) {
    return nullptr;
  }
  return Functions->get(Func.getName());
}

void validateExtraConstraintsAnchor(const llvm::json::Object &Root,
                                    const llvm::Module &M,
                                    llvm::StringRef ModuleSHA256Hex) {
  auto *AnchorValue = Root.get("ir_anchor");
  if (AnchorValue == nullptr) {
    return;
  }
  const auto *Anchor = AnchorValue->getAsObject();
  if (Anchor == nullptr) {
    failExtraConstraints("ir_anchor", "expected object");
  }

  auto Stage = requireString(*Anchor, "stage", "ir_anchor");
  if (Stage != "mlsub-input") {
    failExtraConstraints("ir_anchor.stage",
                         "expected stage = 'mlsub-input'");
  }

  auto SHA256 = Anchor->getString("sha256");
  if (!SHA256) {
    failExtraConstraints("ir_anchor.sha256",
                         "missing or invalid string field 'sha256'");
  }
  if (*SHA256 != ModuleSHA256Hex) {
    failExtraConstraints("ir_anchor.sha256",
                         ("sha256 mismatch: expected " +
                          ModuleSHA256Hex.str() + ", current " + SHA256->str())
                             .c_str());
  }

  if (auto DataLayout = Anchor->getString("data_layout")) {
    auto Current = M.getDataLayout().getStringRepresentation();
    if (*DataLayout != Current) {
      failExtraConstraints("ir_anchor.data_layout",
                           ("data layout mismatch: expected " + Current).c_str());
    }
  }

  if (auto TargetTriple = Anchor->getString("target_triple")) {
    auto Current = M.getTargetTriple().str();
    if (*TargetTriple != Current) {
      failExtraConstraints(
          "ir_anchor.target_triple",
          ("target triple mismatch: expected " + Current).c_str());
    }
  }
}

void validateExtraConstraintFunctions(const llvm::json::Object &Root,
                                      llvm::Module &M,
                                      llvm::json::Value &Doc,
                                      std::set<llvm::Function *> &Funcs) {
  auto *FunctionsValue = Root.get("functions");
  if (FunctionsValue == nullptr) {
    return;
  }
  const auto *Functions = FunctionsValue->getAsObject();
  if (Functions == nullptr) {
    failExtraConstraints("functions", "expected object");
  }

  for (const auto &Ent : *Functions) {
    std::string FuncPath = appendJSONPath("functions", Ent.first);
    const auto *Spec = Ent.second.getAsObject();
    if (Spec == nullptr) {
      failExtraConstraints(FuncPath, "expected object");
    }
    if (!overrideAllowedForModule(*Spec, FuncPath, M)) {
      continue;
    }

    auto *Func = M.getFunction(Ent.first);
    if (Func == nullptr) {
      llvm::errs() << "Warning: MLsub extra constraints function not found: "
                   << Ent.first << "\n";
      continue;
    }

    auto *ActionsValue = Spec->get("actions");
    if (ActionsValue == nullptr) {
      failExtraConstraints(FuncPath, "missing field 'actions'");
    }
    const auto *Actions = ActionsValue->getAsArray();
    if (Actions == nullptr) {
      failExtraConstraints(appendJSONPath(FuncPath, "actions"), "expected array");
    }
    auto *Bindings = getExtraConstraintBindings(*Spec, FuncPath);

    std::map<std::string, OverridePNDiffState> SeenTargets;
    for (size_t Index = 0; Index < Actions->size(); ++Index) {
      auto ActionPath =
          appendJSONIndexPath(appendJSONPath(FuncPath, "actions"), Index);
      const auto *ActionObj = (*Actions)[Index].getAsObject();
      if (ActionObj == nullptr) {
        failExtraConstraints(ActionPath, "expected object");
      }
      auto Kind = ActionObj->getString("kind");
      if (!Kind) {
        failExtraConstraints(ActionPath,
                             "missing or invalid string field 'kind'");
      }
      if (*Kind == "pndiff") {
        auto Target = resolveExtraConstraintPNDiffTarget(*ActionObj, *Func,
                                                         Bindings, ActionPath);
        auto State = parseExtraConstraintPNDiffState(*ActionObj, ActionPath);
        auto [It, Inserted] = SeenTargets.insert({Target.Key, State});
        if (!Inserted && It->second != State) {
          failExtraConstraints(
              ActionPath,
              ("conflicting duplicate pndiff target '" + Target.Key + "'")
                  .c_str());
        }
        continue;
      }
      if (*Kind == "subtype" || *Kind == "equal") {
        auto *LHSValue = ActionObj->get("lhs");
        auto *RHSValue = ActionObj->get("rhs");
        if (LHSValue == nullptr || RHSValue == nullptr) {
          failExtraConstraints(ActionPath, "constraint action requires lhs and rhs");
        }
        validateExtraConstraintOperand(
            *LHSValue, *Func, Bindings, appendJSONPath(ActionPath, "lhs"));
        validateExtraConstraintOperand(
            *RHSValue, *Func, Bindings, appendJSONPath(ActionPath, "rhs"));
        continue;
      }
      failExtraConstraints(
          ActionPath,
          "only actions.kind = 'pndiff', 'subtype', or 'equal' is supported currently");
    }

    auto *DocRoot = Doc.getAsObject();
    assert(DocRoot != nullptr);
    auto *DocFunctions = DocRoot->getObject("functions");
    assert(DocFunctions != nullptr);
    (*DocFunctions)[Ent.first] = Ent.second;
    Funcs.insert(Func);
  }
}

void applyExtraConstraintSubtypeActions(notdec::mlsub::MLsubRecovery &Recovery,
                                        ConstraintsGenerator &G,
                                        llvm::Function &Func,
                                        const llvm::json::Object &Spec,
                                        llvm::StringRef FuncPath) {
  auto *Bindings = getExtraConstraintBindings(Spec, FuncPath);
  auto *ActionsValue = Spec.get("actions");
  if (ActionsValue == nullptr) {
    return;
  }
  const auto *Actions = ActionsValue->getAsArray();
  if (Actions == nullptr) {
    failExtraConstraints(appendJSONPath(FuncPath, "actions"), "expected array");
  }

  notdec::mlsub::MLsubRecovery::OverrideBuildContext Ctx{
      .Generator = G, .Func = Func};
  for (size_t Index = 0; Index < Actions->size(); ++Index) {
    auto ActionPath =
        appendJSONIndexPath(appendJSONPath(FuncPath, "actions"), Index);
    const auto *ActionObj = (*Actions)[Index].getAsObject();
    if (ActionObj == nullptr) {
      failExtraConstraints(ActionPath, "expected object");
    }
    auto Kind = ActionObj->getString("kind");
    if (!Kind) {
      failExtraConstraints(ActionPath, "missing or invalid string field 'kind'");
    }
    if (*Kind != "subtype" && *Kind != "equal") {
      continue;
    }

    auto *LHSValue = ActionObj->get("lhs");
    auto *RHSValue = ActionObj->get("rhs");
    if (LHSValue == nullptr || RHSValue == nullptr) {
      failExtraConstraints(ActionPath, "constraint action requires lhs and rhs");
    }
    auto LHSRecipe =
        buildExtraConstraintOperand(Recovery, G, Ctx, Func, Bindings, *LHSValue,
                                    appendJSONPath(ActionPath, "lhs"));
    auto RHSRecipe =
        buildExtraConstraintOperand(Recovery, G, Ctx, Func, Bindings, *RHSValue,
                                    appendJSONPath(ActionPath, "rhs"));
    Recovery.applyOverrideRecipe(G, LHSRecipe);
    Recovery.applyOverrideRecipe(G, RHSRecipe);
    G.addSubtype(LHSRecipe.Root, RHSRecipe.Root);
    if (*Kind == "equal") {
      G.addSubtype(RHSRecipe.Root, LHSRecipe.Root);
    }
  }
}

struct UTypeVariableDetail {
  std::string Name;
  std::uint32_t Id = 0;
  std::uint32_t Size = 0;
  std::set<std::uint32_t> OriginIds;
};

struct VarOriginEntry {
  UTypeVariableDetail Detail;
  std::set<std::string> RootLabels;
};

std::string formatExtValueMappingLabel(
    ExtValuePtr Value, ExtValueLabelCache *Cache = nullptr);
std::string formatExtValueList(const std::set<ExtValuePtr> &Values,
                               ExtValueLabelCache *Cache);
std::string formatOriginIdSummary(const std::set<std::uint32_t> &OriginIds);
void appendVarOriginEntries(std::map<std::uint32_t, VarOriginEntry> &Entries,
                            const binarysub::UTypePtr &Ty,
                            llvm::StringRef RootLabel);

void appendDebugValueTypes(
    llvm::StringRef DebugDir, llvm::StringRef SCCName,
    DSUMap<ExtValuePtr, SimpleType> &V2N,
    const std::set<ExtValuePtr> &ContraVariantValues,
    const std::map<binarysub::PolarVar, binarysub::UTypePtr> &Res,
    const std::map<std::uint32_t, std::set<ExtValuePtr>> &OriginalVariableSources,
    bool SolveMemory, const binarysub::PolarVar &PolMem,
    ExtValueLabelCache *DebugLabelCache) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(join(DebugDir.str(), kValueTypesFile.str()), EC,
                           llvm::sys::fs::OF_Append);
  if (EC) {
    llvm::errs() << "Error printing to " << kValueTypesFile << ", "
                 << EC.message() << "\n";
    return;
  }

  // A Redis SCC can produce multi-megabyte UType strings for hundreds of
  // thousands of values. Keeping every completed line until the final sort
  // retained the whole 15 GB ValueTypes file in memory. The line prefix is the
  // polarity plus stable value label; sort that small key, then format and
  // write one full UType line at a time.
  struct DebugValueTypeEntry {
    std::string SortPrefix;
    SimpleType Type;
  };
  std::vector<DebugValueTypeEntry> Entries;
  Entries.reserve(V2N.size());
  for (const auto &Ent : V2N) {
    std::string Prefix =
        ContraVariantValues.count(Ent.first) == 0 ? "[+] " : "[-] ";
    Prefix += formatExtValueMappingLabel(Ent.first, DebugLabelCache);
    Entries.push_back({std::move(Prefix), Ent.second});
  }
  std::sort(Entries.begin(), Entries.end(),
            [](const DebugValueTypeEntry &Lhs,
               const DebugValueTypeEntry &Rhs) {
              return Lhs.SortPrefix < Rhs.SortPrefix;
            });

  auto findSolvedType = [&](const DebugValueTypeEntry &Entry, bool Pos)
      -> binarysub::UTypePtr {
    if (!Entry.Type->isVariableState()) {
      return nullptr;
    }
    auto It = Res.find(binarysub::PolarVar{.var = Entry.Type, .pos = Pos});
    if (It == Res.end() || !It->second) {
      return nullptr;
    }
    return It->second;
  };

  // Build one printer for the whole SCC section. This lets a recursive record
  // shared by many value rows be declared once before the rows instead of
  // repeating the same declaration in every printType() result.
  std::vector<binarysub::UTypePtr> PrintRoots;
  PrintRoots.reserve(Entries.size() * 2 + (SolveMemory ? 1 : 0));
  for (const auto &Entry : Entries) {
    if (auto Lower = findSolvedType(Entry, true)) {
      PrintRoots.push_back(Lower);
    }
    if (auto Upper = findSolvedType(Entry, false)) {
      PrintRoots.push_back(Upper);
    }
  }
  if (SolveMemory) {
    auto It = Res.find(PolMem);
    if (It != Res.end() && It->second) {
      PrintRoots.push_back(It->second);
    }
  }
  binarysub::UTypePrintSession PrintSession(PrintRoots);

  auto formatSolvedType = [&](const DebugValueTypeEntry &Entry, bool Pos) {
    if (!Entry.Type->isVariableState()) {
      return binarysub::debug_string(Entry.Type);
    }
    auto It = Res.find(binarysub::PolarVar{.var = Entry.Type, .pos = Pos});
    if (It == Res.end() || !It->second) {
      return std::string("<null>");
    }
    return PrintSession.print(It->second);
  };
  auto formatLine = [&](const DebugValueTypeEntry &Entry) {
    return Entry.SortPrefix + " => lower=" + formatSolvedType(Entry, true) +
           " ; upper=" + formatSolvedType(Entry, false);
  };
  auto writeEntry = [&](const DebugValueTypeEntry &Entry) {
    Out << Entry.SortPrefix << " => lower=" << formatSolvedType(Entry, true)
        << " ; upper=" << formatSolvedType(Entry, false) << "\n";
  };

  Out << "## SCC: " << SCCName << "\n";
  if (PrintSession.hasDeclarations()) {
    Out << "### Shared UTypes\n";
    Out << PrintSession.printDeclarations() << "\n";
  }
  for (std::size_t Begin = 0; Begin < Entries.size();) {
    std::size_t End = Begin + 1;
    while (End < Entries.size() &&
           Entries[End].SortPrefix == Entries[Begin].SortPrefix) {
      ++End;
    }
    if (End == Begin + 1) {
      writeEntry(Entries[Begin]);
    } else {
      // Stable labels should be unique. If malformed input produces a
      // collision, retain the old full-line lexicographic order locally.
      std::vector<std::string> CollisionLines;
      CollisionLines.reserve(End - Begin);
      for (std::size_t Index = Begin; Index < End; ++Index) {
        CollisionLines.push_back(formatLine(Entries[Index]));
      }
      std::sort(CollisionLines.begin(), CollisionLines.end());
      for (const auto &Line : CollisionLines) {
        Out << Line << "\n";
      }
    }
    Begin = End;
  }
  // `[memory]` sorts after the `[+]` and `[-]` value prefixes, so streaming it
  // last preserves the previous complete-file order.
  if (SolveMemory) {
    auto It = Res.find(PolMem);
    Out << "[memory] <memory> => ";
    if (It == Res.end() || !It->second) {
      Out << "<null>";
    } else {
      Out << PrintSession.print(It->second);
    }
    Out << "\n";
  }
  Out << "\n";
}

void appendDebugVarOrigins(
    llvm::StringRef DebugDir, llvm::StringRef SCCName,
    DSUMap<ExtValuePtr, SimpleType> &V2N,
    const std::set<ExtValuePtr> &ContraVariantValues,
    const std::map<binarysub::PolarVar, binarysub::UTypePtr> &Res,
    const std::map<std::uint32_t, std::set<ExtValuePtr>> &OriginalVariableSources,
    bool SolveMemory, const binarysub::PolarVar &PolMem,
    ExtValueLabelCache *DebugLabelCache,
    const std::vector<std::string> *BoundOriginDescriptions) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(join(DebugDir.str(), kVarOriginsFile.str()), EC,
                           llvm::sys::fs::OF_Append);
  if (EC) {
    llvm::errs() << "Error printing to " << kVarOriginsFile << ", "
                 << EC.message() << "\n";
    return;
  }

  auto getPol = [&](ExtValuePtr V) { return !ContraVariantValues.count(V); };
  std::map<std::uint32_t, VarOriginEntry> Entries;

  for (const auto &Ent : V2N) {
    if (!Ent.second->isVariableState()) {
      continue;
    }
    bool Pol = getPol(Ent.first);
    auto It = Res.find(binarysub::PolarVar{.var = Ent.second, .pos = Pol});
    if (It == Res.end() || !It->second) {
      continue;
    }
    appendVarOriginEntries(
        Entries, It->second,
        formatExtValueMappingLabel(Ent.first, DebugLabelCache));
  }
  if (SolveMemory) {
    auto It = Res.find(PolMem);
    if (It != Res.end() && It->second) {
      appendVarOriginEntries(Entries, It->second, "<memory>");
    }
  }

  Out << "## SCC: " << SCCName << "\n";
  if (Entries.empty()) {
    Out << "No UType variables in this SCC.\n\n";
    return;
  }

  for (const auto &[Id, Entry] : Entries) {
    const auto &Detail = Entry.Detail;
    Out << "ut#" << Id << " " << Detail.Name << " ; size=" << Detail.Size
        << " ; origins=" << formatOriginIdSummary(Detail.OriginIds) << "\n";
    Out << "  appears-in: " << llvm::join(Entry.RootLabels, " | ") << "\n";
    for (auto OriginId : Detail.OriginIds) {
      Out << "  vs#" << OriginId << " => ";
      if (auto It = OriginalVariableSources.find(OriginId);
          It != OriginalVariableSources.end()) {
        Out << formatExtValueList(It->second, DebugLabelCache);
      } else {
        Out << "<unknown>";
      }
      Out << "\n";
    }
    Out << "\n";
  }

#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
  // Which host constraint introduced each surviving bound.  A field of the
  // converted HType points at a UType variable whose origins are the vs# ids
  // printed above; the entries below say which ptr-add / access put the bound
  // there in the first place.
  {
    std::map<std::uint32_t, SimpleType> VarsById;
    for (const auto &Ent : V2N) {
      auto *State = Ent.second ? Ent.second->getAsVariableState() : nullptr;
      if (State != nullptr && State->mergedInto == nullptr) {
        VarsById[State->id] = Ent.second;
      }
    }
    std::set<std::uint32_t> UsedOrigins;
    std::ostringstream Body;
    auto dumpSide = [&](const SimpleType &Var, llvm::StringRef Side,
                        const std::vector<SimpleType> &Bounds,
                        const std::vector<std::shared_ptr<const binarysub::SmallIdSet>>
                            &Origins) {
      for (std::size_t I = 0; I < Origins.size() && I < Bounds.size(); ++I) {
        const auto &Set = Origins[I];
        if (!Set || Set->empty()) {
          continue;
        }
        std::string BoundText = binarysub::debug_string(Bounds[I]);
        if (BoundText.size() > 160) {
          BoundText.resize(160);
          BoundText += "...";
        }
        Body << "vs#" << Var->getAsVariableState()->id << " " << Side.str()
             << "[" << I << "] " << BoundText << "\n    origins=[";
        for (auto OriginId : *Set) {
          Body << "o#" << OriginId << " ";
          UsedOrigins.insert(OriginId);
        }
        Body << "]\n";
      }
    };
    for (const auto &[Id, Var] : VarsById) {
      auto *State = Var->getAsVariableState();
      dumpSide(Var, "lower", State->lowerBounds, State->lowerBoundOrigins);
      dumpSide(Var, "upper", State->upperBounds, State->upperBoundOrigins);
    }
    if (!UsedOrigins.empty()) {
      Out << "## Bound origins\n";
      Out << Body.str();
      Out << "## Bound origin legend\n";
      for (auto OriginId : UsedOrigins) {
        Out << "o#" << OriginId << " = ";
        if (BoundOriginDescriptions != nullptr &&
            OriginId < BoundOriginDescriptions->size()) {
          Out << (*BoundOriginDescriptions)[OriginId];
        } else {
          Out << "<unknown>";
        }
        Out << "\n";
      }
      Out << "\n";
    }
  }
#endif
}

void writeCallSlotMergeDecisions(llvm::StringRef Path, const AllGraphs &AG) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(Path, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Error printing to " << Path << ", " << EC.message()
                 << "\n";
    return;
  }

  std::size_t Total = 0;
  std::map<std::string, std::size_t> DecisionCounts;
  for (const auto &Data : AG.AllSCCs) {
    if (Data.Generator == nullptr) {
      continue;
    }
    for (const auto &Decision : Data.Generator->CallSlotMergeDecisions) {
      ++Total;
      ++DecisionCounts[Decision.Decision];
    }
  }

  Out << "# Call slot merge decisions\n\n";
  Out << "total: " << Total << "\n";
  for (const auto &[Decision, Count] : DecisionCounts) {
    Out << Decision << ": " << Count << "\n";
  }
  Out << "\n";

  for (const auto &Data : AG.AllSCCs) {
    if (Data.Generator == nullptr ||
        Data.Generator->CallSlotMergeDecisions.empty()) {
      continue;
    }
    Out << "## SCC: " << Data.SCCName << "\n\n";
    std::size_t Index = 0;
    for (const auto &Decision : Data.Generator->CallSlotMergeDecisions) {
      Out << "### Decision " << ++Index << "\n";
      Out << "transaction: " << Decision.TransactionId << "\n";
      Out << "target: " << Decision.Target << "\n";
      Out << "policy: " << Decision.Policy << "\n";
      Out << "decision: " << Decision.Decision << "\n";
      Out << "reason: " << Decision.Reason << "\n";
      Out << "formal: " << Decision.Formal << "\n";
      Out << "touched-nodes: " << Decision.TouchedNodes << "\n";
      Out << "entries:\n";
      for (const auto &Entry : Decision.Entries) {
        Out << "  - " << Entry << "\n";
      }
      Out << "roots:\n";
      for (const auto &Root : Decision.Roots) {
        Out << "  - " << Root << "\n";
      }
      Out << "recursive-merges:\n";
      for (const auto &Merge : Decision.RecursiveMerges) {
        Out << "  - " << Merge << "\n";
      }
      Out << "\n";
    }
  }
}

llvm::StringRef
localSubtypeMergeModeName(ConstraintsGenerator::LocalSubtypeMergeMode Mode) {
  switch (Mode) {
  case ConstraintsGenerator::LocalSubtypeMergeMode::StructPointer:
    return "struct-pointer";
  case ConstraintsGenerator::LocalSubtypeMergeMode::Pointer:
    return "pointer";
  case ConstraintsGenerator::LocalSubtypeMergeMode::AllLocal:
    return "all-local";
  }
  llvm_unreachable("unknown local subtype merge mode");
}

void writeLocalSubtypeMergeStats(llvm::StringRef Path, const AllGraphs &AG) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(Path, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Error printing to " << Path << ", " << EC.message()
                 << "\n";
    return;
  }

  std::size_t Checks = 0;
  std::size_t BasicCandidates = 0;
  std::size_t SameFunctionCandidates = 0;
  std::size_t PointerCandidates = 0;
  std::size_t Accepted = 0;
  std::size_t ReplaceBoundMergeEvents = 0;
  std::size_t AuxiliaryMergeEvents = 0;
  std::size_t EarlyCallMerged = 0;
  std::map<std::string, std::size_t> SubtypeFailures;
  for (const auto &Data : AG.AllSCCs) {
    if (Data.Generator == nullptr) {
      continue;
    }
    Checks += Data.Generator->LocalSubtypeHookChecks;
    BasicCandidates += Data.Generator->LocalSubtypeBasicCandidates;
    SameFunctionCandidates +=
        Data.Generator->LocalSubtypeSameFunctionCandidates;
    PointerCandidates += Data.Generator->LocalSubtypePointerCandidates;
    Accepted += Data.Generator->LocalSubtypeAccepted;
    ReplaceBoundMergeEvents +=
        Data.Generator->LocalSubtypeReplaceBoundMergeEvents;
    AuxiliaryMergeEvents += Data.Generator->LocalSubtypeAuxiliaryMergeEvents;
    EarlyCallMerged += Data.Generator->EarlyCallInterfaceMerged;
    for (const auto &[Reason, Count] :
         Data.Generator->SubtypeConstraintFailures) {
      SubtypeFailures[Reason] += Count;
    }
  }

  Out << "# Local subtype merge experiment\n\n";
  Out << "hook-checks: " << Checks << "\n";
  Out << "same-level-size-candidates: " << BasicCandidates << "\n";
  Out << "same-function-candidates: " << SameFunctionCandidates << "\n";
  Out << "pointer-candidates: " << PointerCandidates << "\n";
  Out << "accepted: " << Accepted << "\n";
  Out << "policy-replace-bound-merge-events: " << ReplaceBoundMergeEvents
      << "\n";
  Out << "policy-auxiliary-merge-events: " << AuxiliaryMergeEvents << "\n";
  Out << "policy-merge-events: "
      << ReplaceBoundMergeEvents + AuxiliaryMergeEvents << "\n\n";
  Out << "early-call-interface-merged: " << EarlyCallMerged << "\n\n";
  Out << "subtype-constraint-failures: ";
  std::size_t TotalSubtypeFailures = 0;
  for (const auto &[_, Count] : SubtypeFailures) {
    TotalSubtypeFailures += Count;
  }
  Out << TotalSubtypeFailures << "\n";
  for (const auto &[Reason, Count] : SubtypeFailures) {
    Out << "  - " << Count << " x " << Reason << "\n";
  }
  Out << "\n";

  for (const auto &Data : AG.AllSCCs) {
    if (Data.Generator == nullptr) {
      continue;
    }
    const auto &G = *Data.Generator;
    Out << "## SCC: " << Data.SCCName << "\n";
    Out << "mode: " << localSubtypeMergeModeName(G.LocalSubtypeMode) << "\n";
    Out << "hook-checks: " << G.LocalSubtypeHookChecks << "\n";
    Out << "same-level-size-candidates: " << G.LocalSubtypeBasicCandidates
        << "\n";
    Out << "same-function-candidates: " << G.LocalSubtypeSameFunctionCandidates
        << "\n";
    Out << "pointer-candidates: " << G.LocalSubtypePointerCandidates << "\n";
    Out << "accepted: " << G.LocalSubtypeAccepted << "\n";
    Out << "policy-replace-bound-merge-events: "
        << G.LocalSubtypeReplaceBoundMergeEvents << "\n";
    Out << "policy-auxiliary-merge-events: "
        << G.LocalSubtypeAuxiliaryMergeEvents << "\n";
    Out << "policy-merge-events: "
        << G.LocalSubtypeReplaceBoundMergeEvents +
               G.LocalSubtypeAuxiliaryMergeEvents
        << "\n\n";
    Out << "early-call-interface-enabled: "
        << (G.EnableEarlyCallInterfaceMerge ? "true" : "false") << "\n";
    Out << "early-call-interface-merged: " << G.EarlyCallInterfaceMerged
        << "\n\n";
    std::size_t SCCSubtypeFailures = 0;
    for (const auto &[_, Count] : G.SubtypeConstraintFailures) {
      SCCSubtypeFailures += Count;
    }
    Out << "subtype-constraint-failures: " << SCCSubtypeFailures << "\n";
    for (const auto &[Reason, Count] : G.SubtypeConstraintFailures) {
      Out << "  - " << Count << " x " << Reason << "\n";
    }
    Out << "\n";
  }
}

std::string formatTypeBuilderRootLabel(ExtValuePtr Value) {
  return toStableString(Value);
}

std::string sanitizeTraceText(llvm::StringRef Text) {
  std::string Result = Text.str();
  for (char &Ch : Result) {
    if (Ch == '\n' || Ch == '\r' || Ch == '\t') {
      Ch = ' ';
    }
  }
  return Result;
}

std::string formatExtValueMappingLabel(ExtValuePtr Value,
                                       ExtValueLabelCache *Cache) {
  if (Cache != nullptr) {
    if (auto It = Cache->find(Value); It != Cache->end()) {
      return It->second;
    }
  }

  std::string Stable = toStableString(Value);
  if (notdec::isFastWorkDir()) {
    if (Cache != nullptr) {
      Cache->emplace(std::move(Value), Stable);
    }
    return Stable;
  }

  std::string Verbose = sanitizeTraceText(toString(Value, true));
  std::string Result;
  if (Stable.empty()) {
    Result = std::move(Verbose);
  } else if (Verbose.empty() || Verbose == Stable) {
    Result = std::move(Stable);
  } else {
    Result = Stable + " (" + Verbose + ")";
  }
  if (Cache != nullptr) {
    Cache->emplace(std::move(Value), Result);
  }
  return Result;
}

void collectUTypeVariableDetailsImpl(
    const binarysub::UTypePtr &Ty, std::unordered_map<std::uint32_t, UTypeVariableDetail> &Out,
    std::unordered_set<const binarysub::UType *> &Seen) {
  if (!Ty || !Seen.insert(Ty).second) {
    return;
  }

  std::visit(
      [&](auto const &Node) {
        using T = std::decay_t<decltype(Node)>;
        if constexpr (std::is_same_v<T, binarysub::UTypeVariable>) {
          auto &Entry = Out[Node.id];
          Entry.Name = Node.name;
          Entry.Id = Node.id;
          Entry.Size = Node.size;
          Entry.OriginIds.insert(Node.originIds.begin(), Node.originIds.end());
        } else if constexpr (std::is_same_v<T, binarysub::UFunctionType>) {
          for (const auto &Arg : Node.args) {
            collectUTypeVariableDetailsImpl(Arg, Out, Seen);
          }
          collectUTypeVariableDetailsImpl(Node.result, Out, Seen);
        } else if constexpr (std::is_same_v<T, binarysub::UUnion> ||
                             std::is_same_v<T, binarysub::UInter>) {
          for (const auto &Term : Node.types) {
            collectUTypeVariableDetailsImpl(Term, Out, Seen);
          }
        } else if constexpr (std::is_same_v<T, binarysub::URecordType>) {
          for (const auto &[_, FieldTy] : Node.fields) {
            collectUTypeVariableDetailsImpl(FieldTy, Out, Seen);
          }
        } else if constexpr (std::is_same_v<T, binarysub::URecursiveType>) {
          collectUTypeVariableDetailsImpl(Node.body, Out, Seen);
        } else if constexpr (std::is_same_v<T, binarysub::UPointerType>) {
          collectUTypeVariableDetailsImpl(Node.load, Out, Seen);
          collectUTypeVariableDetailsImpl(Node.store, Out, Seen);
        }
      },
      Ty->v);
}

std::unordered_map<std::uint32_t, UTypeVariableDetail>
collectUTypeVariableDetails(const binarysub::UTypePtr &Ty) {
  std::unordered_map<std::uint32_t, UTypeVariableDetail> Details;
  std::unordered_set<const binarysub::UType *> Seen;
  collectUTypeVariableDetailsImpl(Ty, Details, Seen);
  return Details;
}

std::string formatExtValueList(const std::set<ExtValuePtr> &Values,
                               ExtValueLabelCache *Cache) {
  if (Values.empty()) {
    return "<none>";
  }
  std::vector<std::string> Labels;
  Labels.reserve(Values.size());
  for (const auto &Value : Values) {
    Labels.push_back(formatExtValueMappingLabel(Value, Cache));
  }
  std::sort(Labels.begin(), Labels.end());
  return llvm::join(Labels, " | ");
}

std::string formatOriginIdSummary(const std::set<std::uint32_t> &OriginIds) {
  std::string Summary;
  bool First = true;
  for (auto OriginId : OriginIds) {
    if (!First) {
      Summary += ",";
    }
    First = false;
    Summary += "vs#" + std::to_string(OriginId);
  }
  if (Summary.empty()) {
    return "<none>";
  }
  return Summary;
}

void appendVarOriginEntries(
    std::map<std::uint32_t, VarOriginEntry> &Entries, const binarysub::UTypePtr &Ty,
    llvm::StringRef RootLabel) {
  auto Details = collectUTypeVariableDetails(Ty);
  for (const auto &[Id, Detail] : Details) {
    auto &Entry = Entries[Id];
    Entry.Detail = Detail;
    Entry.RootLabels.insert(RootLabel.str());
  }
}

std::shared_ptr<ConstraintsGenerator> getFuncCG(AllGraphs &AG,
                                                const llvm::Function *F) {
  auto *CGN = AG.CG->getOrInsertFunction(const_cast<llvm::Function *>(F));
  const auto It = AG.Func2SCCIndex.find(CGN);
  // A node without an SCC has no summary (the call graph may gain nodes after
  // the partition was built).  The index itself is checked as well: a stale
  // entry must degrade to "no summary" instead of throwing out of range.
  if (It == AG.Func2SCCIndex.end() || It->second >= AG.AllSCCs.size()) {
    return nullptr;
  }
  return AG.AllSCCs[It->second].Generator;
}

struct PNDiffStateInfo {
  std::string State;
  unsigned long NodeId = 0;
};

std::string formatPNDiffStateInfo(const PNDiffStateInfo &Info) {
  return Info.State + "#" + std::to_string(Info.NodeId);
}

std::optional<PNDiffStateInfo> getPNDiffStateInfo(ConstraintsGenerator &CG,
                                                  ExtValuePtr Val) {
  auto *Node = CG.getPNINodeOrNull(Val);
  if (Node == nullptr || !Node->isPNRelated()) {
    return std::nullopt;
  }
  if (Node->isPointer() || Node->isNull()) {
    return PNDiffStateInfo{
        .State = "ptr",
        .NodeId = Node->getId(),
    };
  }
  if (Node->isNumber()) {
    return PNDiffStateInfo{
        .State = "num",
        .NodeId = Node->getId(),
    };
  }
  return PNDiffStateInfo{
      .State = "unknown",
      .NodeId = Node->getId(),
  };
}

std::string joinParts(const std::vector<std::string> &Parts) {
  std::string Result;
  for (size_t I = 0; I < Parts.size(); ++I) {
    if (I != 0) {
      Result += ", ";
    }
    Result += Parts[I];
  }
  return Result;
}

std::optional<std::string>
formatInstructionPNDiffComment(ConstraintsGenerator &CG,
                               const llvm::Instruction &Inst) {
  std::vector<std::string> Parts;
  if (!Inst.getType()->isVoidTy()) {
    if (auto State = getPNDiffStateInfo(
            CG, const_cast<llvm::Instruction *>(&Inst))) {
      Parts.push_back("result=" + formatPNDiffStateInfo(*State));
    }
  }
  for (unsigned I = 0; I < Inst.getNumOperands(); ++I) {
    ExtValuePtr Op = Inst.getOperand(I);
    llvmValue2ExtVal(Op, const_cast<llvm::Instruction *>(&Inst), I);
    if (auto State = getPNDiffStateInfo(CG, Op)) {
      Parts.push_back("op" + std::to_string(I) + "=" +
                      formatPNDiffStateInfo(*State));
    }
  }
  if (Parts.empty()) {
    return std::nullopt;
  }
  return "pndiff: " + joinParts(Parts);
}

std::optional<std::string>
formatFunctionPNDiffComment(ConstraintsGenerator &CG, const llvm::Function &F) {
  std::vector<std::string> Parts;
  if (!F.getReturnType()->isVoidTy()) {
    if (auto State = getPNDiffStateInfo(
            CG, ReturnValue{.Func = const_cast<llvm::Function *>(&F)})) {
      Parts.push_back("ret=" + formatPNDiffStateInfo(*State));
    }
  }
  unsigned ArgIndex = 0;
  for (auto &Arg : F.args()) {
    if (auto State =
            getPNDiffStateInfo(CG, const_cast<llvm::Argument *>(&Arg))) {
      Parts.push_back("arg" + std::to_string(ArgIndex) + "=" +
                      formatPNDiffStateInfo(*State));
    }
    ++ArgIndex;
  }
  if (Parts.empty()) {
    return std::nullopt;
  }
  return "pndiff: " + joinParts(Parts);
}

std::string formatConstraintStateSummary(ConstraintsGenerator &CG,
                                         const ConsNode &Cons) {
  auto Nodes = const_cast<ConsNode &>(Cons).getNodes();
  auto formatStateOrUnknown = [&](PNIValue Val) {
    auto State = getPNDiffStateInfo(CG, CG.getPNIExtValue(Val));
    if (!State) {
      return std::string("unknown");
    }
    return formatPNDiffStateInfo(*State);
  };
  auto ResultState = formatStateOrUnknown(Nodes[2]);
  auto LeftState = formatStateOrUnknown(Nodes[0]);
  auto RightState = formatStateOrUnknown(Nodes[1]);
  return "result=" + ResultState + ", op0=" + LeftState +
         ", op1=" + RightState;
}

bool constraintHasUnknownState(ConstraintsGenerator &CG, const ConsNode &Cons) {
  auto Nodes = const_cast<ConsNode &>(Cons).getNodes();
  for (PNIValue Val : Nodes) {
    auto State = getPNDiffStateInfo(CG, CG.getPNIExtValue(Val));
    if (!State || State->State == "unknown") {
      return true;
    }
  }
  return false;
}

void writePNDiffWarnings(const std::string &Path, AllGraphs &AG,
                         bool PrependBlankLine = false) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(Path, EC,
                           llvm::sys::fs::OF_Text | llvm::sys::fs::OF_Append);
  if (EC) {
    llvm::errs() << "Cannot open PNDiff warning output file " << Path << ": "
                 << EC.message() << "\n";
    std::abort();
  }

  if (PrependBlankLine) {
    Out << "\n";
  }
  Out << "# Residual PNDiff constraints after solve\n\n";
  bool AnyResidual = false;

  for (auto &Data : AG.AllSCCs) {
    if (!Data.Generator || Data.Generator->PG.Constraints.empty()) {
      continue;
    }

    auto &CG = *Data.Generator;
    bool WroteSCCHeader = false;
    for (const auto &Cons : CG.PG.Constraints) {
      if (!constraintHasUnknownState(CG, Cons)) {
        continue;
      }
      if (!WroteSCCHeader) {
        AnyResidual = true;
        WroteSCCHeader = true;
        Out << "## SCC: " << Data.SCCName << "\n";
      }
      auto Nodes = const_cast<ConsNode &>(Cons).getNodes();
      Out << "kind: " << (Cons.isAdd() ? "Add" : "Sub") << "\n";
      Out << "inst: " << formatPNDiffInstruction(Cons.getInst()) << "\n";
      Out << "result: " << toStableString(CG.getPNIExtValue(Nodes[2])) << "\n";
      Out << "op0: " << toStableString(CG.getPNIExtValue(Nodes[0])) << "\n";
      Out << "op1: " << toStableString(CG.getPNIExtValue(Nodes[1])) << "\n";
      Out << "state: " << formatConstraintStateSummary(CG, Cons) << "\n\n";
    }
  }

  if (!AnyResidual) {
    Out << "No residual Add/Sub constraints with unknown state after solve.\n";
  }
}

class PNDiffAnnotationWriter : public llvm::AssemblyAnnotationWriter {
  AllGraphs &AG;

public:
  explicit PNDiffAnnotationWriter(AllGraphs &AG) : AG(AG) {}

  void emitFunctionAnnot(const llvm::Function *F,
                         llvm::formatted_raw_ostream &OS) override {
    auto CG = getFuncCG(AG, F);
    if (!CG) {
      return;
    }
    auto Comment = formatFunctionPNDiffComment(*CG, *F);
    if (!Comment) {
      return;
    }
    OS << "; " << *Comment << "\n";
  }

  void printInfoComment(const llvm::Value &V,
                        llvm::formatted_raw_ostream &OS) override {
    auto *Inst = llvm::dyn_cast<llvm::Instruction>(&V);
    if (Inst == nullptr) {
      return;
    }
    auto CG = getFuncCG(AG, Inst->getFunction());
    if (!CG) {
      return;
    }
    auto Comment = formatInstructionPNDiffComment(*CG, *Inst);
    if (!Comment) {
      return;
    }
    OS << "; " << *Comment;
  }
};

void writePNDiffAnnotatedModule(const llvm::Module &M, const std::string &Path,
                                AllGraphs &AG) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(Path, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Cannot open PNDiff annotated IR output file " << Path
                 << ": " << EC.message() << "\n";
    std::abort();
  }
  PNDiffAnnotationWriter Writer(AG);
  M.print(Out, &Writer);
}

// True when `Name` ends in one of the generic allocator entry points.  The
// name is matched component-wise because the microsub2 lifter does not call
// `malloc` directly: calls go through IDA-named PLT stubs
// (`ida_malloc_0x37b0`, `ida__calloc_0x39f0`, ...) or through the synthesized
// opaque-import declarations
// (`__microsub2_opaque_import___imp_malloc_r64_a1_sig_ri64_ai64`).  An exact
// string comparison therefore never matched anything and no generic malloc
// wrapper was ever detected in the microsub2 corpus (all 534 coreutils work
// dirs reported "No generic malloc wrappers detected"), which left
// allocation-site types unified and mixed.
bool isGenericAllocatorName(llvm::StringRef Name) {
  auto isAllocatorComponent = [](llvm::StringRef Component) {
    return Component == "malloc" || Component == "calloc" ||
           Component == "realloc" || Component == "reallocarray" ||
           Component == "recallocarray" || Component == "strdup" ||
           Component == "strndup";
  };
  llvm::StringRef Rest = Name;
  while (!Rest.empty()) {
    auto [Head, Tail] = Rest.split('_');
    if (!Head.empty() && isAllocatorComponent(Head)) {
      return true;
    }
    Rest = Tail;
  }
  return false;
}

bool isMallocWrapperAllocator(const llvm::Function *F) {
  if (F == nullptr || !F->hasName()) {
    return false;
  }
  // Generic allocator entry points whose pure forwarding wrappers can be
  // treated as polymorphic. The use checker below still rejects any function
  // that initializes or otherwise touches the returned memory, so adding the
  // realloc/strdup family only extends detection to the same wrapper shape
  // already accepted for malloc/calloc (tmux's xrealloc/xstrdup etc.).
  // The lifted IR names the import thunks with a leading underscore
  // (_malloc/_realloc), so normalise before the name table lookup; without this
  // coreutils' xmalloc (whose body calls _malloc) is not recognised, its
  // allocation flow breaks and the wrapper is not marked polymorphic.
  llvm::StringRef Name = F->getName();
  while (!Name.empty() && Name.front() == '_') {
    Name = Name.drop_front();
  }
  return isGenericAllocatorName(Name);
}

// Wrappers detected so far in this run.  A wrapper of a wrapper (e.g.
// coreutils' xpalloc -> xrealloc -> realloc) is only recognisable once the
// inner wrapper is known, so detection iterates to a fixpoint and consults
// this set in addition to the libc allocator names.
std::set<const llvm::Function *> &knownAllocatorWrappers() {
  static std::set<const llvm::Function *> Known;
  return Known;
}

bool isIgnorableMallocWrapperUser(const llvm::Instruction *I) {
  auto *CB = llvm::dyn_cast<llvm::CallBase>(I);
  if (CB == nullptr) {
    return false;
  }
  auto *Callee = CB->getCalledFunction();
  if (Callee == nullptr || !Callee->hasName()) {
    return false;
  }
  auto Name = Callee->getName();
  return Name.starts_with("llvm.dbg") || Name.starts_with("llvm.lifetime");
}

bool isKnownNoReturnCall(const llvm::CallBase &CB) {
  if (CB.hasFnAttr(llvm::Attribute::NoReturn)) {
    return true;
  }
  auto *Callee = CB.getCalledFunction();
  if (Callee == nullptr) {
    return false;
  }
  if (Callee->hasFnAttribute(llvm::Attribute::NoReturn)) {
    return true;
  }
  if (!Callee->hasName()) {
    return false;
  }
  auto Name = Callee->getName();
  return Name == "exit" || Name == "_Exit" || Name == "abort" ||
         Name == "__assert_fail";
}

// A pointer-sized integer is how the microsub2 lifter models a pointer *value*
// (memory addresses stay `ptr`); the NotDec frontends use real pointer types.
bool isPointerShapedType(const llvm::Type *Ty, const llvm::Module *M) {
  if (Ty == nullptr) {
    return false;
  }
  if (Ty->isPointerTy()) {
    return true;
  }
  auto *IT = llvm::dyn_cast<llvm::IntegerType>(Ty);
  return IT != nullptr && M != nullptr &&
         IT->getBitWidth() == M->getDataLayout().getPointerSizeInBits(0);
}

bool isPointerShapedValue(const llvm::Value *V) {
  if (V == nullptr) {
    return false;
  }
  if (auto *F = llvm::dyn_cast<llvm::Function>(V)) {
    return isPointerShapedType(F->getReturnType(), F->getParent());
  }
  if (auto *CB = llvm::dyn_cast<llvm::CallBase>(V)) {
    return isPointerShapedType(CB->getType(), CB->getModule());
  }
  return isPointerShapedType(V->getType(), nullptr);
}

bool isNullPointerValue(const llvm::Value *V, const llvm::Module *M) {
  auto *C = llvm::dyn_cast_or_null<llvm::Constant>(V);
  return C != nullptr && C->isNullValue() &&
         isPointerShapedType(C->getType(), M);
}

bool blockCanReachReturn(llvm::BasicBlock *Start) {
  llvm::SmallPtrSet<llvm::BasicBlock *, 16> Visited;
  llvm::SmallVector<llvm::BasicBlock *, 16> Worklist;
  Worklist.push_back(Start);

  while (!Worklist.empty()) {
    auto *BB = Worklist.pop_back_val();
    if (!Visited.insert(BB).second) {
      continue;
    }
    if (llvm::isa<llvm::ReturnInst>(BB->getTerminator())) {
      return true;
    }

    bool StopsHere = llvm::isa<llvm::UnreachableInst>(BB->getTerminator());
    for (auto &I : *BB) {
      auto *CB = llvm::dyn_cast<llvm::CallBase>(&I);
      if (CB != nullptr && isKnownNoReturnCall(*CB)) {
        StopsHere = true;
        break;
      }
    }
    if (StopsHere) {
      continue;
    }

    for (auto *Succ : llvm::successors(BB)) {
      Worklist.push_back(Succ);
    }
  }
  return false;
}

// The checker only accepts local forwarding of the allocator result. Any real
// use of that pointer, such as GEP, store-through, or passing it to another
// function, means the function is a factory/initializer, not a generic wrapper.
struct MallocWrapperUseChecker {
  // A generic wrapper may allocate on more than one path (gnulib's xpalloc
  // reallocates in two branches); every such call must feed the same forwarded
  // value.
  llvm::SmallPtrSet<const llvm::CallBase *, 4> Allocs;
  const llvm::Function *AllocFn = nullptr;
  const bool Debug = std::getenv("NOTDEC_MALLOC_WRAPPER_DEBUG") != nullptr;

  bool Fail(const char *Why, const llvm::Value *V) {
    if (Debug) {
      llvm::errs() << "[malloc-wrapper-flow] " << Why << ": ";
      if (V != nullptr) {
        V->print(llvm::errs());
      }
      llvm::errs() << "\n";
    }
    return false;
  }
  llvm::SmallPtrSet<const llvm::Value *, 32> ValueSeen;
  llvm::SmallPtrSet<const llvm::Value *, 32> UseSeen;
  llvm::SmallPtrSet<const llvm::AllocaInst *, 8> SlotValueSeen;
  llvm::SmallPtrSet<const llvm::AllocaInst *, 8> SlotUseSeen;

  llvm::AllocaInst *getLocalSlot(llvm::Value *Ptr) const {
    auto *Slot = llvm::dyn_cast_or_null<llvm::AllocaInst>(
        Ptr->stripPointerCasts());
    if (Slot == nullptr || Slot->getFunction() != AllocFn) {
      return nullptr;
    }
    return Slot;
  }

  // Returns the argument index a function forwards unchanged (its body only
  // returns that argument, possibly through casts), or -1.  Blocks that do not
  // return (the abort/unreachable path of coreutils' nonnull) are ignored: the
  // lifted body keeps them and a strict "every terminator is a return" check
  // rejected real forwarding helpers such as xmalloc -> nonnull(malloc(...)).
  int identityForwarderArg(const llvm::Function &F) const {
    if (F.isDeclaration() || F.isIntrinsic() || F.arg_size() == 0) {
      return -1;
    }
    int Index = -1;
    for (const llvm::BasicBlock &BB : F) {
      auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(BB.getTerminator());
      if (Ret == nullptr || Ret->getReturnValue() == nullptr) {
        continue;
      }
      llvm::Value *V = Ret->getReturnValue();
      while (auto *C = llvm::dyn_cast<llvm::CastInst>(V)) {
        V = C->getOperand(0);
      }
      auto *Arg = llvm::dyn_cast<llvm::Argument>(V);
      if (Arg == nullptr || Arg->getParent() != &F) {
        return -1;
      }
      if (Index == -1) {
        Index = static_cast<int>(Arg->getArgNo());
      } else if (Index != static_cast<int>(Arg->getArgNo())) {
        return -1;
      }
    }
    return Index;
  }

  bool isMallocResultValue(llvm::Value *V) {
    if (V == nullptr) {
      return Fail("isMallocResultValue: null value", V);
    }
    if (auto *CB = llvm::dyn_cast<llvm::CallBase>(V);
        CB != nullptr && Allocs.contains(CB)) {
      return true;
    }
    // A wrapper with a "nothing to allocate" path hands back the null pointer
    // there (xpalloc returns null when the caller asks for no growth).
    if (isNullPointerValue(V, AllocFn != nullptr ? AllocFn->getParent()
                                                 : nullptr)) {
      return true;
    }
    if (!ValueSeen.insert(V).second) {
      // Already visited: assume the flow is a wrapper flow (same convention as
      // the slot guards).  Returning false here made the answer depend on the
      // user-list order, which rejected the i64-pointer microsub2 wrappers.
      return true;
    }

    if (auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
      if (llvm::isa<llvm::BitCastInst, llvm::AddrSpaceCastInst>(I)) {
        return isMallocResultValue(I->getOperand(0));
      }
      if (auto *Freeze = llvm::dyn_cast<llvm::FreezeInst>(I)) {
        return isMallocResultValue(Freeze->getOperand(0));
      }
      if (auto *PN = llvm::dyn_cast<llvm::PHINode>(I)) {
        for (auto &Incoming : PN->incoming_values()) {
          if (!isMallocResultValue(Incoming.get())) {
            return false;
          }
        }
        return PN->getNumIncomingValues() > 0;
      }
      if (auto *Sel = llvm::dyn_cast<llvm::SelectInst>(I)) {
        return isMallocResultValue(Sel->getTrueValue()) &&
               isMallocResultValue(Sel->getFalseValue());
      }
      if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(I)) {
        auto *Slot = getLocalSlot(Load->getPointerOperand());
        if (Slot == nullptr) {
          return Fail("load is not from a local slot", Load);
        }
        if (!slotStoresOnlyMallocResult(Slot)) {
          return Fail("slot stores more than the alloc result", Load);
        }
        return true;
      }
      if (auto *CB = llvm::dyn_cast<llvm::CallBase>(V)) {
        if (auto *Callee = CB->getCalledFunction()) {
          const int Forwarded = identityForwarderArg(*Callee);
          if (Forwarded >= 0 &&
              static_cast<unsigned>(Forwarded) < CB->arg_size() &&
              isMallocResultValue(CB->getArgOperand(Forwarded))) {
            return true;
          }
        }
      }
    }
    return Fail("value is not the alloc result", V);
  }

  bool slotStoresOnlyMallocResult(llvm::AllocaInst *Slot) {
    if (!SlotValueSeen.insert(Slot).second) {
      return true;
    }

    bool SawStore = false;
    for (auto *User : Slot->users()) {
      auto *I = llvm::dyn_cast<llvm::Instruction>(User);
      if (I == nullptr) {
        return Fail("non-instruction slot user", User);
      }
      if (isIgnorableMallocWrapperUser(I)) {
        continue;
      }
      if (auto *Store = llvm::dyn_cast<llvm::StoreInst>(I)) {
        if (getLocalSlot(Store->getPointerOperand()) != Slot) {
          return Fail("slot seen as a store side target", Store);
        }
        SawStore = true;
        if (!isMallocResultValue(Store->getValueOperand())) {
          return Fail("slot stores a non-alloc value", Store);
        }
        continue;
      }
      if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(I)) {
        if (getLocalSlot(Load->getPointerOperand()) == Slot) {
          continue;
        }
        return Fail("slot used as a load address of another slot", Load);
      }
      return Fail("unhandled slot user", I);
    }
    if (!SawStore) {
      return Fail("slot never stores the alloc result", Slot);
    }
    return true;
  }

  bool isMallocNullCompare(llvm::ICmpInst &ICmp) {
    auto Pred = ICmp.getPredicate();
    if (Pred != llvm::CmpInst::ICMP_EQ && Pred != llvm::CmpInst::ICMP_NE) {
      return false;
    }

    auto *LHS = ICmp.getOperand(0);
    auto *RHS = ICmp.getOperand(1);
    auto *M = ICmp.getModule();
    return (isNullPointerValue(LHS, M) && isMallocResultValue(RHS)) ||
           (isNullPointerValue(RHS, M) && isMallocResultValue(LHS));
  }

  bool valueUsesOnlyWrapperFlow(llvm::Value *V) {
    if (!UseSeen.insert(V).second) {
      return true;
    }

    for (auto *User : V->users()) {
      auto *I = llvm::dyn_cast<llvm::Instruction>(User);
      if (I == nullptr) {
        return Fail("non-instruction user", User);
      }
      if (isIgnorableMallocWrapperUser(I)) {
        continue;
      }
      if (llvm::isa<llvm::BitCastInst, llvm::AddrSpaceCastInst,
                    llvm::FreezeInst, llvm::PHINode, llvm::SelectInst>(I)) {
        if (!valueUsesOnlyWrapperFlow(I)) {
          return Fail("cast/phi/select chain", I);
        }
        continue;
      }
      if (auto *RI = llvm::dyn_cast<llvm::ReturnInst>(I)) {
        if (!isMallocResultValue(RI->getReturnValue())) {
          return Fail("return value is not the alloc result", RI);
        }
        continue;
      }
      if (auto *ICmp = llvm::dyn_cast<llvm::ICmpInst>(I)) {
        if (!isMallocNullCompare(*ICmp)) {
          return Fail("compare is not alloc-vs-null", ICmp);
        }
        continue;
      }
      if (auto *Store = llvm::dyn_cast<llvm::StoreInst>(I)) {
        if (Store->getValueOperand() != V) {
          return Fail("value stored as a side operand", Store);
        }
        auto *Slot = getLocalSlot(Store->getPointerOperand());
        if (Slot == nullptr || !slotUsesOnlyWrapperFlow(Slot)) {
          return Fail("store target is not a wrapper-only slot", Store);
        }
        continue;
      }
      if (auto *CB = llvm::dyn_cast<llvm::CallBase>(I)) {
        // xmalloc returns nonnull(malloc(n)): the forwarding call consumes the
        // allocation result and hands it straight through, so the wrapper flow
        // continues at the call's result instead of stopping here.
        if (auto *Callee = CB->getCalledFunction()) {
          const int Forwarded = identityForwarderArg(*Callee);
          if (Forwarded >= 0 &&
              static_cast<unsigned>(Forwarded) < CB->arg_size() &&
              CB->getArgOperand(Forwarded) == V) {
            if (!valueUsesOnlyWrapperFlow(CB)) {
              return Fail("forwarder result chain", CB);
            }
            continue;
          }
        }
      }
      return Fail("unhandled user kind", I);
    }
    return true;
  }

  bool slotUsesOnlyWrapperFlow(llvm::AllocaInst *Slot) {
    if (!SlotUseSeen.insert(Slot).second) {
      return true;
    }

    for (auto *User : Slot->users()) {
      auto *I = llvm::dyn_cast<llvm::Instruction>(User);
      if (I == nullptr) {
        return false;
      }
      if (isIgnorableMallocWrapperUser(I)) {
        continue;
      }
      if (auto *Store = llvm::dyn_cast<llvm::StoreInst>(I)) {
        if (getLocalSlot(Store->getPointerOperand()) != Slot ||
            !isMallocResultValue(Store->getValueOperand())) {
          return false;
        }
        continue;
      }
      if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(I)) {
        if (getLocalSlot(Load->getPointerOperand()) != Slot ||
            !valueUsesOnlyWrapperFlow(Load)) {
          return false;
        }
        continue;
      }
      return false;
    }
    return true;
  }

  bool nullErrorBranchesDoNotReturn(llvm::Function &F) {
    for (auto &BB : F) {
      auto *BI = llvm::dyn_cast<llvm::BranchInst>(BB.getTerminator());
      if (BI == nullptr || !BI->isConditional()) {
        continue;
      }
      auto *ICmp = llvm::dyn_cast<llvm::ICmpInst>(BI->getCondition());
      if (ICmp == nullptr || !isMallocNullCompare(*ICmp)) {
        continue;
      }

      auto Pred = ICmp->getPredicate();
      auto *NullSucc =
          BI->getSuccessor(Pred == llvm::CmpInst::ICMP_EQ ? 0 : 1);
      if (!blockCanReachReturn(NullSucc)) {
        continue;
      }
      // The microsub2 lifter does not know that the allocator's failure path is
      // noreturn: it returns the (null) pointer instead.  Accept the branch when
      // every reachable return still hands back the allocator result.
      llvm::SmallVector<llvm::BasicBlock *, 16> Worklist{NullSucc};
      llvm::SmallPtrSet<llvm::BasicBlock *, 16> Visited;
      bool NullBranchOk = true;
      while (!Worklist.empty() && NullBranchOk) {
        auto *BB = Worklist.pop_back_val();
        if (!Visited.insert(BB).second) {
          continue;
        }
        if (auto *RI = llvm::dyn_cast<llvm::ReturnInst>(BB->getTerminator())) {
          if (!isMallocResultValue(RI->getReturnValue())) {
            NullBranchOk = false;
          }
          continue;
        }
        for (auto *Succ : llvm::successors(BB)) {
          Worklist.push_back(Succ);
        }
      }
      if (!NullBranchOk) {
        return Fail("null error branch returns a non-alloc value", BI);
      }
    }
    return true;
  }
};

llvm::CallBase *getGenericMallocWrapperAllocator(llvm::Function &F) {
  static const bool DebugWrapper = std::getenv("NOTDEC_MALLOC_WRAPPER_DEBUG") != nullptr;
  auto Reject = [&](const char *Why) -> llvm::CallBase * {
    if (DebugWrapper && F.getName().starts_with("sub_")) {
      llvm::errs() << "[malloc-wrapper] reject " << F.getName() << ": " << Why
                   << "\n";
    }
    return nullptr;
  };
  if (F.isDeclaration() || F.isIntrinsic() || !isPointerShapedValue(&F) ||
      isMallocWrapperAllocator(&F)) {
    return Reject("decl/intrinsic/not-pointer-shaped/is-allocator");
  }

  llvm::SmallVector<llvm::CallBase *, 4> Allocs;
  for (auto &BB : F) {
    for (auto &I : BB) {
      auto *CB = llvm::dyn_cast<llvm::CallBase>(&I);
      if (CB == nullptr) {
        continue;
      }
      llvm::Function *Callee = CB->getCalledFunction();
      if (!isMallocWrapperAllocator(Callee) &&
          knownAllocatorWrappers().count(Callee) == 0) {
        continue;
      }
      if (!isPointerShapedValue(CB)) {
        return Reject("allocator result is not pointer-shaped");
      }
      Allocs.push_back(CB);
    }
  }
  if (Allocs.empty()) {
    return Reject("no allocator call");
  }
  // More than one call is allowed as long as they all call the same allocator:
  // the wrapper reallocates on several paths and forwards whichever result its
  // path produced.
  for (llvm::CallBase *CB : Allocs) {
    if (CB->getCalledFunction() != Allocs.front()->getCalledFunction()) {
      return Reject("allocator calls have different callees");
    }
  }

  llvm::DominatorTree DT(F);
  MallocWrapperUseChecker Checker;
  Checker.AllocFn = &F;
  for (llvm::CallBase *CB : Allocs) {
    Checker.Allocs.insert(CB);
  }
  bool SawReturn = false;
  for (auto &BB : F) {
    auto *RI = llvm::dyn_cast<llvm::ReturnInst>(BB.getTerminator());
    if (RI == nullptr) {
      continue;
    }
    SawReturn = true;
    // A single allocator call must dominate every return, as before.  With
    // several calls the returned value comes from whichever branch ran (xpalloc
    // returns either realloc result), so no single call dominates the join; the
    // value-flow check right below is what ties the return to an allocation.
    if (Allocs.size() == 1 && !DT.dominates(Allocs.front(), RI)) {
      return Reject("allocator does not dominate a return");
    }
    if (!Checker.isMallocResultValue(RI->getReturnValue())) {
      return Reject("return value is not the alloc result");
    }
  }
  if (!SawReturn) {
    return Reject("no return");
  }
  if (!Checker.nullErrorBranchesDoNotReturn(F)) {
    return Reject("null error branch reaches a return");
  }
  for (llvm::CallBase *CB : Allocs) {
    if (!Checker.valueUsesOnlyWrapperFlow(CB)) {
      return Reject("alloc value used outside wrapper flow");
    }
  }

  return Allocs.front();
}

} // namespace

bool isOpaqueSegmentReadValue(const llvm::Value *V) {
  if (V == nullptr) {
    return false;
  }
  const llvm::Value *Val = V;
  while (true) {
    if (const auto *CE = llvm::dyn_cast<llvm::CastInst>(Val)) {
      Val = CE->getOperand(0);
      continue;
    }
    if (const auto *CE = llvm::dyn_cast<llvm::FreezeInst>(Val)) {
      Val = CE->getOperand(0);
      continue;
    }
    if (const auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(Val);
        CE != nullptr && CE->isCast()) {
      Val = CE->getOperand(0);
      continue;
    }
    break;
  }
  const auto *CB = llvm::dyn_cast<llvm::CallBase>(Val);
  if (CB == nullptr) {
    return false;
  }
  const llvm::Function *Callee = CB->getCalledFunction();
  if (Callee == nullptr) {
    Callee = llvm::dyn_cast<llvm::Function>(
        CB->getCalledOperand()->stripPointerCasts());
  }
  if (Callee == nullptr) {
    return false;
  }
  const auto Name = Callee->getName();
  // microsub2 models FS/GS reads as opaque helpers; GCC stack canaries use
  // readfsqword(0x28) on x86_64 (and the dword/qword siblings).
  return Name.contains("readfsqword") || Name.contains("readfsdword") ||
         Name.contains("readgsqword") || Name.contains("readgsdword");
}

bool isStackCanarySlotValue(const ExtValuePtr &Slot) {
  const llvm::Value *V = nullptr;
  if (const auto *SV = std::get_if<llvm::Value *>(&Slot)) {
    V = *SV;
  } else if (const auto *SO = std::get_if<StackObject>(&Slot)) {
    V = SO->Allocator;
  }
  if (V == nullptr) {
    return false;
  }
  const llvm::Value *SlotVal = V->stripPointerCasts();
  for (const llvm::User *U : SlotVal->users()) {
    const auto *SI = llvm::dyn_cast<llvm::StoreInst>(U);
    if (SI == nullptr || SI->getPointerOperand() != SlotVal) {
      continue;
    }
    if (isOpaqueSegmentReadValue(SI->getValueOperand())) {
      return true;
    }
  }
  return false;
}

void ConstraintsGenerator::emitMappingTrace(llvm::StringRef Event,
                                            ExtValuePtr Val,
                                            const SimpleType &Ty) {
  if (TraceStream == nullptr) {
    return;
  }
  std::ostringstream OS;
  OS << "[mapping:" << Event.str() << "] value="
     << formatExtValueMappingLabel(Val) << " simple="
     << binarysub::debug_string(Ty);
  binarysub::binarysub_trace(OS.str());
}

void ConstraintsGenerator::emitRemapTrace(llvm::StringRef Event, ExtValuePtr Val,
                                          ExtValuePtr Target,
                                          const SimpleType &Ty) {
  if (TraceStream == nullptr) {
    return;
  }
  std::ostringstream OS;
  OS << "[mapping:" << Event.str() << "] value="
     << formatExtValueMappingLabel(Val) << " target="
     << formatExtValueMappingLabel(Target) << " simple="
     << binarysub::debug_string(Ty);
  binarysub::binarysub_trace(OS.str());
}

void ConstraintsGenerator::emitMergeTrace(llvm::StringRef Event, SimpleType From,
                                          SimpleType To,
                                          llvm::ArrayRef<ExtValuePtr> MovedValues) {
  if (TraceStream == nullptr) {
    return;
  }
  std::ostringstream OS;
  OS << "[mapping:" << Event.str() << "] from="
     << binarysub::debug_string(From) << " to=" << binarysub::debug_string(To);
  if (MovedValues.empty()) {
    OS << " values=<none>";
  } else {
    OS << " values=";
    for (size_t I = 0; I < MovedValues.size(); ++I) {
      if (I != 0) {
        OS << ", ";
      }
      OS << formatExtValueMappingLabel(MovedValues[I]);
    }
  }
  binarysub::binarysub_trace(OS.str());
}

void ConstraintsGenerator::emitTypeRecoveryTrace(const std::string &Message) {
  if (TraceStream == nullptr) {
    return;
  }
  binarysub::binarysub_trace(Message);
}

BottomUpTiming ConstraintsGenerator::run() {
  BottomUpTiming Timing;
  ActiveBottomUpTimingContext = &Timing;
  auto Measure = [&](BottomUpStage Stage, auto &&Action) {
    CurrentConstraintStats = {};
    auto Start = startBottomUpPhase(ConstraintDiagEnabled);
    Action();
    Timing.at(Stage) = finishBottomUpPhase(
        Start, CurrentConstraintStats, ConstraintDiagEnabled);
  };

  // SCCs is pointer-ordered, so use function names to keep node creation and
  // recursive constraint insertion stable across ASLR runs.
  std::vector<const llvm::Function *> SortedSCCs(SCCs.begin(), SCCs.end());
  llvm::sort(SortedSCCs, [](const llvm::Function *A,
                            const llvm::Function *B) {
    return A->getName() < B->getName();
  });

  Measure(BottomUpStage::FunctionNodes, [&] {
    for (const llvm::Function *Func1 : SortedSCCs) {
      auto Func = const_cast<llvm::Function *>(Func1);
      auto F = createNode(Func);
      std::vector<SimpleType> Args;
      for (unsigned I = 0; I < Func->arg_size(); ++I) {
        Args.push_back(createNode(Func->getArg(I)));
      }
      SimpleType Ret = nullptr;
      if (!Func->getReturnType()->isVoidTy()) {
        Ret = Func->getReturnType()->isAggregateType()
                  ? makeFunctionAggregateReturnRecord(*Func)
                  : createNode(ReturnValue{.Func = Func});
      }
      addSubtype(binarysub::make_function(Args, Ret), F);
    }
  });

  Measure(BottomUpStage::VisitorConstraints, [&] {
    for (const llvm::Function *Func : SortedSCCs) {
      if (OpaqueBodies.count(const_cast<llvm::Function *>(Func)) != 0) {
        continue;
      }
      MLsubVisitor Visitor(*this);
      Visitor.visit(const_cast<llvm::Function *>(Func));
      Visitor.handlePHINodes();
    }
  });

  Measure(BottomUpStage::SccDeferredCalls,
          [&] { applyDeferredCallConstraints(); });
  for (const llvm::Function *Func1 : SortedSCCs) {
    auto F = getNodeOrNull(const_cast<llvm::Function *>(Func1));
    assert(F->getAsVariableState() != nullptr);
  }
  Measure(BottomUpStage::PNDiffSolve, [&] { PG.solve(); });
  Measure(BottomUpStage::CallInterfaceMerge, [&] {
    auto Merged = applyCallInterfaceMergePolicy();
    llvm::errs() << "Info: transactional call interface merge policy merged "
                 << Merged << " pair(s)\n";
    // Round two: the pairs round one refused, now that every call edge and
    // every field access has been seen.
    recheckDeferredRoundOneMerges();
    if (TwoRoundMergeDiag) {
      llvm::errs() << "[two-round] round-one-refused=" << RoundOneMergeRefused
                   << " round-two-considered=" << RoundTwoMergeConsidered
                   << " round-two-accepted=" << RoundTwoMergeAccepted << "\n";
    }
  });
  Measure(BottomUpStage::StructSlotMerge, [&] {
    if (!EnableStructPtrLoadStoreMerge) {
      return;
    }
    auto Merged = applyStructPtrLoadStoreMergePolicy();
    llvm::errs() << "Info: struct pointer slot merge policy merged "
                 << Merged << " pair(s)\n";
  });
  Measure(BottomUpStage::FieldFollowupMerge, [&] {
    auto Merged = applyStructPtrFieldFollowupMergePolicy();
    llvm::errs() << "Info: struct pointer field follow-up merge policy merged "
                 << Merged << " pair(s)\n";
  });
  // Round two, second pass: late constraints and late field evidence can have
  // queued more pairs since the first pass.
  recheckDeferredRoundOneMerges();
  ActiveBottomUpTimingContext = nullptr;
  if (FrameNoNonFrameMerge || FrameNoCrossFunctionMerge) {
    llvm::errs() << "Info: frame merge vetoes nonframe=" << FrameVetoNonFrame
                 << " cross-function=" << FrameVetoCrossFunction
                 << " unknown-side-silent=" << FrameVetoUnknownSide << "\n";
  }
  return Timing;
}

bool ConstraintsGenerator::configureConstraintContext(
    binarysub::ConstraintContext &Context) {
  bool HasHook = true;
  Context.shouldMergeSameLevelVarVar =
      [this](const SimpleType &LHS, const SimpleType &RHS)
      -> binarysub::expected<bool, binarysub::Error> {
    return shouldMergeSameFunctionStructPtrSubtype(LHS, RHS);
  };
  Context.shouldMergeReverseEdgeVariables =
      [this](const SimpleType &LHS, const SimpleType &RHS)
      -> binarysub::expected<bool, binarysub::Error> {
    return roundOneMergeAllowed(LHS, RHS);
  };
  // The bound-added and nested-rewrite hooks below rescan direct variable
  // neighbors whenever pointer/struct evidence changes. An existing direct
  // edge therefore needs no policy recheck until one of those hooks fires.
  Context.mergePolicyRechecksOnEvidenceChange = true;
  Context.onVariableNonVarBoundAdded =
      [this](const SimpleType &Var, const SimpleType &Bound,
             binarysub::BoundPolarity Polarity,
             const binarysub::EnqueueMergeFn &EnqueueMerge)
      -> binarysub::expected<void, binarysub::Error> {
    if (Var) {
      StructEvidenceMemo.erase(Var.get());
    }
    return onVariableNonVarBoundAdded(Var, Bound, Polarity, EnqueueMerge);
  };
  Context.onVariableNestedBoundRewritten =
      [this](const SimpleType &User, const SimpleType &OldBound,
             const SimpleType &NewBound,
             const binarysub::EnqueueMergeFn &EnqueueMerge)
      -> binarysub::expected<void, binarysub::Error> {
    if (User) {
      StructEvidenceMemo.erase(User.get());
    }
    return onVariableNestedBoundRewritten(User, OldBound, NewBound,
                                          EnqueueMerge);
  };
  Context.onVariableMerged = [this](const binarysub::MergeEvent &Event) {
    if (Event.from) {
      StructEvidenceMemo.erase(Event.from.get());
    }
    if (Event.into) {
      StructEvidenceMemo.erase(Event.into.get());
    }
    if (Event.reason == binarysub::MergeReason::PolicyReplaceBound) {
      ++LocalSubtypeReplaceBoundMergeEvents;
    } else if (Event.reason == binarysub::MergeReason::PolicyAuxiliary) {
      ++LocalSubtypeAuxiliaryMergeEvents;
    }
    // NOTDEC_MERGE_AUDIT=1: log every variable merge with the IR values behind
    // both sides.  Answers "which pair of program values did this merge put in
    // one type variable", including solver-driven merges that never go through
    // a frontend policy (see microsub2/logs 68/69).
    if (std::getenv("NOTDEC_MERGE_AUDIT") != nullptr) {
      auto Describe = [this](const SimpleType &Ty) {
        std::string Text =
            Ty ? binarysub::debug_string(Ty) : std::string("<null>");
        if (Text.size() > 160) {
          Text.resize(160);
          Text += "...";
        }
        std::string Values;
        if (Ty != nullptr) {
          if (auto It = V2N.rev().find(Ty); It != V2N.rev().end()) {
            for (std::size_t I = 0; I < It->second.size() && I < 8; ++I) {
              if (I != 0) {
                Values += ", ";
              }
              Values += toStableString(It->second[I]);
            }
            if (It->second.size() > 8) {
              Values += ", +" + std::to_string(It->second.size() - 8);
            }
          }
        }
        if (Ty != nullptr) {
          if (auto *Owner = getVariableOwningFunction(Ty)) {
            Text += " owner=" + Owner->getName().str();
          }
        }
        return Text + " values=[" + Values + "]";
      };
      if (std::getenv("NOTDEC_BIGCLASS_DIAG") != nullptr) {
        auto Root = binarysub::resolve_variable(Event.into);
        auto *VS = Root ? Root->getAsVariableState() : nullptr;
        if (VS != nullptr && VS->upperBounds.size() >= 1000) {
          // Values whose resolved root is this class: these are the IR values
          // that were merged into it, so they map straight back to the source.
          std::vector<ExtValuePtr> Members;
          for (const auto &Ent : V2N.rev()) {
            if (binarysub::resolve_variable(Ent.first).get() == Root.get()) {
              Members.insert(Members.end(), Ent.second.begin(), Ent.second.end());
            }
          }
          llvm::errs() << "[big-class] root=" << binarysub::debug_string(Root)
                       << " ub=" << VS->upperBounds.size()
                       << " values=" << Members.size() << "\n";
          for (std::size_t I = 0; I < Members.size() && I < 60; ++I) {
            llvm::errs() << "    v=" << toStableString(Members[I]);
            if (auto *O = getExtValueFunction(Members[I])) {
              llvm::errs() << " in=" << O->getName();
            }
            llvm::errs() << "\n";
          }
        }
      }
      llvm::errs() << "[merge-audit] reason="
                   << std::string(binarysub::merge_reason_name(Event.reason))
                   << " from=" << Describe(Event.from)
                   << " into=" << Describe(Event.into) << "\n";
    }
    if (MergeEval) {
      MergeEval->observeVariableMerged(Event);
    }
  };
  Context.stats =
      ConstraintDiagEnabled ? &CurrentConstraintStats : nullptr;
  return HasHook;
}

bool ConstraintsGenerator::tryMergeVariablesForPolicy(
    llvm::StringRef Policy, SimpleType From, SimpleType Into,
    const std::string &TraceDetail) {
  From = binarysub::resolve_variable(From);
  Into = binarysub::resolve_variable(Into);
  auto *FromVar = From ? From->getAsVariableState() : nullptr;
  auto *IntoVar = Into ? Into->getAsVariableState() : nullptr;
  if (FromVar == nullptr || IntoVar == nullptr || From.get() == Into.get()) {
    return false;
  }
  if (FromVar->level != IntoVar->level || FromVar->size != IntoVar->size) {
    return false;
  }
  binarysub::ConstraintContext Context;
  binarysub::ConstraintContext *ContextPtr =
      configureConstraintContext(Context) ? &Context : nullptr;
  // PNDiff unifications are deferred: they are not part of the SimpleType
  // transaction, so applying them before a speculative merge is known to
  // survive leaves the PNDiff graph claiming merges that the rollback undid
  // (the transactional call-slot path defers them the same way, in
  // Effects.PNDiffUnifies).  They are applied once, after the surviving try.
  std::vector<std::pair<SimpleType, SimpleType>> PNDiffUnifies;
  auto MergeObserver = [&](const SimpleType &Lhs, const SimpleType &Rhs) {
    PNDiffUnifies.push_back({Lhs, Rhs});
  };
  PNDiffUnifies.push_back({From, Into});
  // Prototype (NOTDEC_MERGE_LOCAL_RETRY=1): a merge boundary has its own
  // transaction, so a merge that lets an integer view and a structured view
  // coexist can be rolled back and redone with the store->load coupling
  // suppressed -- without moving the checkpoint to the whole constraint call.
  // The value bookkeeping below runs only for the surviving attempt.
  static const bool MergeLocalRetry = [] {
    const char *Value = std::getenv("NOTDEC_MERGE_LOCAL_RETRY");
    return Value != nullptr && Value[0] != '\0' && Value[0] != '0';
  }();
  binarysub::expected<void, binarysub::Error> Result;
  if (MergeLocalRetry && ContextPtr != nullptr) {
    bool Coexist = false;
    Context.onIntStructCoexist = [&Coexist](const SimpleType &,
                                            const SimpleType &) {
      Coexist = true;
    };
    Context.stats = &CurrentConstraintStats;
    binarysub::SimpleTypeTransaction Tx;
    Result = binarysub::merge_variable_into(From, Into, MergeObserver,
                                            ContextPtr);
    if (Result && Coexist) {
      Tx.rollback();
      ++MergeLocalConflicts;
      ++MergeLocalRetries;
      // The merged variables are restored by the rollback; redo the same merge
      // with the store->load coupling suppressed for this attempt.
      Context.suppressStoreToLoad = true;
      Result = binarysub::merge_variable_into(From, Into, MergeObserver,
                                              &Context);
    } else {
      Tx.commit();
    }
  } else {
    Result = binarysub::merge_variable_into(From, Into, MergeObserver,
                                            ContextPtr);
  }
  if (!Result) {
    emitTypeRecoveryTrace("[merge-policy:" + Policy.str() +
                          ":skip] from=" + binarysub::debug_string(From) +
                          " into=" + binarysub::debug_string(Into) +
                          " reason=" + Result.error().msg +
                          " detail=" + TraceDetail);
    return false;
  }
  for (const auto &[Lhs, Rhs] : PNDiffUnifies) {
    maybeUnifyPNDiffTypeVariablePair(Lhs, Rhs);
  }

  std::vector<std::pair<SimpleType, std::vector<ExtValuePtr>>> RootsToMove;
  for (const auto &Ent : V2N.rev()) {
    auto Root = binarysub::resolve_variable(Ent.first);
    if (Root && Root.get() == From.get()) {
      RootsToMove.push_back({Ent.first, Ent.second});
    }
  }
  std::vector<ExtValuePtr> MovedValues;
  for (const auto &[_, Values] : RootsToMove) {
    MovedValues.insert(MovedValues.end(), Values.begin(), Values.end());
  }

  emitMergeTrace(Policy, From, Into, MovedValues);
  if (MergeEval) {
    MergeEval->observeValueMapMerge(From, Into, MovedValues);
  }
  for (const auto &[Root, _] : RootsToMove) {
    V2N.merge(Root, Into);
  }
  auto FieldFollowups =
      collectStructPtrFieldFollowupMergeCandidatesForMergedOwner(Into);
  StructFieldFollowupMergeCandidates.insert(
      StructFieldFollowupMergeCandidates.end(), FieldFollowups.begin(),
      FieldFollowups.end());
  emitTypeRecoveryTrace("[merge-policy:" + Policy.str() +
                        "] from=" + binarysub::debug_string(From) +
                        " into=" + binarysub::debug_string(Into) +
                        " detail=" + TraceDetail);
  return true;
}

const ExtValuePtr *
ConstraintsGenerator::getVariableExternalValue(SimpleType Ty) const {
  Ty = binarysub::resolve_variable(Ty);
  auto *Var = Ty ? Ty->getAsVariableState() : nullptr;
  if (Var == nullptr || Var->externalHandle == nullptr) {
    return nullptr;
  }
  return static_cast<const ExtValuePtr *>(Var->externalHandle);
}

void ConstraintsGenerator::attachExternalValueHandle(SimpleType Ty,
                                                     const ExtValuePtr &Val) {
  Ty = binarysub::resolve_variable(Ty);
  auto *Var = Ty ? Ty->getAsVariableState() : nullptr;
  if (Var == nullptr || Var->externalHandle != nullptr) {
    return;
  }
  Var->externalHandle = getPNIValue(Val);
}

static llvm::Function *getFunctionFromLLVMValue(llvm::Value *Value) {
  if (auto *Inst = llvm::dyn_cast_or_null<llvm::Instruction>(Value)) {
    return Inst->getFunction();
  }
  if (auto *Arg = llvm::dyn_cast_or_null<llvm::Argument>(Value)) {
    return Arg->getParent();
  }
  if (auto *BB = llvm::dyn_cast_or_null<llvm::BasicBlock>(Value)) {
    return BB->getParent();
  }
  return nullptr;
}

llvm::Function *ConstraintsGenerator::getExtValueFunction(
    const ExtValuePtr &Val) const {
  if (auto *V = std::get_if<llvm::Value *>(&Val)) {
    return getFunctionFromLLVMValue(*V);
  }
  if (auto *Ret = std::get_if<ReturnValue>(&Val)) {
    return Ret->Func;
  }
  if (auto *C = std::get_if<UConstant>(&Val)) {
    return getFunctionFromLLVMValue(C->User);
  }
  if (auto *Stack = std::get_if<StackObject>(&Val)) {
    return Stack->Allocator != nullptr ? Stack->Allocator->getFunction()
                                       : nullptr;
  }
  if (auto *Heap = std::get_if<HeapObject>(&Val)) {
    return Heap->Allocator != nullptr ? Heap->Allocator->getFunction()
                                      : nullptr;
  }
  return nullptr;
}

void ConstraintsGenerator::emitFunctionConstraintStats() const {
  std::map<llvm::Function *, std::pair<size_t, size_t>> NodesEdges;
  std::map<llvm::Function *, size_t> SelfRefs;
  std::map<llvm::Function *, size_t> MaxBounds;
  for (auto &Entry : V2N) {
    llvm::Function *F = getExtValueFunction(Entry.first);
    if (F == nullptr) {
      continue;
    }
    SimpleType Ty = binarysub::resolve_variable(Entry.second);
    auto *Var = Ty ? Ty->getAsVariableState() : nullptr;
    if (Var == nullptr) {
      continue;
    }
    size_t Edges = Var->lowerBounds.size() + Var->upperBounds.size();
    auto &P = NodesEdges[F];
    ++P.first;
    P.second += Edges;
    size_t Self = 0;
    for (const auto &B : Var->lowerBounds) {
      if (binarysub::resolve_variable(B).get() == Ty.get()) {
        ++Self;
      }
    }
    for (const auto &B : Var->upperBounds) {
      if (binarysub::resolve_variable(B).get() == Ty.get()) {
        ++Self;
      }
    }
    SelfRefs[F] += Self;
    MaxBounds[F] = std::max(MaxBounds[F], Edges);
  }
  std::vector<std::pair<llvm::Function *, size_t>> Order;
  for (auto &E : NodesEdges) {
    Order.emplace_back(E.first, E.second.first);
  }
  std::sort(Order.begin(), Order.end(),
            [](const auto &A, const auto &B) { return A.second > B.second; });
  llvm::errs() << "Info: constraint stats for SCC " << Name
               << " (funcs=" << NodesEdges.size() << "):\n";
  for (auto &[F, Nodes] : Order) {
    auto &P = NodesEdges[F];
    llvm::errs() << "  func=" << F->getName() << " nodes=" << P.first
                 << " edges=" << P.second << " self_ref=" << SelfRefs[F]
                 << " max_bounds=" << MaxBounds[F] << "\n";
  }
}

static bool hasTypedPointerExternalValue(const ExtValuePtr &Val) {
  llvm::Type *Ty = nullptr;
  if (auto *V = std::get_if<llvm::Value *>(&Val)) {
    Ty = *V == nullptr ? nullptr : (*V)->getType();
  } else if (auto *Ret = std::get_if<ReturnValue>(&Val)) {
    Ty = Ret->Func == nullptr ? nullptr : Ret->Func->getReturnType();
  } else if (auto *Stack = std::get_if<StackObject>(&Val)) {
    Ty = Stack->Allocator == nullptr ? nullptr : Stack->Allocator->getType();
  } else if (auto *Heap = std::get_if<HeapObject>(&Val)) {
    Ty = Heap->Allocator == nullptr ? nullptr : Heap->Allocator->getType();
  }
  return Ty != nullptr && (Ty->isPointerTy() || Ty->isFunctionTy());
}

// ---- Stack-region marking (experiment, see the header) -------------------
const llvm::AllocaInst *
ConstraintsGenerator::frameOwnerOfValue(const llvm::Value *V) const {
  if (V == nullptr) {
    return nullptr;
  }
  if (auto It = FrameOwnerCache.find(V); It != FrameOwnerCache.end()) {
    return It->second;
  }
  // The __microsub2_opaque_call_ADDR stubs this hook used to specialise no
  // longer exist: an arity-mismatched direct call is lifted as a real call
  // (missing arguments become poison, extra ones are dropped), so there is
  // no body-less call artifact left to share an object argument for.
  // A named stack variable is an object of its own: the lifter marks the GEP
  // that defines it (and only that GEP).  Passing "&local" to a function is a
  // correct transfer of that object, so a marked lvar must not be treated as
  // "the frame region".  NOTDEC_FRAME_MARK_INCLUDES_LVARS=1 restores the coarse
  // behaviour where every frame-derived value counts as the region itself.
  if (!FrameMarkIncludesLvars) {
    if (auto *Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
      if (Inst->getMetadata("microsub2.lvar.id") != nullptr) {
        FrameOwnerCache[V] = nullptr;
        return nullptr;
      }
    }
  }
  const llvm::AllocaInst *Owner = nullptr;
  if (auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
    if (Alloca->getName().starts_with("frame")) {
      Owner = Alloca;
    }
  }
  if (Owner == nullptr) {
    const llvm::Value *Base = nullptr;
    auto TransparentBase = [](unsigned Opcode,
                              const llvm::Value *Op0) -> const llvm::Value * {
      if (Op0 == nullptr) {
        return nullptr;
      }
      if (Opcode == llvm::Instruction::BitCast ||
          Opcode == llvm::Instruction::PtrToInt ||
          Opcode == llvm::Instruction::IntToPtr ||
          Opcode == llvm::Instruction::AddrSpaceCast ||
          Opcode == llvm::Instruction::GetElementPtr) {
        return Op0;
      }
      return nullptr;
    };
    if (auto *Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
      Base = TransparentBase(Inst->getOpcode(),
                             Inst->getNumOperands() > 0 ? Inst->getOperand(0)
                                                        : nullptr);
    } else if (auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(V)) {
      Base = TransparentBase(CE->getOpcode(),
                             CE->getNumOperands() > 0 ? CE->getOperand(0)
                                                      : nullptr);
    }
    if (Base != nullptr) {
      Owner = frameOwnerOfValue(Base);
    }
  }
  FrameOwnerCache[V] = Owner;
  return Owner;
}

ConstraintsGenerator::AddressOrigin
ConstraintsGenerator::addressOriginOfValue(const llvm::Value *V) const {
  if (V == nullptr) {
    return AddressOrigin::Unknown;
  }
  if (auto *Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
    if (Inst->getMetadata("microsub2.lvar.id") != nullptr) {
      return AddressOrigin::Object;
    }
  }
  if (auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
    return Alloca->getName().starts_with("frame") ? AddressOrigin::Frame
                                                  : AddressOrigin::Object;
  }
  if (llvm::isa<llvm::GlobalValue>(V) || llvm::isa<llvm::Argument>(V) ||
      llvm::isa<llvm::Function>(V)) {
    return AddressOrigin::Object;
  }
  const llvm::Value *Base = nullptr;
  auto TransparentBase = [](unsigned Opcode,
                            const llvm::Value *Op0) -> const llvm::Value * {
    if (Op0 == nullptr) {
      return nullptr;
    }
    if (Opcode == llvm::Instruction::BitCast ||
        Opcode == llvm::Instruction::PtrToInt ||
        Opcode == llvm::Instruction::IntToPtr ||
        Opcode == llvm::Instruction::AddrSpaceCast ||
        Opcode == llvm::Instruction::GetElementPtr) {
      return Op0;
    }
    return nullptr;
  };
  if (auto *Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
    Base = TransparentBase(Inst->getOpcode(),
                           Inst->getNumOperands() > 0 ? Inst->getOperand(0)
                                                      : nullptr);
  } else if (auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(V)) {
    Base = TransparentBase(CE->getOpcode(),
                           CE->getNumOperands() > 0 ? CE->getOperand(0)
                                                    : nullptr);
  }
  if (Base == nullptr) {
    return AddressOrigin::Unknown;
  }
  return addressOriginOfValue(Base);
}

std::optional<std::set<const llvm::AllocaInst *>>
ConstraintsGenerator::frameOwnersOfNode(SimpleType Ty) const {
  Ty = binarysub::resolve_variable(Ty);
  if (Ty == nullptr) {
    return std::nullopt;
  }
  const auto &Rev = V2N.rev();
  auto It = Rev.find(Ty);
  if (It == Rev.end()) {
    return std::nullopt;
  }
  std::set<const llvm::AllocaInst *> Owners;
  for (const auto &Val : It->second) {
    const llvm::Value *V = extValueAsValue(Val);
    if (V == nullptr) {
      continue;
    }
    if (const auto *Owner = frameOwnerOfValue(V)) {
      Owners.insert(Owner);
      continue;
    }
    // Relaxed classification: a value whose address cannot be attributed to a
    // known object makes the whole node "unknown", so it can neither count as
    // provably-non-frame nor introduce an owner.
    if (FrameVetoKnownObjectOnly &&
        addressOriginOfValue(V) == AddressOrigin::Unknown) {
      return std::nullopt;
    }
  }
  return Owners;
}

// Diagnostic (NOTDEC_FRAME_VETO_DIAG=1): print the two nodes that a veto
// would block, with the values behind each side and which of them is the
// frame-derived one.  Answers "which merge was refused, and was it a bad value
// poisoning an otherwise legitimate merge".
static bool frameVetoDiagEnabled() {
  static const bool Enabled = [] {
    const char *Value = std::getenv("NOTDEC_FRAME_VETO_DIAG");
    return Value != nullptr && Value[0] != '\0' && Value[0] != '0';
  }();
  return Enabled;
}

void ConstraintsGenerator::dumpFrameVetoSide(const char *Tag,
                                             SimpleType Ty) const {
  Ty = binarysub::resolve_variable(Ty);
  llvm::errs() << "[frame-veto] " << Tag << " ty="
               << (Ty != nullptr ? Ty->str() : std::string("<null>"))
               << " owners=";
  auto Owners = frameOwnersOfNode(Ty);
  if (!Owners.has_value()) {
    llvm::errs() << "unknown";
  } else if (Owners->empty()) {
    llvm::errs() << "none";
  } else {
    for (const auto *Alloca : *Owners) {
      llvm::errs() << Alloca->getName() << ",";
    }
  }
  llvm::errs() << " values=";
  const auto &Rev = V2N.rev();
  auto It = Ty != nullptr ? Rev.find(Ty) : Rev.end();
  if (It != Rev.end()) {
    std::size_t Count = 0;
    for (const auto &Val : It->second) {
      if (Count++ >= 6) {
        llvm::errs() << "...";
        break;
      }
      const llvm::Value *V = extValueAsValue(Val);
      llvm::errs() << toStableString(Val);
      if (V != nullptr && frameOwnerOfValue(V) != nullptr) {
        llvm::errs() << "(FRAME)";
      }
      llvm::errs() << " ";
    }
  }
  llvm::errs() << "\n";
}

bool ConstraintsGenerator::frameMergeVeto(SimpleType LHS, SimpleType RHS,
                                          const char **Why) const {
  if (!FrameNoNonFrameMerge && !FrameNoCrossFunctionMerge) {
    return false;
  }
  const bool Diag = frameVetoDiagEnabled();
  if (Diag) {
    dumpFrameVetoSide("try-LHS", LHS);
    dumpFrameVetoSide("try-RHS", RHS);
  }
  auto Left = frameOwnersOfNode(LHS);
  auto Right = frameOwnersOfNode(RHS);
  if (FrameNoNonFrameMerge) {
    const bool LeftFrame = Left && !Left->empty();
    const bool RightFrame = Right && !Right->empty();
    if ((LeftFrame && Right.has_value() && !RightFrame) ||
        (RightFrame && Left.has_value() && !LeftFrame)) {
      ++FrameVetoNonFrame;
      if (Diag) {
        llvm::errs() << "[frame-veto] VETO nonframe\n";
      }
      if (Why != nullptr) {
        *Why = "frame-region-vs-non-frame-region";
      }
      return true;
    }
    // Diagnostic only: one side is a stack region and the other side has no
    // value bookkeeping at all, so the rule above cannot tell whether it is an
    // unrelated object.  Counts how often the guard stays silent for that
    // reason; see microsub2/logs/20260930-65-record-merge-frame-veto-explained.md.
    if ((LeftFrame && !Right.has_value()) ||
        (RightFrame && !Left.has_value())) {
      ++FrameVetoUnknownSide;
    }
  }
  if (FrameNoCrossFunctionMerge && Left && Right && !Left->empty() &&
      !Right->empty() && *Left != *Right) {
    ++FrameVetoCrossFunction;
    if (Why != nullptr) {
      *Why = "frame-regions-of-different-functions";
    }
    return true;
  }
  return false;
}

void ConstraintsGenerator::attachSharedNodeOwner(SimpleType Ty,
                                                const ExtValuePtr &Val) {
  attachExternalValueHandle(Ty, Val);
  if (std::getenv("NOTDEC_POLY_MONO_DIAG") != nullptr) {
    auto *Owner = getVariableOwningFunction(Ty);
    llvm::errs() << "[poly-mono-owner] shared node owned by "
                 << (Owner != nullptr ? Owner->getName().str()
                                      : std::string("<none>"))
                 << "\n";
  }
}

void ConstraintsGenerator::collectSharedNodeSiblings(SimpleType Shared,
                                                     llvm::Value *Actual) {
  if (Shared == nullptr || Actual == nullptr) {
    return;
  }
  const llvm::Value *Base = llvm::getUnderlyingObject(Actual);
  // Derive the owning function directly: this value may be a constant (e.g.
  // &default_tuning), and ExtValuePtr requires a user for canonicalization.
  llvm::Function *Caller = nullptr;
  if (auto *I = llvm::dyn_cast<llvm::Instruction>(Actual)) {
    Caller = const_cast<llvm::Function *>(I->getFunction());
  } else if (auto *A = llvm::dyn_cast<llvm::Argument>(Actual)) {
    Caller = A->getParent();
  }
  if (Base == nullptr || Caller == nullptr) {
    return;
  }
  std::size_t Collected = 0;
  std::size_t SameObject = 0;
  std::size_t Cands = 0;
  std::size_t NoNode = 0;
  std::size_t GateRejected = 0;
  for (auto &BB : *Caller) {
    for (auto &I : BB) {
      auto *Call = llvm::dyn_cast<llvm::CallBase>(&I);
      if (Call == nullptr) {
        continue;
      }
      for (unsigned Arg = 0; Arg < Call->arg_size(); ++Arg) {
        llvm::Value *Operand = Call->getArgOperand(Arg);
        if (llvm::getUnderlyingObject(Operand) != Base) {
          continue;
        }
        ++SameObject;
        SimpleType Other = getNodeOrNull(getExtValuePtr(Operand, Call, Arg));
        if (Other == nullptr) {
          ++NoNode;
          continue;
        }
        Other = binarysub::resolve_variable(Other);
        if (Other == nullptr || Other.get() == Shared.get()) {
          continue;
        }
        ++Cands;
        // Reuse the policy's own gate: same level, same size, struct-pointer
        // evidence on both sides, and both variables owned by this caller.
        auto ShouldMerge = shouldMergeSameFunctionStructPtrSubtype(Shared, Other);
        if (!ShouldMerge || !ShouldMerge.value()) {
          ++GateRejected;
          continue;
        }
        // Two same-level variable nodes connected in both directions end up in
        // one merge class (see addRemapType), which is what carries the members
        // only the other call paths of the object touch.
        addSubtype(Shared, Other);
        addSubtype(Other, Shared);
        ++Collected;
      }
    }
  }
  if (std::getenv("NOTDEC_POLY_MONO_DIAG") != nullptr) {
    llvm::errs() << "[poly-mono-collect] " << Caller->getName()
                 << " base=" << Base << " same_object_operands=" << SameObject
                 << " no_node=" << NoNode << " candidates=" << Cands
                 << " gate_rejected=" << GateRejected
                 << " collected=" << Collected << "\n";
  }
}

llvm::Function *
ConstraintsGenerator::getVariableOwningFunction(SimpleType Ty) const {
  const auto *Val = getVariableExternalValue(Ty);
  if (Val == nullptr) {
    return nullptr;
  }
  return getExtValueFunction(*Val);
}

binarysub::expected<bool, binarysub::Error>
ConstraintsGenerator::shouldMergeSameFunctionStructPtrSubtype(
    SimpleType LHS, SimpleType RHS) const {
  ++LocalSubtypeHookChecks;
  LHS = binarysub::resolve_variable(LHS);
  RHS = binarysub::resolve_variable(RHS);
  auto *LHSVar = LHS ? LHS->getAsVariableState() : nullptr;
  auto *RHSVar = RHS ? RHS->getAsVariableState() : nullptr;
  if (LHSVar == nullptr || RHSVar == nullptr || LHS.get() == RHS.get()) {
    return false;
  }
  if (LHSVar->level != RHSVar->level || LHSVar->size != RHSVar->size) {
    return false;
  }
  if (frameMergeVeto(LHS, RHS)) {
    return false;
  }
  ++LocalSubtypeBasicCandidates;
  if (LocalSubtypeMode != LocalSubtypeMergeMode::AllLocal &&
      LHSVar->size != PointerSize) {
    return false;
  }

  // Most variable edges connect values owned by different functions. Reject
  // those before walking record fields, which can be large after propagation.
  auto *LHSFunc = getVariableOwningFunction(LHS);
  auto *RHSFunc = getVariableOwningFunction(RHS);
  if (LHSFunc == nullptr || RHSFunc == nullptr || LHSFunc != RHSFunc) {
    return false;
  }
  ++LocalSubtypeSameFunctionCandidates;
  if (LocalSubtypeMode == LocalSubtypeMergeMode::AllLocal) {
    ++LocalSubtypeAccepted;
    return true;
  }
  if (!hasPointerLikeEvidence(LHS) || !hasPointerLikeEvidence(RHS)) {
    return false;
  }
  ++LocalSubtypePointerCandidates;
  if (LocalSubtypeMode == LocalSubtypeMergeMode::Pointer) {
    ++LocalSubtypeAccepted;
    return true;
  }
  if (!hasStructPointerEvidence(LHS) && !hasStructPointerEvidence(RHS)) {
    return false;
  }
  if (!roundOneMergeAllowed(LHS, RHS)) {
    return false;
  }
  ++LocalSubtypeAccepted;
  return true;
}

binarysub::expected<void, binarysub::Error>
ConstraintsGenerator::queueLocalSubtypeMerges(
    SimpleType Var, const binarysub::EnqueueMergeFn &EnqueueMerge) const {
  Var = binarysub::resolve_variable(Var);
  auto *VarState = Var ? Var->getAsVariableState() : nullptr;
  if (VarState == nullptr) {
    return binarysub::expected<void, binarysub::Error>{};
  }

  // A node can get pointer/struct evidence after the variable-variable edge
  // was already processed. Scan direct variable neighbors only when the
  // caller has established that a relevant evidence transition occurred.
  auto TryQueue =
      [&](SimpleType Other) -> binarysub::expected<void, binarysub::Error> {
    // The hook is called for a non-variable bound, but it scans both bound
    // vectors. Reject those entries before root resolution; only variable
    // neighbors can trigger the same-function merge policy.
    if (!Other || !Other->isVariableState()) {
      return binarysub::expected<void, binarysub::Error>{};
    }
    Other = binarysub::resolve_variable(Other);
    auto *OtherState = Other ? Other->getAsVariableState() : nullptr;
    if (OtherState == nullptr || Other.get() == Var.get()) {
      return binarysub::expected<void, binarysub::Error>{};
    }
    auto ShouldMerge = shouldMergeSameFunctionStructPtrSubtype(Var, Other);
    if (!ShouldMerge) {
      return binarysub::unexpected<binarysub::Error>(ShouldMerge.error());
    }
    if (!ShouldMerge.value()) {
      return binarysub::expected<void, binarysub::Error>{};
    }

    auto From = VarState->id <= OtherState->id ? Other : Var;
    auto Into = VarState->id <= OtherState->id ? Var : Other;
    EnqueueMerge(From, Into);
    return binarysub::expected<void, binarysub::Error>{};
  };

  for (const auto &Lower : VarState->lowerBounds) {
    if (!Lower || !Lower->isVariableState()) {
      continue;
    }
    if (auto Result = TryQueue(Lower); !Result) {
      return Result;
    }
  }
  for (const auto &Upper : VarState->upperBounds) {
    if (!Upper || !Upper->isVariableState()) {
      continue;
    }
    if (auto Result = TryQueue(Upper); !Result) {
      return Result;
    }
  }
  return binarysub::expected<void, binarysub::Error>{};
}

bool ConstraintsGenerator::hasStructPointerEvidenceInBound(
    SimpleType Bound) const {
  Bound = binarysub::resolve_variable(Bound);
  auto *Mem = Bound ? Bound->getAsTMemObject() : nullptr;
  if (Mem == nullptr) {
    return false;
  }
  for (const auto &[FieldName, FieldTy] : Mem->fields) {
    auto Offset = parseConstantFieldOffset(FieldName);
    if (!Offset || *Offset < 4) {
      continue;
    }
    if (collectMaxDirectFieldAccessSizeBytes(FieldTy).has_value()) {
      return true;
    }
  }
  return false;
}

bool ConstraintsGenerator::hasPointerLikeEvidenceBeforeNonVarBound(
    SimpleType Ty, binarysub::BoundPolarity Polarity) const {
  Ty = binarysub::resolve_variable(Ty);
  auto *Var = Ty ? Ty->getAsVariableState() : nullptr;
  if (Var == nullptr || Var->size != PointerSize) {
    return false;
  }
  if (const auto *Val = getVariableExternalValue(Ty)) {
    if (hasTypedPointerExternalValue(*Val)) {
      return true;
    }
  }

  auto IsPointerLikeBound = [](SimpleType Bound) {
    Bound = binarysub::resolve_variable(Bound);
    return Bound && (Bound->getAsTMemObject() != nullptr ||
                     Bound->getAsTFunction() != nullptr);
  };
  auto HasEvidence = [&](const std::vector<SimpleType> &Bounds,
                         bool SkipLast) {
    const auto Limit =
        SkipLast && !Bounds.empty() ? Bounds.size() - 1 : Bounds.size();
    for (std::size_t I = 0; I < Limit; ++I) {
      if (IsPointerLikeBound(Bounds[I])) {
        return true;
      }
    }
    return false;
  };
  return HasEvidence(Var->lowerBounds,
                     Polarity == binarysub::BoundPolarity::Lower) ||
         HasEvidence(Var->upperBounds,
                     Polarity == binarysub::BoundPolarity::Upper);
}

bool ConstraintsGenerator::hasStructPointerEvidenceBeforeNonVarBound(
    SimpleType Ty, binarysub::BoundPolarity Polarity) const {
  Ty = binarysub::resolve_variable(Ty);
  auto *Var = Ty ? Ty->getAsVariableState() : nullptr;
  if (Var == nullptr || Var->size != PointerSize) {
    return false;
  }
  auto HasEvidence = [&](const std::vector<SimpleType> &Bounds,
                         bool SkipLast) {
    const auto Limit =
        SkipLast && !Bounds.empty() ? Bounds.size() - 1 : Bounds.size();
    for (std::size_t I = 0; I < Limit; ++I) {
      if (hasStructPointerEvidenceInBound(Bounds[I])) {
        return true;
      }
    }
    return false;
  };
  return HasEvidence(Var->lowerBounds,
                     Polarity == binarysub::BoundPolarity::Lower) ||
         HasEvidence(Var->upperBounds,
                     Polarity == binarysub::BoundPolarity::Upper);
}

binarysub::expected<void, binarysub::Error>
ConstraintsGenerator::onVariableNonVarBoundAdded(
    SimpleType Var, SimpleType Bound, binarysub::BoundPolarity Polarity,
    const binarysub::EnqueueMergeFn &EnqueueMerge) const {
  Var = binarysub::resolve_variable(Var);
  auto *VarState = Var ? Var->getAsVariableState() : nullptr;
  if (VarState == nullptr || VarState->size != PointerSize ||
      Bound == nullptr || Bound->isVariableState()) {
    return binarysub::expected<void, binarysub::Error>{};
  }

  // all-local is an opt-in diagnostic mode whose trigger behavior is kept
  // unchanged. The evidence transition gate is only for the pointer policies.
  const bool BoundCanAddPointerEvidence =
      Bound->getAsTMemObject() != nullptr || Bound->getAsTFunction() != nullptr;
  if (LocalSubtypeMode == LocalSubtypeMergeMode::AllLocal) {
    if (!hasPointerLikeEvidence(Var)) {
      return binarysub::expected<void, binarysub::Error>{};
    }
    return queueLocalSubtypeMerges(Var, EnqueueMerge);
  }
  if (!BoundCanAddPointerEvidence) {
    return binarysub::expected<void, binarysub::Error>{};
  }

  // The callback runs after the new bound was appended. Compare against the
  // preceding vector contents by ignoring its just-appended tail entry. This
  // keeps the gate transaction-safe without storing policy state on nodes.
  const bool HadPointerEvidence =
      hasPointerLikeEvidenceBeforeNonVarBound(Var, Polarity);
  const bool PointerEvidenceUpgraded = !HadPointerEvidence;
  if (LocalSubtypeMode == LocalSubtypeMergeMode::Pointer) {
    if (!PointerEvidenceUpgraded) {
      return binarysub::expected<void, binarysub::Error>{};
    }
    return queueLocalSubtypeMerges(Var, EnqueueMerge);
  }

  const bool HadStructEvidence =
      hasStructPointerEvidenceBeforeNonVarBound(Var, Polarity);
  const bool HasStructEvidence = hasStructPointerEvidence(Var);
  if (!PointerEvidenceUpgraded &&
      !(HasStructEvidence && !HadStructEvidence)) {
    return binarysub::expected<void, binarysub::Error>{};
  }
  return queueLocalSubtypeMerges(Var, EnqueueMerge);
}

binarysub::expected<void, binarysub::Error>
ConstraintsGenerator::onVariableNestedBoundRewritten(
    SimpleType User, SimpleType OldBound, SimpleType NewBound,
    const binarysub::EnqueueMergeFn &EnqueueMerge) const {
  if (LocalSubtypeMode != LocalSubtypeMergeMode::StructPointer) {
    return binarysub::expected<void, binarysub::Error>{};
  }
  User = binarysub::resolve_variable(User);
  auto *UserState = User ? User->getAsVariableState() : nullptr;
  if (UserState == nullptr || UserState->size != PointerSize) {
    return binarysub::expected<void, binarysub::Error>{};
  }
  if (hasStructPointerEvidenceInBound(OldBound) ||
      !hasStructPointerEvidenceInBound(NewBound)) {
    return binarysub::expected<void, binarysub::Error>{};
  }
  return queueLocalSubtypeMerges(User, EnqueueMerge);
}

std::optional<std::vector<ConstraintsGenerator::StructFieldSlice>>
ConstraintsGenerator::collectOneLevelStructFieldSlices(SimpleType Ty) const {
  Ty = binarysub::resolve_variable(Ty);
  auto *Var = Ty ? Ty->getAsVariableState() : nullptr;
  if (Var == nullptr || Var->size != PointerSize) {
    return std::nullopt;
  }

  std::map<uint64_t, uint64_t> MaxSizeByOffset;
  auto CollectBounds = [&](const std::vector<SimpleType> &Bounds) {
    for (const auto &Bound : Bounds) {
      auto ResolvedBound = binarysub::resolve_variable(Bound);
      auto *Mem = ResolvedBound ? ResolvedBound->getAsTMemObject() : nullptr;
      if (Mem == nullptr) {
        continue;
      }
      for (const auto &[FieldName, FieldTy] : Mem->fields) {
        auto Offset = parseConstantFieldOffset(FieldName);
        if (!Offset || *Offset < 4) {
          continue;
        }
        auto ResolvedFieldTy = binarysub::resolve_variable(FieldTy);
        if (!ResolvedFieldTy) {
          continue;
        }
        auto SizeBytes =
            collectMaxDirectFieldAccessSizeBytes(ResolvedFieldTy);
        if (!SizeBytes) {
          continue;
        }
        auto &CurrentMax = MaxSizeByOffset[*Offset];
        CurrentMax = std::max(CurrentMax, *SizeBytes);
      }
    }
  };

  CollectBounds(Var->lowerBounds);
  CollectBounds(Var->upperBounds);
  if (MaxSizeByOffset.empty()) {
    return std::nullopt;
  }
  std::vector<StructFieldSlice> Slices;
  Slices.reserve(MaxSizeByOffset.size());
  for (const auto &[Offset, SizeBytes] : MaxSizeByOffset) {
    Slices.push_back(StructFieldSlice{.Offset = Offset, .SizeBytes = SizeBytes});
  }
  return Slices;
}

std::optional<uint64_t>
ConstraintsGenerator::collectMaxDirectFieldAccessSizeBytes(
    SimpleType FieldAddrTy) const {
  uint64_t MaxBits = 0;
  auto CollectAccessSize = [&](SimpleType Ty) {
    Ty = binarysub::resolve_variable(Ty);
    if (!Ty) {
      return;
    }
    auto Consider = [&](const binarysub::AccessType *Access) {
      if (Access == nullptr || Access->Size == 0 || Access->Size % 8 != 0) {
        return;
      }
      MaxBits = std::max<uint64_t>(MaxBits, Access->Size);
    };
    Consider(Ty->getAsPtrLoad());
    Consider(Ty->getAsPtrStore());
  };

  FieldAddrTy = binarysub::resolve_variable(FieldAddrTy);
  CollectAccessSize(FieldAddrTy);

  auto *Var = FieldAddrTy ? FieldAddrTy->getAsVariableState() : nullptr;
  if (Var != nullptr) {
    auto CollectBounds = [&](const std::vector<SimpleType> &Bounds) {
      for (const auto &Bound : Bounds) {
        CollectAccessSize(Bound);
      }
    };
    CollectBounds(Var->lowerBounds);
    CollectBounds(Var->upperBounds);
  }

  if (MaxBits == 0) {
    return std::nullopt;
  }
  return MaxBits / 8;
}

std::vector<SimpleType>
ConstraintsGenerator::collectDirectStructPointerAccessTargets(
    SimpleType FieldAddrTy) const {
  std::vector<SimpleType> Targets;
  std::set<const binarysub::TypeNode *> Seen;

  auto ConsiderAccess = [&](const binarysub::AccessType *Access) {
    if (Access == nullptr || Access->Size != PointerSize) {
      return;
    }
    auto Target = binarysub::resolve_variable(Access->to);
    auto *TargetVar = Target ? Target->getAsVariableState() : nullptr;
    if (TargetVar == nullptr || TargetVar->size != PointerSize) {
      return;
    }
    if (!hasStructPointerEvidence(Target)) {
      return;
    }
    if (Seen.insert(Target.get()).second) {
      Targets.push_back(Target);
    }
  };

  auto CollectAccesses = [&](SimpleType Ty) {
    Ty = binarysub::resolve_variable(Ty);
    if (!Ty) {
      return;
    }
    ConsiderAccess(Ty->getAsPtrLoad());
    ConsiderAccess(Ty->getAsPtrStore());
  };

  FieldAddrTy = binarysub::resolve_variable(FieldAddrTy);
  CollectAccesses(FieldAddrTy);

  auto *Var = FieldAddrTy ? FieldAddrTy->getAsVariableState() : nullptr;
  if (Var != nullptr) {
    for (const auto &Bound : Var->lowerBounds) {
      CollectAccesses(Bound);
    }
    for (const auto &Bound : Var->upperBounds) {
      CollectAccesses(Bound);
    }
  }

  return Targets;
}

std::map<uint64_t, std::vector<SimpleType>>
ConstraintsGenerator::collectOneLevelStructPtrFieldTargetGroups(
    SimpleType Ty) const {
  Ty = binarysub::resolve_variable(Ty);
  auto *Var = Ty ? Ty->getAsVariableState() : nullptr;
  if (Var == nullptr || Var->size != PointerSize) {
    return {};
  }

  std::map<uint64_t, std::vector<SimpleType>> TargetsByOffset;
  auto CollectBounds = [&](const std::vector<SimpleType> &Bounds) {
    for (const auto &Bound : Bounds) {
      auto ResolvedBound = binarysub::resolve_variable(Bound);
      auto *Mem = ResolvedBound ? ResolvedBound->getAsTMemObject() : nullptr;
      if (Mem == nullptr) {
        continue;
      }
      for (const auto &[FieldName, FieldTy] : Mem->fields) {
        auto Offset = parseConstantFieldOffset(FieldName);
        if (!Offset || *Offset < 4) {
          continue;
        }
        auto Targets = collectDirectStructPointerAccessTargets(FieldTy);
        auto &Slot = TargetsByOffset[*Offset];
        for (const auto &Target : Targets) {
          auto Resolved = binarysub::resolve_variable(Target);
          if (!Resolved) {
            continue;
          }
          bool AlreadySeen =
              std::any_of(Slot.begin(), Slot.end(),
                          [&](const SimpleType &Existing) {
                            auto ExistingResolved =
                                binarysub::resolve_variable(Existing);
                            return ExistingResolved &&
                                   ExistingResolved.get() == Resolved.get();
                          });
          if (!AlreadySeen) {
            Slot.push_back(Resolved);
          }
        }
      }
    }
  };

  CollectBounds(Var->lowerBounds);
  CollectBounds(Var->upperBounds);
  for (auto &[_, Targets] : TargetsByOffset) {
    std::sort(Targets.begin(), Targets.end(),
              [](const SimpleType &LHS, const SimpleType &RHS) {
                auto L = binarysub::resolve_variable(LHS);
                auto R = binarysub::resolve_variable(RHS);
                auto *LV = L ? L->getAsVariableState() : nullptr;
                auto *RV = R ? R->getAsVariableState() : nullptr;
                if (LV != nullptr && RV != nullptr && LV->id != RV->id) {
                  return LV->id < RV->id;
                }
                return L.get() < R.get();
              });
  }
  return TargetsByOffset;
}

std::vector<ConstraintsGenerator::StructFieldFollowupMergeCandidate>
ConstraintsGenerator::collectStructPtrFieldFollowupMergeCandidatesForMergedOwner(
    SimpleType Owner) const {
  const bool MeasureSubphases =
      ConstraintDiagEnabled && ActiveBottomUpTimingContext != nullptr;
  BottomUpPhaseStart CollectStart;
  if (MeasureSubphases) {
    CollectStart = startBottomUpPhase(/*Enabled=*/true);
  }
  std::vector<StructFieldFollowupMergeCandidate> Candidates;
  Owner = binarysub::resolve_variable(Owner);
  auto TargetGroups = collectOneLevelStructPtrFieldTargetGroups(Owner);
  std::set<std::tuple<const binarysub::TypeNode *,
                      const binarysub::TypeNode *, uint64_t>>
      Seen;

  for (const auto &[Offset, Targets] : TargetGroups) {
    if (Targets.size() < 2) {
      continue;
    }
    for (std::size_t I = 0; I < Targets.size(); ++I) {
      for (std::size_t J = I + 1; J < Targets.size(); ++J) {
        auto LHS = binarysub::resolve_variable(Targets[I]);
        auto RHS = binarysub::resolve_variable(Targets[J]);
        auto *LHSVar = LHS ? LHS->getAsVariableState() : nullptr;
        auto *RHSVar = RHS ? RHS->getAsVariableState() : nullptr;
        if (LHSVar == nullptr || RHSVar == nullptr || LHS.get() == RHS.get()) {
          continue;
        }
        if (LHSVar->level != RHSVar->level || LHSVar->size != RHSVar->size ||
            LHSVar->size != PointerSize) {
          continue;
        }
        if (!hasNonConflictingStructFieldSlices(LHS, RHS,
                                                /*RequireEvidence=*/true)) {
          continue;
        }

        auto FromTarget = LHSVar->id <= RHSVar->id ? RHS : LHS;
        auto IntoTarget = LHSVar->id <= RHSVar->id ? LHS : RHS;
        if (!Seen.insert({FromTarget.get(), IntoTarget.get(), Offset}).second) {
          continue;
        }
        Candidates.push_back(StructFieldFollowupMergeCandidate{
            .FromOwner = Owner,
            .IntoOwner = Owner,
            .Offset = Offset,
            .FromFieldTarget = FromTarget,
            .IntoFieldTarget = IntoTarget});
      }
    }
  }
  if (MeasureSubphases) {
    recordBottomUpSubphase(ActiveBottomUpTimingContext,
                           BottomUpSubstage::FieldFollowupCandidateCollect,
                           CollectStart, Candidates.size(),
                           ActiveBottomUpPostSummaryContext);
  }
  return Candidates;
}

std::size_t ConstraintsGenerator::applyStructPtrFieldFollowupMergePolicy() {
  std::size_t Merged = 0;
  while (true) {
    const bool MeasureSubphases =
        ConstraintDiagEnabled && ActiveBottomUpTimingContext != nullptr;
    BottomUpPhaseStart ApplyStart;
    if (MeasureSubphases) {
      ApplyStart = startBottomUpPhase(/*Enabled=*/true);
    }
    bool Changed = false;
    std::set<std::tuple<const binarysub::TypeNode *,
                        const binarysub::TypeNode *, uint64_t>>
        Seen;
    for (const auto &Candidate : StructFieldFollowupMergeCandidates) {
      auto FromTarget = binarysub::resolve_variable(Candidate.FromFieldTarget);
      auto IntoTarget = binarysub::resolve_variable(Candidate.IntoFieldTarget);
      auto *FromTargetVar =
          FromTarget ? FromTarget->getAsVariableState() : nullptr;
      auto *IntoTargetVar =
          IntoTarget ? IntoTarget->getAsVariableState() : nullptr;
      if (FromTargetVar == nullptr || IntoTargetVar == nullptr ||
          FromTarget.get() == IntoTarget.get()) {
        continue;
      }
      if (FromTargetVar->level != IntoTargetVar->level ||
          FromTargetVar->size != IntoTargetVar->size ||
          FromTargetVar->size != PointerSize) {
        continue;
      }
      if (!Seen.insert({FromTarget.get(), IntoTarget.get(), Candidate.Offset})
               .second) {
        continue;
      }
      if (!hasNonConflictingStructFieldSlices(FromTarget, IntoTarget,
                                              /*RequireEvidence=*/true)) {
        continue;
      }
      std::string Detail = "owner-from=";
      Detail += binarysub::debug_string(
          binarysub::resolve_variable(Candidate.FromOwner));
      Detail += " owner-into=";
      Detail += binarysub::debug_string(
          binarysub::resolve_variable(Candidate.IntoOwner));
      Detail += " offset=" + std::to_string(Candidate.Offset);
      if (!tryMergeVariablesForPolicy("struct-field-followup", FromTarget,
                                      IntoTarget, Detail)) {
        continue;
      }
      ++Merged;
      Changed = true;
      break;
    }
    if (MeasureSubphases) {
      recordBottomUpSubphase(ActiveBottomUpTimingContext,
                             BottomUpSubstage::FieldFollowupCandidateApply,
                             ApplyStart,
                             StructFieldFollowupMergeCandidates.size(),
                             ActiveBottomUpPostSummaryContext);
    }
    if (!Changed) {
      return Merged;
    }
  }
}

bool ConstraintsGenerator::hasNonConflictingStructFieldSlices(
    SimpleType LHS, SimpleType RHS, bool RequireEvidence) const {
  if (frameMergeVeto(LHS, RHS)) {
    return false;
  }
  return !explainStructFieldSliceCompatibilityFailure(LHS, RHS,
                                                      RequireEvidence)
              .has_value();
}

bool ConstraintsGenerator::roundOneMergeAllowed(SimpleType LHS,
                                                SimpleType RHS) const {
  if (!TwoRoundMerge || RoundTwoRecheck) {
    return true;
  }
  LHS = binarysub::resolve_variable(LHS);
  RHS = binarysub::resolve_variable(RHS);
  auto Left = collectOneLevelStructFieldSlices(LHS);
  auto Right = collectOneLevelStructFieldSlices(RHS);
  if (!Left || !Right || Left->empty() || Right->empty()) {
    deferRoundOneMergePair(LHS, RHS);
    return false;
  }
  auto L = *Left;
  auto R = *Right;
  std::sort(L.begin(), L.end());
  std::sort(R.begin(), R.end());
  // StructFieldSlice only defines operator<; compare with that ordering.
  auto SameSlice = [](const StructFieldSlice &A, const StructFieldSlice &B) {
    return !(A < B) && !(B < A);
  };
  if (L.size() != R.size() ||
      !std::equal(L.begin(), L.end(), R.begin(), SameSlice)) {
    deferRoundOneMergePair(LHS, RHS);
    return false;
  }
  // Identical (offset,width) slices are not enough: two different objects can
  // share a layout.  Round one also requires the field targets to be the same
  // nodes, so only a merge whose evidence already agrees completely passes.
  auto LeftTargets = collectOneLevelStructPtrFieldTargetGroups(LHS);
  auto RightTargets = collectOneLevelStructPtrFieldTargetGroups(RHS);
  if (LeftTargets.size() != RightTargets.size()) {
    deferRoundOneMergePair(LHS, RHS);
    return false;
  }
  for (const auto &[Offset, Targets] : LeftTargets) {
    auto It = RightTargets.find(Offset);
    if (It == RightTargets.end() || It->second.size() != Targets.size()) {
      deferRoundOneMergePair(LHS, RHS);
      return false;
    }
    std::vector<const binarysub::TypeNode *> LeftNodes;
    std::vector<const binarysub::TypeNode *> RightNodes;
    for (const auto &Target : Targets) {
      LeftNodes.push_back(binarysub::resolve_variable(Target).get());
    }
    for (const auto &Target : It->second) {
      RightNodes.push_back(binarysub::resolve_variable(Target).get());
    }
    std::sort(LeftNodes.begin(), LeftNodes.end());
    std::sort(RightNodes.begin(), RightNodes.end());
    if (LeftNodes != RightNodes) {
      deferRoundOneMergePair(LHS, RHS);
      return false;
    }
  }
  return true;
}

void ConstraintsGenerator::deferRoundOneMergePair(SimpleType LHS,
                                                  SimpleType RHS) const {
  ++RoundOneMergeRefused;
  // The list is walked once in round two; keep it bounded for pathological
  // inputs where almost every candidate is refused.
  constexpr std::size_t kMaxDeferredRoundOneMerges = 200000;
  if (DeferredRoundOneMerges.size() < kMaxDeferredRoundOneMerges) {
    DeferredRoundOneMerges.emplace_back(LHS, RHS);
  }
}

void ConstraintsGenerator::recheckDeferredRoundOneMerges() {
  if (!TwoRoundMerge || DeferredRoundOneMerges.empty()) {
    return;
  }
  std::vector<std::pair<SimpleType, SimpleType>> Deferred;
  Deferred.swap(DeferredRoundOneMerges);
  std::size_t Accepted = 0;
  RoundTwoRecheck = true;
  for (const auto &[Left, Right] : Deferred) {
    auto LHS = binarysub::resolve_variable(Left);
    auto RHS = binarysub::resolve_variable(Right);
    auto *LHSVar = LHS ? LHS->getAsVariableState() : nullptr;
    auto *RHSVar = RHS ? RHS->getAsVariableState() : nullptr;
    if (LHSVar == nullptr || RHSVar == nullptr || LHS.get() == RHS.get()) {
      continue;
    }
    if (LHSVar->level != RHSVar->level || LHSVar->size != RHSVar->size) {
      continue;
    }
    ++RoundTwoMergeConsidered;
    if (!hasNonConflictingStructFieldSlices(LHS, RHS,
                                            /*RequireEvidence=*/true)) {
      continue;
    }
    auto From = LHSVar->id <= RHSVar->id ? RHS : LHS;
    auto Into = LHSVar->id <= RHSVar->id ? LHS : RHS;
    if (tryMergeVariablesForPolicy("two-round", From, Into,
                                   "round-one-deferred")) {
      ++Accepted;
    }
  }
  RoundTwoRecheck = false;
  RoundTwoMergeAccepted += Accepted;
  if (TwoRoundMergeDiag) {
    llvm::errs() << "[two-round] refused-round-one=" << RoundOneMergeRefused
                 << " deferred=" << Deferred.size() << " accepted=" << Accepted
                 << "\n";
  }
}


static std::string formatStructFieldSlicesForTrace(
    const std::optional<std::vector<ConstraintsGenerator::StructFieldSlice>>
        &Slices) {
  if (!Slices) {
    return "<none>";
  }
  std::ostringstream OS;
  OS << "[";
  for (std::size_t I = 0; I < Slices->size(); ++I) {
    if (I != 0) {
      OS << ",";
    }
    OS << "@" << (*Slices)[I].Offset << ":" << (*Slices)[I].SizeBytes;
  }
  OS << "]";
  return OS.str();
}

// Experiment switch (NOTDEC_LAYOUT_OVERLAP_MODE): which struct-field slice
// overlaps veto a merge between two pointer roots.
//   0: every overlapping pair of different slices is a conflict.
//   1: only partially overlapping pairs (neither range contains the other);
//      nested access views of the same bytes are allowed through.
//   2: never veto on overlap; the HType layout organises the views later.
//      两个访问视图落在同一批字节上是对同一对象的证据，不足以把两个类型节点
//      分开；分开只会让两侧都变浅（常见结果是 void*）。但放行所有重叠会让同一
//      批字节上的不同解释合并成一个大 union（见 log 52 的 §50.1 O3/split）。
//   3: like 1, but only for call-argument slot groups (see the callers).
//   4 (default): like 3, but nesting is allowed only for a byte-granular
//      (1-byte) view.  全库 A/B（log 52 §50）里 mode 4 在汇总 F1、precision、
//      per-case 均值三项上都优于 mode 0/2。
static int layoutOverlapMode() {
  static const int Mode = [] {
    const char *Value = std::getenv("NOTDEC_LAYOUT_OVERLAP_MODE");
    return Value == nullptr ? 4 : std::atoi(Value);
  }();
  return Mode;
}

std::optional<std::string>
ConstraintsGenerator::explainStructFieldSliceCompatibilityFailure(
    SimpleType LHS, SimpleType RHS, bool RequireEvidence,
    int OverlapMode) const {
  int Mode = OverlapMode < 0 ? layoutOverlapMode() : OverlapMode;
  // 3 selects "relax only where the caller asked for it"; callers that did not
  // resolve it keep the strict rule.
  if (Mode == 3) {
    Mode = 0;
  }
  auto Left = collectOneLevelStructFieldSlices(LHS);
  auto Right = collectOneLevelStructFieldSlices(RHS);
  if (RequireEvidence && !Left && !Right) {
    return "missing-struct-evidence lhs=<none> rhs=<none>";
  }
  if (!Left || !Right) {
    return std::nullopt;
  }

  for (const auto &L : *Left) {
    for (const auto &R : *Right) {
      auto LEnd = L.Offset + L.SizeBytes;
      auto REnd = R.Offset + R.SizeBytes;
      bool Overlap = std::max(L.Offset, R.Offset) < std::min(LEnd, REnd);
      bool SameSlice = L.Offset == R.Offset && L.SizeBytes == R.SizeBytes;
      bool Nested = (L.Offset <= R.Offset && LEnd >= REnd) ||
                    (R.Offset <= L.Offset && REnd >= LEnd);
      bool Conflict = false;
      if (Overlap && !SameSlice) {
        switch (Mode) {
        case 2:
          Conflict = false;
          break;
        case 1:
          Conflict = !Nested;
          break;
        case 4:
          // Only a byte-granular view may sit inside a wider access; two wider
          // views that disagree about the same bytes stay a conflict.
          Conflict = !(Nested && std::min(L.SizeBytes, R.SizeBytes) == 1);
          break;
        default:
          Conflict = true;
          break;
        }
      }
      if (Conflict) {
        std::ostringstream OS;
        OS << "overlap-conflict lhs=@" << L.Offset << ":" << L.SizeBytes
           << " rhs=@" << R.Offset << ":" << R.SizeBytes
           << " lhs-slices=" << formatStructFieldSlicesForTrace(Left)
           << " rhs-slices=" << formatStructFieldSlicesForTrace(Right);
        return OS.str();
      }
    }
  }
  return std::nullopt;
}

bool ConstraintsGenerator::hasStructPointerEvidence(SimpleType Ty) const {
  Ty = binarysub::resolve_variable(Ty);
  auto *Var = Ty ? Ty->getAsVariableState() : nullptr;
  if (Var == nullptr || Var->size != PointerSize) {
    return false;
  }
  auto It = StructEvidenceMemo.find(Ty.get());
  if (It != StructEvidenceMemo.end()) {
    return It->second;
  }

  // This query only needs existence. Avoid constructing the complete offset map
  // used by layout conflict checks, especially for large propagated records.
  bool Result = false;
  for (const auto &Bound : Var->lowerBounds) {
    if (hasStructPointerEvidenceInBound(Bound)) {
      Result = true;
      break;
    }
  }
  if (!Result) {
    for (const auto &Bound : Var->upperBounds) {
      if (hasStructPointerEvidenceInBound(Bound)) {
        Result = true;
        break;
      }
    }
  }
  StructEvidenceMemo.emplace(Ty.get(), Result);
  return Result;
}

bool ConstraintsGenerator::hasPointerLikeEvidence(SimpleType Ty) const {
  Ty = binarysub::resolve_variable(Ty);
  auto *Var = Ty ? Ty->getAsVariableState() : nullptr;
  if (Var == nullptr || Var->size != PointerSize) {
    return false;
  }

  if (const auto *Val = getVariableExternalValue(Ty)) {
    if (hasTypedPointerExternalValue(*Val)) {
      return true;
    }
  }

  // Keep this shallow.  The same-function policy only needs to reject ordinary
  // pointer-sized integers; struct evidence is checked by the caller.
  auto IsPointerLikeBound = [](SimpleType Bound) {
    Bound = binarysub::resolve_variable(Bound);
    return Bound && (Bound->getAsTMemObject() != nullptr ||
                     Bound->getAsTFunction() != nullptr);
  };

  for (const auto &Lower : Var->lowerBounds) {
    if (IsPointerLikeBound(Lower)) {
      return true;
    }
  }
  for (const auto &Upper : Var->upperBounds) {
    if (IsPointerLikeBound(Upper)) {
      return true;
    }
  }
  return false;
}

static std::string
formatCallArgStructPtrMergeCandidateDetail(
    const ConstraintsGenerator::CallArgStructPtrMergeCandidate &Candidate) {
  std::string Detail = "call=";
  Detail += Candidate.Call ? toStableString(ExtValuePtr{Candidate.Call})
                           : "<unknown>";
  Detail += " target=";
  Detail += Candidate.Target && Candidate.Target->hasName()
                ? Candidate.Target->getName().str()
                : "<unknown>";
  Detail += " arg=" + std::to_string(Candidate.ArgIndex);
  return Detail;
}

static std::string formatSimpleTypeForTrace(SimpleType Ty) {
  Ty = binarysub::resolve_variable(Ty);
  return Ty ? binarysub::debug_string(Ty) : "<null>";
}

static std::string formatCallSlotRootSnapshot(ConstraintsGenerator &CG,
                                              SimpleType Ty) {
  Ty = binarysub::resolve_variable(Ty);
  auto *Var = Ty ? Ty->getAsVariableState() : nullptr;
  if (Var == nullptr) {
    return Ty ? binarysub::debug_string(Ty) : "<null>";
  }

  std::ostringstream OS;
  OS << "vs#" << Var->id << " level=" << Var->level
     << " size_bits=" << Var->size << " slices="
     << formatStructFieldSlicesForTrace(
            CG.collectOneLevelStructFieldSlices(Ty));
  return OS.str();
}

struct CallSlotMergeEntry {
  llvm::Function *Target = nullptr;
  std::string Slot;
  std::string Policy;
  SimpleType Actual = nullptr;
  SimpleType Formal = nullptr;
  std::string Detail;
  bool RequireStructEvidence = false;
};

struct CallSlotMergeGroup {
  llvm::Function *Target = nullptr;
  SimpleType Formal = nullptr;
  std::set<std::string> Slots;
  std::set<std::string> Policies;
  std::vector<CallSlotMergeEntry> Entries;
};

struct CallFunctionMergePlan {
  llvm::Function *Target = nullptr;
  std::size_t FirstEntryOrder = std::numeric_limits<std::size_t>::max();
  std::size_t FirstArgOrder = std::numeric_limits<std::size_t>::max();
  std::vector<CallSlotMergeGroup> Groups;
};

// Formal labels are collected before any function transaction starts. The raw
// node is kept so resolving it later follows committed merges and lets a later
// transaction see formal-slot identities inherited from earlier propagation.
struct CallFormalSlotLabel {
  SimpleType Formal = nullptr;
  std::string Label;
};

struct CallSlotMergeRoot {
  SimpleType Type = nullptr;
  // ReturnInst operands keep their historical unconditional equality with the
  // function-return root. Cross-call members only merge when the group has
  // struct evidence.
  bool MergeWithoutStructEvidence = false;
};

// A prepared group keeps the exact pre-transaction evidence used for both the
// decision and the debug file. RootActions is filled while the transaction runs
// but the snapshots themselves never change after the checkpoint starts.
struct PreparedCallSlotMergeGroup {
  CallSlotMergeGroup *Group = nullptr;
  std::vector<CallSlotMergeRoot> Roots;
  std::vector<std::string> EntrySnapshots;
  std::vector<std::string> RootSnapshots;
  std::vector<std::string> RootActions;
  std::string FormalSnapshot;
  bool HasStructEvidence = false;
  std::optional<std::string> Failure;
  std::optional<std::string> SkipReason;
};

struct PendingCallPolicyMerge {
  std::string Policy;
  SimpleType From = nullptr;
  SimpleType Into = nullptr;
  std::string Detail;
  std::vector<ExtValuePtr> MovedValues;
};

// PNDiff must be updated only after a transaction commits, but its value sets
// must be captured before SimpleType roots and V2N keys are changed.
struct PendingPNDiffUnify {
  SimpleType Lhs = nullptr;
  SimpleType Rhs = nullptr;
  std::vector<ExtValuePtr> LeftValues;
  std::vector<ExtValuePtr> RightValues;
};

struct PendingCallMergeEffects {
  std::vector<PendingCallPolicyMerge> PolicyMerges;
  std::vector<PendingPNDiffUnify> PNDiffUnifies;
  std::vector<std::string> MergeAttempts;
};

static std::string formatStringSet(const std::set<std::string> &Values) {
  std::string Out;
  for (const auto &Value : Values) {
    if (!Out.empty()) {
      Out += ",";
    }
    Out += Value;
  }
  return Out.empty() ? "<unknown>" : Out;
}

static std::string formatCallMergeTarget(const llvm::Function *Target) {
  return Target && Target->hasName() ? Target->getName().str() : "<unknown>";
}

// Check every slot before opening the transaction, then execute all parameter
// and return merges for one target function under one SimpleType checkpoint.
// A recursive failure rolls every already-processed slot back together.
static std::size_t applyTransactionalCallSlotMergePolicy(
    ConstraintsGenerator &CG, std::vector<CallSlotMergeEntry> Entries) {
  if (const char *OnlyFn = std::getenv("NOTDEC_FATNODE_FUNC")) {
    std::map<SimpleType, std::vector<ExtValuePtr>> ByRoot;
    for (const auto &Ent : CG.V2N.rev()) {
      auto Root = binarysub::resolve_variable(Ent.first);
      if (Root) {
        auto &V = ByRoot[Root];
        V.insert(V.end(), Ent.second.begin(), Ent.second.end());
      }
    }
    unsigned Printed = 0;
    for (const auto &[Root, Vals] : ByRoot) {
      bool Match = false;
      for (const auto &V : Vals) {
        llvm::Function *Owner = nullptr;
        if (auto *VP = std::get_if<llvm::Value *>(&V)) {
          if (auto *Inst = llvm::dyn_cast<llvm::Instruction>(*VP)) {
            Owner = Inst->getFunction();
          } else if (auto *Arg = llvm::dyn_cast<llvm::Argument>(*VP)) {
            Owner = Arg->getParent();
          }
        }
        if (Owner != nullptr && Owner->getName() == OnlyFn) {
          Match = true;
          break;
        }
      }
      if (!Match) {
        continue;
      }
      auto *VS = Root ? Root->getAsVariableState() : nullptr;
      llvm::errs() << "[fat-state] root=" << binarysub::debug_string(Root)
                   << " lb=" << (VS ? VS->lowerBounds.size() : 0)
                   << " ub=" << (VS ? VS->upperBounds.size() : 0)
                   << " members=" << Vals.size() << "\n";
      if (++Printed >= 4) {
        break;
      }
    }
  }
  if (std::getenv("NOTDEC_GRAPH_SIZE_DIAG") != nullptr) {
    binarysub::emit_type_graph_summary("pre-policy");
    llvm::errs() << "[graph-size] v2n=" << CG.V2N.rev().size()
                 << " decisions=" << CG.CallSlotMergeDecisions.size() << "\n";
  }
  const bool MeasureSubphases =
      CG.ConstraintDiagEnabled && ActiveBottomUpTimingContext != nullptr;
  auto RecordSubphase = [&](BottomUpSubstage Stage,
                            const BottomUpPhaseStart &Start,
                            std::uint64_t WorkItems) {
    if (MeasureSubphases) {
      recordBottomUpSubphase(ActiveBottomUpTimingContext, Stage, Start,
                             WorkItems, ActiveBottomUpPostSummaryContext);
    }
  };
  BottomUpPhaseStart PlanStart;
  if (MeasureSubphases) {
    PlanStart = startBottomUpPhase(/*Enabled=*/true);
  }
  std::vector<CallFunctionMergePlan> Plans;
  std::map<llvm::Function *, std::size_t> TargetToPlan;
  // Index V2N values by their resolved root once per policy application.
  // The per-candidate scan below used to walk the whole reverse map for every
  // root, which made resolve_variable dominate tmux call-slot merge time when
  // one SCC produced many candidates over a large V2N. V2N itself does not
  // change inside this function (merges are transactional and committed only
  // through binarysub state), so the index stays valid for the whole call.
  // Debug switch: skip whole slot kinds (ret / arg), so the responsible class of
  // commits can be isolated.
  {
    const char *Raw = std::getenv("NOTDEC_SKIP_SLOT_MERGE_SLOTS");
    if (Raw != nullptr && *Raw != '\0') {
      bool SkipRet = false;
      bool SkipArg = false;
      llvm::SmallVector<llvm::StringRef, 8> Kinds;
      llvm::StringRef(Raw).split(Kinds, ',');
      for (llvm::StringRef Kind : Kinds) {
        Kind = Kind.trim();
        if (Kind == "ret") {
          SkipRet = true;
        }
        if (Kind == "arg") {
          SkipArg = true;
        }
      }
      Entries.erase(std::remove_if(Entries.begin(), Entries.end(),
                                   [&](const CallSlotMergeEntry &Entry) {
                                     const bool IsRet =
                                         Entry.Slot.rfind("ret", 0) == 0 ||
                                         Entry.Policy.find("return-slot") !=
                                             std::string::npos;
                                     const bool IsArg =
                                         Entry.Slot.rfind("arg", 0) == 0 ||
                                         Entry.Policy.find("call-arg-slot") !=
                                             std::string::npos;
                                     return (SkipRet && IsRet) ||
                                            (SkipArg && IsArg);
                                   }),
                    Entries.end());
    }
  }
  // External merge-strategy input.  Every candidate gets a stable key
  // (target|slot|call-site); NOTDEC_SLOT_MERGE_EXPORT=<path> dumps them and
  // NOTDEC_SLOT_MERGE_IMPORT=<path> reads "key<TAB>skip|apply" lines, so a merge
  // set can be edited and replayed without rebuilding.
  auto EntryKey = [](const CallSlotMergeEntry &E) {
    std::string K =
        (E.Target != nullptr && E.Target->hasName()) ? E.Target->getName().str()
                                                     : std::string("?");
    K += "|" + E.Slot + "|" + E.Detail;
    return K;
  };
  if (const char *Path = std::getenv("NOTDEC_SLOT_MERGE_EXPORT")) {
    std::error_code EC;
    llvm::raw_fd_ostream Out(Path, EC, llvm::sys::fs::OF_Append);
    if (!EC) {
      for (const auto &E : Entries) {
        Out << EntryKey(E) << "\tcandidate\t" << E.Policy << "\n";
      }
    }
  }
  if (const char *Path = std::getenv("NOTDEC_SLOT_MERGE_IMPORT")) {
    std::set<std::string> SkipKeys;
    std::set<std::string> ApplyKeys;
    bool HaveApply = false;
    if (auto Buf = llvm::MemoryBuffer::getFile(Path)) {
      llvm::SmallVector<llvm::StringRef, 2> Parts;
      for (llvm::StringRef Line : llvm::split((*Buf)->getBuffer(), '\n')) {
        Line = Line.trim();
        if (Line.empty() || Line.starts_with("#")) {
          continue;
        }
        Parts.clear();
        Line.split(Parts, '\t');
        if (Parts.size() < 2) {
          continue;
        }
        llvm::StringRef Action = Parts[1].trim();
        if (Action == "skip") {
          SkipKeys.insert(Parts[0].trim().str());
        } else if (Action == "apply") {
          ApplyKeys.insert(Parts[0].trim().str());
          HaveApply = true;
        }
      }
    }
    Entries.erase(std::remove_if(Entries.begin(), Entries.end(),
                                 [&](const CallSlotMergeEntry &E) {
                                   const std::string K = EntryKey(E);
                                   if (SkipKeys.count(K) != 0) {
                                     return true;
                                   }
                                   if (HaveApply && ApplyKeys.count(K) == 0) {
                                     return true;
                                   }
                                   return false;
                                 }),
                  Entries.end());
  }
  // Debug switch: skip the call-slot candidates of the listed target
  // functions (comma separated, matched with leading underscores stripped), so
  // a single responsible merge can be isolated.
  {
    const char *Raw = std::getenv("NOTDEC_SKIP_SLOT_MERGE_TARGETS");
    if (Raw != nullptr && *Raw != '\0') {
      llvm::SmallVector<llvm::StringRef, 16> Names;
      llvm::StringRef(Raw).split(Names, ',');
      Entries.erase(std::remove_if(Entries.begin(), Entries.end(),
                                   [&](const CallSlotMergeEntry &Entry) {
                                     if (Entry.Target == nullptr) {
                                       return false;
                                     }
                                     llvm::StringRef Target =
                                         Entry.Target->getName();
                                     while (!Target.empty() &&
                                            Target.front() == '_') {
                                       Target = Target.drop_front();
                                     }
                                     for (llvm::StringRef Name : Names) {
                                       if (Target == Name.trim()) {
                                         return true;
                                       }
                                     }
                                     return false;
                                   }),
                    Entries.end());
    }
  }
  if (std::getenv("NOTDEC_SLOT_MERGE_DIAG2") != nullptr) {
    for (const auto &Entry : Entries) {
      llvm::errs() << "[slot-merge-entry] target="
                   << formatCallMergeTarget(Entry.Target)
                   << " policy=" << Entry.Policy << " " << Entry.Detail << "\n";
    }
  }
  std::map<SimpleType, std::vector<ExtValuePtr>> ValuesByRoot;
  for (const auto &Ent : CG.V2N.rev()) {
    auto Root = binarysub::resolve_variable(Ent.first);
    if (Root) {
      auto &Values = ValuesByRoot[Root];
      Values.insert(Values.end(), Ent.second.begin(), Ent.second.end());
    }
  }

  for (std::size_t EntryOrder = 0; EntryOrder < Entries.size(); ++EntryOrder) {
    auto &Entry = Entries[EntryOrder];
    auto Formal = binarysub::resolve_variable(Entry.Formal);
    if (!Formal) {
      CG.CallSlotMergeDecisions.push_back(
          ConstraintsGenerator::CallSlotMergeDecision{
              .Target = formatCallMergeTarget(Entry.Target),
              .Policy = Entry.Policy,
              .Decision = "precheck-skipped",
              .Reason = "null-formal",
              .Formal = "<null>",
              .Entries = {Entry.Detail + " actual=" +
                          formatCallSlotRootSnapshot(CG, Entry.Actual)},
              .Roots = {formatCallSlotRootSnapshot(CG, Entry.Actual) +
                        " action=skipped"}});
      CG.emitTypeRecoveryTrace(
          "[merge-policy:" + Entry.Policy +
          ":group-skip] reason=null-formal detail=" + Entry.Detail);
      continue;
    }

    auto [PlanIt, NewPlan] =
        TargetToPlan.emplace(Entry.Target, Plans.size());
    if (NewPlan) {
      Plans.push_back(CallFunctionMergePlan{.Target = Entry.Target});
    }
    auto &Plan = Plans[PlanIt->second];
    Plan.FirstEntryOrder = std::min(Plan.FirstEntryOrder, EntryOrder);
    if (Entry.Slot.rfind("arg", 0) == 0) {
      Plan.FirstArgOrder = std::min(Plan.FirstArgOrder, EntryOrder);
    }
    auto GroupIt = std::find_if(
        Plan.Groups.begin(), Plan.Groups.end(), [&](const auto &Group) {
          return Group.Formal.get() == Formal.get();
        });
    if (GroupIt == Plan.Groups.end()) {
      Plan.Groups.push_back(
          CallSlotMergeGroup{.Target = Entry.Target, .Formal = Formal});
      GroupIt = std::prev(Plan.Groups.end());
    }
    GroupIt->Slots.insert(Entry.Slot);
    GroupIt->Policies.insert(Entry.Policy);
    GroupIt->Entries.push_back(std::move(Entry));
  }

  std::stable_sort(Plans.begin(), Plans.end(), [](const auto &LHS,
                                                  const auto &RHS) {
    const auto LHSOrder =
        LHS.FirstArgOrder == std::numeric_limits<std::size_t>::max()
            ? LHS.FirstEntryOrder
            : LHS.FirstArgOrder;
    const auto RHSOrder =
        RHS.FirstArgOrder == std::numeric_limits<std::size_t>::max()
            ? RHS.FirstEntryOrder
            : RHS.FirstArgOrder;
    return LHSOrder < RHSOrder;
  });

  // Preserve the old useful ordering where return slots were merged before
  // their values were consumed as another call's arguments. A producer plan
  // precedes a consumer when one of its return roots is the consumer's actual
  // argument root. Cycles keep their original stable order.
  std::vector<std::set<std::size_t>> Successors(Plans.size());
  std::vector<std::size_t> InDegree(Plans.size(), 0);
  for (std::size_t Producer = 0; Producer < Plans.size(); ++Producer) {
    std::set<const binarysub::TypeNode *> ReturnRoots;
    for (const auto &Group : Plans[Producer].Groups) {
      if (Group.Slots.count("ret") == 0) {
        continue;
      }
      ReturnRoots.insert(binarysub::resolve_variable(Group.Formal).get());
      for (const auto &Entry : Group.Entries) {
        ReturnRoots.insert(binarysub::resolve_variable(Entry.Actual).get());
      }
    }
    if (ReturnRoots.empty()) {
      continue;
    }
    for (std::size_t Consumer = 0; Consumer < Plans.size(); ++Consumer) {
      if (Producer == Consumer) {
        continue;
      }
      bool DependsOnProducer = false;
      for (const auto &Group : Plans[Consumer].Groups) {
        if (Group.Slots.empty() ||
            Group.Slots.begin()->rfind("arg", 0) != 0) {
          continue;
        }
        for (const auto &Entry : Group.Entries) {
          if (ReturnRoots.count(
                  binarysub::resolve_variable(Entry.Actual).get()) != 0) {
            DependsOnProducer = true;
            break;
          }
        }
        if (DependsOnProducer) {
          break;
        }
      }
      if (DependsOnProducer && Successors[Producer].insert(Consumer).second) {
        ++InDegree[Consumer];
      }
    }
  }
  std::set<std::size_t> Ready;
  for (std::size_t I = 0; I < Plans.size(); ++I) {
    if (InDegree[I] == 0) {
      Ready.insert(I);
    }
  }
  std::vector<CallFunctionMergePlan> OrderedPlans;
  OrderedPlans.reserve(Plans.size());
  std::vector<bool> Moved(Plans.size(), false);
  while (!Ready.empty()) {
    auto I = *Ready.begin();
    Ready.erase(Ready.begin());
    Moved[I] = true;
    OrderedPlans.push_back(std::move(Plans[I]));
    for (auto Successor : Successors[I]) {
      if (--InDegree[Successor] == 0) {
        Ready.insert(Successor);
      }
    }
  }
  for (std::size_t I = 0; I < Plans.size(); ++I) {
    if (!Moved[I]) {
      OrderedPlans.push_back(std::move(Plans[I]));
    }
  }
  Plans = std::move(OrderedPlans);
  RecordSubphase(BottomUpSubstage::CallInterfacePlan, PlanStart,
                 Entries.size());

  std::vector<CallFormalSlotLabel> FormalSlotLabels;
  for (const auto &Plan : Plans) {
    const auto TargetName = formatCallMergeTarget(Plan.Target);
    for (const auto &Group : Plan.Groups) {
      for (const auto &Slot : Group.Slots) {
        FormalSlotLabels.push_back(CallFormalSlotLabel{
            .Formal = Group.Formal, .Label = TargetName + "::" + Slot});
      }
    }
  }

  std::size_t TotalMerged = 0;
  for (auto &Plan : Plans) {
    const auto TransactionId = CG.NextCallMergeTransactionId++;
    const auto TargetName = formatCallMergeTarget(Plan.Target);
    std::vector<PreparedCallSlotMergeGroup> Prepared;
    Prepared.reserve(Plan.Groups.size());
    std::optional<std::string> PlanFailure;
    std::string FailureDecision = "precheck-skipped";
    BottomUpPhaseStart PrecheckStart;
    if (MeasureSubphases) {
      PrecheckStart = startBottomUpPhase(/*Enabled=*/true);
    }

    for (auto &Group : Plan.Groups) {
      PreparedCallSlotMergeGroup Item{.Group = &Group};
      auto Formal = binarysub::resolve_variable(Group.Formal);
      auto *FormalVar = Formal ? Formal->getAsVariableState() : nullptr;
      std::map<const binarysub::TypeNode *, std::size_t> RootToIndex;
      auto AddRoot = [&](SimpleType Ty, bool MergeWithoutStructEvidence) {
        Ty = binarysub::resolve_variable(Ty);
        if (!Ty) {
          return;
        }
        auto [It, Inserted] =
            RootToIndex.emplace(Ty.get(), Item.Roots.size());
        if (Inserted) {
          Item.Roots.push_back(CallSlotMergeRoot{
              .Type = Ty,
              .MergeWithoutStructEvidence = MergeWithoutStructEvidence});
        } else {
          Item.Roots[It->second].MergeWithoutStructEvidence |=
              MergeWithoutStructEvidence;
        }
      };
      AddRoot(Formal, /*MergeWithoutStructEvidence=*/true);
      for (const auto &Entry : Group.Entries) {
        AddRoot(Entry.Actual, !Entry.RequireStructEvidence);
        Item.EntrySnapshots.push_back(
            Entry.Detail + " actual=" +
            formatCallSlotRootSnapshot(CG, Entry.Actual) +
            " require_struct_evidence=" +
            (Entry.RequireStructEvidence ? "true" : "false"));
      }
      for (const auto &Root : Item.Roots) {
        Item.RootSnapshots.push_back(
            formatCallSlotRootSnapshot(CG, Root.Type));
      }
      Item.RootActions.resize(Item.Roots.size(), "pending");
      if (!Item.RootActions.empty()) {
        Item.RootActions.front() = "formal-anchor";
      }
      Item.FormalSnapshot = formatCallSlotRootSnapshot(CG, Formal);

      if (Group.Slots.size() > 1) {
        Item.Failure = "formal-root-already-shared-by-slots=" +
                       formatStringSet(Group.Slots);
        FailureDecision = "preexisting-alias";
      } else if (FormalVar == nullptr) {
        Item.Failure = "non-variable-formal";
      } else {
        for (const auto &Root : Item.Roots) {
          auto *Var = Root.Type->getAsVariableState();
          if (Var == nullptr) {
            Item.Failure = "non-variable-member";
            break;
          }
          if (Var->level != FormalVar->level || Var->size != FormalVar->size) {
            Item.Failure = "level-size-mismatch";
            break;
          }
        }
      }

      Item.HasStructEvidence =
          std::any_of(Item.Roots.begin(), Item.Roots.end(),
                      [&](const auto &Root) {
                        return CG.hasStructPointerEvidence(Root.Type);
                      });
      bool HasMemberAllowedWithoutEvidence = false;
      for (std::size_t I = 1; I < Item.Roots.size(); ++I) {
        HasMemberAllowedWithoutEvidence |=
            Item.Roots[I].MergeWithoutStructEvidence;
      }
      if (!Item.Failure && !Item.HasStructEvidence &&
          !HasMemberAllowedWithoutEvidence) {
        // Missing evidence is not a conflict. Keep this slot out of the plan,
        // while allowing other slots of the same function to run atomically.
        Item.SkipReason = "missing-struct-evidence";
      }
      // Mode 3 relaxes the overlap rule only for call-argument slots; return
      // slots keep the strict rule because a callee return view and an unrelated
      // caller usage legitimately disagree about the same bytes.
      int ItemOverlapMode = layoutOverlapMode();
      if (ItemOverlapMode == 3 || ItemOverlapMode == 4) {
        ItemOverlapMode = Group.Policies.count("call-arg-slot-group") != 0
                              ? ItemOverlapMode
                              : 0;
      }
      for (std::size_t I = 0; !Item.Failure && I < Item.Roots.size(); ++I) {
        for (std::size_t J = I + 1; J < Item.Roots.size(); ++J) {
          if (CG.frameMergeVeto(Item.Roots[I].Type, Item.Roots[J].Type)) {
            Item.Failure = "frame-region-conflict";
            break;
          }
          if (auto Conflict = CG.explainStructFieldSliceCompatibilityFailure(
                  Item.Roots[I].Type, Item.Roots[J].Type,
                  /*RequireEvidence=*/false, ItemOverlapMode)) {
            Item.Failure = *Conflict;
            break;
          }
        }
      }
      if (Item.Failure && !PlanFailure) {
        PlanFailure = *Item.Failure;
      }
      Prepared.push_back(std::move(Item));
    }

    // A value can legitimately be both a formal argument and a return operand.
    // Only report a preexisting alias when two distinct pre-transaction nodes
    // already resolve to the same root; the transaction cannot undo that merge.
    for (std::size_t I = 0; !PlanFailure && I < Prepared.size(); ++I) {
      for (std::size_t J = I + 1; J < Prepared.size(); ++J) {
        if (Prepared[I].Group->Slots == Prepared[J].Group->Slots) {
          continue;
        }
        auto HasMergedAlias = [](SimpleType Actual, SimpleType Formal) {
          if (!Actual || !Formal || Actual.get() == Formal.get()) {
            return false;
          }
          return binarysub::resolve_variable(Actual).get() ==
                 binarysub::resolve_variable(Formal).get();
        };
        for (const auto &LeftEntry : Prepared[I].Group->Entries) {
          for (const auto &RightEntry : Prepared[J].Group->Entries) {
            if (!HasMergedAlias(LeftEntry.Actual, RightEntry.Formal) &&
                !HasMergedAlias(RightEntry.Actual, LeftEntry.Formal)) {
              continue;
            }
            PlanFailure = "preexisting-alias slots=" +
                          formatStringSet(Prepared[I].Group->Slots) + "," +
                          formatStringSet(Prepared[J].Group->Slots);
            FailureDecision = "preexisting-alias";
            break;
          }
          if (PlanFailure) {
            break;
          }
        }
      }
    }

    RecordSubphase(BottomUpSubstage::CallInterfacePrecheck, PrecheckStart,
                   Prepared.size());

    auto RecordDecisions = [&](llvm::StringRef Decision,
                               llvm::StringRef Reason,
                               std::size_t TouchedNodes,
                               const std::vector<std::string> &MergeAttempts) {
      for (auto &Item : Prepared) {
        std::vector<std::string> Roots = Item.RootSnapshots;
        for (std::size_t I = 0; I < Roots.size(); ++I) {
          Roots[I] += " action=" + Item.RootActions[I];
        }
        std::string ItemDecision = Decision.str();
        std::string ItemReason = Reason.str();
        std::size_t ItemTouchedNodes = TouchedNodes;
        std::vector<std::string> ItemMergeAttempts = MergeAttempts;
        if (Item.SkipReason &&
            (Decision == "committed" || Decision == "rolled-back")) {
          ItemDecision = "precheck-skipped";
          ItemReason = *Item.SkipReason;
          ItemTouchedNodes = 0;
          ItemMergeAttempts.clear();
        }
        CG.CallSlotMergeDecisions.push_back(
            ConstraintsGenerator::CallSlotMergeDecision{
                .TransactionId = TransactionId,
                .Target = TargetName,
                .Policy = formatStringSet(Item.Group->Policies),
                .Decision = std::move(ItemDecision),
                .Reason = std::move(ItemReason),
                .Formal = Item.FormalSnapshot,
                .TouchedNodes = ItemTouchedNodes,
                .Entries = Item.EntrySnapshots,
                .Roots = std::move(Roots),
                .RecursiveMerges = std::move(ItemMergeAttempts)});
      }
    };

    if (PlanFailure) {
      for (auto &Item : Prepared) {
        std::fill(Item.RootActions.begin(), Item.RootActions.end(), "skipped");
      }
      RecordDecisions(FailureDecision, *PlanFailure, 0, {});
      CG.emitTypeRecoveryTrace("[call-merge-tx:skip] id=" +
                               std::to_string(TransactionId) + " target=" +
                               TargetName + " reason=" + *PlanFailure);
      continue;
    }

    for (auto &Item : Prepared) {
      if (Item.SkipReason) {
        std::fill(Item.RootActions.begin(), Item.RootActions.end(),
                  "skipped-missing-struct-evidence");
      }
    }

    BottomUpPhaseStart TransactionStart;
    if (MeasureSubphases) {
      TransactionStart = startBottomUpPhase(/*Enabled=*/true);
    }
    PendingCallMergeEffects Effects;
    auto QueuePNDiffUnify = [&](const SimpleType &Lhs, const SimpleType &Rhs) {
      if (!CG.EnablePNDiffTypeVariableClosureUnification || !Lhs || !Rhs ||
          !Lhs->isVariableState() || !Rhs->isVariableState()) {
        return;
      }
      auto CollectValues = [&](const SimpleType &Ty) {
        std::vector<ExtValuePtr> Values;
        auto It = CG.V2N.rev().find(Ty);
        if (It != CG.V2N.rev().end()) {
          Values.assign(It->second.begin(), It->second.end());
        }
        return Values;
      };
      auto LeftValues = CollectValues(Lhs);
      auto RightValues = CollectValues(Rhs);
      if (LeftValues.empty() || RightValues.empty()) {
        return;
      }
      Effects.PNDiffUnifies.push_back(PendingPNDiffUnify{
          .Lhs = Lhs,
          .Rhs = Rhs,
          .LeftValues = std::move(LeftValues),
          .RightValues = std::move(RightValues)});
    };
    const std::size_t EvalCheckpoint =
        CG.MergeEval ? CG.MergeEval->badUnionCheckpoint() : 0;
    binarysub::SimpleTypeTransaction Transaction;
    CG.emitTypeRecoveryTrace("[call-merge-tx:begin] id=" +
                             std::to_string(TransactionId) + " target=" +
                             TargetName);

    // The active explicit pair lets a return operand that is literally another
    // formal slot merge as requested. If an earlier operation in this same
    // transaction redirects that operand to the other formal, the raw root no
    // longer matches and the cross-slot validator still rejects it.
    SimpleType ActiveExplicitFrom = nullptr;
    SimpleType ActiveExplicitInto = nullptr;

    // These sets never resolve their members again. They describe only the
    // roots which belonged to one prepared slot at transaction start, so a
    // merge performed by the transaction cannot manufacture a new exception.
    std::vector<std::set<const binarysub::TypeNode *>> PreparedGroupRoots;
    PreparedGroupRoots.reserve(Prepared.size());
    for (const auto &Item : Prepared) {
      auto &Roots = PreparedGroupRoots.emplace_back();
      for (const auto &Root : Item.Roots) {
        Roots.insert(Root.Type.get());
      }
    }

    auto FormalLabelsForRoot = [&](SimpleType Ty) {
      std::set<std::string> Labels;
      Ty = binarysub::resolve_variable(Ty);
      for (const auto &FormalLabel : FormalSlotLabels) {
        auto Formal = binarysub::resolve_variable(FormalLabel.Formal);
        if (Formal && Formal.get() == Ty.get()) {
          Labels.insert(FormalLabel.Label);
        }
      }
      return Labels;
    };

    auto SharePreparedGroup = [&](const SimpleType &From,
                                  const SimpleType &Into) {
      return std::any_of(PreparedGroupRoots.begin(), PreparedGroupRoots.end(),
                         [&](const auto &Roots) {
                           return Roots.count(From.get()) != 0 &&
                                  Roots.count(Into.get()) != 0;
                         });
    };

    // Mode of the slot group currently being applied; ValidateMerge runs for
    // the explicit merge and for every recursively queued one.
    int ActiveOverlapMode = 0;

    auto ValidateMerge = [&](const binarysub::MergeEvent &Event)
        -> binarysub::expected<void, binarysub::Error> {
      auto From = binarysub::resolve_variable(Event.from);
      auto Into = binarysub::resolve_variable(Event.into);
      auto FromLabels = FormalLabelsForRoot(From);
      auto IntoLabels = FormalLabelsForRoot(Into);
      std::string Attempt =
          std::string(binarysub::merge_reason_name(Event.reason)) +
          " from=" + formatCallSlotRootSnapshot(CG, From) +
          " into=" + formatCallSlotRootSnapshot(CG, Into);
      Effects.MergeAttempts.push_back(Attempt);
      QueuePNDiffUnify(Event.from, Event.into);

      bool SharesSlot = false;
      for (const auto &Label : FromLabels) {
        SharesSlot |= IntoLabels.count(Label) != 0;
      }
      bool IsUnchangedPlannedExplicit =
          Event.reason == binarysub::MergeReason::Explicit &&
          ActiveExplicitFrom && ActiveExplicitInto &&
          Event.from.get() == ActiveExplicitFrom.get() &&
          Event.into.get() == ActiveExplicitInto.get();
      bool IsPreparedGroupMerge = SharePreparedGroup(Event.from, Event.into);
      if (!IsUnchangedPlannedExplicit && !IsPreparedGroupMerge &&
          !FromLabels.empty() && !IntoLabels.empty() && !SharesSlot) {
        return binarysub::unexpected<binarysub::Error>(binarysub::Error::make(
            "recursive merge crosses formal slots: from=" +
            formatStringSet(FromLabels) + " into=" +
            formatStringSet(IntoLabels)));
      }
      if (CG.frameMergeVeto(From, Into)) {
        return binarysub::unexpected<binarysub::Error>(binarysub::Error::make(
            "frame-region merge veto: from=" +
            formatCallSlotRootSnapshot(CG, From) +
            " into=" + formatCallSlotRootSnapshot(CG, Into)));
      }
      if (auto Conflict = CG.explainStructFieldSliceCompatibilityFailure(
              From, Into, /*RequireEvidence=*/false, ActiveOverlapMode)) {
        return binarysub::unexpected<binarysub::Error>(binarysub::Error::make(
            "recursive layout conflict: " + *Conflict));
      }
      return binarysub::expected<void, binarysub::Error>{};
    };

    std::optional<std::string> RuntimeFailure;
    std::size_t PlanMerged = 0;
    for (auto &Item : Prepared) {
      if (Item.SkipReason) {
        continue;
      }
      auto Formal = binarysub::resolve_variable(Item.Group->Formal);
      std::string GroupDetail =
          "target=" + TargetName + " slots=" +
          formatStringSet(Item.Group->Slots) +
          " members=" + std::to_string(Item.Group->Entries.size());
      for (std::size_t I = 1; I < Item.Roots.size(); ++I) {
        if (!Item.HasStructEvidence &&
            !Item.Roots[I].MergeWithoutStructEvidence) {
          Item.RootActions[I] = "skipped-missing-struct-evidence";
          continue;
        }
        auto From = binarysub::resolve_variable(Item.Roots[I].Type);
        auto Into = binarysub::resolve_variable(Formal);
        if (!From || !Into || From.get() == Into.get()) {
          Item.RootActions[I] = "already-merged";
          continue;
        }

        ActiveExplicitFrom =
            From.get() == Item.Roots[I].Type.get() ? From : nullptr;
        ActiveExplicitInto = ActiveExplicitFrom ? Into : nullptr;

        PendingCallPolicyMerge Pending{
            .Policy = formatStringSet(Item.Group->Policies),
            .From = From,
            .Into = Into,
            .Detail = GroupDetail};
        if (auto It = ValuesByRoot.find(From); It != ValuesByRoot.end()) {
          Pending.MovedValues.insert(Pending.MovedValues.end(),
                                     It->second.begin(), It->second.end());
        }

        binarysub::ConstraintContext Context;
        CG.configureConstraintContext(Context);
        Context.validateVariableMerge = ValidateMerge;
        // Experiment switch: a shallow conflict normally makes the policy skip
        // that single auto-merge while the rest of the plan proceeds.  Inside a
        // transaction we turn such a skip into a rollback so the plan stays
        // atomic; NOTDEC_ALLOW_SHALLOW_MERGE_CONFLICT=1 restores the
        // non-transactional "skip this merge and continue" behaviour.
        Context.rejectShallowMergeConflict =
            std::getenv("NOTDEC_ALLOW_SHALLOW_MERGE_CONFLICT") == nullptr;
        int RootOverlapMode = layoutOverlapMode();
        if (RootOverlapMode == 3 || RootOverlapMode == 4) {
          RootOverlapMode =
              Item.Group->Policies.count("call-arg-slot-group") != 0
                  ? RootOverlapMode
                  : 0;
        }
        ActiveOverlapMode = RootOverlapMode;
        auto Result = binarysub::merge_variable_into(
            From, Into,
            [&](const SimpleType &Lhs, const SimpleType &Rhs) {
              QueuePNDiffUnify(Lhs, Rhs);
            },
            &Context);
        ActiveExplicitFrom = nullptr;
        ActiveExplicitInto = nullptr;
        if (!Result) {
          RuntimeFailure = Result.error().msg;
          break;
        }
        Effects.PolicyMerges.push_back(std::move(Pending));
        Item.RootActions[I] = "merged-into-formal";
        ++PlanMerged;
      }
      if (RuntimeFailure) {
        break;
      }
    }

    if (RuntimeFailure) {
      Transaction.rollback();
      if (CG.MergeEval) {
        CG.MergeEval->rollbackBadUnions(EvalCheckpoint);
      }
      for (auto &Item : Prepared) {
        // This slot never participated in the transaction. Keep its action
        // consistent with the per-slot precheck-skipped decision below.
        if (Item.SkipReason) {
          continue;
        }
        for (auto &Action : Item.RootActions) {
          if (Action == "pending" || Action == "merged-into-formal" ||
              Action == "already-merged" || Action == "formal-anchor") {
            Action = "rolled-back";
          }
        }
      }
      RecordDecisions("rolled-back", *RuntimeFailure,
                      Transaction.touchedNodeCount(), Effects.MergeAttempts);
      CG.emitTypeRecoveryTrace("[call-merge-tx:rollback] id=" +
                               std::to_string(TransactionId) + " target=" +
                               TargetName + " touched=" +
                               std::to_string(Transaction.touchedNodeCount()) +
                               " reason=" + *RuntimeFailure);
      RecordSubphase(BottomUpSubstage::CallInterfaceTransaction,
                     TransactionStart, Plan.Groups.size());
      continue;
    }

    Transaction.commit();
    for (const auto &Pending : Effects.PNDiffUnifies) {
      CG.unifyPNDiffValueGroups(Pending.LeftValues, Pending.RightValues,
                               Pending.Lhs, Pending.Rhs);
    }
    for (const auto &Pending : Effects.PolicyMerges) {
      CG.emitMergeTrace(Pending.Policy, Pending.From, Pending.Into,
                        Pending.MovedValues);
      if (CG.MergeEval) {
        CG.MergeEval->observeValueMapMerge(Pending.From, Pending.Into,
                                           Pending.MovedValues);
      }
      CG.emitTypeRecoveryTrace("[merge-policy:" + Pending.Policy +
                               "] from=" +
                               binarysub::debug_string(Pending.From) +
                               " into=" +
                               binarysub::debug_string(Pending.Into) +
                               " detail=" + Pending.Detail);
    }

    // V2N is intentionally unchanged during speculation. Move every old key to
    // its final root only after PNDiff has consumed the original node identities.
    std::vector<std::pair<SimpleType, SimpleType>> V2NMerges;
    for (const auto &Ent : CG.V2N.rev()) {
      auto FinalRoot = binarysub::resolve_variable(Ent.first);
      if (FinalRoot && FinalRoot.get() != Ent.first.get()) {
        V2NMerges.push_back({Ent.first, FinalRoot});
      }
    }
    for (const auto &[From, Into] : V2NMerges) {
      CG.V2N.merge(From, Into);
    }
    for (const auto &Pending : Effects.PolicyMerges) {
      auto FinalRoot = binarysub::resolve_variable(Pending.Into);
      auto Followups =
          CG.collectStructPtrFieldFollowupMergeCandidatesForMergedOwner(
              FinalRoot);
      CG.StructFieldFollowupMergeCandidates.insert(
          CG.StructFieldFollowupMergeCandidates.end(), Followups.begin(),
          Followups.end());
    }

    for (auto &Item : Prepared) {
      for (auto &Action : Item.RootActions) {
        if (Action == "pending") {
          Action = "already-merged";
        }
      }
    }
    RecordDecisions("committed", "compatible-layouts",
                    Transaction.touchedNodeCount(), Effects.MergeAttempts);
    CG.emitTypeRecoveryTrace("[call-merge-tx:commit] id=" +
                             std::to_string(TransactionId) + " target=" +
                             TargetName + " touched=" +
                             std::to_string(Transaction.touchedNodeCount()));
    TotalMerged += PlanMerged;
    RecordSubphase(BottomUpSubstage::CallInterfaceTransaction,
                   TransactionStart, Plan.Groups.size());
  }
  return TotalMerged;
}

static std::string formatReturnValueMergeCandidateDetail(
    const ConstraintsGenerator::ReturnValueMergeCandidate &Candidate) {
  auto *Func = Candidate.Return ? Candidate.Return->getFunction() : nullptr;
  std::string Detail;
  if (Candidate.Call != nullptr) {
    Detail = "call=";
    Detail += toStableString(ExtValuePtr{Candidate.Call});
    Detail += " target=";
    Detail += Candidate.Target && Candidate.Target->hasName()
                  ? Candidate.Target->getName().str()
                  : "<unknown>";
  } else {
    Detail = "func=";
    Detail += Func && Func->hasName() ? Func->getName().str() : "<unknown>";
  }
  if (Candidate.Return != nullptr) {
    Detail += " ret=";
    Detail += toStableString(ExtValuePtr{Candidate.Return});
  }
  return Detail;
}

void ConstraintsGenerator::recordCallArgStructPtrMergeCandidate(
    llvm::CallBase &Call, llvm::Function &Target, unsigned ArgIndex,
    SimpleType ActualArg, SimpleType FormalArg) {
  CallArgStructPtrMergeCandidate Candidate{
      .Call = &Call,
      .Target = &Target,
      .ArgIndex = ArgIndex,
      .ActualArg = ActualArg,
      .FormalArg = FormalArg};
  if (ActualArg == nullptr || FormalArg == nullptr) {
    CallSlotMergeDecisions.push_back(CallSlotMergeDecision{
        .Policy = "call-arg-slot-group",
        .Decision = "skipped",
        .Reason = "null-actual-or-formal",
        .Formal = formatCallSlotRootSnapshot(*this, FormalArg),
        .Entries = {formatCallArgStructPtrMergeCandidateDetail(Candidate) +
                    " actual=" +
                    formatCallSlotRootSnapshot(*this, ActualArg)},
        .Roots = {formatCallSlotRootSnapshot(*this, ActualArg) +
                  " action=skipped"}});
    emitTypeRecoveryTrace(
        "[merge-policy:call-arg-struct-ptr:candidate-skip] " +
        formatCallArgStructPtrMergeCandidateDetail(Candidate) +
        " reason=null-actual-or-formal actual=" +
        formatSimpleTypeForTrace(ActualArg) +
        " formal=" + formatSimpleTypeForTrace(FormalArg));
    return;
  }
  if (std::getenv("NOTDEC_POLY_MONO_DIAG") != nullptr &&
      !resolvePolymorphicPositionSpec(Target).empty()) {
    llvm::errs() << "[poly-mono-mergecand] target=" << Target.getName()
                 << " arg=" << ArgIndex
                 << " actual=" << binarysub::debug_string(ActualArg)
                 << " formal=" << binarysub::debug_string(FormalArg) << "\n";
  }
  CallArgStructPtrMergeCandidates.push_back(Candidate);
  emitTypeRecoveryTrace(
      "[merge-policy:call-arg-struct-ptr:candidate] " +
      formatCallArgStructPtrMergeCandidateDetail(Candidate) +
      " actual=" + formatSimpleTypeForTrace(ActualArg) +
      " formal=" + formatSimpleTypeForTrace(FormalArg));
}

namespace {

// The structural function type of a value: the value itself when it is a
// function node, otherwise the first function bound of the variable that
// carries it (the summary root of a function is a variable at its SCC level).
const binarysub::TFunction *findStructuralFunctionType(const SimpleType &Ty,
                                                       unsigned Depth = 0) {
  if (!Ty || Depth > 4) {
    return nullptr;
  }
  if (auto *Fn = Ty->getAsTFunction()) {
    return Fn;
  }
  auto *Var = Ty->getAsVariableState();
  if (Var == nullptr) {
    return nullptr;
  }
  for (const SimpleType &Bound : Var->lowerBounds) {
    if (auto *Fn = findStructuralFunctionType(Bound, Depth + 1)) {
      return Fn;
    }
  }
  for (const SimpleType &Bound : Var->upperBounds) {
    if (auto *Fn = findStructuralFunctionType(Bound, Depth + 1)) {
      return Fn;
    }
  }
  return nullptr;
}

// Sentinel position for the return value in the shared-position cache.
constexpr unsigned kMonomorphicReturnPosition = ~0u;

// One shared, level-fixed copy per (function, position, caller level).  The
// copy comes from binarysub::extrude, which also installs the one-way bridge
// to the callee's original formal; every call site at that level reuses the
// same node, so they all constrain one type there.
SimpleType sharedPositionType(const llvm::Function &F, unsigned Position,
                              const SimpleType &Original, bool IsReturn) {
  // One node per (function, position), always at the module level 0, so every
  // call site in the module - whatever level it lives at - constrains the same
  // type.  extrude returns the original node itself when it is already at or
  // below the requested level.
  static std::map<std::tuple<const llvm::Function *, unsigned, bool>, SimpleType>
      Cache;
  const auto Key = std::make_tuple(&F, Position, IsReturn);
  if (auto It = Cache.find(Key); It != Cache.end()) {
    return It->second;
  }
  // Simple-sub Fig. 7 + the level-violation rules: a type that is the TARGET
  // of a constraint (a formal parameter receiving the caller's actual, i.e.
  // the rhs of "actual <: formal") is extruded with pol=false, while a type
  // that is the SOURCE of a constraint (a returned value flowing into the
  // call result, the lhs of "ret <: result") is extruded with pol=true.
  // pol=false copies the formal's upper bounds (its shape) into the shared
  // copy and installs "copy <: original", so caller-side values still reach
  // the callee's own variable; pol=true is the mirror image.
  const bool Polarity = IsReturn;
  constexpr int Level = 0;
  std::map<binarysub::PolarVar, SimpleType> ExtrudeCache;
  SimpleType Shared =
      binarysub::extrude(Original, Polarity, Level, ExtrudeCache);
  Cache.emplace(Key, Shared);
  if (std::getenv("NOTDEC_POLY_MONO_DIAG") != nullptr) {
    llvm::errs() << "[poly-mono-extrude] " << F.getName()
                 << " pos=" << Position << " level=" << Level
                 << " pol=" << Polarity
                 << " orig=" << binarysub::debug_string(Original)
                 << " shared=" << binarysub::debug_string(Shared) << "\n";
  }
  return Shared;
}

} // namespace

void ConstraintsGenerator::recordCallArgStructPtrMergeCandidates(
    llvm::CallBase &Call, llvm::Function &Target, SimpleType ActualFunc,
    SimpleType FormalFunc) {
  // The call-site function types are usually variables carrying a function
  // bound (an instantiated summary), so resolve through the bounds instead of
  // only following variable merges; otherwise polymorphic calls never reach
  // the merge policy at all.
  auto *Actual = findStructuralFunctionType(ActualFunc);
  auto *Formal = findStructuralFunctionType(FormalFunc);
  if (Actual == nullptr || Formal == nullptr) {
    return;
  }
  // Partially-monomorphic positions: pair the caller's ACTUAL argument with
  // the one shared (non-polymorphic) node of that position, so every call
  // site merges its object into the same type.  The shared node is created
  // from the callee's own formal during instantiation, which runs before this
  // collection, so the cache returns that node.
  const auto Spec = resolvePolymorphicPositionSpec(Target);
  auto Count = std::min(Actual->args.size(), Formal->args.size());
  for (unsigned I = 0; I < Count; ++I) {
    bool IsMonomorphic = false;
    for (unsigned Mono : Spec.MonomorphicArgs) {
      if (Mono == I) {
        IsMonomorphic = true;
        break;
      }
    }
    if (IsMonomorphic) {
      SimpleType Shared = sharedPositionType(Target, I, Formal->args[I],
                                             /*IsReturn=*/false);
      recordCallArgStructPtrMergeCandidate(Call, Target, I, Actual->args[I],
                                           Shared);
      continue;
    }
    recordCallArgStructPtrMergeCandidate(Call, Target, I, Actual->args[I],
                                         Formal->args[I]);
  }
}

void ConstraintsGenerator::recordCallReturnStructPtrMergeCandidate(
    llvm::CallBase &Call, llvm::Function &Target, SimpleType ActualRet,
    SimpleType FormalRet) {
  ReturnValueMergeCandidate Candidate{.Call = &Call,
                                      .Target = &Target,
                                      .Operand = ActualRet,
                                      .FunctionReturn = FormalRet};
  if (ActualRet == nullptr || FormalRet == nullptr) {
    CallSlotMergeDecisions.push_back(CallSlotMergeDecision{
        .Policy = "return-slot-group",
        .Decision = "skipped",
        .Reason = "null-actual-or-formal",
        .Formal = formatCallSlotRootSnapshot(*this, FormalRet),
        .Entries = {formatReturnValueMergeCandidateDetail(Candidate) +
                    " actual=" +
                    formatCallSlotRootSnapshot(*this, ActualRet)},
        .Roots = {formatCallSlotRootSnapshot(*this, ActualRet) +
                  " action=skipped"}});
    return;
  }
  ReturnValueMergeCandidates.push_back(std::move(Candidate));
}

void ConstraintsGenerator::recordCallReturnStructPtrMergeCandidates(
    llvm::CallBase &Call, llvm::Function &Target, SimpleType ActualFunc,
    SimpleType FormalFunc) {
  auto ActualTy = binarysub::resolve_variable(ActualFunc);
  auto FormalTy = binarysub::resolve_variable(FormalFunc);
  auto *Actual = ActualTy ? ActualTy->getAsTFunction() : nullptr;
  auto *Formal = FormalTy ? FormalTy->getAsTFunction() : nullptr;
  if (Actual == nullptr || Formal == nullptr || Actual->result == nullptr ||
      Formal->result == nullptr) {
    return;
  }
  recordCallReturnStructPtrMergeCandidate(Call, Target, Actual->result,
                                          Formal->result);
}

void ConstraintsGenerator::deferCallConstraint(
    llvm::CallBase &Call, llvm::Function &Target, SimpleType SubtypeLHS,
    SimpleType ActualFunc, SimpleType FormalFunc) {
  assert(SubtypeLHS != nullptr && ActualFunc != nullptr &&
         FormalFunc != nullptr);
  DeferredCallConstraints.push_back(DeferredCallConstraint{
      .Call = &Call,
      .Target = &Target,
      .SubtypeLHS = SubtypeLHS,
      .ActualFunc = ActualFunc,
      .FormalFunc = FormalFunc});
}

void ConstraintsGenerator::applyDeferredCallConstraints() {
  auto RunSubphase = [&](BottomUpSubstage Stage, std::uint64_t WorkItems,
                         auto &&Action) {
    if (!ConstraintDiagEnabled || ActiveBottomUpTimingContext == nullptr) {
      Action();
      return;
    }
    auto Start = startBottomUpPhase(/*Enabled=*/true);
    Action();
    recordBottomUpSubphase(ActiveBottomUpTimingContext, Stage, Start, WorkItems,
                           ActiveBottomUpPostSummaryContext);
  };
  if (EnableEarlyCallInterfaceMerge) {
    // This is intentionally opt-in. It measures how much work can be avoided by
    // eliminating actual/formal edges before propagation, while exposing the
    // risk that later call edges would have supplied new conflict evidence.
    RunSubphase(BottomUpSubstage::DeferredCallCandidateCollect,
                DeferredCallConstraints.size(), [&] {
                  for (const auto &Deferred : DeferredCallConstraints) {
                    recordCallArgStructPtrMergeCandidates(
                        *Deferred.Call, *Deferred.Target, Deferred.ActualFunc,
                        Deferred.FormalFunc);
                    recordCallReturnStructPtrMergeCandidates(
                        *Deferred.Call, *Deferred.Target, Deferred.ActualFunc,
                        Deferred.FormalFunc);
                  }
                });
    EarlyCallInterfaceMerged += applyCallInterfaceMergePolicy();
    RunSubphase(BottomUpSubstage::DeferredCallSubtype,
                DeferredCallConstraints.size(), [&] {
                  for (const auto &Deferred : DeferredCallConstraints) {
                    addSubtype(Deferred.SubtypeLHS, Deferred.ActualFunc);
                  }
                });
  } else {
    // Keep the production ordering exact: every call subtype is added before
    // any candidate is recorded, so the policy sees the completed call graph.
    static const bool DeferredDiag =
        std::getenv("NOTDEC_DEFERRED_DIAG") != nullptr;
    RunSubphase(BottomUpSubstage::DeferredCallSubtype,
                DeferredCallConstraints.size(), [&] {
                  std::size_t DeferredIndex = 0;
                  for (const auto &Deferred : DeferredCallConstraints) {
                    if (DeferredDiag) {
                      std::string LhsText =
                          binarysub::debug_string(Deferred.SubtypeLHS);
                      std::string RhsText =
                          binarysub::debug_string(Deferred.ActualFunc);
                      if (LhsText.size() > 240) {
                        LhsText.resize(240);
                        LhsText += "...";
                      }
                      if (RhsText.size() > 240) {
                        RhsText.resize(240);
                        RhsText += "...";
                      }
                      llvm::errs()
                          << "[deferred-subtype] i=" << DeferredIndex << "/"
                          << DeferredCallConstraints.size()
                          << " target="
                          << (Deferred.Target && Deferred.Target->hasName()
                                  ? Deferred.Target->getName()
                                  : StringRef("<anon>"))
                          << " lhs=" << LhsText << " rhs=" << RhsText << "\n";
                    }
                    ++DeferredIndex;
                    addSubtype(Deferred.SubtypeLHS, Deferred.ActualFunc);
                    if (DeferredDiag) {
                      std::string Label =
                          "deferred-after:" +
                          (Deferred.Target && Deferred.Target->hasName()
                               ? Deferred.Target->getName().str()
                               : std::string("<anon>"));
                      binarysub::emit_type_graph_summary(Label.c_str());
                    }
                  }
                });
    RunSubphase(BottomUpSubstage::DeferredCallCandidateCollect,
                DeferredCallConstraints.size(), [&] {
                  for (const auto &Deferred : DeferredCallConstraints) {
                    recordCallArgStructPtrMergeCandidates(
                        *Deferred.Call, *Deferred.Target, Deferred.ActualFunc,
                        Deferred.FormalFunc);
                    recordCallReturnStructPtrMergeCandidates(
                        *Deferred.Call, *Deferred.Target, Deferred.ActualFunc,
                        Deferred.FormalFunc);
                  }
                });
  }
  DeferredCallConstraints.clear();
}

static void appendReturnValueMergeEntries(
    std::vector<CallSlotMergeEntry> &Entries,
    const std::vector<ConstraintsGenerator::ReturnValueMergeCandidate>
        &Candidates) {
  Entries.reserve(Entries.size() + Candidates.size());
  for (const auto &Candidate : Candidates) {
    auto *Target = Candidate.Target;
    if (Target == nullptr && Candidate.Return != nullptr) {
      Target = Candidate.Return->getFunction();
    }
    Entries.push_back(CallSlotMergeEntry{
        .Target = Target,
        .Slot = "ret",
        .Policy = "return-slot-group",
        .Actual = Candidate.Operand,
        .Formal = Candidate.FunctionReturn,
        .Detail = formatReturnValueMergeCandidateDetail(Candidate),
        .RequireStructEvidence = Candidate.Call != nullptr});
  }
}

static void appendCallArgMergeEntries(
    std::vector<CallSlotMergeEntry> &Entries,
    const std::vector<ConstraintsGenerator::CallArgStructPtrMergeCandidate>
        &Candidates) {
  Entries.reserve(Entries.size() + Candidates.size());
  for (const auto &Candidate : Candidates) {
    Entries.push_back(CallSlotMergeEntry{
        .Target = Candidate.Target,
        .Slot = "arg" + std::to_string(Candidate.ArgIndex),
        .Policy = "call-arg-slot-group",
        .Actual = Candidate.ActualArg,
        .Formal = Candidate.FormalArg,
        .Detail = formatCallArgStructPtrMergeCandidateDetail(Candidate),
        .RequireStructEvidence = true});
  }
}

std::size_t ConstraintsGenerator::applyCallInterfaceMergePolicy() {
  std::vector<CallSlotMergeEntry> Entries;
  appendReturnValueMergeEntries(Entries, ReturnValueMergeCandidates);
  appendCallArgMergeEntries(Entries, CallArgStructPtrMergeCandidates);
  ReturnValueMergeCandidates.clear();
  CallArgStructPtrMergeCandidates.clear();
  // Debug switch: isolate the cost/behaviour of the transactional call-slot
  // merge policy (NOTDEC_DISABLE_CALLSLOT_MERGE=1 makes it a no-op).
  if (std::getenv("NOTDEC_DISABLE_CALLSLOT_MERGE") != nullptr) {
    return 0;
  }
  return applyTransactionalCallSlotMergePolicy(*this, std::move(Entries));
}

std::size_t ConstraintsGenerator::applyReturnValueMergePolicy() {
  std::vector<CallSlotMergeEntry> Entries;
  appendReturnValueMergeEntries(Entries, ReturnValueMergeCandidates);
  ReturnValueMergeCandidates.clear();
  // Debug switch: isolate the cost/behaviour of the transactional call-slot
  // merge policy (NOTDEC_DISABLE_CALLSLOT_MERGE=1 makes it a no-op).
  if (std::getenv("NOTDEC_DISABLE_CALLSLOT_MERGE") != nullptr) {
    return 0;
  }
  return applyTransactionalCallSlotMergePolicy(*this, std::move(Entries));
}

std::size_t ConstraintsGenerator::applyCallArgStructPtrMergePolicy() {
  std::vector<CallSlotMergeEntry> Entries;
  appendCallArgMergeEntries(Entries, CallArgStructPtrMergeCandidates);
  CallArgStructPtrMergeCandidates.clear();
  // Debug switch: isolate the cost/behaviour of the transactional call-slot
  // merge policy (NOTDEC_DISABLE_CALLSLOT_MERGE=1 makes it a no-op).
  if (std::getenv("NOTDEC_DISABLE_CALLSLOT_MERGE") != nullptr) {
    return 0;
  }
  return applyTransactionalCallSlotMergePolicy(*this, std::move(Entries));
}

std::vector<ConstraintsGenerator::StructPtrSlotMergeCandidate>
ConstraintsGenerator::collectStructPtrSameAccessKindMergeCandidates(
    bool CollectLoads) const {
  std::vector<StructPtrSlotMergeCandidate> Candidates;
  std::set<std::tuple<const binarysub::TypeNode *, const binarysub::TypeNode *,
                      const binarysub::TypeNode *, unsigned>>
      Seen;

  // hasStructPointerEvidence 会全量走目标的 bound/field 证据；同一目标在
  // 收集过程中不会变化（合并只发生在收集之后的候选循环里），成员级
  // StructEvidenceMemo 直接提供每变量一次的 memo（无 merge 则无失效）。
  auto HasStructEvidence = [&](SimpleType Ty) {
    return hasStructPointerEvidence(Ty);
  };

  for (const auto &[_, Node] : V2N) {
    auto Ptr = binarysub::resolve_variable(Node);
    auto *PtrVar = Ptr ? Ptr->getAsVariableState() : nullptr;
    if (PtrVar == nullptr) {
      continue;
    }

    // 同一变量的 upperBounds 里同一访问目标常出现很多份 bound，直接按 bound
    // 两两配对是 B^2；大输入（redis 数千 bound/变量）因此卡死。先解析并按
    // (目标, 访问大小) 去重，只对不同目标配对。候选集合与旧实现一致：旧实现
    // 本来就用 Seen 按解析后的目标对去重，且同目标对会被跳过。
    std::vector<std::pair<SimpleType, unsigned>> Targets;
    auto AddTarget = [&](SimpleType Target, unsigned Size) {
      Target = binarysub::resolve_variable(Target);
      if (Target && Target->getAsVariableState() != nullptr) {
        Targets.push_back({Target, Size});
      }
    };
    for (const auto &Bound : PtrVar->upperBounds) {
      if (CollectLoads) {
        auto *Load = Bound ? Bound->getAsPtrLoad() : nullptr;
        if (Load != nullptr) {
          AddTarget(Load->to, Load->Size);
        }
      } else {
        auto *Store = Bound ? Bound->getAsPtrStore() : nullptr;
        if (Store != nullptr) {
          AddTarget(Store->to, Store->Size);
        }
      }
    }
    std::sort(Targets.begin(), Targets.end(),
              [](const auto &Lhs, const auto &Rhs) {
                if (Lhs.first.get() != Rhs.first.get()) {
                  return Lhs.first.get() < Rhs.first.get();
                }
                return Lhs.second < Rhs.second;
              });
    Targets.erase(
        std::unique(Targets.begin(), Targets.end(),
                    [](const auto &Lhs, const auto &Rhs) {
                      return Lhs.first.get() == Rhs.first.get() &&
                             Lhs.second == Rhs.second;
                    }),
        Targets.end());

    for (std::size_t I = 0; I < Targets.size(); ++I) {
      for (std::size_t J = I + 1; J < Targets.size(); ++J) {
        if (Targets[I].second != Targets[J].second) {
          continue;
        }
        auto FirstTarget = Targets[I].first;
        auto SecondTarget = Targets[J].first;
        auto *FirstVar =
            FirstTarget ? FirstTarget->getAsVariableState() : nullptr;
        auto *SecondVar =
            SecondTarget ? SecondTarget->getAsVariableState() : nullptr;
        if (FirstVar == nullptr || SecondVar == nullptr ||
            FirstTarget.get() == SecondTarget.get()) {
          continue;
        }
        if (FirstVar->level != SecondVar->level ||
            FirstVar->size != SecondVar->size ||
            FirstVar->size != PointerSize) {
          continue;
        }
        if (!HasStructEvidence(FirstTarget) ||
            !HasStructEvidence(SecondTarget)) {
          continue;
        }

        const auto *First = FirstTarget.get();
        const auto *Second = SecondTarget.get();
        if (Second < First) {
          std::swap(First, Second);
        }
        auto Key = std::make_tuple(Ptr.get(), First, Second,
                                   Targets[I].second);
        if (!Seen.insert(Key).second) {
          continue;
        }
        Candidates.push_back(StructPtrSlotMergeCandidate{
            .Pointer = Ptr,
            .FromTarget = FirstTarget,
            .IntoTarget = SecondTarget,
            .AccessSize = Targets[I].second});
      }
    }
  }

  return Candidates;
}

std::vector<ConstraintsGenerator::StructPtrSlotMergeCandidate>
ConstraintsGenerator::collectStructPtrSameLoadMergeCandidates() const {
  return collectStructPtrSameAccessKindMergeCandidates(/*CollectLoads=*/true);
}

std::vector<ConstraintsGenerator::StructPtrSlotMergeCandidate>
ConstraintsGenerator::collectStructPtrSameStoreMergeCandidates() const {
  return collectStructPtrSameAccessKindMergeCandidates(/*CollectLoads=*/false);
}

// Ablation switch NOTDEC_MEM_NO_STORE_TO_LOAD=1: the library drops the
// store->load subtype edge at this setting (storeToLoadFlowEnabled in
// binarysub/src/binarysub-core.cpp).  The same setting must drop the
// merge-level read/write coupling too, otherwise the load target and the store
// target of one pointer variable are still unified into one type variable.
// Only the exact string "1" disables the flow; unset keeps the default.
static bool storeToLoadFlowDisabled() {
  static const bool Disabled = [] {
    const char *Value = std::getenv("NOTDEC_MEM_NO_STORE_TO_LOAD");
    return Value != nullptr && std::string(Value) == "1";
  }();
  return Disabled;
}

std::vector<ConstraintsGenerator::StructPtrSlotMergeCandidate>
ConstraintsGenerator::collectStructPtrLoadStoreMergeCandidates() const {
  if (storeToLoadFlowDisabled()) {
    return {};
  }
  std::vector<StructPtrSlotMergeCandidate> Candidates;
  std::set<std::tuple<const binarysub::TypeNode *, const binarysub::TypeNode *,
                      const binarysub::TypeNode *, unsigned>>
      Seen;

  // 同 collectStructPtrSameAccessKindMergeCandidates：load×store 候选对是
  // B^2 的，evidence 由成员级 StructEvidenceMemo 提供每变量一次 memo。
  auto HasStructEvidence = [&](SimpleType Ty) {
    return hasStructPointerEvidence(Ty);
  };

  for (const auto &[_, Node] : V2N) {
    auto Ptr = binarysub::resolve_variable(Node);
    auto *PtrVar = Ptr ? Ptr->getAsVariableState() : nullptr;
    if (PtrVar == nullptr) {
      continue;
    }

    // 同 collectStructPtrSameAccessKindMergeCandidates：load×store 是 B^2，
    // 先按 (解析后目标, 大小) 去重再配对，候选集合与旧实现一致。
    std::vector<std::pair<SimpleType, unsigned>> LoadTargets;
    std::vector<std::pair<SimpleType, unsigned>> StoreTargets;
    auto AddTarget = [](std::vector<std::pair<SimpleType, unsigned>> &Out,
                        SimpleType Target, unsigned Size) {
      Target = binarysub::resolve_variable(Target);
      if (Target && Target->getAsVariableState() != nullptr) {
        Out.push_back({Target, Size});
      }
    };
    for (const auto &LoadBound : PtrVar->upperBounds) {
      auto *Load = LoadBound ? LoadBound->getAsPtrLoad() : nullptr;
      if (Load != nullptr) {
        AddTarget(LoadTargets, Load->to, Load->Size);
      }
    }
    for (const auto &StoreBound : PtrVar->upperBounds) {
      auto *Store = StoreBound ? StoreBound->getAsPtrStore() : nullptr;
      if (Store != nullptr) {
        AddTarget(StoreTargets, Store->to, Store->Size);
      }
    }
    auto DedupeTargets =
        [](std::vector<std::pair<SimpleType, unsigned>> &Targets) {
          std::sort(Targets.begin(), Targets.end(),
                    [](const auto &Lhs, const auto &Rhs) {
                      if (Lhs.first.get() != Rhs.first.get()) {
                        return Lhs.first.get() < Rhs.first.get();
                      }
                      return Lhs.second < Rhs.second;
                    });
          Targets.erase(
              std::unique(Targets.begin(), Targets.end(),
                          [](const auto &Lhs, const auto &Rhs) {
                            return Lhs.first.get() == Rhs.first.get() &&
                                   Lhs.second == Rhs.second;
                          }),
              Targets.end());
        };
    DedupeTargets(LoadTargets);
    DedupeTargets(StoreTargets);

    for (const auto &[LoadTarget, LoadSize] : LoadTargets) {
      for (const auto &[StoreTarget, StoreSize] : StoreTargets) {
        if (LoadSize != StoreSize) {
          continue;
        }
        auto *LoadVar = LoadTarget ? LoadTarget->getAsVariableState() : nullptr;
        auto *StoreVar =
            StoreTarget ? StoreTarget->getAsVariableState() : nullptr;
        if (LoadVar == nullptr || StoreVar == nullptr ||
            LoadTarget.get() == StoreTarget.get()) {
          continue;
        }
        if (LoadVar->level != StoreVar->level ||
            LoadVar->size != StoreVar->size || LoadVar->size != PointerSize) {
          continue;
        }
        if (!HasStructEvidence(LoadTarget) ||
            !HasStructEvidence(StoreTarget)) {
          continue;
        }

        const auto *First = LoadTarget.get();
        const auto *Second = StoreTarget.get();
        if (Second < First) {
          std::swap(First, Second);
        }
        auto Key = std::make_tuple(Ptr.get(), First, Second, LoadSize);
        if (!Seen.insert(Key).second) {
          continue;
        }
        Candidates.push_back(StructPtrSlotMergeCandidate{
            .Pointer = Ptr,
            .FromTarget = LoadTarget,
            .IntoTarget = StoreTarget,
            .AccessSize = LoadSize});
      }
    }
  }

  return Candidates;
}

std::size_t
ConstraintsGenerator::applyStructPtrSlotMergeCandidates(llvm::StringRef Policy) {
  std::size_t Merged = 0;
  // 候选循环里每对都要重新确认 evidence；同一轮内合并发生前目标状态不变，
  // 按变量 memo。合并只会向目标变量增加 bound（evidence 单调：true 永不翻
  // false，false 可能翻 true），所以每次成功合并只需要失效 Into 和所有
  // false 条目，true 条目可以一直复用；全清会让每合并一对就把所有变量的
  // 证据全量重算（redis 31 万候选的 load-load 轮次因此数分钟无进展）。
  // evidence 只随 merge 把 From 的 ptrLoad/ptrStore bound 转移给 Into
  // 而变化（onVariableMerged 已精确失效 From/Into），循环里直接走成员
  // 级 memo，不需要每轮维护局部缓存，也避免了旧实现每合并一对就清空
  // 全部 false 缓存导致剩余候选全量重算（redis 32 万候选轮次卡死）。
  auto HasStructEvidence = [&](SimpleType Ty) {
    return hasStructPointerEvidence(Ty);
  };
  while (true) {
    const bool MeasureSubphases =
        ConstraintDiagEnabled && ActiveBottomUpTimingContext != nullptr;
    BottomUpPhaseStart CollectStart;
    if (MeasureSubphases) {
      CollectStart = startBottomUpPhase(/*Enabled=*/true);
    }
    bool Changed = false;
    const bool SlotMergeDiag = std::getenv("NOTDEC_SLOT_MERGE_DIAG") != nullptr;
    auto SlotMergeDiagStart = std::chrono::steady_clock::now();
    auto SlotMergeDiagCollect = SlotMergeDiagStart;
    auto SlotMergeDiagElapsedMs = [](std::chrono::steady_clock::time_point From) {
      return std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now() - From)
          .count();
    };
    std::vector<StructPtrSlotMergeCandidate> Candidates;
    if (Policy == "load-load-struct-ptr") {
      Candidates = collectStructPtrSameLoadMergeCandidates();
    } else if (Policy == "store-store-struct-ptr") {
      Candidates = collectStructPtrSameStoreMergeCandidates();
    } else {
      Candidates = collectStructPtrLoadStoreMergeCandidates();
    }
    if (MeasureSubphases) {
      recordBottomUpSubphase(
          ActiveBottomUpTimingContext,
          BottomUpSubstage::StructSlotCandidateCollect, CollectStart,
          Candidates.size(), ActiveBottomUpPostSummaryContext);
    }
    if (SlotMergeDiag) {
      llvm::errs() << "[slot-merge] " << Policy.str()
                   << " round collect_ms=" << SlotMergeDiagElapsedMs(SlotMergeDiagStart)
                   << " candidates=" << Candidates.size()
                   << " merged_so_far=" << Merged << "\n";
    }
    SlotMergeDiagCollect = std::chrono::steady_clock::now();
    BottomUpPhaseStart ApplyStart;
    if (MeasureSubphases) {
      ApplyStart = startBottomUpPhase(/*Enabled=*/true);
    }

    for (const auto &Candidate : Candidates) {
      auto FirstTarget = binarysub::resolve_variable(Candidate.FromTarget);
      auto SecondTarget = binarysub::resolve_variable(Candidate.IntoTarget);
      auto *FirstVar =
          FirstTarget ? FirstTarget->getAsVariableState() : nullptr;
      auto *SecondVar =
          SecondTarget ? SecondTarget->getAsVariableState() : nullptr;
      if (FirstVar == nullptr || SecondVar == nullptr ||
          FirstTarget.get() == SecondTarget.get()) {
        continue;
      }
      if (FirstVar->level != SecondVar->level ||
          FirstVar->size != SecondVar->size || FirstVar->size != PointerSize) {
        continue;
      }
      if (!HasStructEvidence(FirstTarget) ||
          !HasStructEvidence(SecondTarget)) {
        continue;
      }

      auto From = FirstVar->id <= SecondVar->id ? SecondTarget : FirstTarget;
      auto Into = FirstVar->id <= SecondVar->id ? FirstTarget : SecondTarget;
      std::string Detail = "ptr=";
      Detail += binarysub::debug_string(Candidate.Pointer);
      Detail += " size=" + std::to_string(Candidate.AccessSize);
      if (!tryMergeVariablesForPolicy(Policy, From, Into, Detail)) {
        continue;
      }

      ++Merged;
      Changed = true;
      // 成员级 StructEvidenceMemo 已由 onVariableMerged 精确失效
      // (From, Into)；这里的局部快照缓存已删除，避免全清 false。
      // 一轮内尽量合并所有候选（break 会导致每合并一对就全量重收集，
      // tmux/redis 这类大输入的 merge policy 因此卡数小时）；失效的
      // 候选会在检查里被跳过，新出现的候选由外层 while 下一轮处理。
      continue;
    }
    if (MeasureSubphases) {
      recordBottomUpSubphase(
          ActiveBottomUpTimingContext,
          BottomUpSubstage::StructSlotCandidateApply, ApplyStart,
          Candidates.size(), ActiveBottomUpPostSummaryContext);
    }
    if (!Changed) {
      if (SlotMergeDiag) {
        llvm::errs() << "[slot-merge] " << Policy.str()
                     << " round loop_ms=" << SlotMergeDiagElapsedMs(SlotMergeDiagCollect)
                     << " merged=" << Merged << " done\n";
      }
      return Merged;
    }
    if (SlotMergeDiag) {
      llvm::errs() << "[slot-merge] " << Policy.str()
                   << " round loop_ms=" << SlotMergeDiagElapsedMs(SlotMergeDiagCollect)
                   << " merged=" << Merged << "\n";
    }
  }
}

std::size_t ConstraintsGenerator::applyStructPtrLoadStoreMergePolicy() {
  std::size_t Merged = 0;
  Merged += applyStructPtrSlotMergeCandidates("load-load-struct-ptr");
  Merged += applyStructPtrSlotMergeCandidates("store-store-struct-ptr");
  Merged += applyStructPtrSlotMergeCandidates("load-store-struct-ptr");
  return Merged;
}

void ConstraintsGenerator::recordStore(ExtValuePtr Addr, SimpleType ValueTy,
                                       unsigned BitSize,
                                       llvm::Instruction *Source) {
  if (Source == nullptr || Source->getModule() == nullptr ||
      !isEVMModule(*Source->getModule())) {
    return;
  }
  EVMStoreAccesses.StoresByAddr[Addr].push_back(
      RecordedStore{.Addr = Addr, .ValueTy = ValueTy, .BitSize = BitSize,
                    .Source = Source});
}

void ConstraintsGenerator::addConstantMemoryField(ExtValuePtr Addr) {
  auto *ConstAddr = std::get_if<ConstantAddr>(&Addr);
  if (ConstAddr == nullptr) {
    return;
  }

  // A constant address is an offset into the one memory object, so record the
  // accessed field there. Keep only constant offsets here; dynamic offsets need
  // separate array/unknown modeling.
  auto Offset = getSigned64ConstantAddress(*ConstAddr->Val);
  if (!Offset.has_value()) {
    return;
  }
  // 统一字段约定：槽里放成员的**地址**（与 setAsPtrAdd 一致），不放加载出来的
  // 值。地址节点上已经挂着 PtrLoad/PtrStore(_, BitSize)（见 visitLoadInst /
  // visitStoreInst），TypeBuilder 对它剥一层得到成员类型、用 psize 得到成员宽度；
  // 值约定下同一份 record 会混用两种语义，指针成员还会被误剥成 pointee。
  std::vector<std::pair<std::string, SimpleType>> Fields;
  Fields.emplace_back(OffsetRange{.offset = *Offset}.str(),
                      getOrInsertNode(Addr));
  std::vector<std::vector<std::uint32_t>> FieldOrigins;
  {
    ConstraintsGenerator::BoundOriginScope ConstOrigin(
        *this, "constaddr:" + toStableString(Addr) + "@" +
                   std::to_string(*Offset),
        ConstraintsGenerator::BoundOriginKind::ConstAddr);
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
    if (ConstOrigin.originId() !=
        ConstraintsGenerator::BoundOriginScope::kNoOriginId) {
      FieldOrigins.push_back({ConstOrigin.originId()});
    }
#endif
    addSubtype(MemoryType, binarysub::make_record(std::move(Fields),
                                                  std::move(FieldOrigins)));
  }
}

SimpleType ConstraintsGenerator::getOrCreateStorageField(
    llvm::StringRef FieldName, unsigned BitSize) {
  assert(StorageType != nullptr && "StorageType must be initialized");
  assert(StorageFields != nullptr && "StorageFields must be initialized");

  auto It = StorageFields->find(FieldName.str());
  if (It != StorageFields->end()) {
    return It->second;
  }

  auto FieldTy =
      binarysub::make_variable(0, BitSize == 0 ? PointerSize : BitSize);
  std::string StableName = FieldName.str();
  StorageFields->insert({StableName, FieldTy});

  std::vector<std::pair<std::string, SimpleType>> Fields;
  Fields.emplace_back(StableName, FieldTy);
  addSubtype(StorageType, binarysub::make_record(std::move(Fields)));

  if (TraceStream != nullptr) {
    *TraceStream << "[storage:field] name=" << StableName
                 << " ty=" << binarysub::debug_string(FieldTy) << "\n";
    TraceStream->flush();
  }
  return FieldTy;
}

void ConstraintsGenerator::addMergeNode(SimpleType From, SimpleType To) {
  if (From == To) {
    return;
  }
  std::vector<ExtValuePtr> MovedValues;
  if (auto It = V2N.rev().find(From); It != V2N.rev().end()) {
    MovedValues = It->second;
  }
  emitMergeTrace("merge", From, To, MovedValues);
  if (MergeEval) {
    MergeEval->observeValueMapMerge(From, To, MovedValues);
  }
  V2N.merge(From, To);
}

void ConstraintsGenerator::configurePNDiffCallbacks() {
  PG.Policy = loadPNDiffPolicyConfig();
  PG.AllocateNodeId = []() { return ValueNamer::getId(); };
  PG.GetIntConstant = [this](PNIValue Val) {
    return getPNDiffIntConstantValue(getPNIExtValue(Val));
  };
  PG.MatchOffsetRange = [this](PNIValue Val) {
    return matchPNDiffOffsetRangeNoNegativeAccess(getPNIExtValue(Val));
  };
  PG.FormatValue = [this](PNIValue Val) {
    return toStableString(getPNIExtValue(Val));
  };
  PG.FormatInstruction = [](PNIInstruction Inst) {
    return formatPNDiffInstruction(Inst);
  };
  PG.OnUpdatePNType = [this](PNIValue Val) {
    onUpdatePNType(getPNIExtValue(Val));
  };
  PG.OnPtrAdd = [this](PNIValue Base, PNIValue Result, OffsetRange Off) {
    auto BaseValue = getPNIExtValue(Base);
    auto ResultValue = getPNIExtValue(Result);
    std::string Description = "ptradd:" + toStableString(BaseValue) + Off.str() +
                              "->" + toStableString(ResultValue);
    setAsPtrAdd(BaseValue, ResultValue, std::move(Off), Description);
  };
}

PNIValue ConstraintsGenerator::getPNIValue(const ExtValuePtr &Val) {
  auto It = PNDiffValueHandles.find(Val);
  if (It == PNDiffValueHandles.end()) {
    auto Inserted = PNDiffValueHandles.emplace(Val, nullptr).first;
    Inserted->second = std::make_unique<ExtValuePtr>(Inserted->first);
    It = Inserted;
  }
  return static_cast<PNIValue>(It->second.get());
}

const ExtValuePtr &ConstraintsGenerator::getPNIExtValue(PNIValue Val) const {
  assert(Val != nullptr);
  return *static_cast<const ExtValuePtr *>(Val);
}

PNTy ConstraintsGenerator::getPNILatticeType(const ExtValuePtr &Val) const {
  return retypd::makePNTyFromLLVMType(getType(Val), PointerSize);
}

PNINode *ConstraintsGenerator::getPNINodeOrNull(ExtValuePtr Val) {
  return PG.getPNIVarOrNull(getPNIValue(Val));
}

PNINode &ConstraintsGenerator::getPNINode(ExtValuePtr Val) {
  return PG.getPNIVar(getPNIValue(Val));
}

PNINode &ConstraintsGenerator::getOrInsertPNINode(ExtValuePtr Val) {
  return PG.getOrInsertPNINode(getPNIValue(Val), getPNILatticeType(Val));
}

void ConstraintsGenerator::remapPNINode(ExtValuePtr Val, ExtValuePtr Target) {
  PG.remapPNIVar(getPNIValue(Val), getPNIValue(Target));
}

void ConstraintsGenerator::unifyPNIValues(ExtValuePtr V1, ExtValuePtr V2) {
  PG.unifyVar(getPNIValue(V1), getPNIValue(V2));
}

SimpleType ConstraintsGenerator::createNode(ExtValuePtr Val) {
  // A value can legitimately be reached before its defining instruction is
  // visited: dominance does not imply textual block order, and canonicalisation
  // (PromotePass/SimplifyCFG, and the constants/globals pre-passes) can move a
  // definition into a block that is printed after one of its uses.  In that
  // case the use already materialised the value's node, which is the very node
  // this call is asking for, so reuse it instead of aborting.
  if (auto Existing = getNodeOrNull(Val); Existing != nullptr) {
    emitMappingTrace("create-reuse", Val, Existing);
    return Existing;
  }
  auto N = convertSimpleType(Val);
  auto It = V2N.insert(Val, N);
  if (!It.second) {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "createNode: Value already mapped to "
                 << It.first->second->str() << ", but now set to "
                 << toString(Val) << "\n";
    std::abort();
  }
  if (auto *VS = N->getAsVariableState()) {
    attachExternalValueHandle(N, Val);
    OriginalVariableSources[VS->id].insert(Val);
  }
  if (MergeEval) {
    MergeEval->observeValueNode(Val, N);
  }
  emitMappingTrace("create", Val, N);
  getOrInsertPNINode(Val);
  if (std::get_if<ConstantAddr>(&Val)) {
    setPointer(Val);
  }
  return N;
}

SimpleType ConstraintsGenerator::addRemapType(ExtValuePtr Val,
                                              ExtValuePtr Target) {
  auto Ty = getNodeOrNull(Target);
  assert(Ty != nullptr);
  auto N = getNodeOrNull(Val);
  if (N == Ty) {
    attachExternalValueHandle(Ty, Val);
    emitRemapTrace("remap-alias", Val, Target, Ty);
    remapPNINode(Val, Target);
    return N;
  }
  if (N != nullptr && N != Ty) {
    // The value already has a node of its own: it was materialised before its
    // defining instruction was visited (a use in an earlier-printed block, a
    // pre-pass, or a memory-object path), or an earlier cast already gave it
    // one.  The cast says the value denotes the same object as Target, so
    // connect the two nodes instead of overwriting the existing mapping --
    // constraints may already hang off it, and dropping them would lose type
    // evidence.  Two same-level variable nodes connected in both directions are
    // merged by the solver's reverse-edge rule, which is exactly the "same
    // object" relation this remap expresses.
    attachExternalValueHandle(N, Val);
    emitRemapTrace("remap-constrain", Val, Target, N);
    addSubtype(N, Ty);
    addSubtype(Ty, N);
    remapPNINode(Val, Target);
    return N;
  }
  auto It = V2N.insert(Val, Ty);
  if (!It.second) {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "setTypeVar: Value already mapped to "
                 << It.first->second->str() << ", but now set to "
                 << toString(Val) << "\n";
    std::abort();
  }
  attachExternalValueHandle(Ty, Val);
  emitRemapTrace("remap", Val, Target, Ty);
  if (MergeEval) {
    MergeEval->observeValueNode(Val, Ty);
  }
  remapPNINode(Val, Target);
  return It.first->second;
}

void MLsubRecovery::runImpl() {
  auto &M = const_cast<llvm::Module &>(Mod);
  LoadedPostConstraintState = false;
  if (!MergeEvalDir.empty()) {
    MergeEval = std::make_shared<MergePolicyEval>(M, MergeEvalDir, PointerSize);
  } else {
    MergeEval.reset();
  }

  // 0.4 prepare debug dir and SCCsCatalog
  auto WorkDir = notdec::getWorkDirOpt();
  if (WorkDir) {
    std::error_code EC = llvm::sys::fs::create_directories(*WorkDir);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open create directory " << *WorkDir << ": ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    SCCsCatalog.emplace(join(*WorkDir, "SCCs.txt"), EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output file SCCs.txt: ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }

    llvm::raw_fd_ostream ValueTypes(join(*WorkDir, kValueTypesFile.str()), EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output file " << kValueTypesFile.str() << ": ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    ValueTypes << "# Final Value -> binarysub UType mapping\n\n";

    llvm::raw_fd_ostream VarOrigins(join(*WorkDir, kVarOriginsFile.str()), EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output file " << kVarOriginsFile.str() << ": ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    VarOrigins << "# Final UType variable -> original ExtValuePtr mapping\n\n";
    auto PNDiffWarnPath = join(*WorkDir, kPNDiffWarnFile.str());
    if (std::error_code RemoveEC = llvm::sys::fs::remove(PNDiffWarnPath);
        RemoveEC && RemoveEC != std::errc::no_such_file_or_directory) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot remove output file " << kPNDiffWarnFile.str()
                << ": " << RemoveEC.message() << std::endl;
      std::abort();
    }
  }

  WrotePNDiffOverrideWarningHeader = false;

  if (envFlagEnabled(kBinarysubTraceEnv)) {
    if (!WorkDir) {
      llvm::errs() << "Warning: " << kBinarysubTraceEnv
                   << "=1 but no workdir is configured; skip "
                   << kBinarysubTraceFile << ".\n";
    } else {
      BinarysubTraceFile = std::make_unique<std::ofstream>(
          join(*WorkDir, kBinarysubTraceFile.str()), std::ios::trunc);
      if (!*BinarysubTraceFile) {
        llvm::errs() << "Cannot open " << kBinarysubTraceFile << " in "
                     << *WorkDir << ".\n";
        std::abort();
      }
      *BinarysubTraceFile << "# binarysub / pndiff trace\n";
      *BinarysubTraceFile << "# enabled by " << kBinarysubTraceEnv.str()
                          << "=1\n";
      *BinarysubTraceFile << "# value/simple-type bindings use [mapping:*]\n";
      BinarysubTraceFile->flush();
    }
  } else {
    BinarysubTraceFile.reset();
  }

  binarysub::binarysub_set_trace_stream(BinarysubTraceFile.get());

  // A load run must begin with an empty binarysub context so restore can
  // recreate the saved variable IDs exactly. Fresh roots are created only for
  // the constraint-generation path.
  if (LoadPostConstraintStateDir.empty() && !MemoryType) {
    MemoryType = binarysub::make_variable(0, PointerSize);
  }
  if (LoadPostConstraintStateDir.empty() && isEVMModule(M) && !StorageType) {
    StorageType = binarysub::make_variable(0, PointerSize);
  }

  // The summary/signature override documents and the primitive semantic
  // lattices are part of recognition; PolymorphicMarkingPass loads them before
  // it marks, and standalone users get them here.
  if (!OverridesLoaded) {
    loadOverridesAndPrimitives(M);
  }

  // set global pointer size variable for binarysub
  binarysub::pointer_size = PointerSize;

  auto MLsubInputText = renderModuleToString(M);
  auto ModuleSHA256Hex = computeSHA256Hex(MLsubInputText);

  // 0.5 print module for debugging
  emitTRInputArtifacts(M, "");

  if (ExtraConstraintsFile != nullptr) {
    validateExtraConstraintsFile(M, ExtraConstraintsFile, ModuleSHA256Hex);
  }

  // Recognition and marking normally happen in PolymorphicMarkingPass, which
  // the pipeline runs before this pass: the mark has to exist before SCC level
  // assignment and constraint generation.  Standalone users (tests, direct
  // MLsubRecovery callers that do not add the pass) still get the same two
  // sources here.
  if (!PolymorphicMarkingDone) {
    detectMallocWrappers(M);
    markBuiltinPolymorphicBufferFunctions(M);
  }

  // Recognition-only mode: PolymorphicMarkingPass has already written the
  // per-function report, so stop before the call graph and the constraints.
  if (!polymorphicMarkingReportPath().empty()) {
    return;
  }

  CallGraphAnalysis Ana;
  CallG = std::make_unique<CallGraph>(Ana.run(M, MAM));


  if (WorkDir) {
    std::error_code EC;
    auto Path = join(*WorkDir, "CallGraph.txt");
    llvm::raw_fd_ostream CGTxt(Path, EC);
    if (EC) {
      llvm::errs() << "Error printing to " << Path << ", " << EC.message()
                   << "\n";
    }
    CallG->print(CGTxt);
    CGTxt.close();
    // print dot
    Path = join(*WorkDir, "CallGraph.dot");
    llvm::raw_fd_ostream CGDot(Path, EC);
    if (EC) {
      llvm::errs() << "Error printing to " << Path << ", " << EC.message()
                   << "\n";
    }
    notdec::utils::CallGraphDOTInfo CFGInfo(&M, &*CallG, nullptr, true);
    llvm::WriteGraph(CGDot, &CFGInfo, false);
    CGDot.close();

    Path = join(*WorkDir, "CallGraphFull.dot");
    llvm::raw_fd_ostream CGFullDot(Path, EC);
    if (EC) {
      llvm::errs() << "Error printing to " << Path << ", " << EC.message()
                   << "\n";
    }
    notdec::utils::CallGraphDOTInfo FullCFGInfo(&M, &*CallG, nullptr, false);
    llvm::WriteGraph(CGFullDot, &FullCFGInfo, false);
    CGFullDot.close();
  }

  prepareSCC(*CallG);

  if (!LoadPostConstraintStateDir.empty()) {
    loadPostConstraintState(LoadPostConstraintStateDir, ModuleSHA256Hex);
  } else {
    bottomUpPhase();

    topDownConstraintPhase();

    if (!EmitPostConstraintStateDir.empty()) {
      savePostConstraintState(EmitPostConstraintStateDir, ModuleSHA256Hex);
    }
  }

  solveAndLowerTypes();

  if (WorkDir && !LoadedPostConstraintState) {
    writeCallSlotMergeDecisions(
        join(*WorkDir, kCallSlotMergeDecisionsFile.str()), AG);
    writeLocalSubtypeMergeStats(
        join(*WorkDir, kLocalSubtypeMergeStatsFile.str()), AG);
  } else if (WorkDir && LoadedPostConstraintState) {
    llvm::errs() << "Warning: post-constraint checkpoint format version 1 does "
                    "not restore merge-decision history; skip its workdir "
                    "diagnostics.\n";
  }

  if (WorkDir && ResultVal == nullptr) {
    genASTTypes(M);
  }

  if (WorkDir) {
    writePNDiffWarnings(join(*WorkDir, kPNDiffWarnFile.str()), AG,
                        WrotePNDiffOverrideWarningHeader);
    writePNDiffAnnotatedModule(
        M, join(*WorkDir, kPNDiffAnnotatedFile.str()), AG);
  }

  std::cerr << "Constraint generation done! SCC count:" << AG.AllSCCs.size()
            << "\n";
  {
    // NOTDEC_RULE_LOCAL_RETRY diagnostics: coexistence conflicts seen, calls
    // rolled back and redone without the store->load coupling, and calls kept
    // because they merged variables (those run host hooks outside the
    // transaction and are therefore not redone).
    std::size_t Conflicts = 0;
    std::size_t Redone = 0;
    std::size_t SkippedMerged = 0;
    for (const auto &Data : AG.AllSCCs) {
      if (Data.Generator == nullptr) {
        continue;
      }
      Conflicts += Data.Generator->RuleLocalConflicts;
      Redone += Data.Generator->RuleLocalRetries;
      SkippedMerged += Data.Generator->RuleLocalSkippedMerged;
    }
    if (Conflicts != 0 || Redone != 0 || SkippedMerged != 0) {
      std::size_t MergeConflicts = 0;
      std::size_t MergeRedone = 0;
      for (const auto &Data : AG.AllSCCs) {
        if (Data.Generator == nullptr) {
          continue;
        }
        MergeConflicts += Data.Generator->MergeLocalConflicts;
        MergeRedone += Data.Generator->MergeLocalRetries;
      }
      std::cerr << "[rule-local-retry] conflicts=" << Conflicts
                << " redone=" << Redone << " kept-because-merged="
                << SkippedMerged << " merge-conflicts=" << MergeConflicts
                << " merge-redone=" << MergeRedone
                << " coexist-events-total="
                << binarysub::ConstraintContext::intStructCoexistEventCount()
                << " coexist-events-in-merge="
                << binarysub::ConstraintContext::intStructCoexistEventInMergeCount()
                << "\n";
    }
  }

  binarysub::binarysub_set_trace_stream(nullptr);
  if (BinarysubTraceFile) {
    BinarysubTraceFile->flush();
    BinarysubTraceFile.reset();
  }
}

void MLsubRecovery::run() {
  FailureReason.clear();
  try {
    runImpl();
  } catch (const SimplifyDiagnosticStop &Stop) {
    if (ThrowOnDiagnosticStop) {
      throw;
    }
    FailureReason = Stop.what();
  } catch (const std::exception &Error) {
    if (ThrowOnDiagnosticStop) {
      throw;
    }
    FailureReason = Error.what();
  }
}

void MLsubRecovery::loadSummaryFile(llvm::Module &M, const char *Path,
                                    bool StrictValidation) {
  loadOverrideFileImpl(M, Path, SummaryOverrideDoc, SummaryOverrideFuncs,
                       StrictValidation, false);
}

void MLsubRecovery::loadSignatureFile(llvm::Module &M, const char *Path,
                                      bool StrictValidation) {
  loadOverrideFileImpl(M, Path, SignatureOverrideDoc, SignatureOverrideFuncs,
                       StrictValidation, true);
}

void MLsubRecovery::validateExtraConstraintsFile(llvm::Module &M,
                                                 const char *Path,
                                                 llvm::StringRef ModuleSHA256Hex) {
  llvm::errs() << "Loading MLsub extra constraints from: " << Path << "\n";
  ExtraConstraintsDoc = makeEmptyOverrideDoc();
  ExtraConstraintsFuncs.clear();
  auto Parsed = llvm::json::parse(readFileToString(Path));
  if (!Parsed) {
    failExtraConstraints("",
                         ("JSON parse failed for " + std::string(Path)).c_str());
  }

  const auto *Root = Parsed->getAsObject();
  if (Root == nullptr) {
    failExtraConstraints("<root>", "expected object");
  }

  auto Version = Root->getInteger("version");
  if (!Version || *Version != 1) {
    failExtraConstraints("<root>", "expected version = 1");
  }

  validateExtraConstraintsAnchor(*Root, M, ModuleSHA256Hex);
  validateExtraConstraintFunctions(*Root, M, ExtraConstraintsDoc,
                                   ExtraConstraintsFuncs);
}

void MLsubRecovery::emitTRInputArtifacts(llvm::Module &M,
                                         llvm::StringRef OutputPath) {
  auto MLsubInputText = renderModuleToString(M);
  auto ModuleSHA256Hex = computeSHA256Hex(MLsubInputText);

  if (auto WorkDir = notdec::getWorkDirOpt()) {
    auto MLsubInputPath = join(*WorkDir, kMLsubInputIRFile.str());
    printModule(M, MLsubInputPath.c_str());
    writeMLsubInputAnchor(M, join(*WorkDir, kMLsubInputAnchorFile.str()),
                          ModuleSHA256Hex);
    writeSelectableValues(M, join(*WorkDir, kSelectableValuesFile.str()));
  }

  if (!OutputPath.empty()) {
    writeTRInputModule(M, OutputPath);
    llvm::errs() << "TR input IR emitted to " << OutputPath << "\n";
  }
}

void MLsubRecovery::loadOverridesAndPrimitives(llvm::Module &M) {
  SummaryOverrideFuncs.clear();
  SummaryOverrideDoc = makeEmptyOverrideDoc();
  SignatureOverrideFuncs.clear();
  SignatureOverrideDoc = makeEmptyOverrideDoc();
  if (auto RegistrySetup = registerBuiltinPrimitiveSemanticLatticesForModule(M);
      !RegistrySetup) {
    llvm::errs() << "Error: failed to register builtin primitive semantic "
                    "lattice: "
                 << RegistrySetup.error().msg << "\n";
    std::abort();
  }
  if (!kDefaultMLsubBuiltinSummaryOverridePath.empty() &&
      llvm::sys::fs::exists(kDefaultMLsubBuiltinSummaryOverridePath)) {
    loadSummaryFile(M, kDefaultMLsubBuiltinSummaryOverridePath.data(), false);
  }
  if (SummaryFile != nullptr) {
    loadSummaryFile(M, SummaryFile, true);
  }
  if (SignatureFile != nullptr) {
    loadSignatureFile(M, SignatureFile, true);
  }
  OverridesLoaded = true;
}

void MLsubRecovery::detectMallocWrappers(llvm::Module &M) {
  DetectedMallocWrappers.clear();
  std::vector<std::pair<std::string, std::string>> Rows;

  bool Discovered = true;
  while (Discovered) {
    Discovered = false;
    for (auto &F : M) {
    if (DetectedMallocWrappers.count(&F) != 0) {
      continue;
    }
    auto *Alloc = getGenericMallocWrapperAllocator(F);
    if (Alloc == nullptr) {
      continue;
    }
    Discovered = true;

    F.setMetadata(KIND_MLSUB_POLYMORPHIC_FUNCTION,
                  llvm::MDNode::get(F.getContext(), {}));
    DetectedMallocWrappers.insert(&F);
    knownAllocatorWrappers().insert(&F);

    auto AllocName = Alloc->getCalledFunction()->getName().str();
    Rows.push_back({F.getName().str(), AllocName});
    llvm::errs() << "Info: detected generic malloc wrapper: " << F.getName()
                 << " via " << AllocName << "\n";
  }
  }

  if (auto WorkDir = notdec::getWorkDirOpt()) {
    std::error_code EC;
    auto Path = join(*WorkDir, kMallocWrappersFile.str());
    llvm::raw_fd_ostream Out(Path, EC, llvm::sys::fs::OF_Text);
    if (EC) {
      llvm::errs() << "Warning: cannot write " << kMallocWrappersFile << ": "
                   << EC.message() << "\n";
      return;
    }

    Out << "# Generic malloc wrappers detected before SCC partitioning\n";
    if (Rows.empty()) {
      Out << "No generic malloc wrappers detected.\n";
      return;
    }
    for (const auto &[WrapperName, AllocName] : Rows) {
      Out << WrapperName << "\t" << AllocName << "\n";
    }
  }
}

void MLsubRecovery::markBuiltinPolymorphicBufferFunctions(llvm::Module &M) {
  DetectedPolymorphicBufferFunctions.clear();
  std::vector<std::string> Rows;
  if (!polymorphicFunctionMarkingEnabled()) {
    llvm::errs() << "Info: polymorphic function marking disabled "
                    "(NOTDEC_POLYMORPHIC_FUNCTIONS is not 1)\n";
    return;
  }

  for (auto &F : M) {
    if (!F.hasName()) {
      continue;
    }
    if (!isBuiltinPolymorphicBufferFunctionName(F.getName())) {
      continue;
    }

    // These APIs operate on caller-owned buffers or generic object contexts. A
    // single declaration node would otherwise force unrelated callsites to
    // share one formal argument type. Defined functions in the list (e.g. the
    // redis zmalloc family) get the same treatment: the SCC boundary
    // instantiates each callsite independently instead of merging all of them
    // through one function type.
    F.setMetadata(KIND_MLSUB_POLYMORPHIC_FUNCTION,
                  llvm::MDNode::get(F.getContext(), {}));
    DetectedPolymorphicBufferFunctions.insert(&F);
    Rows.push_back(F.getName().str());
    llvm::errs() << "Info: builtin polymorphic buffer function: "
                 << F.getName() << "\n";
  }

  if (auto WorkDir = notdec::getWorkDirOpt()) {
    std::error_code EC;
    auto Path = join(*WorkDir, kPolymorphicBufferFunctionsFile.str());
    llvm::raw_fd_ostream Out(Path, EC, llvm::sys::fs::OF_Text);
    if (EC) {
      llvm::errs() << "Warning: cannot write "
                   << kPolymorphicBufferFunctionsFile << ": "
                   << EC.message() << "\n";
      return;
    }

    Out << "# Builtin generic functions marked polymorphic before SCC "
           "partitioning\n";
    if (Rows.empty()) {
      Out << "No builtin polymorphic buffer functions detected.\n";
      return;
    }
    for (const auto &Name : Rows) {
      Out << Name << "\n";
    }
  }
}

const llvm::json::Value *
MLsubRecovery::getExtraConstraintsSpec(const llvm::Function &Func) const {
  return getOverrideSpecImpl(ExtraConstraintsDoc, ExtraConstraintsFuncs, Func);
}

namespace {

// Cached once, like the other policy accessors above.
const std::map<std::string, llvm::json::Value> &unifiedPndiffSpecs() {
  static const std::map<std::string, llvm::json::Value> Specs =
      loadPolyPolicyConfig().PndiffSpecs;
  return Specs;
}

llvm::StringRef stripUnderscores(llvm::StringRef Name) {
  while (!Name.empty() && Name.front() == '_') {
    Name = Name.drop_front();
  }
  return Name;
}

} // namespace

const llvm::json::Value *
MLsubRecovery::getSummaryOverrideSpec(const llvm::Function &Func) const {
  if (auto *Spec =
          getOverrideSpecImpl(SummaryOverrideDoc, SummaryOverrideFuncs, Func)) {
    return Spec;
  }
  // Unified policy documents may carry PNDiff annotations; hand them to the same
  // application path the summary override uses.
  const auto &Unified = unifiedPndiffSpecs();
  if (Unified.empty()) {
    return nullptr;
  }
  if (auto It = Unified.find(Func.getName().str()); It != Unified.end()) {
    return &It->second;
  }
  for (const auto &Ent : Unified) {
    if (stripUnderscores(Ent.first) == stripUnderscores(Func.getName())) {
      return &Ent.second;
    }
  }
  return nullptr;
}

// Exposed to PolymorphicMarkingPass so its report can name the policy-file
// source; the policy struct itself stays private to this translation unit.
std::set<std::string> loadPolyPolicyFunctionNames() {
  return loadPolyPolicyConfig().PolyFuncs;
}

namespace {

llvm::StringRef stripLeadingUnderscores(llvm::StringRef Name) {
  while (!Name.empty() && Name.front() == '_') {
    Name = Name.drop_front();
  }
  return Name;
}

// NOTDEC_RET_ALIAS="memcpy:0,memmove:0" (same syntax as the JSON key
// "ret_alias_of_arg": {"memcpy": 0}); the env wins so the annotation can be
// tried without editing the policy file.
const std::map<std::string, unsigned int> &envRetAliasOfArg() {
  static const std::map<std::string, unsigned int> Aliases = [] {
    std::map<std::string, unsigned int> Result;
    const char *Raw = std::getenv("NOTDEC_RET_ALIAS");
    if (Raw == nullptr || *Raw == '\0') {
      return Result;
    }
    llvm::SmallVector<llvm::StringRef, 8> Entries;
    llvm::StringRef(Raw).split(Entries, ',');
    for (llvm::StringRef Entry : Entries) {
      auto [Name, IndexText] = Entry.split(':');
      Name = Name.trim();
      IndexText = IndexText.trim();
      if (Name.empty() || IndexText.empty()) {
        continue;
      }
      unsigned int AliasArg = 0;
      if (IndexText.getAsInteger(10, AliasArg)) {
        continue;
      }
      Result[stripLeadingUnderscores(Name).str()] = AliasArg;
    }
    return Result;
  }();
  return Aliases;
}

} // namespace

const std::set<unsigned int> &envPointerArgs() {
  static const std::set<unsigned int> Empty;
  return Empty;
}

bool annotationMatches(const std::string &Key, llvm::StringRef Normalized) {
  return stripLeadingUnderscores(Key) == Normalized;
}

// Built-in semantics for the recognized C library family.  These are what the
// lifted PLT stub bodies used to encode (a body recognized as llvm.memcpy marks
// both pointer arguments and ties the result to the destination); declaring the
// stubs instead of lifting them must not lose that information, otherwise the
// accuracy drops (O0/stat 0.6416 -> 0.4116 measured before this table existed).
struct BuiltinImportSemantics {
  std::set<unsigned int> PointerArgs;
  bool PointerRet = false;
  std::optional<unsigned int> RetAlias;
  std::vector<std::pair<unsigned int, unsigned int>> ArgAliases;
};

const std::map<std::string, BuiltinImportSemantics> &
builtinImportSemantics() {
  static const std::map<std::string, BuiltinImportSemantics> Table = [] {
    std::map<std::string, BuiltinImportSemantics> T;
    auto Add = [&](const char *Name, std::set<unsigned int> Ptr, bool PtrRet,
                   std::optional<unsigned int> RetAlias,
                   std::vector<std::pair<unsigned int, unsigned int>> Aliases) {
      T.emplace(Name, BuiltinImportSemantics{std::move(Ptr), PtrRet, RetAlias,
                                             std::move(Aliases)});
    };
    // Copies: destination and source have the same pointee type, and the
    // destination is returned (mempcpy returns dest + n, bcopy returns void).
    Add("memcpy", {0, 1}, false, 0u, {{0, 1}});
    Add("memmove", {0, 1}, false, 0u, {{0, 1}});
    Add("strcpy", {0, 1}, false, 0u, {{0, 1}});
    Add("strncpy", {0, 1}, false, 0u, {{0, 1}});
    Add("mempcpy", {0, 1}, false, std::nullopt, {{0, 1}});
    Add("bcopy", {0, 1}, false, std::nullopt, {{0, 1}});
    Add("__memcpy_chk", {0, 1}, false, 0u, {{0, 1}});
    Add("__memmove_chk", {0, 1}, false, 0u, {{0, 1}});
    Add("__strcpy_chk", {0, 1}, false, 0u, {{0, 1}});
    Add("__strncpy_chk", {0, 1}, false, 0u, {{0, 1}});
    // Not copies: only the pointer evidence / the returned destination.
    Add("memset", {0}, false, 0u, {});
    Add("strcat", {0, 1}, false, 0u, {});
    Add("strncat", {0, 1}, false, 0u, {});
    return T;
  }();
  return Table;
}

bool importSemanticsEnabled(const char *Kind) {
  static const std::string Spec = [] {
    const char *V = std::getenv("NOTDEC_IMPORT_SEMANTICS");
    return std::string(V == nullptr ? "all" : V);
  }();
  if (Spec == "none") {
    return false;
  }
  if (Spec == "ret1") {
    return std::string(Kind) == "ret";
  }
  if (Spec == "all") {
    return true;
  }
  llvm::SmallVector<llvm::StringRef, 8> Parts;
  llvm::StringRef(Spec).split(Parts, ',');
  for (llvm::StringRef Part : Parts) {
    if (Part.trim() == Kind) {
      return true;
    }
  }
  return false;
}

const BuiltinImportSemantics *lookupBuiltinSemantics(llvm::StringRef Name) {
  const auto &Table = builtinImportSemantics();
  const std::string Normalized = stripLeadingUnderscores(Name).str();
  for (const auto &Ent : Table) {
    if (stripLeadingUnderscores(Ent.first) == Normalized) {
      return &Ent.second;
    }
  }
  return nullptr;
}

std::vector<std::pair<unsigned int, unsigned int>>
lookupArgAliases(const llvm::Function &F) {
  std::vector<std::pair<unsigned int, unsigned int>> Out;
  if (const auto *B = lookupBuiltinSemantics(F.getName());
      B && importSemanticsEnabled("arg")) {
    Out = B->ArgAliases;
  }
  const std::string Normalized = stripLeadingUnderscores(F.getName()).str();
  const auto &Config = loadPolyPolicyConfig();
  for (const auto &Ent : Config.ArgAliases) {
    if (annotationMatches(Ent.first, Normalized)) {
      Out.insert(Out.end(), Ent.second.begin(), Ent.second.end());
    }
  }
  if (const char *Raw = std::getenv("NOTDEC_ARG_ALIAS")) {
    llvm::SmallVector<llvm::StringRef, 8> Entries;
    llvm::StringRef(Raw).split(Entries, ',');
    for (llvm::StringRef Entry : Entries) {
      auto [Name, Pairs] = Entry.split(':');
      if (stripLeadingUnderscores(Name.trim()) != Normalized) {
        continue;
      }
      llvm::SmallVector<llvm::StringRef, 8> Items;
      Pairs.split(Items, ';');
      for (llvm::StringRef Item : Items) {
        auto [L, R] = Item.split('=');
        unsigned int LI = 0;
        unsigned int RI = 0;
        if (!L.trim().getAsInteger(10, LI) && !R.trim().getAsInteger(10, RI)) {
          Out.emplace_back(LI, RI);
        }
      }
    }
  }
  return Out;
}


std::set<unsigned int> lookupPointerArgs(const llvm::Function &F) {
  const std::string Normalized = stripLeadingUnderscores(F.getName()).str();
  std::set<unsigned int> Out;
  if (const auto *B = lookupBuiltinSemantics(F.getName());
      B && importSemanticsEnabled("ptr")) {
    Out = B->PointerArgs;
  }
  const auto &Config = loadPolyPolicyConfig().PointerArgs;
  for (const auto &Ent : Config) {
    if (annotationMatches(Ent.first, Normalized)) {
      Out.insert(Ent.second.begin(), Ent.second.end());
    }
  }
  // Env form: NOTDEC_POINTER_ARGS="memcpy:0,1;free:0"
  if (const char *Raw = std::getenv("NOTDEC_POINTER_ARGS")) {
    llvm::SmallVector<llvm::StringRef, 8> Entries;
    llvm::StringRef(Raw).split(Entries, ';');
    for (llvm::StringRef Entry : Entries) {
      auto [Name, ArgsText] = Entry.split(':');
      if (stripLeadingUnderscores(Name.trim()) != Normalized) {
        continue;
      }
      llvm::SmallVector<llvm::StringRef, 8> Args;
      ArgsText.split(Args, ',');
      for (llvm::StringRef A : Args) {
        unsigned int Index = 0;
        if (!A.trim().getAsInteger(10, Index)) {
          Out.insert(Index);
        }
      }
    }
  }
  return Out;
}

bool lookupPointerRet(const llvm::Function &F) {
  if (const auto *B = lookupBuiltinSemantics(F.getName());
      B && B->PointerRet && importSemanticsEnabled("ptr")) {
    return true;
  }
  const std::string Normalized = stripLeadingUnderscores(F.getName()).str();
  for (const auto &Name : loadPolyPolicyConfig().PointerRet) {
    if (annotationMatches(Name, Normalized)) {
      return true;
    }
  }
  if (const char *Raw = std::getenv("NOTDEC_POINTER_RET")) {
    llvm::SmallVector<llvm::StringRef, 8> Entries;
    llvm::StringRef(Raw).split(Entries, ',');
    for (llvm::StringRef Entry : Entries) {
      if (stripLeadingUnderscores(Entry.trim()) == Normalized) {
        return true;
      }
    }
  }
  return false;
}

std::optional<unsigned int> lookupRetAliasOfArg(const llvm::Function &F) {
  if (const auto *B = lookupBuiltinSemantics(F.getName());
      B && B->RetAlias && importSemanticsEnabled("ret")) {
    return B->RetAlias;
  }
  const std::string Normalized = stripLeadingUnderscores(F.getName()).str();
  if (auto It = envRetAliasOfArg().find(Normalized);
      It != envRetAliasOfArg().end()) {
    return It->second;
  }
  const auto &Aliases = loadPolyPolicyConfig().RetAliasOfArg;
  for (const auto &Ent : Aliases) {
    if (stripLeadingUnderscores(Ent.first) == Normalized) {
      return Ent.second;
    }
  }
  return std::nullopt;
}

namespace {

// "name:args=0,2;ret=1,name2:args=1" - parsed once, cached.
const std::map<std::string, PolymorphicPositionSpec> &
monomorphicPositionOverrides() {
  static const std::map<std::string, PolymorphicPositionSpec> Overrides = [] {
    std::map<std::string, PolymorphicPositionSpec> Result;
    const char *Raw = std::getenv("NOTDEC_POLY_MONOMORPHIC");
    if (Raw == nullptr || *Raw == '\0') {
      return Result;
    }
    llvm::SmallVector<llvm::StringRef, 8> Entries;
    llvm::StringRef(Raw).split(Entries, ',');
    for (llvm::StringRef Entry : Entries) {
      auto [Name, SpecText] = Entry.split(':');
      Name = Name.trim();
      if (Name.empty()) {
        continue;
      }
      auto Spec = PolymorphicPositionSpec::parse(SpecText.trim());
      if (!Spec) {
        llvm::errs() << "Warning: ignoring malformed NOTDEC_POLY_MONOMORPHIC "
                        "entry: "
                     << Entry << "\n";
        continue;
      }
      Result[Name.str()] = *Spec;
    }
    return Result;
  }();
  return Overrides;
}

} // namespace

PolymorphicPositionSpec
resolvePolymorphicPositionSpec(const llvm::Function &F) {
  if (auto Spec = getPolymorphicPositionSpec(F)) {
    if (std::getenv("NOTDEC_POLY_MONO_DIAG") != nullptr) {
      llvm::errs() << "[poly-mono-resolve] " << F.getName()
                   << " from metadata: " << Spec->str() << "\n";
    }
    return *Spec;
  }
  const std::string Name = F.getName().str();
  const auto &Overrides = monomorphicPositionOverrides();
  if (auto It = Overrides.find(Name); It != Overrides.end()) {
    if (std::getenv("NOTDEC_POLY_MONO_DIAG") != nullptr) {
      llvm::errs() << "[poly-mono-resolve] " << Name
                   << " from env: " << It->second.str() << "\n";
    }
    return It->second;
  }
  // Read the policy once: this resolver runs for every unhandled call.
  static const std::map<std::string, PolymorphicPositionSpec> Policy =
      loadPolyPolicyConfig().MonomorphicPositions;
  if (auto It = Policy.find(Name); It != Policy.end()) {
    if (std::getenv("NOTDEC_POLY_MONO_DIAG") != nullptr) {
      llvm::errs() << "[poly-mono-resolve] " << Name
                   << " from policy: " << It->second.str() << "\n";
    }
    return It->second;
  }
  return {};
}

bool MLsubRecovery::isSummaryOverridePolymorphic(
    const llvm::Function &Func) const {
  auto *Spec = getSummaryOverrideSpec(Func);
  if (Spec == nullptr) {
    return false;
  }
  const auto *Obj = Spec->getAsObject();
  if (Obj == nullptr) {
    return false;
  }
  if (auto Flag = Obj->getBoolean("is_polymorphic")) {
    return *Flag;
  }
  return false;
}

bool MLsubRecovery::isOpaqueBody(const llvm::Function &Func) const {
  auto *Spec = getSummaryOverrideSpec(Func);
  if (Spec == nullptr) {
    return false;
  }
  const auto *Obj = Spec->getAsObject();
  if (Obj == nullptr) {
    return false;
  }
  if (auto Flag = Obj->getBoolean("opaque_body")) {
    return *Flag;
  }
  return false;
}

const llvm::json::Value *
MLsubRecovery::getSignatureOverrideSpec(const llvm::Function &Func) const {
  return getOverrideSpecImpl(SignatureOverrideDoc, SignatureOverrideFuncs,
                             Func);
}

MLsubRecovery::OverrideTypeRecipe
MLsubRecovery::buildOverrideType(const llvm::json::Value &Expr,
                                 OverrideBuildContext &Ctx,
                                 llvm::StringRef Path, bool AllowNull) {
  OverrideTypeRecipe Recipe;
  if (Expr.getAsNull()) {
    if (!AllowNull) {
      failSignatureOverride(Path, "null is only allowed for function return");
    }
    return Recipe;
  }

  const auto &Obj = requireObject(Expr, Path);
  auto Kind = Obj.getString("kind");
  if (!Kind) {
    failSignatureOverride(Path, "missing string field 'kind'");
  }

  if (*Kind == "primitive") {
    auto Name = requireString(Obj, "name", Path);
    auto Bits = requireInteger(Obj, "bits", Path);
    if (Bits <= 0) {
      failSignatureOverride(Path, "primitive bits must be positive");
    }

    auto &Registry = binarysub::globalPrimitiveSemanticRegistry();
    if (const auto *Family = Registry.findFamilyByCanonicalName(Name)) {
      if (Family->bits != static_cast<std::uint32_t>(Bits)) {
        failSignatureOverride(Path,
                              "semantic primitive bit width mismatch");
      }
    } else if (Name != "bool" && Name != "sint" && Name != "uint" &&
               Name != "float" && Name != "double" && Name != "char") {
      failSignatureOverride(Path, "unknown primitive name");
    }

    Recipe.Root =
        binarysub::make_primitive(Name.str(), static_cast<std::uint32_t>(Bits));
    return Recipe;
  }

  if (*Kind == "var") {
    auto ID = requireInteger(Obj, "id", Path);
    auto Bits = requireInteger(Obj, "bits", Path);
    if (ID < 0 || Bits <= 0) {
      failSignatureOverride(Path, "var id/bits must be positive");
    }
    auto Width = static_cast<std::uint32_t>(Bits);
    auto WidthIt = Ctx.VarBitWidths.find(ID);
    if (WidthIt != Ctx.VarBitWidths.end() && WidthIt->second != Width) {
      failSignatureOverride(Path, "var bits mismatch for reused id");
    }
    auto It = Ctx.Vars.find(ID);
    if (It == Ctx.Vars.end()) {
      auto Ty = binarysub::make_variable(Ctx.Generator.lvl, Width);
      Ctx.Vars.insert({ID, Ty});
      Ctx.VarBitWidths.insert({ID, Width});
      Recipe.Root = Ty;
    } else {
      Recipe.Root = It->second;
    }
    return Recipe;
  }

  if (*Kind == "ref") {
    auto ID = requireInteger(Obj, "id", Path);
    if (ID < 0) {
      failSignatureOverride(Path, "ref id must be non-negative");
    }
    auto It = Ctx.Vars.find(ID);
    if (It == Ctx.Vars.end()) {
      failSignatureOverride(Path, "ref points to an undefined var id");
    }
    Recipe.Root = It->second;
    return Recipe;
  }

  if (*Kind == "ptr") {
    Recipe.Root = binarysub::fresh_variable(Ctx.Generator.lvl,
                                            Ctx.Generator.PointerSize);
    if (auto *Load = Obj.get("load")) {
      auto LoadRecipe = buildOverrideType(
          *Load, Ctx, appendJSONPath(Path, "load"), false);
      appendOverrideConstraints(Recipe, std::move(LoadRecipe));
      auto AccessBits = binarysub::get_size(LoadRecipe.Root);
      Recipe.Constraints.push_back(
          {Recipe.Root,
           ptrAccessCapability(LoadRecipe.Root, AccessBits, /*Store=*/false)});
    }
    if (auto *Store = Obj.get("store")) {
      auto StoreRecipe = buildOverrideType(
          *Store, Ctx, appendJSONPath(Path, "store"), false);
      appendOverrideConstraints(Recipe, std::move(StoreRecipe));
      auto AccessBits = binarysub::get_size(StoreRecipe.Root);
      Recipe.Constraints.push_back(
          {Recipe.Root,
           ptrAccessCapability(StoreRecipe.Root, AccessBits, /*Store=*/true)});
    }
    if (Obj.get("load") == nullptr && Obj.get("store") == nullptr) {
      failSignatureOverride(Path, "ptr requires at least one of load/store");
    }
    return Recipe;
  }

  if (*Kind == "record") {
    const auto *FieldsValue = Obj.get("fields");
    if (FieldsValue == nullptr) {
      failSignatureOverride(Path, "missing array field 'fields'");
    }
    const auto &FieldsArray =
        requireArray(*FieldsValue, appendJSONPath(Path, "fields"));
    std::vector<std::pair<std::string, SimpleType>> Fields;
    Fields.reserve(FieldsArray.size());
    for (size_t Index = 0; Index < FieldsArray.size(); ++Index) {
      auto EntryPath = appendJSONIndexPath(appendJSONPath(Path, "fields"), Index);
      const auto &FieldObj = requireObject(FieldsArray[Index], EntryPath);
      auto Offset = requireString(FieldObj, "offset", EntryPath);
      auto *TypeValue = FieldObj.get("type");
      if (TypeValue == nullptr) {
        failSignatureOverride(EntryPath, "missing field 'type'");
      }
      auto FieldRecipe = buildOverrideType(
          *TypeValue, Ctx, appendJSONPath(EntryPath, "type"), false);
      appendOverrideConstraints(Recipe, std::move(FieldRecipe));
      Fields.push_back({Offset.str(), FieldRecipe.Root});
    }
    Recipe.Root = binarysub::make_record(std::move(Fields));
    return Recipe;
  }

  failSignatureOverride(Path, "unsupported override type kind");
}

void MLsubRecovery::applyOverrideRecipe(ConstraintsGenerator &G,
                                        const OverrideTypeRecipe &Recipe) {
  for (const auto &[LHS, RHS] : Recipe.Constraints) {
    G.addSubtype(LHS, RHS);
  }
}

notdec::mlsub::MLsubRecovery::OverrideTypeRecipe buildSummaryConstraintOperand(
    notdec::mlsub::MLsubRecovery &Recovery, ConstraintsGenerator &G,
    notdec::mlsub::MLsubRecovery::OverrideBuildContext &Ctx,
    llvm::Function &Func, const llvm::json::Value &Value,
    llvm::StringRef Path) {
  if (isSummaryConstraintTargetOperand(Value)) {
    auto Target =
        resolveSummaryConstraintTarget(*Value.getAsObject(), Func, Path);
    auto Node = G.getNodeOrNull(Target.Value);
    if (Node == nullptr) {
      failSignatureOverride(
          Path, ("target '" + Target.Key + "' has no corresponding node").c_str());
    }
    notdec::mlsub::MLsubRecovery::OverrideTypeRecipe Recipe;
    Recipe.Root = Node;
    return Recipe;
  }

  return Recovery.buildOverrideType(Value, Ctx, Path, false);
}

void MLsubRecovery::applySummaryOverride(ConstraintsGenerator &G,
                                         llvm::Function &Func,
                                         const llvm::json::Value &Spec) {
  const auto &Obj = requireObject(Spec, Func.getName());
  std::string FuncPath = Func.getName().str();
  OverrideBuildContext Ctx{.Generator = G, .Func = Func};
  OverrideTypeRecipe Aggregate;
  std::vector<SimpleType> Args;
  OverrideTypeRecipe RetRecipe;
  bool HasFunctionType = hasFunctionTypeOverride(Obj, FuncPath, false);
  if (HasFunctionType) {
    const auto &ArgsValue =
        requireArray(*Obj.get("args"), appendJSONPath(FuncPath, "args"));
    Args.reserve(ArgsValue.size());
    for (size_t Index = 0; Index < ArgsValue.size(); ++Index) {
      auto ArgPath = appendJSONIndexPath(appendJSONPath(FuncPath, "args"),
                                         Index);
      auto ArgRecipe = buildOverrideType(ArgsValue[Index], Ctx, ArgPath, false);
      appendOverrideConstraints(Aggregate, std::move(ArgRecipe));
      Args.push_back(ArgRecipe.Root);
    }

    auto *RetValue = Obj.get("ret");
    assert(RetValue != nullptr);
    RetRecipe = buildOverrideType(*RetValue, Ctx,
                                  appendJSONPath(FuncPath, "ret"), true);
    appendOverrideConstraints(Aggregate, std::move(RetRecipe));
  }

  if (auto *ConstraintsValue = Obj.get("constraints")) {
    const auto &Constraints = requireArray(
        *ConstraintsValue, appendJSONPath(FuncPath, "constraints"));
    for (size_t Index = 0; Index < Constraints.size(); ++Index) {
      auto ConstraintPath = appendJSONIndexPath(
          appendJSONPath(FuncPath, "constraints"), Index);
      const auto &ConstraintObj = requireObject(Constraints[Index],
                                                ConstraintPath);
      auto Kind = requireString(ConstraintObj, "kind", ConstraintPath);
      if (Kind != "subtype") {
        failSignatureOverride(ConstraintPath,
                              "only subtype constraints are supported");
      }
      auto *LHSValue = ConstraintObj.get("lhs");
      auto *RHSValue = ConstraintObj.get("rhs");
      if (LHSValue == nullptr || RHSValue == nullptr) {
        failSignatureOverride(ConstraintPath,
                              "constraint requires lhs and rhs");
      }
      auto LHSRecipe = buildSummaryConstraintOperand(
          *this, G, Ctx, Func, *LHSValue, appendJSONPath(ConstraintPath, "lhs"));
      auto RHSRecipe = buildSummaryConstraintOperand(
          *this, G, Ctx, Func, *RHSValue, appendJSONPath(ConstraintPath, "rhs"));
      appendOverrideConstraints(Aggregate, std::move(LHSRecipe));
      appendOverrideConstraints(Aggregate, std::move(RHSRecipe));
      Aggregate.Constraints.push_back({LHSRecipe.Root, RHSRecipe.Root});
    }
  }

  applyOverrideRecipe(G, Aggregate);

  if (HasFunctionType) {
    auto FuncNode = G.getNodeOrNull(&Func);
    assert(FuncNode != nullptr);
    G.addSubtype(binarysub::make_function(Args, RetRecipe.Root), FuncNode);
  }
  applyPNDiffOverrides(G, Func, Obj, FuncPath,
                       &WrotePNDiffOverrideWarningHeader);
}

void MLsubRecovery::applyUpperBoundSignatureOverride(
    ConstraintsGenerator &G, llvm::Function &Func,
    const llvm::json::Value &Spec) {
  const auto &Obj = requireObject(Spec, Func.getName());
  std::string FuncPath = Func.getName().str();
  OverrideBuildContext Ctx{.Generator = G, .Func = Func};

  const auto &ArgsValue =
      requireArray(*Obj.get("args"), appendJSONPath(FuncPath, "args"));
  std::vector<SimpleType> Args;
  Args.reserve(ArgsValue.size());
  OverrideTypeRecipe Aggregate;
  for (size_t Index = 0; Index < ArgsValue.size(); ++Index) {
    auto ArgPath = appendJSONIndexPath(appendJSONPath(FuncPath, "args"),
                                       Index);
    auto ArgRecipe = buildOverrideType(ArgsValue[Index], Ctx, ArgPath, false);
    appendOverrideConstraints(Aggregate, std::move(ArgRecipe));
    Args.push_back(ArgRecipe.Root);
  }

  auto *RetValue = Obj.get("ret");
  assert(RetValue != nullptr);
  auto RetRecipe =
      buildOverrideType(*RetValue, Ctx, appendJSONPath(FuncPath, "ret"), true);
  appendOverrideConstraints(Aggregate, std::move(RetRecipe));

  if (auto *ConstraintsValue = Obj.get("constraints")) {
    const auto &Constraints = requireArray(
        *ConstraintsValue, appendJSONPath(FuncPath, "constraints"));
    for (size_t Index = 0; Index < Constraints.size(); ++Index) {
      auto ConstraintPath = appendJSONIndexPath(
          appendJSONPath(FuncPath, "constraints"), Index);
      const auto &ConstraintObj = requireObject(Constraints[Index],
                                                ConstraintPath);
      auto Kind = requireString(ConstraintObj, "kind", ConstraintPath);
      if (Kind != "subtype") {
        failSignatureOverride(ConstraintPath,
                              "only subtype constraints are supported");
      }
      auto *LHSValue = ConstraintObj.get("lhs");
      auto *RHSValue = ConstraintObj.get("rhs");
      if (LHSValue == nullptr || RHSValue == nullptr) {
        failSignatureOverride(ConstraintPath,
                              "constraint requires lhs and rhs");
      }
      auto LHSRecipe = buildOverrideType(
          *LHSValue, Ctx, appendJSONPath(ConstraintPath, "lhs"), false);
      auto RHSRecipe = buildOverrideType(
          *RHSValue, Ctx, appendJSONPath(ConstraintPath, "rhs"), false);
      appendOverrideConstraints(Aggregate, std::move(LHSRecipe));
      appendOverrideConstraints(Aggregate, std::move(RHSRecipe));
      Aggregate.Constraints.push_back({LHSRecipe.Root, RHSRecipe.Root});
    }
  }

  applyOverrideRecipe(G, Aggregate);

  auto FuncNode = G.getNodeOrNull(&Func);
  assert(FuncNode != nullptr);
  G.addSubtype(FuncNode, binarysub::make_function(Args, RetRecipe.Root));
  applyPNDiffOverrides(G, Func, Obj, FuncPath,
                       &WrotePNDiffOverrideWarningHeader);
}

void MLsubRecovery::applyExtraConstraints(ConstraintsGenerator &G,
                                          llvm::Function &Func,
                                          const llvm::json::Value &Spec) {
  const auto *Obj = Spec.getAsObject();
  if (Obj == nullptr) {
    failExtraConstraints(Func.getName(), "expected object");
  }
  applyExtraConstraintSubtypeActions(*this, G, Func, *Obj, Func.getName());
  applyExtraConstraintPNDiffs(G, Func, *Obj, Func.getName(),
                              &WrotePNDiffOverrideWarningHeader);
}

void MLsubRecovery::bottomUpPhase() {
  const bool ConstraintDiag =
      std::getenv("NOTDEC_CONSTRAINT_DIAG") != nullptr;
  std::optional<BottomUpTiming> TotalTiming;
  static constexpr const char *StageNames[] = {
      "function_nodes",
      "visitor_constraints",
      "scc_deferred_calls",
      "pndiff_solve",
      "call_interface_merge",
      "struct_slot_merge",
      "field_followup_merge",
      "overrides",
      "summary_instantiation",
      "post_summary_deferred_calls",
      "post_summary_call_interface_merge",
      "post_summary_field_followup_merge",
  };
  static_assert(std::size(StageNames) ==
                static_cast<std::size_t>(BottomUpStage::Count));
  static constexpr const char *SubstageNames[] = {
      "deferred_call_subtype",
      "deferred_call_candidate_collect",
      "call_interface_plan",
      "call_interface_precheck",
      "call_interface_transaction",
      "struct_slot_candidate_collect",
      "struct_slot_candidate_apply",
      "field_followup_candidate_collect",
      "field_followup_candidate_apply",
  };
  static_assert(std::size(SubstageNames) ==
                static_cast<std::size_t>(BottomUpSubstage::Count));
  // 与 ConstraintsGenerator::run() 一致：SCC 内函数按名字排序遍历，避免
  // std::set<llvm::Function*> 指针顺序随 ASLR 变化影响 run-to-run 结果。
  auto SortedFunctions = [](const std::set<llvm::Function *> &Funcs) {
    std::vector<llvm::Function *> Out(Funcs.begin(), Funcs.end());
    llvm::sort(Out, [](llvm::Function *A, llvm::Function *B) {
      return A->getName() < B->getName();
    });
    return Out;
  };
  // unhandledCalls 是 std::map<llvm::CallBase*, ...>，指针序随 ASLR 变化。
  // 按模块声明序给调用点编号，保证跨 run 的实例化顺序一致（变量 id 和最终
  // 类型文本依赖这个顺序）。
  std::unordered_map<const llvm::CallBase *, std::size_t> CallOrder;
  {
    std::size_t Next = 0;
    for (const llvm::Function &F : Mod) {
      for (const llvm::BasicBlock &BB : F) {
        for (const llvm::Instruction &I : BB) {
          if (auto *CB = llvm::dyn_cast<llvm::CallBase>(&I)) {
            CallOrder.emplace(CB, Next++);
          }
        }
      }
    }
  }
  // Iterate bottom up.
  for (std::size_t Ind = AG.AllSCCs.size(); Ind-- > 0;) {
    auto &Data = AG.AllSCCs.at(Ind);
    std::set<llvm::Function *> OpaqueBodies;
    for (auto *Func : SortedFunctions(Data.SCCSet)) {
      if (!isOpaqueBody(*Func)) {
        continue;
      }
      OpaqueBodies.insert(Func);
      llvm::errs() << "Info: opaque body skipped for " << Func->getName()
                   << " (level " << Data.level << ")\n";
    }
    Data.Generator = std::make_shared<ConstraintsGenerator>(
        Data.SCCName, PointerSize, Data.SCCSet, MemoryType, StorageType,
        &StorageFields, Data.level, BinarysubTraceFile.get(), MergeEval,
        EnableStructPtrLoadStoreMerge, std::move(OpaqueBodies));
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
    Data.Generator->BoundOrigins = RecoveryBoundOrigins;
#endif
    auto &G = Data.Generator;
    // insert ContraVariantValues
    if (Ind == 0) {
      // // add main arguments
      // if (auto Main = Mod.getFunction("main")) {
      //   for (auto &Arg: Main->args()) {
      //     G->ContraVariantValues.insert(&Arg);
      //   }
      // }
      // TODO parameters with unknown type
    } else {
      // find SCC interface functions.
      for (auto N : Data.Nodes) {
        if (AG.Callee2Callers.count(N) == 0) {
          continue;
        }
        for (auto Caller : AG.Callee2Callers[N]) {
          if (auto F = Caller->getFunction()) {
            for (auto &Arg : F->args()) {
              G->ContraVariantValues.insert(&Arg);
            }
          }
        }
      }
    }

    auto CurrentTiming = G->run();
    ActiveBottomUpTimingContext = &CurrentTiming;
    ActiveBottomUpPostSummaryContext = false;

    // 函数内部约束生成完、跨函数调用边未连接：按函数统计约束图规模，
    // 用于评估哪些函数把图撑大。NOTDEC_CONSTRAINT_STATS=1 开启。
    if (std::getenv("NOTDEC_CONSTRAINT_STATS") != nullptr) {
      G->emitFunctionConstraintStats();
    }

    auto Measure = [&](BottomUpStage Stage, auto &&Action) {
      G->CurrentConstraintStats = {};
      auto Start = startBottomUpPhase(ConstraintDiag);
      Action();
      CurrentTiming.at(Stage) = finishBottomUpPhase(
          Start, G->CurrentConstraintStats, ConstraintDiag);
    };
    Measure(BottomUpStage::Overrides, [&] {
      for (auto *Func : SortedFunctions(Data.SCCSet)) {
        if (auto *ExtraSpec = getExtraConstraintsSpec(*Func)) {
          llvm::errs() << "Applying MLsub extra constraints to "
                       << Func->getName() << "\n";
          applyExtraConstraints(*G, *Func, *ExtraSpec);
        }
        if (auto *SummarySpec = getSummaryOverrideSpec(*Func)) {
          llvm::errs() << "Applying MLsub summary override to "
                       << Func->getName() << "\n";
          applySummaryOverride(*G, *Func, *SummarySpec);
        }
        auto *Spec = getSignatureOverrideSpec(*Func);
        if (Spec == nullptr) {
          continue;
        }
        assert(!Func->isDeclaration() &&
               "signature overrides are validated to require definitions");
        llvm::errs() << "Applying MLsub signature override to "
                     << Func->getName() << "\n";
        applyUpperBoundSignatureOverride(*G, *Func, *Spec);
      }
    });

    // Create poly schemes and instantiate every unhandled call first.  The
    // deferred flush below preserves the old subtype relations while ensuring
    // none of the call-slot merge candidates are recorded early.
    std::vector<std::pair<llvm::CallBase *, SimpleType>> SortedCalls(
        Data.Generator->unhandledCalls.begin(),
        Data.Generator->unhandledCalls.end());
    llvm::sort(SortedCalls, [&](const auto &A, const auto &B) {
      auto ItA = CallOrder.find(A.first);
      auto ItB = CallOrder.find(B.first);
      return ItA->second < ItB->second;
    });
    // The level-0 group (monomorphic module scope) owns shared nodes: every
    // level-0 SCC is merged into it, and its genTypes solves them last.
    ConstraintsGenerator *LevelZeroGen = nullptr;
    for (auto &Group : AG.AllSCCs) {
      if (Group.level == 0 && Group.Generator != nullptr) {
        LevelZeroGen = Group.Generator.get();
        break;
      }
    }
    Measure(BottomUpStage::SummaryInstantiation, [&] {
      for (auto &Ent : SortedCalls) {
        auto F = Ent.first->getCalledFunction();
        auto TargetNode = AG.CG->getOrInsertFunction(F);
        auto TargetIt = AG.Func2SCCIndex.find(TargetNode);
        if (TargetIt == AG.Func2SCCIndex.end()) {
          llvm::errs() << "Warning: skip unhandled call without MLsub SCC: "
                       << F->getName() << "\n";
          continue;
        }
        auto Ind2 = TargetIt->second;
        if (Ind2 >= AG.AllSCCs.size()) {
          llvm::errs() << "Warning: skip call with out-of-range MLsub SCC index: "
                       << F->getName() << " (index " << Ind2 << " of "
                       << AG.AllSCCs.size() << ")\n";
          continue;
        }
        assert(Ind2 > Ind);
        auto &TData = AG.AllSCCs[Ind2];
        auto TargetG = TData.Generator;
        auto TargetFTy = TargetG->getNodeOrNull(F);
        auto TargetLevel = binarysub::level_of(TargetFTy);
        assert(TargetLevel >= 0);
        assert(TData.level == static_cast<unsigned int>(TargetLevel));
        // prepareSCC() keeps polymorphic SCCs as summary boundaries, so a
        // polymorphic callee is always strictly higher. Non-polymorphic
        // callees can be reached from one level below (same-level
        // cross-generator calls remain possible and are instantiated at the
        // same level).
        assert(TData.level >= Data.level);
        // Simple-sub creates all variables in a let RHS at lvl + 1 and stores
        // lvl as the polymorphic cutoff.  The target SCC has already been
        // constructed at that RHS level (TData.level), so its summary must
        // generalize variables starting at TData.level, inclusively. Passing
        // TData.level to instantiate_type_graph is the direct equivalent of
        // paper-level PolymorphicType(TData.level - 1, TargetFTy), without an
        // unsigned underflow or a misleading cutoff conversion.
        static const bool InstantiationDiag =
            std::getenv("NOTDEC_DEFERRED_DIAG") != nullptr;
        if (InstantiationDiag) {
          std::string Label = "instantiate-before:" + F->getName().str();
          binarysub::emit_type_graph_summary(Label.c_str());
        }
        auto InsFunc = binarysub::instantiate_type_graph(
            TargetFTy, TargetLevel, static_cast<int>(Data.level));
        // Partial parameter polymorphism.  The function is still instantiated
        // as a whole and the call constraint stays untouched; for the positions
        // the function declared monomorphic we additionally tie the per-call
        // site copy to ONE shared, level-fixed copy of the callee's formal.
        // Every call site then feeds the same node there, so a shared object
        // (the obstack / hash header) keeps one type across call sites while
        // the element positions stay per-call-site polymorphic.
        //
        // Direction follows the Simple-sub application rule
        // constrain(f, Function(arg, res)): the argument is a subtype of the
        // formal parameter, and the callee's result is a subtype of the call's
        // result, so argument positions link copy <: shared and the return
        // links shared <: copy.
        if (auto Positions = resolvePolymorphicPositionSpec(*F);
            !Positions.empty()) {
          const binarysub::TFunction *InsFn =
              findStructuralFunctionType(InsFunc);
          const binarysub::TFunction *OrigFn =
              findStructuralFunctionType(TargetFTy);
          const binarysub::TFunction *ActualFn =
              findStructuralFunctionType(Ent.second);
          const bool Diag = std::getenv("NOTDEC_POLY_MONO_DIAG") != nullptr;
          unsigned Linked = 0;
          if (InsFn != nullptr && OrigFn != nullptr) {
            for (unsigned ArgIndex : Positions.MonomorphicArgs) {
              if (ArgIndex >= InsFn->args.size() ||
                  ArgIndex >= OrigFn->args.size()) {
                continue;
              }
              SimpleType Shared = sharedPositionType(
                  *F, ArgIndex, OrigFn->args[ArgIndex], /*IsReturn=*/false);
              if (Diag) {
                auto *SV = Shared ? Shared->getAsVariableState() : nullptr;
                llvm::errs() << "[poly-mono-node] shared=" << Shared.get()
                             << " level=" << (SV ? SV->level : -1)
                             << " lowers=" << (SV ? SV->lowerBounds.size() : 0)
                             << " uppers=" << (SV ? SV->upperBounds.size() : 0)
                             << " data_gen=" << static_cast<const void *>(Data.Generator.get())
                             << " level0_gen=" << static_cast<const void *>(LevelZeroGen)
                             << " same_gen=" << (Data.Generator.get() == LevelZeroGen)
                             << "\n";
                if (SV != nullptr) {
                  for (const auto &UB : SV->upperBounds) {
                    llvm::errs() << "    upper bound: "
                                 << (UB ? binarysub::debug_string(UB) : "<null>")
                                 << "\n";
                  }
                }
              }
              // "Same function" in the struct-pointer merge policy means the two
              // variables are owned by ONE llvm::Function
              // (shouldMergeSameFunctionStructPtrSubtype ->
              // getVariableOwningFunction -> Var->externalHandle).  The shared
              // node is born from the callee's formal, so leaving its handle to
              // the callee makes that policy refuse to merge it with the
              // caller's other copies of the same C object -- which is the merge
              // that carries the members no other call path of THIS function
              // touches (obstack: chunk_size / alignment_mask / chunkfun are
              // written by _obstack_begin_worker and read by call_chunkfun, but
              // never read by _obstack_free).  Attach the call's actual operand
              // first, so the extruded node counts as a node of the CALLER and
              // the same-function struct-pointer merge can unify it with the
              // caller's other views of the object.
              // NOTE: attaching the caller's handle to the shared node and
              // scanning the caller for same-object operands were tried here.
              // The mechanism turned out to be wrong (the family's members come
              // from the call-slot merges, not from getUnderlyingObject
              // siblings), and with monomorphic annotations on the hash family
              // they made cases balloon to gigabytes of RSS.  Removed.
              // 实参 <= 形参: connect the ACTUAL argument of this call site (and
              // the instantiated copy, as a fallback) to the one shared node,
              // so all call sites feed the same type there.
              if (ActualFn != nullptr && ArgIndex < ActualFn->args.size()) {
                // Only the call semantics direction (actual <: formal) may be
                // added here.
                //
                // NEVER add the reverse edge "shared <: actual".  It looks
                // harmless -- the shared node would pick up the call sites'
                // fields through the actual's upper bounds -- but it drags every
                // caller's whole type graph into the LEVEL-0 generator, which is
                // where the blow-up happens: measured on O0/chmod with only
                // "hash_find_entry args=0" annotated, adding that edge turns a
                // 4.1 s / 251 MB run into bad_alloc at 41 s (nodes 1403 ->
                // 292055, lower-bound edges 692 -> 5050473 in a single step),
                // and removing it makes the same run pass again.  The fields the
                // shared node needs do arrive without it: with the edge gone,
                // "_obstack_free args=0" still yields the full 10-member obstack
                // record (O0/sort, 8.5 s, identical to the numbers with it).
                Data.Generator->addSubtype(ActualFn->args[ArgIndex], Shared);
              }
              Data.Generator->addSubtype(InsFn->args[ArgIndex], Shared);
              // Register the shared node in the LEVEL-0 generator (the
              // monomorphic module scope, where every level-0 SCC is merged):
              // that generator solves it at the very end and is also the one
              // whose report entry the module assembly prefers.
              if (auto *SharedGen = LevelZeroGen) {
                SharedGen->V2N.insert(
                    getExtValuePtr(
                        const_cast<llvm::Argument *>(F->getArg(ArgIndex)),
                        nullptr),
                    Shared);
              }
              ++Linked;
              if (Diag) {
                llvm::errs() << "[poly-mono-link] " << F->getName()
                             << " at " << Ent.first->getFunction()->getName()
                             << " arg=" << ArgIndex << "\n";
              }
            }
            if (Positions.MonomorphicReturn && InsFn->result != nullptr &&
                OrigFn->result != nullptr) {
              SimpleType Shared = sharedPositionType(
                  *F, kMonomorphicReturnPosition, OrigFn->result,
                  /*IsReturn=*/true);
              // 返回值反过来: the callee's (shared) result is a subtype of the
              // call's result.
              Data.Generator->addSubtype(Shared, InsFn->result);
              if (ActualFn != nullptr && ActualFn->result != nullptr) {
                Data.Generator->addSubtype(Shared, ActualFn->result);
              }
              if (auto *SharedGen = LevelZeroGen) {
                SharedGen->V2N.insert(
                    ExtValuePtr{
                        ReturnValue{const_cast<llvm::Function *>(F), 0}},
                    Shared);
              }
              ++Linked;
            }
          }
          if (Linked != 0 && Diag) {
            llvm::errs() << "[poly-mono] " << F->getName() << " at "
                         << Ent.first->getFunction()->getName() << " links "
                         << Linked << " position(s) " << Positions.str()
                         << "\n";
          }
        }
        Data.Generator->deferCallConstraint(*Ent.first, *F, InsFunc,
                                            Ent.second, InsFunc);
      }
    });
    ActiveBottomUpPostSummaryContext = true;
    Measure(BottomUpStage::PostSummaryDeferredCalls,
            [&] { Data.Generator->applyDeferredCallConstraints(); });
    Measure(BottomUpStage::PostSummaryCallInterfaceMerge, [&] {
      auto Merged = Data.Generator->applyCallInterfaceMergePolicy();
      if (Merged != 0) {
        llvm::errs()
            << "Info: post-summary transactional call interface merge policy "
               "merged "
            << Merged << " pair(s)\n";
      }
    });
    Measure(BottomUpStage::PostSummaryFieldFollowupMerge, [&] {
      auto Merged =
          Data.Generator->applyStructPtrFieldFollowupMergePolicy();
      if (Merged != 0) {
        llvm::errs()
            << "Info: post-summary struct pointer field follow-up merge policy "
               "merged "
            << Merged << " pair(s)\n";
      }
    });
    Data.Generator->unhandledCalls.clear();
    ActiveBottomUpPostSummaryContext = false;
    ActiveBottomUpTimingContext = nullptr;

    if (ConstraintDiag) {
      for (std::size_t I = 0; I < CurrentTiming.Phases.size(); ++I) {
        llvm::errs() << "[bottom-up-phase] stage=" << StageNames[I]
                     << " scc_index=" << Ind
                     << " bottom_up_done=" << AG.AllSCCs.size() - Ind << "/"
                     << AG.AllSCCs.size() << " functions=" << Data.SCCSet.size()
                     << " scc=" << llvm::StringRef(Data.SCCName).take_front(80);
        printBottomUpTimingFields(CurrentTiming.Phases[I]);
        llvm::errs() << "\n";
      }
      for (std::size_t I = 0; I < CurrentTiming.Subphases.size(); ++I) {
        llvm::errs() << "[bottom-up-subphase] stage=" << SubstageNames[I]
                     << " scc_index=" << Ind
                     << " bottom_up_done=" << AG.AllSCCs.size() - Ind << "/"
                     << AG.AllSCCs.size() << " functions=" << Data.SCCSet.size()
                     << " scc=" << llvm::StringRef(Data.SCCName).take_front(80);
        printBottomUpSubphaseTimingFields(CurrentTiming.Subphases[I]);
        llvm::errs() << "\n";
      }
      for (std::size_t I = 0;
           I < CurrentTiming.PostSummarySubphases.size(); ++I) {
        const auto &Subphase = CurrentTiming.PostSummarySubphases[I];
        if (!hasBottomUpSubphaseTiming(Subphase)) {
          continue;
        }
        llvm::errs() << "[bottom-up-post-summary-subphase] stage="
                     << SubstageNames[I] << " scc_index=" << Ind
                     << " bottom_up_done=" << AG.AllSCCs.size() - Ind << "/"
                     << AG.AllSCCs.size() << " functions=" << Data.SCCSet.size()
                     << " scc=" << llvm::StringRef(Data.SCCName).take_front(80);
        printBottomUpSubphaseTimingFields(Subphase);
        llvm::errs() << "\n";
      }
      llvm::errs() << "[bottom-up-scc-complete] scc_index=" << Ind
                   << " bottom_up_done=" << AG.AllSCCs.size() - Ind << "/"
                   << AG.AllSCCs.size() << " functions=" << Data.SCCSet.size()
                   << " values=" << G->V2N.size() << " scc="
                   << llvm::StringRef(Data.SCCName).take_front(80)
                   << " rss_kb=" << currentResidentRssKb() << "\n";
      if (!TotalTiming) {
        TotalTiming = CurrentTiming;
      } else {
        addBottomUpTiming(*TotalTiming, CurrentTiming);
      }
    }
  }
  if (TotalTiming) {
    for (std::size_t I = 0; I < TotalTiming->Phases.size(); ++I) {
      llvm::errs() << "[bottom-up-summary] stage=" << StageNames[I]
                   << " sccs=" << AG.AllSCCs.size();
      printBottomUpTimingFields(TotalTiming->Phases[I]);
      llvm::errs() << "\n";
    }
    for (std::size_t I = 0; I < TotalTiming->Subphases.size(); ++I) {
      llvm::errs() << "[bottom-up-subphase-summary] stage="
                   << SubstageNames[I] << " sccs=" << AG.AllSCCs.size();
      printBottomUpSubphaseTimingFields(TotalTiming->Subphases[I]);
      llvm::errs() << "\n";
    }
    for (std::size_t I = 0;
         I < TotalTiming->PostSummarySubphases.size(); ++I) {
      const auto &Subphase = TotalTiming->PostSummarySubphases[I];
      if (!hasBottomUpSubphaseTiming(Subphase)) {
        continue;
      }
      llvm::errs() << "[bottom-up-post-summary-subphase-summary] stage="
                   << SubstageNames[I] << " sccs=" << AG.AllSCCs.size();
      printBottomUpSubphaseTimingFields(Subphase);
      llvm::errs() << "\n";
    }
  }
}

GenTypesTiming
ConstraintsGenerator::genTypes(ast::HTypeContext &HCtx,
                               unsigned PointerSizeBytes, bool SolveGlobals,
                               ExtValueLabelCache *DebugLabelCache) {
  const bool GenTypesDiag =
      std::getenv("NOTDEC_SIMPLIFY_DIAG") != nullptr ||
      std::getenv(kSimplifyStopAfterGroupsEnv.data()) != nullptr;
  GenTypesTiming Timing;
  auto PhaseWallStart = GenTypesClock::now();
  auto PhaseCpuStart =
      GenTypesDiag ? currentProcessCpuTime() : ProcessCpuSnapshot{};
  binarysub::TypeSimplifier Ts;
  using binarysub::PolarVar;
  SnapshotContraVariantValues = ContraVariantValues;
  // Late merge policies can leave V2N entries pointing at variables that now
  // forward to a representative through mergedInto.  Normalize here so the final
  // solve roots, eval grouping, and HType lowering all use the same root.
  std::vector<std::pair<SimpleType, SimpleType>> ResolvedV2NMerges;
  ResolvedV2NMerges.reserve(V2N.rev().size());
  for (const auto &Ent : V2N.rev()) {
    auto Resolved = binarysub::resolve_variable(Ent.first);
    if (Resolved && Resolved.get() != Ent.first.get()) {
      ResolvedV2NMerges.push_back({Ent.first, Resolved});
    }
  }
  for (const auto &[From, To] : ResolvedV2NMerges) {
    V2N.merge(From, To);
  }
  std::set<PolarVar> Tys;
  for (auto &Ent : V2N) {
    if (!Ent.second->isVariableState()) {
      continue;
    }
    Tys.insert(PolarVar{.var = Ent.second, .pos = true});
    Tys.insert(PolarVar{.var = Ent.second, .pos = false});
  }
  auto PolMem = PolarVar{.var = MemoryType, .pos = false};
  auto PolStorage = PolarVar{.var = StorageType, .pos = false};
  if (SolveGlobals) {
    Tys.insert(PolMem);
    if (StorageType != nullptr) {
      Tys.insert(PolStorage);
    }
  }
  // Each line is a completed, process-wide interval. The timing snapshot is
  // taken before reading RSS or printing so diagnostics do not become part of
  // the following phase.
  auto reportGenTypesStage = [&](llvm::StringRef Stage,
                                 const GenTypesPhaseTiming &Phase) {
    if (!GenTypesDiag) {
      return;
    }
    llvm::errs() << "[gen-types-diag] stage=" << Stage
                 << " scc=" << llvm::StringRef(Name).take_front(80)
                 << " values=" << V2N.size() << " roots=" << Tys.size();
    printGenTypesTimingFields(Phase);
    llvm::errs() << " rss_kb=" << currentResidentRssKb() << "\n";
  };
  binarysub::BulkSimplifyOptions BulkOptions;
  BulkOptions.enableParallel = true;
  BulkOptions.diagnosticStopAfterGroups = getSimplifyStopAfterGroups();
  // 静态图分析探针：canonicalize 之前 dump roots 可达的引用图，用于
  // 折叠展开爆炸的预检（SCC/路径计数/节点贡献），见 logs/20260731-03。
  if (const char *DumpPath = std::getenv("NOTDEC_DUMP_TYPE_GRAPH")) {
    binarysub::dumpTypeGraph(Tys, DumpPath);
  }
  // CompactType 层图探针：go1 构建 merged snapshot 时记录引用边，用于
  // 折叠展开爆炸的预检（见 logs/20260731-03）。
  if (const char *DumpPath = std::getenv("NOTDEC_DUMP_COMPACT_GRAPH")) {
    Ts.setCompactGraphDump(DumpPath);
  }
  Timing.Prepare = finishGenTypesPhase(PhaseWallStart, PhaseCpuStart);
  reportGenTypesStage("prepare", Timing.Prepare);

  PhaseWallStart = GenTypesClock::now();
  PhaseCpuStart =
      GenTypesDiag ? currentProcessCpuTime() : ProcessCpuSnapshot{};
  auto BulkResult = Ts.bulkSimplifyDetailed(Tys, false, BulkOptions);
  Timing.Bulk = finishGenTypesPhase(PhaseWallStart, PhaseCpuStart);
  reportGenTypesStage("bulk", Timing.Bulk);
  if (BulkResult.status ==
      binarysub::BulkSimplifyResult::Status::DiagnosticStop) {
    std::ostringstream Message;
    Message << "simplify stopped at a complete group boundary: scc="
            << llvm::StringRef(Name).take_front(80).str()
            << " groups_completed=" << BulkResult.groupsCompleted
            << " groups_total=" << BulkResult.groupsTotal;
    llvm::errs() << "[type-recovery-diagnostic-stop] " << Message.str()
                 << " lower_started=0 upper_started=0 debug_output_started=0\n";
    throw SimplifyDiagnosticStop(Message.str());
  }
  const auto &Res = BulkResult.types;

  PhaseWallStart = GenTypesClock::now();
  PhaseCpuStart =
      GenTypesDiag ? currentProcessCpuTime() : ProcessCpuSnapshot{};
  // Create TypeBuilder context and builder.  Keep this SCC-local until cross-SCC
  // type sharing has a dedicated design.
  TypeBuilderContext TBCtx(HCtx, PointerSizeBytes, notdec::getWorkDirOpt());
  // μ 回边的解析现在由类型本身携带：递归变量节点的
  // UTypeVariable::recursiveType 直接指向它所属的递归类型节点（生成侧写好），
  // 解析时不再需要按名字扫本轮根建索引，也不再受同名影响 / 不受 SCC 边界限制。
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
  // Field-level traceability: map each original variable id back to the bound
  // origins the solver recorded on it, so TypeBuilder can explain a field.
  // Two consumers share the lookup: the human-readable trace
  // (NOTDEC_FIELD_ORIGINS) and the layout-side operation filter
  // (BINARYSUB_FILTER_BULK_ORIGIN_FIELDS).  The origins are the per-edge sets
  // the solver keeps on VariableState bounds; reading them on demand keeps
  // this O(#variables).
  const bool WantOriginTrace = std::getenv("NOTDEC_FIELD_ORIGINS") != nullptr;
  const char *BulkPolicyEnv =
      std::getenv("BINARYSUB_FILTER_BULK_ORIGIN_FIELDS");
  const bool WantBulkPolicy =
      BulkPolicyEnv != nullptr && BulkPolicyEnv[0] != '\0' &&
      BulkPolicyEnv[0] != '0';
  if (WantOriginTrace || WantBulkPolicy) {
    using VarStatePtr = decltype(std::declval<SimpleType>()
                                 ->getAsVariableState());
    auto VarById =
        std::make_shared<std::map<std::uint32_t, VarStatePtr>>();
    for (const auto &[Val, Node] : V2N) {
      auto *VS = Node ? Node->getAsVariableState() : nullptr;
      if (VS != nullptr) {
        (*VarById)[VS->id] = VS;
      }
    }
    const std::vector<std::string> *Descriptions =
        &boundOriginTable().Descriptions;
    if (WantOriginTrace) {
      TBCtx.BoundOriginLookup =
          [VarById, Descriptions](
              const std::string & /*FieldKey*/,
              const std::vector<std::uint32_t> &VarIds,
              std::vector<std::string> &Out) {
            for (auto Id : VarIds) {
              auto It = VarById->find(Id);
              if (It == VarById->end()) {
                continue;
              }
              for (const auto &Set : It->second->upperBoundOrigins) {
                if (!Set) {
                  continue;
                }
                for (auto OriginId : *Set) {
                  if (OriginId < Descriptions->size()) {
                    Out.push_back((*Descriptions)[OriginId]);
                  }
                }
              }
              for (const auto &Set : It->second->lowerBoundOrigins) {
                if (!Set) {
                  continue;
                }
                for (auto OriginId : *Set) {
                  if (OriginId < Descriptions->size()) {
                    Out.push_back((*Descriptions)[OriginId]);
                  }
                }
              }
            }
            std::sort(Out.begin(), Out.end());
            Out.erase(std::unique(Out.begin(), Out.end()), Out.end());
          };
      // The backend prints the field-level origins (not only the value-side
      // ones), so hand it the id -> description table.
      TBCtx.BoundOriginDescriptions =
          std::make_shared<const std::vector<std::string>>(*Descriptions);
    }
    if (WantOriginTrace || WantBulkPolicy) {
      // Machine-readable origin ids: the offline dataset (origin_ids=) and the
      // layout-side operation filter both match on ids, not descriptions.
      TBCtx.BoundOriginIdLookup =
          [VarById](const std::string & /*FieldKey*/,
                    const std::vector<std::uint32_t> &VarIds,
                    std::vector<std::uint32_t> &OutIds) {
            auto Push = [&](const std::shared_ptr<const binarysub::SmallIdSet>
                            &Set) {
              if (!Set) {
                return;
              }
              for (auto OriginId : *Set) {
                OutIds.push_back(OriginId);
              }
            };
            for (auto Id : VarIds) {
              auto It = VarById->find(Id);
              if (It == VarById->end()) {
                continue;
              }
              for (const auto &Set : It->second->upperBoundOrigins) {
                Push(Set);
              }
              for (const auto &Set : It->second->lowerBoundOrigins) {
                Push(Set);
              }
            }
          };
    }
    if (WantBulkPolicy) {
      // Origin classes for the layout-side bulk-operation filter.
      TBCtx.BoundOriginKinds =
          std::make_shared<const std::vector<std::uint8_t>>(
              boundOriginTable().Kinds);
    }
  }
#ifdef BINARYSUB_ENABLE_FIELD_ORIGINS
  // Field-origin coverage: how many record fields still carry a stamped
  // origin after solving/merging.  Gated so it costs nothing unless asked for.
  if (std::getenv("NOTDEC_FIELD_ORIGIN_DIAG") != nullptr) {
    std::set<const binarysub::TypeNode *> Seen;
    std::size_t Records = 0;
    std::size_t Fields = 0;
    std::size_t FieldsWithOrigins = 0;
    std::function<void(const SimpleType &)> Walk = [&](const SimpleType &Ty) {
      if (!Ty || !Seen.insert(Ty.get()).second) {
        return;
      }
      if (auto *Mem = Ty->getAsTMemObject()) {
        ++Records;
        for (std::size_t I = 0; I < Mem->fields.size(); ++I) {
          ++Fields;
          if (I < Mem->fieldOrigins.size() && !Mem->fieldOrigins[I].empty()) {
            ++FieldsWithOrigins;
          }
        }
        for (const auto &[FieldName, FieldTy] : Mem->fields) {
          Walk(FieldTy);
        }
        if (Mem->directLoad) {
          Walk(Mem->directLoad->to);
        }
        if (Mem->directStore) {
          Walk(Mem->directStore->to);
        }
        return;
      }
      if (auto *F = Ty->getAsTFunction()) {
        for (const auto &Arg : F->args) {
          Walk(Arg);
        }
        Walk(F->result);
      }
    };
    for (const auto &[Val, Node] : V2N) {
      if (auto *VS = Node ? Node->getAsVariableState() : nullptr) {
        for (const auto &Bound : VS->lowerBounds) {
          Walk(Bound);
        }
        for (const auto &Bound : VS->upperBounds) {
          Walk(Bound);
        }
      }
    }
    std::cerr << "[field-origin] scc=" << Name << " records=" << Records
              << " fields=" << Fields << " with_origins=" << FieldsWithOrigins
              << "\n";
  }
#endif
#endif
  TypeBuilder TB(TBCtx);

  auto convertSolvedType = [&](SimpleType Ty, bool Pos,
                               llvm::StringRef RootLabel) {
    // 诊断（NOTDEC_TRACE_VALUE=<子串>）：打印某个值在求解后挂了哪些界。
    // 用来对比两次运行里同一个值"取类型的来源"是否相同。
    const char *TraceWant = std::getenv("NOTDEC_TRACE_VALUE");
    const bool TraceThisValue = TraceWant != nullptr && TraceWant[0] != '\0' &&
                                RootLabel.contains(TraceWant);
    if (TraceThisValue) {
      {
        std::cerr << "[value-trace] " << RootLabel.str() << " pos=" << Pos
                  << "\n";
        auto PrintBounds = [&](const char *Side,
                               const std::vector<SimpleType> &Bounds) {
          std::cerr << "  " << Side << " count=" << Bounds.size() << "\n";
          for (std::size_t I = 0; I < Bounds.size() && I < 400; ++I) {
            std::string Text = binarysub::debug_string(Bounds[I]);
            if (Text.size() > 150) {
              Text.resize(150);
            }
            std::cerr << "  " << Side << "[" << I << "] " << Text << "\n";
          }
        };
        if (auto *VS = Ty->getAsVariableState()) {
          std::cerr << "  var id=" << VS->id << " size=" << VS->size << "\n";
          PrintBounds("lower", VS->lowerBounds);
          PrintBounds("upper", VS->upperBounds);
        } else {
          std::cerr << "  (root is not a variable)\n";
        }
      }
    }
    binarysub::UTypePtr SolvedTy = nullptr;
    if (Ty->isVariableState()) {
      auto It = Res.find(PolarVar{.var = Ty, .pos = Pos});
      if (It != Res.end()) {
        SolvedTy = It->second;
      }
    } else {
      SolvedTy = binarysub::coalesceType(Ty, Pos);
    }
    if (TraceThisValue) {
      std::string SolvedText =
          SolvedTy ? binarysub::printType(SolvedTy) : std::string("null");
      if (SolvedText.size() > 1500) {
        SolvedText.resize(1500);
        SolvedText += "...";
      }
      std::cerr << "  solved=" << SolvedText << "\n";
    }
    if (!SolvedTy) {
      return static_cast<ast::HType *>(nullptr);
    }
    TB.setDebugRootLabel(RootLabel.str());
    auto *Converted = TB.convert(SolvedTy);
    TB.setDebugRootLabel(std::nullopt);
    return Converted;
  };

  const bool ConvertProgress =
      std::getenv("NOTDEC_CONVERT_PROGRESS") != nullptr;
  auto convertStart = std::chrono::steady_clock::now();
  auto reportConvertProgress = [&](std::size_t Done, std::size_t Total) {
    if (!ConvertProgress) {
      return;
    }
    auto Elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - convertStart)
                       .count();
    llvm::errs() << "[convert-progress] scc=" << Name << " done=" << Done
                 << " total=" << Total << " elapsed_ms=" << Elapsed << "\n";
  };
  // Partial parameter polymorphism (C1): a declared-monomorphic position is
  // reported as the one shared node every call site merged into - for BOTH
  // bounds - instead of the function's own generalized formal.  genTypes runs
  // after the whole bottom-up phase and re-solves this group's roots, so the
  // shared node's solution already contains every call-site merge.
  std::size_t ConvertCount = 0;
  for (auto &Ent : V2N) {
    auto RootLabel = formatTypeBuilderRootLabel(Ent.first);
    auto *Lower = convertSolvedType(Ent.second, true, RootLabel + " lower");
    ValueTypesLower.insert({Ent.first, Lower});
    ++ConvertCount;
    if (ConvertCount % 5000 == 0) {
      reportConvertProgress(ConvertCount, V2N.size());
    }
  }
  Timing.Lower = finishGenTypesPhase(PhaseWallStart, PhaseCpuStart);
  reportGenTypesStage("lower", Timing.Lower);

  PhaseWallStart = GenTypesClock::now();
  PhaseCpuStart =
      GenTypesDiag ? currentProcessCpuTime() : ProcessCpuSnapshot{};
  if (SolveGlobals) {
    auto MemUTy = Res.at(PolMem);
    TB.setDebugRootLabel(std::string("<memory>"));
    ValueTypesUpper.insert({nullptr, TB.convert(MemUTy)});
    TB.setDebugRootLabel(std::nullopt);

    if (StorageType != nullptr && StorageFields != nullptr) {
      auto It = Res.find(PolStorage);
      if (It != Res.end() && It->second != nullptr) {
        TB.setDebugRootLabel(std::string("<storage>"));
        StorageHType = TB.convertStorageRecord(It->second, *StorageFields);
        TB.setDebugRootLabel(std::nullopt);
        if (StorageHType != nullptr && StorageHType->isRecordType()) {
          StorageDecl = StorageHType->getAsRecordDecl();
        }
      }
    }
  }
  for (auto &Ent : V2N) {
    auto RootLabel = formatTypeBuilderRootLabel(Ent.first);
    auto *Upper = convertSolvedType(Ent.second, false, RootLabel + " upper");
    ValueTypesUpper.insert({Ent.first, Upper});
    ++ConvertCount;
    if (ConvertCount % 5000 == 0) {
      reportConvertProgress(ConvertCount, V2N.size() * 2);
    }
  }
  reportConvertProgress(V2N.size() * 2, V2N.size() * 2);
  Timing.Upper = finishGenTypesPhase(PhaseWallStart, PhaseCpuStart);
  reportGenTypesStage("upper", Timing.Upper);

  PhaseWallStart = GenTypesClock::now();
  PhaseCpuStart =
      GenTypesDiag ? currentProcessCpuTime() : ProcessCpuSnapshot{};
  if (auto WorkDir = notdec::getWorkDirOpt()) {
    appendDebugValueTypes(*WorkDir, Name, V2N, ContraVariantValues, Res,
                          OriginalVariableSources, SolveGlobals, PolMem,
                          DebugLabelCache);
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
    const std::vector<std::string> *BoundOriginTable =
        &boundOriginTable().Descriptions;
#else
    const std::vector<std::string> *BoundOriginTable = nullptr;
#endif
    appendDebugVarOrigins(*WorkDir, Name, V2N, ContraVariantValues, Res,
                          OriginalVariableSources, SolveGlobals, PolMem,
                          DebugLabelCache, BoundOriginTable);
  }
  Timing.DebugOutput = finishGenTypesPhase(PhaseWallStart, PhaseCpuStart);
  reportGenTypesStage("debug_output", Timing.DebugOutput);
  return Timing;
}

void ConstraintsGenerator::releaseBinarysubState() {
  for (auto &Ent : V2N) {
    binarysub::release_type_graph(Ent.second);
  }
  binarysub::release_type_graph(MemoryType);
  V2N = {};
  ContraVariantValues.clear();
  OriginalVariableSources.clear();
  unhandledCalls.clear();
}

namespace {

// True when the report entry belongs to a position the function declared
// non-polymorphic.  Such an entry may come from a caller's generator (which
// solves the shared node) and must win over the owning SCC's entry, because
// every call site merged into that shared node.
bool isMonomorphicReportKey(const ExtValuePtr &Key) {
  if (auto *V = std::get_if<llvm::Value *>(&Key)) {
    auto *Arg = llvm::dyn_cast_or_null<llvm::Argument>(*V);
    if (Arg == nullptr) {
      return false;
    }
    const auto Spec = resolvePolymorphicPositionSpec(*Arg->getParent());
    for (unsigned Mono : Spec.MonomorphicArgs) {
      if (Mono == Arg->getArgNo()) {
        return true;
      }
    }
    return false;
  }
  if (auto *RV = std::get_if<ReturnValue>(&Key)) {
    return resolvePolymorphicPositionSpec(*RV->Func).MonomorphicReturn;
  }
  return false;
}

} // namespace

void MLsubRecovery::genASTTypes(llvm::Module &M) {
  // Read-only: which type variable do the obstack-style argument-0 views
  // resolve to?  Same representative pointer == same variable, which answers
  // whether the union of members comes from variable merging or from several
  // variables sharing one memory object.
  if (std::getenv("NOTDEC_POLY_MONO_DIAG") != nullptr) {
    ConstraintsGenerator *L0 = nullptr;
    for (auto &Group : AG.AllSCCs) {
      if (Group.level == 0 && Group.Generator != nullptr) {
        L0 = Group.Generator.get();
        break;
      }
    }
    std::size_t Printed = 0;
    if (L0 != nullptr) {
      for (llvm::Function &Fn : M) {
        if (Printed >= 40 || Fn.arg_size() == 0) {
          continue;
        }
        auto *Arg = const_cast<llvm::Argument *>(Fn.getArg(0));
        SimpleType Node = L0->getNodeOrNull(getExtValuePtr(Arg, nullptr));
        if (Node == nullptr) {
          continue;
        }
        SimpleType Rep = binarysub::resolve_variable(Node);
        std::string Text =
            Rep ? binarysub::debug_string(Rep) : std::string("<null>");
        std::set<long> Offsets;
        for (std::size_t P = 0; (P = Text.find('@', P)) != std::string::npos;
             ++P) {
          std::size_t Q = P + 1;
          long Value = 0;
          bool Any = false;
          while (Q < Text.size() && std::isdigit((unsigned char)Text[Q])) {
            Value = Value * 10 + (Text[Q] - '0');
            ++Q;
            Any = true;
          }
          if (Any && Q < Text.size() && Text[Q] == ':') {
            Offsets.insert(Value);
          }
        }
        const bool Obstackish =
            Fn.getName().contains("obstack") ||
            (Offsets.count(8) != 0 && Offsets.count(64) != 0 &&
             Offsets.count(80) != 0);
        if (!Obstackish) {
          continue;
        }
        llvm::errs() << "[argshare] " << Fn.getName() << " arg0 node="
                     << static_cast<const void *>(Node.get())
                     << " rep=" << static_cast<const void *>(Rep.get())
                     << " offsets=";
        for (long O : Offsets) {
          llvm::errs() << "@" << O << " ";
        }
        llvm::errs() << "\n";
        ++Printed;
      }
    }
  }
  ResultVal = std::make_unique<Result>();
  ResultVal->PrintEmptyStorageSection = isEVMModule(M);
  EVMStores.clear();
  // 合并所有类型到一个大的 HTypeResult 里面。
  for (std::size_t Ind = 0; Ind < AG.AllSCCs.size(); ++Ind) {
    auto &Data = AG.AllSCCs.at(Ind);
    for (auto &Ent : Data.Generator->ValueTypesLower) {
      // Polymorphic callees temporarily solve caller interface values so calls can
      // instantiate their summaries. Keep the owning SCC's HType and ignore those
      // later interface copies when building the final module-level result.
      if (isMonomorphicReportKey(Ent.first)) {
        // Prefer the level-0 generator's entry: the shared node is registered
        // as a root there, so its solution contains every call site's merges.
        if (Data.level == 0 ||
            ResultVal->ValueTypesLower.count(Ent.first) == 0) {
          ResultVal->ValueTypesLower[Ent.first] = Ent.second;
        }
      } else {
        ResultVal->ValueTypesLower.insert(Ent);
      }
    }
    for (auto &Ent : Data.Generator->ValueTypesUpper) {
      if (isMonomorphicReportKey(Ent.first)) {
        if (Data.level == 0 ||
            ResultVal->ValueTypesUpper.count(Ent.first) == 0) {
          ResultVal->ValueTypesUpper[Ent.first] = Ent.second;
        }
      } else {
        ResultVal->ValueTypesUpper.insert(Ent);
      }
    }
    ResultVal->ContraVariantValues.insert(
        Data.Generator->SnapshotContraVariantValues.begin(),
        Data.Generator->SnapshotContraVariantValues.end());
    if (isEVMModule(M)) {
      for (const auto &Ent : Data.Generator->EVMStoreAccesses.StoresByAddr) {
        for (const RecordedStore &Store : Ent.second) {
          auto *StoreInst =
              llvm::dyn_cast_or_null<llvm::StoreInst>(Store.Source);
          if (StoreInst == nullptr) {
            continue;
          }
          EVMStores.push_back(EVMStoreEvidence{
              .Addr = Store.Addr,
              .StoredValue = StoreInst->getValueOperand(),
              .BitSize = Store.BitSize,
              .Source = Store.Source,
          });
        }
      }
    }
  }
  ResultVal->HTCtx = HCtx;
  // handle Memory type.
  auto Mem = AG.AllSCCs.at(0).Generator->ValueTypesUpper.at(nullptr);
  ResultVal->MemoryType = Mem;
  if (Mem->isRecordType()) {
    ResultVal->MemoryDecl = Mem->getAsRecordDecl();
  }
  auto *Storage = AG.AllSCCs.at(0).Generator->StorageHType;
  ResultVal->StorageType = Storage;
  ResultVal->StorageDecl = AG.AllSCCs.at(0).Generator->StorageDecl;
  normalizeHTypeResult(*ResultVal);
  if (auto WorkDir = notdec::getWorkDirOpt()) {
    writeDebugEVMMarkerFacts(*WorkDir, M);
    writeDebugValueHTypes(*WorkDir, *ResultVal);
    writeDebugImportantHTypes(*WorkDir, *ResultVal);
  }
}

void MLsubRecovery::topDownConstraintPhase() {
  // The current MLsub implementation finishes all constraint generation and
  // cross-SCC summary instantiation in bottomUpPhase(). Keep this boundary
  // explicit so a future caller-to-callee propagation pass can mutate the
  // completed SimpleType graph before solveAndLowerTypes() starts.
}

void MLsubRecovery::solveAndLowerTypes() {

  if (!HCtx) {
    HCtx = std::make_shared<ast::HTypeContext>();
  }
  // The cache spans every SCC because instantiated values can appear in more
  // than one report. It is unused when workdir output is disabled.
  ExtValueLabelCache DebugValueLabels;
  ExtValueLabelCache *DebugLabelCache =
      notdec::getWorkDirOpt() ? &DebugValueLabels : nullptr;
  const bool GenTypesDiag =
      std::getenv("NOTDEC_SIMPLIFY_DIAG") != nullptr;
  std::optional<GenTypesTiming> TotalTiming;
  for (std::size_t Ind = 0; Ind < AG.AllSCCs.size(); ++Ind) {
    auto &Data = AG.AllSCCs.at(Ind);
    // 尝试运行简化算法，保存到ValueTypes里面。
    // solve memory if ind == 0
    auto CurrentTiming = Data.Generator->genTypes(
        *HCtx, Mod.getDataLayout().getPointerSize(), Ind == 0,
        DebugLabelCache);
    if (GenTypesDiag) {
      if (!TotalTiming) {
        TotalTiming = CurrentTiming;
      } else {
        addGenTypesTiming(*TotalTiming, CurrentTiming);
      }
    }
  }
  if (TotalTiming) {
    auto ReportTotal = [&](llvm::StringRef Stage,
                           const GenTypesPhaseTiming &Phase) {
      llvm::errs() << "[gen-types-summary] stage=" << Stage
                   << " sccs=" << AG.AllSCCs.size();
      printGenTypesTimingFields(Phase);
      llvm::errs() << "\n";
    };
    ReportTotal("prepare", TotalTiming->Prepare);
    ReportTotal("bulk", TotalTiming->Bulk);
    ReportTotal("lower", TotalTiming->Lower);
    ReportTotal("upper", TotalTiming->Upper);
    ReportTotal("debug_output", TotalTiming->DebugOutput);
  }
  if (std::getenv("NOTDEC_WORKLIST_SNAPSHOT") != nullptr) {
    binarysub::emit_type_graph_summary("solve-and-lower-types-end");
  }
  if (MergeEval) {
    MergeEval->finish(AG);
  }
  for (auto &Data : AG.AllSCCs) {
    Data.Generator->releaseBinarysubState();
  }
  binarysub::release_type_graph(StorageType);
  StorageType = nullptr;
  StorageFields.clear();
}

void MLsubRecovery::prepareSCC(CallGraph &CG) {
  AG.CG = &CG;
  AG.AllSCCs.clear();
  AG.Func2SCCIndex.clear();
  AG.Callee2Callers.clear();

  auto PolyPolicy = loadPolyPolicyConfig();
  const auto &PolyFuncs = PolyPolicy.PolyFuncs;
  const auto &LevelOverrides = PolyPolicy.LevelOverrides;

  // LLVM's CallGraph has synthetic external-caller and external-callee nodes.
  // The former conservatively reaches every address-taken function, while many
  // real functions reach the latter through ordinary library declarations.
  // Including either node in Tarjan's graph turns an otherwise acyclic module
  // into one artificial SCC, so decompose only real Function nodes here.
  std::vector<std::vector<CallGraphNode *>> SCCResults;
  std::map<CallGraphNode *, unsigned int> TarjanIndex;
  std::map<CallGraphNode *, unsigned int> TarjanLowLink;
  std::set<CallGraphNode *> TarjanOnStack;
  std::vector<CallGraphNode *> TarjanStack;
  unsigned int NextTarjanIndex = 0;
  std::function<void(CallGraphNode *)> VisitNode = [&](CallGraphNode *Node) {
    TarjanIndex.emplace(Node, NextTarjanIndex);
    TarjanLowLink.emplace(Node, NextTarjanIndex);
    ++NextTarjanIndex;
    TarjanStack.push_back(Node);
    TarjanOnStack.insert(Node);

    // opaque_body 函数的 body 不进入约束生成，其出边也不应参与 SCC 划分：
    // 否则这些惰性调用会充当同层合并的"粘合剂"（如 allocator -> malloc_usable_size
    // 把独立 polymorphic 区域粘成一个巨型 generator）。见 logs/20260819-01。
    if (auto *SrcFn = Node->getFunction();
        SrcFn != nullptr && isOpaqueBody(*SrcFn)) {
      // 不跟随 opaque 函数的任何出边。
    } else {
      for (auto &CallRecord : *Node) {
        CallGraphNode *Callee = CallRecord.second;
        if (Callee == nullptr || Callee->getFunction() == nullptr) {
          continue;
        }
        if (TarjanIndex.find(Callee) == TarjanIndex.end()) {
          VisitNode(Callee);
          TarjanLowLink.at(Node) =
              std::min(TarjanLowLink.at(Node), TarjanLowLink.at(Callee));
        } else if (TarjanOnStack.count(Callee) != 0) {
          TarjanLowLink.at(Node) =
              std::min(TarjanLowLink.at(Node), TarjanIndex.at(Callee));
        }
      }
    }

    if (TarjanLowLink.at(Node) != TarjanIndex.at(Node)) {
      return;
    }
    std::vector<CallGraphNode *> Component;
    while (true) {
      CallGraphNode *Member = TarjanStack.back();
      TarjanStack.pop_back();
      TarjanOnStack.erase(Member);
      Component.push_back(Member);
      if (Member == Node) {
        break;
      }
    }
    // Tarjan emits callee components before their callers, matching the old
    // all_scc_iterator order consumed in reverse below.
    SCCResults.push_back(std::move(Component));
  };
  // Iterate in module declaration order instead of CallGraph's
  // std::map<const Function *, ...> (which sorts by function object address).
  // Pointer-order iteration changes with ASLR/allocator layout, so Tarjan SCC
  // numbering, variable ids and downstream simplification would otherwise
  // differ between runs and make memory/results non-reproducible.
  auto &Module = CG.getModule();
  for (Function &F : Module) {
    CallGraphNode *Node = CG[&F];
    if (Node->getFunction() == nullptr ||
        TarjanIndex.find(Node) != TarjanIndex.end()) {
      continue;
    }
    VisitNode(Node);
  }
  // 遍历所有的CallGraphNode，然后构建一个反向的，从callee到所有caller的map。
  // opaque_body 调用者的出边同样跳过，与 Tarjan 的 SCC 划分保持一致。
  for (Function &F : Module) {
    if (isOpaqueBody(F)) {
      continue;
    }
    CallGraphNode *Caller = CG[&F];
    for (auto &CallRecord : *Caller) {
      CallGraphNode *Callee = CallRecord.second;
      AG.Callee2Callers[Callee].insert(Caller);
    }
  }

  auto isAllDeclarationAndIntrinsics =
      [](const std::vector<CallGraphNode *> &NodeVec) -> bool {
    for (auto *CGN : NodeVec) {
      if (auto *Fn = CGN->getFunction()) {
        if (!Fn->isDeclaration() || !Fn->isIntrinsic()) {
          return false;
        }
      }
    }
    return true;
  };

  auto hasPolymorphic =
      [&](const std::vector<CallGraphNode *> &NodeVec) -> bool {
    for (auto *CGN : NodeVec) {
      if (auto *Fn = CGN->getFunction()) {
        if (isSummaryOverridePolymorphic(*Fn)) {
          return true;
        }
        if (Fn->hasName() && PolyFuncs.count(Fn->getName().str())) {
          return true;
        }
        if (Fn->getMetadata(KIND_MLSUB_POLYMORPHIC_FUNCTION) != nullptr) {
          return true;
        }
        if (isPolymorphic(Fn)) {
          return true;
        }
      }
    }
    return false;
  };

  struct RawSCCInfo {
    std::vector<CallGraphNode *> Nodes;
    std::set<std::size_t> Preds;
    std::set<std::size_t> Succs;
    bool IsPolymorphic = false;
    unsigned int UserLevelLowerBound = 0;
    unsigned int Level = 0;
    std::size_t TopoPosition = 0;
  };

  std::vector<RawSCCInfo> RawSCCs;
  std::map<CallGraphNode *, std::size_t> Node2RawSCCIndex;
  for (auto It = SCCResults.rbegin(); It != SCCResults.rend(); ++It) {
    const std::vector<CallGraphNode *> &NodeVec = *It;
    if (isAllDeclarationAndIntrinsics(NodeVec)) {
      continue;
    }
    std::size_t RawIndex = RawSCCs.size();
    RawSCCs.push_back(RawSCCInfo{.Nodes = NodeVec});
    for (auto *Node : NodeVec) {
      Node2RawSCCIndex[Node] = RawIndex;
    }
  }

  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    auto &Raw = RawSCCs[RawIndex];
    Raw.IsPolymorphic = hasPolymorphic(Raw.Nodes);
    for (auto *Node : Raw.Nodes) {
      auto *Fn = Node->getFunction();
      if (Fn == nullptr || !Fn->hasName()) {
        continue;
      }
      auto It = LevelOverrides.find(Fn->getName().str());
      if (It != LevelOverrides.end()) {
        Raw.UserLevelLowerBound =
            std::max(Raw.UserLevelLowerBound, It->second);
      }
    }
  }

  for (auto &[Callee, Callers] : AG.Callee2Callers) {
    auto CalleeIt = Node2RawSCCIndex.find(Callee);
    if (CalleeIt == Node2RawSCCIndex.end()) {
      continue;
    }
    std::size_t CalleeIndex = CalleeIt->second;
    for (auto *Caller : Callers) {
      auto CallerIt = Node2RawSCCIndex.find(Caller);
      if (CallerIt == Node2RawSCCIndex.end()) {
        continue;
      }
      std::size_t CallerIndex = CallerIt->second;
      if (CallerIndex == CalleeIndex) {
        continue;
      }
      RawSCCs[CallerIndex].Succs.insert(CalleeIndex);
      RawSCCs[CalleeIndex].Preds.insert(CallerIndex);
    }
  }

  // Topologically order the SCC DAG so every caller level is known before its
  // callees are processed.
  std::vector<std::size_t> TopoOrder;
  std::vector<unsigned int> InDegree(RawSCCs.size(), 0);
  std::set<std::size_t> Ready;
  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    InDegree[RawIndex] = RawSCCs[RawIndex].Preds.size();
    if (InDegree[RawIndex] == 0) {
      Ready.insert(RawIndex);
    }
  }
  while (!Ready.empty()) {
    std::size_t RawIndex = *Ready.begin();
    Ready.erase(Ready.begin());
    RawSCCs[RawIndex].TopoPosition = TopoOrder.size();
    TopoOrder.push_back(RawIndex);
    for (std::size_t SuccIndex : RawSCCs[RawIndex].Succs) {
      assert(InDegree[SuccIndex] > 0);
      --InDegree[SuccIndex];
      if (InDegree[SuccIndex] == 0) {
        Ready.insert(SuccIndex);
      }
    }
  }
  assert(TopoOrder.size() == RawSCCs.size() &&
         "SCC condensation graph must be a DAG");

  // Phase 1: work on the original call-graph SCC DAG first. Levels are solved
  // on raw SCCs before any same-level groups are merged.
  // 两阶段识别"粘合剂" callee 并自动升层：
  // Phase 1: work on the original call-graph SCC DAG first. Levels are solved
  // on raw SCCs before any same-level groups are merged.
  for (std::size_t RawIndex : TopoOrder) {
    auto &Raw = RawSCCs[RawIndex];
    unsigned int BaseLevel = 0;
    for (std::size_t PredIndex : Raw.Preds) {
      BaseLevel = std::max(BaseLevel, RawSCCs[PredIndex].Level);
    }
    // Polymorphic SCCs force a summary boundary, so calls into them instantiate
    // from one level below.
    if (Raw.IsPolymorphic) {
      ++BaseLevel;
    }
    Raw.Level = std::max(BaseLevel, Raw.UserLevelLowerBound);
  }

  // Phase 2: collapse every same-level call region into one generator, but
  // keep polymorphic SCCs as summary boundaries so they do not silently share
  // variables with monomorphic same-level neighbors. Level 0 remains the
  // monomorphic module scope, so globals can connect functions even when
  // optimization removed call edges; all level-0 raw SCCs are therefore merged
  // below even when disconnected.
  std::vector<std::size_t> Parent(RawSCCs.size());
  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    Parent[RawIndex] = RawIndex;
  }
  auto FindRoot = [&](std::size_t Index) {
    std::size_t Root = Index;
    while (Parent[Root] != Root) {
      Root = Parent[Root];
    }
    while (Parent[Index] != Index) {
      std::size_t Next = Parent[Index];
      Parent[Index] = Root;
      Index = Next;
    }
    return Root;
  };
  auto Union = [&](std::size_t LHS, std::size_t RHS) {
    LHS = FindRoot(LHS);
    RHS = FindRoot(RHS);
    if (LHS == RHS) {
      return;
    }
    if (LHS > RHS) {
      std::swap(LHS, RHS);
    }
    Parent[RHS] = LHS;
  };
  // A polymorphic SCC is a summary boundary and normally stays in its own
  // generator.  Exception: when it declares monomorphic positions, keeping the
  // boundary would split the container object across generators, and the shared
  // level-0 node created for those positions could never join the other views
  // of the same object (its fields then never reach the callee's formal).
  // Those SCCs -- and the whole level they sit on -- are therefore collapsed
  // into one generator, exactly like level 0.
  auto DeclaresMonomorphicPositions = [&](std::size_t RawIndex) {
    for (auto *Node : RawSCCs[RawIndex].Nodes) {
      auto *Fn = Node == nullptr ? nullptr : Node->getFunction();
      if (Fn == nullptr || Fn->isDeclaration()) {
        continue;
      }
      if (!resolvePolymorphicPositionSpec(*Fn).empty()) {
        return true;
      }
    }
    return false;
  };
  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    for (std::size_t SuccIndex : RawSCCs[RawIndex].Succs) {
      if (RawSCCs[RawIndex].Level != RawSCCs[SuccIndex].Level) {
        continue;
      }
      if (RawSCCs[RawIndex].IsPolymorphic || RawSCCs[SuccIndex].IsPolymorphic) {
        const bool Allowed =
            (RawSCCs[RawIndex].IsPolymorphic &&
             DeclaresMonomorphicPositions(RawIndex)) ||
            (RawSCCs[SuccIndex].IsPolymorphic &&
             DeclaresMonomorphicPositions(SuccIndex));
        if (!Allowed) {
          continue;
        }
      }
      Union(RawIndex, SuccIndex);
    }
  }
  std::optional<std::size_t> LevelZeroRoot;
  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    if (RawSCCs[RawIndex].Level != 0) {
      continue;
    }
    if (!LevelZeroRoot) {
      LevelZeroRoot = RawIndex;
      continue;
    }
    Union(*LevelZeroRoot, RawIndex);
  }
  // NOTE: collapsing the whole level here was tried and blows up on the hash
  // family (4 coreutils cases OOM-killed at 6-17 GB RSS each): one generator for
  // a level with dozens of SCCs is far too large.  The edge based rule above
  // already unions the polymorphic SCC with the same-level neighbours it
  // actually calls, which is what the shared position needs.

  struct GroupInfo {
    std::size_t Root = 0;
    unsigned int Level = 0;
    std::size_t FirstTopoPosition = 0;
  };

  std::map<std::size_t, std::vector<std::size_t>> Groups;
  std::map<std::size_t, std::size_t> FirstTopoPosition;
  for (std::size_t RawIndex : TopoOrder) {
    std::size_t Root = FindRoot(RawIndex);
    Groups[Root].push_back(RawIndex);
    if (!FirstTopoPosition.count(Root)) {
      FirstTopoPosition[Root] = RawSCCs[RawIndex].TopoPosition;
    }
  }

  std::map<std::size_t, GroupInfo> GroupInfos;
  for (auto &[Root, Members] : Groups) {
    (void)Members;
    GroupInfos.emplace(Root, GroupInfo{
        .Root = Root,
        .Level = RawSCCs[Root].Level,
        .FirstTopoPosition = FirstTopoPosition.at(Root),
    });
  }

  // A DSU group can contain raw SCCs from separated parts of the original
  // topological order. Sorting only by its first member can therefore place a
  // caller group after a callee group. Rebuild the order from the contracted
  // call graph so bottomUpPhase() always sees every callee generator first.
  std::map<std::size_t, std::set<std::size_t>> GroupSuccs;
  std::map<std::size_t, std::size_t> GroupInDegree;
  for (const auto &[Root, Info] : GroupInfos) {
    (void)Info;
    GroupInDegree.emplace(Root, 0);
  }
  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    std::size_t Root = FindRoot(RawIndex);
    for (std::size_t SuccIndex : RawSCCs[RawIndex].Succs) {
      std::size_t SuccRoot = FindRoot(SuccIndex);
      if (Root == SuccRoot) {
        continue;
      }
      // Equal-level edges between polymorphic SCCs and their monomorphic
      // neighbors are kept (polymorphic SCCs are summary boundaries and are
      // not contracted), so a remaining generator edge can be same-level too.
      assert(RawSCCs[RawIndex].Level <= RawSCCs[SuccIndex].Level);
      if (GroupSuccs[Root].insert(SuccRoot).second) {
        ++GroupInDegree.at(SuccRoot);
      }
    }
  }

  std::set<std::tuple<unsigned int, std::size_t, std::size_t>> ReadyGroups;
  auto addReadyGroup = [&](std::size_t Root) {
    const auto &Info = GroupInfos.at(Root);
    ReadyGroups.emplace(Info.Level, Info.FirstTopoPosition, Root);
  };
  for (const auto &[Root, InDegree] : GroupInDegree) {
    if (InDegree == 0) {
      addReadyGroup(Root);
    }
  }

  std::vector<GroupInfo> OrderedGroups;
  OrderedGroups.reserve(Groups.size());
  while (!ReadyGroups.empty()) {
    auto ReadyIt = ReadyGroups.begin();
    std::size_t Root = std::get<2>(*ReadyIt);
    ReadyGroups.erase(ReadyIt);
    OrderedGroups.push_back(GroupInfos.at(Root));
    for (std::size_t SuccRoot : GroupSuccs[Root]) {
      auto &InDegree = GroupInDegree.at(SuccRoot);
      assert(InDegree > 0);
      --InDegree;
      if (InDegree == 0) {
        addReadyGroup(SuccRoot);
      }
    }
  }
  assert(OrderedGroups.size() == Groups.size() &&
         "contracted SCC group graph must be a DAG");

  std::vector<SCCData> &AllSCCs = AG.AllSCCs;
  std::map<CallGraphNode *, std::size_t> &Func2SCCIndex = AG.Func2SCCIndex;
  for (const auto &Group : OrderedGroups) {
    SCCData Data;
    Data.level = Group.Level;
    for (std::size_t RawIndex : Groups.at(Group.Root)) {
      assert(RawSCCs[RawIndex].Level == Data.level);
      Data.Nodes.insert(Data.Nodes.end(), RawSCCs[RawIndex].Nodes.begin(),
                        RawSCCs[RawIndex].Nodes.end());
    }
    AllSCCs.push_back(Data);
  }

  // 2. Calc name and SCCSet
  for (size_t SCCIndex = 0; SCCIndex < AllSCCs.size(); ++SCCIndex) {
    SCCData &Data = AllSCCs[SCCIndex];
    std::set<llvm::Function *> &SCCSet = Data.SCCSet;
    SCCSet.clear();

    std::string Name;
    for (auto *CGN : Data.Nodes) {
      auto *Fn = CGN->getFunction();
      if (Fn == nullptr) {
        continue;
      }
      if (!Name.empty()) {
        Name += ",";
      }
      Name += Fn->getName().str();
      SCCSet.insert(Fn);
      Func2SCCIndex[CGN] = SCCIndex;
    }
    if (!Name.empty()) {
      Data.SCCName = Name;
    }
    // write the SCC index to file
    if (SCCsCatalog) {
      *SCCsCatalog << "SCC" << SCCIndex << "," << Name
                   << " (level = " << Data.level << ")" << "\n";
    }
  }
  // Keep the partition available when a large benchmark is stopped after
  // constraint generation but before the normal workdir teardown.
  if (SCCsCatalog) {
    SCCsCatalog->flush();
  }
}

SimpleType ConstraintsGenerator::convertSimpleType(ExtValuePtr Val) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    return convertSimpleTypeVal(*V, nullptr, -1);
  } else if (std::get_if<ReturnValue>(&Val)) {
    return binarysub::make_variable(lvl, getSize(Val));
  } else if (auto IC = std::get_if<UConstant>(&Val)) {
    return convertSimpleTypeVal(IC->Val, IC->User, IC->OpInd);
  } else if (auto CA = std::get_if<ConstantAddr>(&Val)) {
    // as field access.
    auto res = binarysub::fresh_variable(lvl, getSize(Val));
    if (auto Offset = getSigned64ConstantAddress(*CA->Val)) {
      std::vector<std::pair<std::string, SimpleType>> fields;
      fields.push_back({OffsetRange{.offset = *Offset}.str(), res});
      addSubtype(MemoryType, binarysub::make_record(std::move(fields)));
    }
    addSubtype(res, binarysub::make_record({}));
    return res;
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: ConstraintsGenerator::convertSimpleType unhandled "
                  "type of ExtValPtr\n";
  std::abort();
}

SimpleType ConstraintsGenerator::makeAggregateReturnRecord(
    llvm::Type *Ty,
    const std::function<SimpleType(unsigned, llvm::Type *)> &MakeField) {
  std::vector<std::pair<std::string, SimpleType>> Fields;
  if (auto *StructTy = dyn_cast<StructType>(Ty)) {
    Fields.reserve(StructTy->getNumElements());
    for (unsigned I = 0; I < StructTy->getNumElements(); ++I) {
      Fields.emplace_back(std::to_string(I),
                          MakeField(I, StructTy->getElementType(I)));
    }
  } else if (auto *ArrayTy = dyn_cast<ArrayType>(Ty)) {
    Fields.reserve(ArrayTy->getNumElements());
    for (unsigned I = 0; I < ArrayTy->getNumElements(); ++I) {
      Fields.emplace_back(std::to_string(I),
                          MakeField(I, ArrayTy->getElementType()));
    }
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "ERROR: unsupported aggregate return type: " << *Ty
                 << "\n";
    std::abort();
  }
  return binarysub::make_record(std::move(Fields));
}

static SimpleType makeAggregateReturnFieldSeed(llvm::Type *Ty,
                                               unsigned PointerSize) {
  if (Ty->isIntegerTy(1)) {
    return binarysub::make_primitive("bool", 1);
  }
  if (Ty->isIntegerTy()) {
    return binarysub::make_primitive("uint",
                                     Ty->getScalarSizeInBits());
  }
  if (Ty->isFloatingPointTy()) {
    return binarysub::make_primitive("float",
                                     Ty->getScalarSizeInBits());
  }
  if (Ty->isPointerTy() || Ty->isFunctionTy()) {
    return binarysub::make_record({});
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: unsupported aggregate return field type: " << *Ty
               << "\n";
  std::abort();
}

SimpleType
ConstraintsGenerator::makeFunctionAggregateReturnRecord(llvm::Function &Func) {
  return makeAggregateReturnRecord(
      Func.getReturnType(), [&](unsigned Index, llvm::Type *FieldTy) {
        auto Slot = getOrInsertNode(ReturnValue{
            .Func = &Func, .Index = static_cast<int32_t>(Index)});
        addSubtype(makeAggregateReturnFieldSeed(FieldTy, PointerSize), Slot);
        return Slot;
      });
}

SimpleType
ConstraintsGenerator::getOrCreateCallAggregateReturnSlot(llvm::CallBase &Call,
                                                         unsigned Index) {
  auto Key = std::make_pair(&Call, Index);
  auto It = AggregateCallReturnSlots.find(Key);
  if (It != AggregateCallReturnSlots.end()) {
    return It->second;
  }
  auto *Ty = Call.getType();
  llvm::Type *FieldTy = nullptr;
  if (auto *StructTy = dyn_cast<StructType>(Ty)) {
    FieldTy = StructTy->getElementType(Index);
  } else if (auto *ArrayTy = dyn_cast<ArrayType>(Ty)) {
    FieldTy = ArrayTy->getElementType();
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "ERROR: call is not aggregate typed: " << Call << "\n";
    std::abort();
  }
  auto Slot = binarysub::make_variable(lvl, getLLVMTypeSize(FieldTy));
  addSubtype(makeAggregateReturnFieldSeed(FieldTy, PointerSize), Slot);
  AggregateCallReturnSlots.emplace(Key, Slot);
  return Slot;
}

SimpleType
ConstraintsGenerator::makeCallAggregateReturnRecord(llvm::CallBase &Call) {
  return makeAggregateReturnRecord(
      Call.getType(), [&](unsigned Index, llvm::Type *) {
        return getOrCreateCallAggregateReturnSlot(Call, Index);
      });
}

void ConstraintsGenerator::addAggregateReturnConstraints(llvm::Value *Agg,
                                                         llvm::ReturnInst &Ret) {
  auto *Insert = dyn_cast<InsertValueInst>(Agg);
  if (Insert == nullptr) {
    return;
  }
  addAggregateReturnConstraints(Insert->getAggregateOperand(), Ret);
  auto Indices = Insert->getIndices();
  if (Indices.size() != 1) {
    return;
  }
  auto *Inserted = Insert->getInsertedValueOperand();
  if (Inserted->getType()->isAggregateType() || Inserted->getType()->isVoidTy()) {
    return;
  }
  auto Src = getOrInsertNode(getExtValuePtr(
      Inserted, Insert, InsertValueInst::getInsertedValueOperandIndex()));
  auto Dst = getOrInsertNode(ReturnValue{
      .Func = Ret.getFunction(), .Index = static_cast<int32_t>(Indices[0])});
  addSubtype(Src, Dst);
}

void ConstraintsGenerator::maybeUnifyPNDiffTypeVariablePair(
    const SimpleType &Lhs, const SimpleType &Rhs) {
  if (!EnablePNDiffTypeVariableClosureUnification || !Lhs || !Rhs) {
    return;
  }
  if (!Lhs->isVariableState() || !Rhs->isVariableState()) {
    return;
  }

  auto gatherMappedValues = [&](const SimpleType &Ty) {
    std::vector<ExtValuePtr> Values;
    const auto &Reverse = V2N.rev();
    auto It = Reverse.find(Ty);
    if (It == Reverse.end()) {
      return Values;
    }
    Values.assign(It->second.begin(), It->second.end());
    return Values;
  };

  auto LeftValues = gatherMappedValues(Lhs);
  auto RightValues = gatherMappedValues(Rhs);
  if (LeftValues.empty() || RightValues.empty()) {
    return;
  }

  unifyPNDiffValueGroups(LeftValues, RightValues, Lhs, Rhs);
}

void ConstraintsGenerator::unifyPNDiffValueGroups(
    llvm::ArrayRef<ExtValuePtr> LeftValues,
    llvm::ArrayRef<ExtValuePtr> RightValues, const SimpleType &Lhs,
    const SimpleType &Rhs) {
  if (!EnablePNDiffTypeVariableClosureUnification) {
    return;
  }

  PNINode *Leader = nullptr;
  auto unifyMappedValues = [&](const std::vector<ExtValuePtr> &Values) {
    for (const auto &Val : Values) {
      auto *Node = getPNINodeOrNull(Val);
      if (Node == nullptr) {
        continue;
      }
      if (Leader == nullptr) {
        Leader = Node;
      } else {
        Leader = Leader->unify(*Node);
      }
    }
  };

  unifyMappedValues(LeftValues);
  unifyMappedValues(RightValues);
  if (Leader != nullptr && PG.traceEnabled()) {
    std::ostringstream OS;
    OS << "[pndiff:closure-unify] lhs=" << binarysub::debug_string(Lhs)
       << " rhs=" << binarysub::debug_string(Rhs)
       << " leader=" << Leader->serialize();
    PG.trace(OS.str());
  }
}

SimpleType ConstraintsGenerator::convertSimpleTypeVal(Value *Val,
                                                      llvm::User *User,
                                                      long OpInd) {
  if (Val->getType()->isIntegerTy(1)) {
    return binarysub::make_primitive("bool", 1);
  } else if (Val->getType()->isFloatingPointTy()) {
    return binarysub::make_primitive(
        "float", getSize(getExtValuePtr(Val, User, OpInd)));
  }

  if (Constant *C = dyn_cast<Constant>(Val)) {
    // check for constantExpr
    if (auto CE = dyn_cast<ConstantExpr>(C)) {
      // ignore bitcast ConstantExpr
      if (CE->getOpcode() == Instruction::BitCast) {
        return convertSimpleType(getExtValuePtr(CE->getOperand(0), CE, 0));
      } else if (CE->getOpcode() == Instruction::IntToPtr ||
                 CE->getOpcode() == Instruction::PtrToInt) {
        // ConstantExpr casts can appear in globals or optimized stores, e.g. an
        // integer table cell initialized from the address of a string. There is
        // no instruction to visit and remap first, so keep the cast result as an
        // ordinary value of its own LLVM type instead of aborting or connecting
        // the source pointer into struct-layout constraints.
        return binarysub::make_variable(
            lvl, getSize(getExtValuePtr(C, User, OpInd)));
      } else if (CE->getOpcode() == Instruction::GetElementPtr) {
        // getelementptr of table, i.e., function pointer array
        if (auto GV = dyn_cast<GlobalVariable>(CE->getOperand(0))) {
          if (isFunctionPointerTable(*GV)) {
            // if constant offset
            if (auto CI1 = dyn_cast<ConstantInt>(CE->getOperand(1))) {
              if (CI1->isZero()) {
                if (isa<ConstantInt>(CE->getOperand(2))) {
                  assert(false && "TODO");
                }
              }
            }
          }
        }
        return binarysub::make_variable(
            lvl, getSize(getExtValuePtr(C, User, OpInd)));
      } else {
        // Constant address arithmetic such as
        // `i64 add (i64 ptrtoint (ptr @mem to i64), C)` appears in
        // Microsub2-lifted IR.  It cannot be connected to a pointee layout
        // here, but aborting the whole type-recovery run turns a local
        // unknown into a benchmark-wide failure.  Keep it as an ordinary
        // size-typed variable instead.
        llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                     << "WARN: ConstraintsGenerator::convertSimpleTypeVal "
                        "unhandled ConstantExpr, treating as variable: "
                     << *C << "\n";
        return binarysub::make_variable(
            lvl, getSize(getExtValuePtr(C, User, OpInd)));
      }
    } else if (auto gv = dyn_cast<GlobalValue>(C)) { // global variable
      return binarysub::make_variable(lvl, getSize(gv));
      // if (gv == Ctx.StackPointer) {
      //   std::cerr
      //       << "Error: convertTypeVarVal: direct use of stack pointer?,
      //       ensure "
      //          "StackAllocationRecovery is run before, Or add external
      //          summary " "for this function.\n";
      //   return makeTv(Ctx.TRCtx, ValueNamer::getName());
      // } else if (auto Func = dyn_cast<Function>(C)) {
      //   return makeTv(Ctx.TRCtx, getFuncTvName(Func));
      // }
      // return makeTv(Ctx.TRCtx, gv->getName().str());
    } else if (isa<ConstantInt>(C) || isa<ConstantFP>(C)) {
      if (auto CI = dyn_cast<ConstantInt>(C)) {
        return binarysub::make_variable(
            lvl, getSize(getExtValuePtr(CI, User, OpInd)));
      }
      return binarysub::make_primitive(
          "float", getSize(getExtValuePtr(C, User, OpInd)));
    } else if (isa<ConstantPointerNull>(C)) {
      return binarysub::make_variable(
          lvl, getSize(getExtValuePtr(C, User, OpInd)));
    } else if (C->getType()->isAggregateType() || C->getType()->isVectorTy()) {
      return binarysub::make_variable(
          lvl, getSize(getExtValuePtr(C, User, OpInd)));
    } else if (isa<UndefValue>(C)) {
      return binarysub::make_variable(
          lvl, getSize(getExtValuePtr(C, User, OpInd)));
    }
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "WARN: ConstraintsGenerator::convertSimpleTypeVal "
                    "unhandled type of constant, treating as variable: "
                 << *C << "\n";
    return binarysub::make_variable(
        lvl, getSize(getExtValuePtr(C, User, OpInd)));
  } else if (auto arg = dyn_cast<Argument>(Val)) { // for function argument
    return binarysub::make_variable(lvl, getSize(arg));
  }

  if (auto *I = dyn_cast<Instruction>(Val)) {
    return binarysub::make_variable(lvl, getSize(I));
  }
  llvm::errs()
      << __FILE__ << ":" << __LINE__ << ": "
      << "WARN: ConstraintsGenerator::convertSimpleTypeVal unhandled value: "
      << *Val << "\n";
  return binarysub::make_variable(lvl, getSize(Val));
}

// #region ConstraintsGenerator::MLsubVisitor

// Helper functions
static bool isWithOverflowIntrinsicSigned(llvm::Intrinsic::ID ID) {
  if (ID == Intrinsic::sadd_with_overflow ||
      ID == Intrinsic::ssub_with_overflow ||
      ID == Intrinsic::smul_with_overflow) {
    return true;
  }
  return false;
}

static bool isWithOverflowIntrinsicUnsigned(llvm::Intrinsic::ID ID) {
  if (ID == Intrinsic::uadd_with_overflow ||
      ID == Intrinsic::usub_with_overflow ||
      ID == Intrinsic::umul_with_overflow) {
    return true;
  }
  return false;
}

static bool isMinMaxIntrinsic(llvm::Intrinsic::ID ID) {
  return ID == Intrinsic::smax || ID == Intrinsic::smin ||
         ID == Intrinsic::umax || ID == Intrinsic::umin;
}

static bool isEVMModule(const llvm::Module &M) {
  return StringRef(M.getTargetTriple().getTriple()).starts_with("evm");
}

static SimpleType makeEVMSemanticPrimitive(llvm::StringRef Name) {
  return binarysub::make_primitive(("prim.uint256.evm." + Name).str(), 256);
}

static bool constantHasAllPointerTagMiddleBits(const ConstantInt &CI) {
  const APInt &Val = CI.getValue();
  unsigned BitWidth = Val.getBitWidth();
  if (BitWidth <= 8) {
    return false;
  }
  APInt Mask = APInt::getBitsSet(BitWidth, 8, std::min(BitWidth, 30u));
  return (Val & Mask) == Mask;
}

static bool constantHasNoPointerTagMiddleBits(const ConstantInt &CI) {
  const APInt &Val = CI.getValue();
  unsigned BitWidth = Val.getBitWidth();
  if (BitWidth <= 8) {
    return true;
  }
  APInt Mask = APInt::getBitsSet(BitWidth, 8, std::min(BitWidth, 30u));
  return !(Val & Mask);
}

// Experiment switch (NOTDEC_NO_UINT_CONSTRAINTS): stop emitting "uint"
// primitive bounds at generation time.  Both the pcode-op signature table and
// the overflow intrinsics then treat uint like the neutral "int" tag, so the
// solver never creates a uint <: sint or uint <: record pair from them.
static bool skipUintPrimitiveConstraints() {
  static const bool Skip = std::getenv("NOTDEC_NO_UINT_CONSTRAINTS") != nullptr;
  return Skip;
}

static std::optional<std::uint64_t> getUInt64Constant(const Value *V) {
  const auto *CI = dyn_cast_or_null<ConstantInt>(V);
  if (CI == nullptr || CI->getValue().getActiveBits() > 64) {
    return std::nullopt;
  }
  return CI->getZExtValue();
}

static inline void ensureSequence(Value *&Src1, Value *&Src2,
                                  unsigned &Src1Ind, unsigned &Src2Ind) {
  if (llvm::isa<llvm::ConstantInt>(Src1) &&
      llvm::isa<llvm::ConstantInt>(Src2)) {
    assert(false && "Constant at both sides. Run Optimization first!");
  }
  if (llvm::isa<llvm::ConstantInt>(Src1) &&
      !llvm::isa<llvm::ConstantInt>(Src2)) {
    // X64 源码级 IR 分支不跑 buildFunctionOptimizations（InstCombine），
    // -O0 下 and/or 的常量操作数保持源码顺序可能在左侧；and/or 可交换，
    // 换到右侧语义不变。操作数索引必须一起换，否则后续
    // getExtValuePtr(Src, &I, OpInd) 的 user 检查会失败。
    std::swap(Src1, Src2);
    std::swap(Src1Ind, Src2Ind);
  }
}

// Visitor method implementations
void ConstraintsGenerator::MLsubVisitor::visitExtractValueInst(
    ExtractValueInst &I) {
  if (auto Call = dyn_cast<CallBase>(I.getAggregateOperand())) {
    if (auto Target = Call->getCalledFunction()) {
      if (Target->isIntrinsic()) {
        // 这里判断返回值是不是那种extract
        // value的东西，根据llvm类型直接设置为数字类型
        auto Ind = I.getIndices()[0];
        if (Ind == 0) {
          if (isWithOverflowIntrinsicSigned(Target->getIntrinsicID())) {
            auto N = cg.createNode(&I);
            // Overflow intrinsics return {value, overflow}; field 0 is the
            // arithmetic result and should stay a plain number.
            cg.setNonPointer(&I);
            auto SintNode = binarysub::make_primitive("sint", cg.getSize(&I));
            cg.addSubtype(SintNode, N);
            return;
          } else if (isWithOverflowIntrinsicUnsigned(
                         Target->getIntrinsicID())) {
            auto N = cg.createNode(&I);
            // Overflow intrinsics return {value, overflow}; field 0 is the
            // arithmetic result and should stay a plain number.
            cg.setNonPointer(&I);
            if (skipUintPrimitiveConstraints()) {
              return;
            }
            auto UintNode = binarysub::make_primitive("uint", cg.getSize(&I));
            cg.addSubtype(UintNode, N);
            return;
          }
        } else if (Ind == 1 &&
                   (isWithOverflowIntrinsicSigned(Target->getIntrinsicID()) ||
                    isWithOverflowIntrinsicUnsigned(
                        Target->getIntrinsicID()))) {
          // 只有 overflow intrinsic 的 field 1 是 i1；其他 aggregate 返回
          // 的 intrinsic（如 llvm.modf.f64 的 {double, double}）走下面的
          // 通用路径，避免对非 i1 断言崩溃。
          assert(I.getType()->isIntegerTy(1));
          cg.createNode(&I);
          return;
        }
      }
    }
    auto Indices = I.getIndices();
    if (Indices.size() == 1 && !I.getType()->isAggregateType()) {
      auto Dst = cg.createNode(&I);
      auto Src = cg.getOrCreateCallAggregateReturnSlot(*Call, Indices[0]);
      cg.addSubtype(Src, Dst);
      return;
    }
  }
  if (!I.getType()->isAggregateType()) {
    cg.createNode(&I);
    return;
  }
  assert(false && "TODO: ExtractValueInst aggregate result");
}

void ConstraintsGenerator::MLsubVisitor::visitInsertValueInst(
    InsertValueInst &I) {
  // The aggregate value itself is only an LLVM carrier for multi-return values.
  // ReturnInst walks the insertvalue chain and connects inserted scalar values
  // to the current function's per-field return slots.
  if (I.getType()->isAggregateType()) {
    return;
  }
  visitInstruction(I);
}

void ConstraintsGenerator::MLsubVisitor::visitCastInst(CastInst &I) {
  if (isa<BitCastInst, PtrToIntInst, IntToPtrInst>(I)) {
    // A constant inttoptr is an address: hand it to the memory object as a
    // field so the constant node is connected by an offset edge.  This is not
    // EVM-specific: a constant only becomes an inttoptr when the code is about
    // to use it as an address.
    if (isa<IntToPtrInst>(I)) {
      if (auto *CI = dyn_cast<ConstantInt>(I.getOperand(0))) {
        ConstantAddr Addr{.Val = CI};
        cg.getOrInsertNode(Addr);
        cg.addRemapType(&I, Addr);
        return;
      }
    }
    // Treat pointer/int casts as aliases for PNDiff even when the source
    // hasn't been materialized as a MLsub type variable yet.
    auto *Src = I.getOperand(0);
    auto SrcVal = getExtValuePtr(Src, &I, 0);
    cg.getOrInsertNode(SrcVal);
    if (cg.getPNINodeOrNull(SrcVal) != nullptr) {
      // Materialize the cast result first so its LLVM low type participates in
      // the merge; otherwise inttoptr can lose the result-side pointer hint and
      // collapse to the source's unknown node.
      cg.getOrInsertPNINode(&I);
      cg.remapPNINode(&I, SrcVal);
    }
    if (cg.getNodeOrNull(SrcVal) != nullptr) {
      cg.addRemapType(&I, SrcVal);
    }
    return;
  } else if (isa<TruncInst, ZExtInst, SExtInst, FPToUIInst, FPToSIInst,
                 UIToFPInst, SIToFPInst, FPTruncInst, FPExtInst>(&I)) {
    visitInstruction(I);
    return;
  }

  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: unhandled CastInst: " << I << "\n";
  std::abort();
}

bool ConstraintsGenerator::MLsubVisitor::isHeapAllocationCall(
    llvm::CallBase &I) {
  if (auto F = I.getCalledFunction()) {
    StringRef Name = F->getName();
    if (Name == "malloc" || Name == "calloc") {
      return true;
    }
    auto *M = I.getModule();
    bool IsEVM = M != nullptr && isEVMModule(*M);
    if (IsEVM && (Name == "calloc_unbounded" ||
                  Name == "notdec_evm_alloc" ||
                  Name == "notdec_evm_alloc_unbounded")) {
      return true;
    }
  }
  return false;
}

bool ConstraintsGenerator::MLsubVisitor::shouldIgnoreRuntimeCall(
    llvm::CallBase &I) {
  auto *F = I.getCalledFunction();
  auto *M = I.getModule();
  if (F == nullptr || M == nullptr || !isEVMModule(*M)) {
    return false;
  }

  StringRef Name = F->getName();
  if (Name == "notdec_evm_finalize_alloc") {
    return true;
  }
  if (Name.starts_with("notdec_solidity_")) {
    return true;
  }
  return Name.starts_with("evm_");
}

bool ConstraintsGenerator::MLsubVisitor::handleEVMMarkerCall(
    llvm::CallBase &I) {
  auto *F = I.getCalledFunction();
  auto *M = I.getModule();
  if (F == nullptr || M == nullptr || !isEVMModule(*M)) {
    return false;
  }

  StringRef Name = F->getName();
  if (Name == "notdec_evm_calldata_min_size") {
    if (I.arg_size() >= 1 && !I.getType()->isVoidTy()) {
      auto Calldata = getExtValuePtr(I.getArgOperand(0), &I, 0);
      cg.getOrInsertNode(Calldata);
      cg.addRemapType(&I, Calldata);
    }
    if (I.arg_size() >= 2 && I.getArgOperand(1)->getType()->isIntegerTy()) {
      cg.setNonPointer(getExtValuePtr(I.getArgOperand(1), &I, 1));
    }
    return true;
  }

  bool IsWordWrite =
      Name == "notdec_solidity_memory_write" ||
      Name == "notdec_solidity_abi_return_data_word_write" ||
      Name == "notdec_solidity_event_data_word_write" ||
      Name == "notdec_solidity_external_call_input_word_write";
  bool IsWordRead = Name == "notdec_solidity_memory_read" ||
                    Name == "notdec_solidity_external_call_output_word_read";
  if (!IsWordWrite && !IsWordRead) {
    return false;
  }
  if (I.arg_size() < 3) {
    return true;
  }

  auto Offset = getUInt64Constant(I.getArgOperand(1));
  if (!Offset.has_value()) {
    return true;
  }

  OffsetRange Field{.offset = static_cast<OffsetTy>(*Offset)};
  auto BaseTy = cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(0), &I, 0));
  auto ValueTy = cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(2), &I, 2));
  std::vector<std::pair<std::string, SimpleType>> Fields;
  Fields.emplace_back(Field.str(), ValueTy);
  auto RecordTy = binarysub::make_record(std::move(Fields));

  cg.addSubtype(BaseTy, RecordTy);
  return true;
}

void addEVMSemanticConstraint(ConstraintsGenerator &CG, ExtValuePtr Val,
                              llvm::StringRef SemanticName) {
  CG.setNonPointer(Val);
  CG.addSubtype(makeEVMSemanticPrimitive(SemanticName),
                CG.getOrInsertNode(Val));
}

std::optional<std::string> getDirectStorageSlotFieldName(llvm::Value *Slot) {
  auto DirectSlot = getUInt64Constant(Slot);
  if (!DirectSlot.has_value()) {
    return std::nullopt;
  }
  return "slot:" + std::to_string(*DirectSlot);
}

struct MappingStorageKeyConstraint {
  std::string FieldName;
  ExtValuePtr Key = static_cast<llvm::Value *>(nullptr);
};

struct StorageFieldPrefixMatch {
  std::string Prefix;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct StorageValueFieldPrefixMatch {
  std::string FieldName;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct MappingStorageFieldMatch {
  std::string Prefix;
  std::string ValueFieldName;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct ArrayStorageFieldMatch {
  std::string Prefix;
  ExtValuePtr Index = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct ArrayLengthStorageFieldMatch {
  std::string Prefix;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct StaticArrayStorageFieldMatch {
  std::string Prefix;
  ExtValuePtr Index = static_cast<llvm::Value *>(nullptr);
};

struct PackedStorageFieldReadMatch {
  std::string FieldName;
  ExtValuePtr Value = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct PackedStorageFieldWriteMatch {
  std::string FieldName;
  ExtValuePtr Value = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

llvm::Value *getStorageEvidenceValue(const ExtValuePtr &Evidence) {
  if (auto *V = std::get_if<llvm::Value *>(&Evidence)) {
    return *V;
  }
  if (auto *C = std::get_if<UConstant>(&Evidence)) {
    return C->Val;
  }
  return nullptr;
}

void addStorageValueEvidenceConstraint(ConstraintsGenerator &cg,
                                       llvm::StringRef FieldName,
                                       const ExtValuePtr &Value) {
  if (getStorageEvidenceValue(Value) == nullptr) {
    return;
  }
  auto FieldTy = cg.getOrCreateStorageField(FieldName, 256);
  auto ValueTy = cg.getOrInsertNode(Value);
  cg.addSubtype(ValueTy, FieldTy);
}

std::optional<SimpleType> getStorageEvidenceType(ConstraintsGenerator &cg,
                                                 const ExtValuePtr &Value) {
  if (getStorageEvidenceValue(Value) == nullptr) {
    return std::nullopt;
  }
  return cg.getOrInsertNode(Value);
}

void addStorageKeyConstraints(
    ConstraintsGenerator &cg,
    llvm::ArrayRef<MappingStorageKeyConstraint> KeyConstraints) {
  for (const auto &KeyConstraint : KeyConstraints) {
    addStorageValueEvidenceConstraint(cg, KeyConstraint.FieldName,
                                      KeyConstraint.Key);
  }
}

struct BytesStorageLengthMatch {
  std::string FieldName;
  ExtValuePtr Length = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct BytesStorageShortDataMatch {
  std::string FieldName;
  ExtValuePtr Data = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct BytesStorageLongElemMatch {
  std::string Prefix;
  ExtValuePtr Index = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

std::optional<StorageFieldPrefixMatch>
getStorageFieldPrefixFromBaseSlot(llvm::Value *BaseSlot, unsigned Depth = 4) {
  if (auto DirectSlot = getUInt64Constant(BaseSlot)) {
    return StorageFieldPrefixMatch{
        .Prefix = "slot:" + std::to_string(*DirectSlot),
    };
  }

  if (Depth == 0) {
    return std::nullopt;
  }

  auto *Sha3 = dyn_cast<llvm::CallBase>(BaseSlot);
  if (Sha3 == nullptr) {
    return std::nullopt;
  }

  auto Scratch =
      notdec::passes::evm::detail::matchStorageScratchKeccak(*Sha3);
  if (!Scratch.has_value()) {
    return std::nullopt;
  }

  auto ScratchBaseSlot = getStorageEvidenceValue(Scratch->BaseSlot);
  auto Base = getStorageFieldPrefixFromBaseSlot(ScratchBaseSlot, Depth - 1);
  if (!Base.has_value()) {
    return std::nullopt;
  }

  auto Prefix = Base->Prefix + ".map";
  Base->KeyConstraints.push_back(MappingStorageKeyConstraint{
      .FieldName = Prefix + ".key",
      .Key = Scratch->Key,
  });
  Base->Prefix = Prefix + ".value";
  return Base;
}

std::optional<uint64_t> getConstantOffsetFromBase(llvm::Value *V,
                                                  llvm::Value *Base,
                                                  unsigned Depth) {
  if (notdec::passes::evm::detail::isSameValue(V, Base)) {
    return 0;
  }
  if (Depth == 0) {
    return std::nullopt;
  }

  auto getOffsetFromOperands =
      [&](llvm::Value *LHS, llvm::Value *RHS) -> std::optional<uint64_t> {
    if (auto LOffset = getConstantOffsetFromBase(LHS, Base, Depth - 1)) {
      if (auto RConst = getUInt64Constant(RHS)) {
        return *LOffset + *RConst;
      }
    }
    if (auto ROffset = getConstantOffsetFromBase(RHS, Base, Depth - 1)) {
      if (auto LConst = getUInt64Constant(LHS)) {
        return *ROffset + *LConst;
      }
    }
    return std::nullopt;
  };

  if (auto *Add = dyn_cast<llvm::BinaryOperator>(V)) {
    if (Add->getOpcode() == llvm::Instruction::Add) {
      return getOffsetFromOperands(Add->getOperand(0), Add->getOperand(1));
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_add") &&
        Call->arg_size() == 2) {
      return getOffsetFromOperands(Call->getArgOperand(0),
                                   Call->getArgOperand(1));
    }
  }

  return std::nullopt;
}

std::optional<StorageValueFieldPrefixMatch>
getStorageValueFieldPrefixFromSlotExpression(llvm::Value *StorageSlot,
                                             bool UseField0ForMappingBase,
                                             unsigned Depth = 2) {
  if (auto Base = getStorageFieldPrefixFromBaseSlot(StorageSlot)) {
    std::string FieldName = Base->Prefix;
    if (UseField0ForMappingBase && !Base->KeyConstraints.empty()) {
      FieldName += ".field@slot+0";
    }
    return StorageValueFieldPrefixMatch{
        .FieldName = FieldName,
        .KeyConstraints = std::move(Base->KeyConstraints),
    };
  }
  if (Depth == 0) {
    return std::nullopt;
  }

  auto getOffsetPrefix =
      [&](llvm::Value *LHS,
          llvm::Value *RHS) -> std::optional<StorageValueFieldPrefixMatch> {
    if (auto Prefix = getStorageValueFieldPrefixFromSlotExpression(
            LHS, /*UseField0ForMappingBase=*/false, Depth - 1)) {
      if (auto RConst = getUInt64Constant(RHS)) {
        if (*RConst != 0) {
          Prefix->FieldName += ".field@slot+" + std::to_string(*RConst);
        }
        return Prefix;
      }
    }
    if (auto Prefix = getStorageValueFieldPrefixFromSlotExpression(
            RHS, /*UseField0ForMappingBase=*/false, Depth - 1)) {
      if (auto LConst = getUInt64Constant(LHS)) {
        if (*LConst != 0) {
          Prefix->FieldName += ".field@slot+" + std::to_string(*LConst);
        }
        return Prefix;
      }
    }
    return std::nullopt;
  };

  if (auto *Add = dyn_cast<llvm::BinaryOperator>(StorageSlot)) {
    if (Add->getOpcode() == llvm::Instruction::Add) {
      return getOffsetPrefix(Add->getOperand(0), Add->getOperand(1));
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(StorageSlot)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_add") &&
        Call->arg_size() == 2) {
      return getOffsetPrefix(Call->getArgOperand(0), Call->getArgOperand(1));
    }
  }

  return std::nullopt;
}

// Recognize Solidity's mstore(key, 0), mstore(baseSlot, 32), keccak(0, 64)
// mapping shape. Base slots are normalized to string prefixes; nested mappings
// work when the base slot is itself a matched mapping hash.
std::optional<MappingStorageFieldMatch>
getMappingStorageFieldMatch(llvm::CallBase &Access, llvm::Value *StorageSlot,
                            uint64_t AccessKind) {
  auto Match = notdec::passes::evm::detail::matchStorageMappingAccess(
      Access, StorageSlot, AccessKind);
  if (!Match.has_value()) {
    return std::nullopt;
  }

  auto MatchBaseSlot = getStorageEvidenceValue(Match->BaseSlot);
  auto Base = getStorageFieldPrefixFromBaseSlot(MatchBaseSlot);
  if (!Base.has_value()) {
    return std::nullopt;
  }
  auto Prefix = Base->Prefix + ".map";
  Base->KeyConstraints.push_back(MappingStorageKeyConstraint{
      .FieldName = Prefix + ".key",
      .Key = Match->Key,
  });

  std::string ValueFieldName = Prefix + ".value";
  if (auto Offset =
          getConstantOffsetFromBase(StorageSlot, Match->Hash, /*Depth=*/2)) {
    if (*Offset != 0) {
      ValueFieldName += ".field@slot+" + std::to_string(*Offset);
    }
  }

  return MappingStorageFieldMatch{
      .Prefix = Prefix,
      .ValueFieldName = ValueFieldName,
      .KeyConstraints = std::move(Base->KeyConstraints),
  };
}

std::optional<StorageValueFieldPrefixMatch>
getStorageValueFieldPrefixMatch(llvm::CallBase &Access, llvm::Value *StorageSlot,
                                uint64_t AccessKind,
                                bool UseField0ForMappingBase = false) {
  if (auto FieldName = getDirectStorageSlotFieldName(StorageSlot)) {
    return StorageValueFieldPrefixMatch{.FieldName = *FieldName};
  }

  if (auto FieldPrefix = getStorageValueFieldPrefixFromSlotExpression(
          StorageSlot, UseField0ForMappingBase)) {
    return FieldPrefix;
  }

  auto Mapping = getMappingStorageFieldMatch(Access, StorageSlot, AccessKind);
  if (!Mapping.has_value()) {
    return std::nullopt;
  }

  std::string FieldName = Mapping->ValueFieldName;
  if (UseField0ForMappingBase &&
      FieldName == Mapping->Prefix + ".value") {
    FieldName += ".field@slot+0";
  }

  return StorageValueFieldPrefixMatch{
      .FieldName = FieldName,
      .KeyConstraints = std::move(Mapping->KeyConstraints),
  };
}

ExtValuePtr getDynamicArrayIndex(llvm::Value *StorageSlot,
                                 llvm::Value *DataHash) {
  if (notdec::passes::evm::detail::isSameValue(StorageSlot, DataHash)) {
    return static_cast<llvm::Value *>(nullptr);
  }

  auto getIndexFromOperands = [&](llvm::Value *LHS,
                                  llvm::Value *RHS,
                                  llvm::User *User) -> ExtValuePtr {
    bool LHSIsBase = notdec::passes::evm::detail::dependsOnValue(LHS, DataHash);
    bool RHSIsBase = notdec::passes::evm::detail::dependsOnValue(RHS, DataHash);
    if (LHSIsBase == RHSIsBase) {
      return static_cast<llvm::Value *>(nullptr);
    }
    unsigned Index = LHSIsBase ? 1 : 0;
    return getExtValuePtr(LHSIsBase ? RHS : LHS, User, Index);
  };

  if (auto *Add = dyn_cast<llvm::BinaryOperator>(StorageSlot)) {
    if (Add->getOpcode() == llvm::Instruction::Add) {
      return getIndexFromOperands(Add->getOperand(0), Add->getOperand(1), Add);
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(StorageSlot)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_add") &&
        Call->arg_size() == 2) {
      return getIndexFromOperands(Call->getArgOperand(0), Call->getArgOperand(1),
                                  Call);
    }
  }

  return static_cast<llvm::Value *>(nullptr);
}

std::optional<ArrayStorageFieldMatch>
getArrayStorageFieldMatch(llvm::CallBase &Access, llvm::Value *StorageSlot,
                          uint64_t AccessKind) {
  auto Match = notdec::passes::evm::detail::matchStorageArrayDataAccess(
      Access, StorageSlot, AccessKind);
  if (!Match.has_value()) {
    return std::nullopt;
  }

  auto MatchBaseSlot = getStorageEvidenceValue(Match->BaseSlot);
  auto Base = getStorageFieldPrefixFromBaseSlot(MatchBaseSlot);
  auto BaseValue = getStorageValueFieldPrefixFromSlotExpression(
      MatchBaseSlot, /*UseField0ForMappingBase=*/false);
  if (!Base.has_value() && !BaseValue.has_value()) {
    return std::nullopt;
  }

  std::string Prefix;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
  if (BaseValue.has_value()) {
    Prefix = BaseValue->FieldName;
    KeyConstraints = std::move(BaseValue->KeyConstraints);
  } else {
    Prefix = Base->Prefix;
    KeyConstraints = std::move(Base->KeyConstraints);
  }

  return ArrayStorageFieldMatch{
      .Prefix = Prefix + ".dynamic_array",
      .Index = getDynamicArrayIndex(StorageSlot, Match->DataHash),
      .KeyConstraints = std::move(KeyConstraints),
  };
}

bool functionHasArrayDataAccessForBaseSlot(llvm::Function *F,
                                           llvm::Value *BaseSlot) {
  if (F == nullptr) {
    return false;
  }

  for (llvm::BasicBlock &BB : *F) {
    for (llvm::Instruction &Inst : BB) {
      auto *Call = dyn_cast<llvm::CallBase>(&Inst);
      if (Call == nullptr) {
        continue;
      }

      llvm::Value *StorageSlot = nullptr;
      uint64_t AccessKind = 0;
      if (notdec::passes::evm::detail::isCallTo(Call, "evm_sload") &&
          Call->arg_size() == 1) {
        StorageSlot = Call->getArgOperand(0);
        AccessKind = 1;
      } else if (notdec::passes::evm::detail::isCallTo(Call, "evm_sstore") &&
                 Call->arg_size() == 2) {
        StorageSlot = Call->getArgOperand(0);
        AccessKind = 2;
      } else {
        continue;
      }

      auto Match = notdec::passes::evm::detail::matchStorageArrayDataAccess(
          *Call, StorageSlot, AccessKind);
      if (Match.has_value() &&
          notdec::passes::evm::detail::isSameValue(
              getStorageEvidenceValue(Match->BaseSlot), BaseSlot)) {
        return true;
      }
    }
  }
  return false;
}

std::optional<ArrayLengthStorageFieldMatch>
getArrayLengthStorageFieldMatch(llvm::CallBase &Load) {
  llvm::Value *BaseSlot = Load.getArgOperand(0);
  auto Base = getStorageFieldPrefixFromBaseSlot(BaseSlot);
  auto BaseValue = getStorageValueFieldPrefixFromSlotExpression(
      BaseSlot, /*UseField0ForMappingBase=*/false);
  if (!Base.has_value() && !BaseValue.has_value()) {
    return std::nullopt;
  }
  if (!functionHasArrayDataAccessForBaseSlot(Load.getFunction(), BaseSlot)) {
    return std::nullopt;
  }

  std::string Prefix;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
  if (BaseValue.has_value()) {
    Prefix = BaseValue->FieldName;
    KeyConstraints = std::move(BaseValue->KeyConstraints);
  } else {
    Prefix = Base->Prefix;
    KeyConstraints = std::move(Base->KeyConstraints);
  }

  return ArrayLengthStorageFieldMatch{
      .Prefix = Prefix + ".dynamic_array",
      .KeyConstraints = std::move(KeyConstraints),
  };
}

std::optional<StaticArrayStorageFieldMatch>
getStaticArrayStorageFieldMatch(llvm::Value *StorageSlot) {
  auto getMatchFromOperands =
      [](llvm::Value *LHS,
         llvm::Value *RHS,
         llvm::User *User) -> std::optional<StaticArrayStorageFieldMatch> {
    auto LSlot = getUInt64Constant(LHS);
    auto RSlot = getUInt64Constant(RHS);
    if (LSlot.has_value() == RSlot.has_value()) {
      return std::nullopt;
    }
    uint64_t BaseSlot = LSlot.has_value() ? *LSlot : *RSlot;
    llvm::Value *Index = LSlot.has_value() ? RHS : LHS;
    unsigned OperandIndex = LSlot.has_value() ? 1 : 0;
    return StaticArrayStorageFieldMatch{
        .Prefix = "slot:" + std::to_string(BaseSlot) + ".static_array",
        .Index = getExtValuePtr(Index, User, OperandIndex),
    };
  };

  if (auto *Add = dyn_cast<llvm::BinaryOperator>(StorageSlot)) {
    if (Add->getOpcode() == llvm::Instruction::Add) {
      return getMatchFromOperands(Add->getOperand(0), Add->getOperand(1), Add);
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(StorageSlot)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_add") &&
        Call->arg_size() == 2) {
      return getMatchFromOperands(Call->getArgOperand(0), Call->getArgOperand(1),
                                  Call);
    }
  }

  return std::nullopt;
}

std::optional<unsigned> getLowMaskBitWidth(llvm::Value *V) {
  auto *CI = dyn_cast_or_null<llvm::ConstantInt>(V);
  if (CI == nullptr) {
    return std::nullopt;
  }

  const llvm::APInt &Mask = CI->getValue();
  unsigned Width = Mask.getActiveBits();
  if (Width == 0 || Width >= Mask.getBitWidth()) {
    return std::nullopt;
  }
  for (unsigned I = 0; I < Mask.getBitWidth(); ++I) {
    bool Expected = I < Width;
    if (Mask[I] != Expected) {
      return std::nullopt;
    }
  }
  return Width;
}

struct PackedBitRange {
  unsigned Offset = 0;
  unsigned Width = 0;
};

std::optional<PackedBitRange> getClearMaskBitRange(llvm::Value *V) {
  auto *CI = dyn_cast_or_null<llvm::ConstantInt>(V);
  if (CI == nullptr) {
    return std::nullopt;
  }

  const llvm::APInt &Mask = CI->getValue();
  if (Mask.isAllOnes() || Mask.isZero()) {
    return std::nullopt;
  }

  std::optional<unsigned> Start;
  unsigned End = 0;
  for (unsigned I = 0; I < Mask.getBitWidth(); ++I) {
    if (!Mask[I]) {
      if (!Start.has_value()) {
        Start = I;
      }
      End = I + 1;
    } else if (Start.has_value()) {
      for (unsigned J = I + 1; J < Mask.getBitWidth(); ++J) {
        if (!Mask[J]) {
          return std::nullopt;
        }
      }
      break;
    }
  }

  if (!Start.has_value() || End <= *Start) {
    return std::nullopt;
  }
  return PackedBitRange{.Offset = *Start, .Width = End - *Start};
}

llvm::Value *getBitwiseAndOtherOperand(llvm::Value *V, unsigned &MaskWidth) {
  auto getOther = [&](llvm::Value *LHS, llvm::Value *RHS) -> llvm::Value * {
    if (auto Width = getLowMaskBitWidth(LHS)) {
      MaskWidth = *Width;
      return RHS;
    }
    if (auto Width = getLowMaskBitWidth(RHS)) {
      MaskWidth = *Width;
      return LHS;
    }
    return nullptr;
  };

  if (auto *And = dyn_cast<llvm::BinaryOperator>(V)) {
    if (And->getOpcode() == llvm::Instruction::And) {
      return getOther(And->getOperand(0), And->getOperand(1));
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_and") &&
        Call->arg_size() == 2) {
      return getOther(Call->getArgOperand(0), Call->getArgOperand(1));
    }
  }
  return nullptr;
}

llvm::Value *getBitwiseAndOtherOperandForClearMask(llvm::Value *V,
                                                   PackedBitRange &Range) {
  auto getOther = [&](llvm::Value *LHS, llvm::Value *RHS) -> llvm::Value * {
    if (auto Match = getClearMaskBitRange(LHS)) {
      Range = *Match;
      return RHS;
    }
    if (auto Match = getClearMaskBitRange(RHS)) {
      Range = *Match;
      return LHS;
    }
    return nullptr;
  };

  if (auto *And = dyn_cast<llvm::BinaryOperator>(V)) {
    if (And->getOpcode() == llvm::Instruction::And) {
      return getOther(And->getOperand(0), And->getOperand(1));
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_and") &&
        Call->arg_size() == 2) {
      return getOther(Call->getArgOperand(0), Call->getArgOperand(1));
    }
  }
  return nullptr;
}

bool getBitwiseOrOperands(llvm::Value *V, llvm::Value *&LHS,
                          ExtValuePtr &LHSEvidence, llvm::Value *&RHS,
                          ExtValuePtr &RHSEvidence) {
  if (auto *Or = dyn_cast<llvm::BinaryOperator>(V)) {
    if (Or->getOpcode() == llvm::Instruction::Or) {
      LHS = Or->getOperand(0);
      LHSEvidence = getExtValuePtr(LHS, Or, 0);
      RHS = Or->getOperand(1);
      RHSEvidence = getExtValuePtr(RHS, Or, 1);
      return true;
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_or") &&
        Call->arg_size() == 2) {
      LHS = Call->getArgOperand(0);
      LHSEvidence = getExtValuePtr(LHS, Call, 0);
      RHS = Call->getArgOperand(1);
      RHSEvidence = getExtValuePtr(RHS, Call, 1);
      return true;
    }
  }
  return false;
}

bool binaryOpHasSameValueOperand(llvm::BinaryOperator *Op, llvm::Value *V) {
  return Op != nullptr &&
         (notdec::passes::evm::detail::isSameValue(Op->getOperand(0), V) ||
          notdec::passes::evm::detail::isSameValue(Op->getOperand(1), V));
}

bool isBytesLengthFullLen(llvm::Value *V, llvm::Value *Slot) {
  if (auto *Call = dyn_cast_or_null<llvm::CallBase>(V)) {
    if (Call->arg_size() != 2) {
      return false;
    }
    bool IsDivByTwo =
        notdec::passes::evm::detail::isCallTo(Call, "evm_div") &&
        notdec::passes::evm::detail::isSameValue(Call->getArgOperand(0), Slot) &&
        notdec::passes::evm::detail::isConstantIntValue(Call->getArgOperand(1),
                                                        2);
    bool IsShrByOne =
        notdec::passes::evm::detail::isCallTo(Call, "evm_shr") &&
        notdec::passes::evm::detail::isConstantIntValue(Call->getArgOperand(0),
                                                        1) &&
        notdec::passes::evm::detail::isSameValue(Call->getArgOperand(1), Slot);
    return IsDivByTwo || IsShrByOne;
  }

  if (auto *Shift = dyn_cast_or_null<llvm::BinaryOperator>(V)) {
    return Shift->getOpcode() == llvm::Instruction::LShr &&
           notdec::passes::evm::detail::isSameValue(Shift->getOperand(0),
                                                    Slot) &&
           notdec::passes::evm::detail::isConstantIntValue(Shift->getOperand(1),
                                                           1);
  }
  return false;
}

bool isBytesLengthShortLen(llvm::Value *V, llvm::Value *FullLen) {
  auto *ShortLen = dyn_cast_or_null<llvm::BinaryOperator>(V);
  return ShortLen != nullptr && ShortLen->getOpcode() == llvm::Instruction::And &&
         binaryOpHasSameValueOperand(ShortLen, FullLen) &&
         (notdec::passes::evm::detail::isConstantIntValue(
              ShortLen->getOperand(0), 127) ||
          notdec::passes::evm::detail::isConstantIntValue(
              ShortLen->getOperand(1), 127));
}

bool matchStorageBytesLengthValue(llvm::Value *Length, llvm::Value *Slot) {
  auto *Select = dyn_cast_or_null<llvm::SelectInst>(Length);
  if (Select == nullptr) {
    return false;
  }

  auto *ShortCond = dyn_cast<llvm::ICmpInst>(Select->getCondition());
  if (ShortCond == nullptr ||
      ShortCond->getPredicate() != llvm::ICmpInst::ICMP_EQ ||
      !notdec::passes::evm::detail::isConstantIntValue(
          ShortCond->getOperand(1), 0)) {
    return false;
  }

  auto *LowBit = dyn_cast<llvm::BinaryOperator>(ShortCond->getOperand(0));
  if (LowBit == nullptr || LowBit->getOpcode() != llvm::Instruction::And ||
      !binaryOpHasSameValueOperand(LowBit, Slot) ||
      !(notdec::passes::evm::detail::isConstantIntValue(LowBit->getOperand(0),
                                                        1) ||
        notdec::passes::evm::detail::isConstantIntValue(LowBit->getOperand(1),
                                                        1))) {
    return false;
  }

  llvm::Value *ShortLen = Select->getTrueValue();
  llvm::Value *FullLen = Select->getFalseValue();
  return isBytesLengthFullLen(FullLen, Slot) &&
         isBytesLengthShortLen(ShortLen, FullLen);
}

llvm::Value *getShiftedValue(llvm::Value *V, unsigned &Offset) {
  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_shr") &&
        Call->arg_size() == 2) {
      auto Shift = getUInt64Constant(Call->getArgOperand(0));
      if (Shift.has_value() && *Shift < 256) {
        Offset = static_cast<unsigned>(*Shift);
        return Call->getArgOperand(1);
      }
    }
  }

  if (auto *Shift = dyn_cast<llvm::BinaryOperator>(V)) {
    if (Shift->getOpcode() == llvm::Instruction::LShr) {
      auto Amount = getUInt64Constant(Shift->getOperand(1));
      if (Amount.has_value() && *Amount < 256) {
        Offset = static_cast<unsigned>(*Amount);
        return Shift->getOperand(0);
      }
    }
  }

  Offset = 0;
  return V;
}

ExtValuePtr getShiftedWriteValue(llvm::Value *V, const ExtValuePtr &Evidence,
                                 unsigned &Offset) {
  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_shl") &&
        Call->arg_size() == 2) {
      auto Shift = getUInt64Constant(Call->getArgOperand(0));
      if (Shift.has_value() && *Shift < 256) {
        Offset = static_cast<unsigned>(*Shift);
        return getExtValuePtr(Call->getArgOperand(1), Call, 1);
      }
    }
  }

  if (auto *Shift = dyn_cast<llvm::BinaryOperator>(V)) {
    if (Shift->getOpcode() == llvm::Instruction::Shl) {
      auto Amount = getUInt64Constant(Shift->getOperand(1));
      if (Amount.has_value() && *Amount < 256) {
        Offset = static_cast<unsigned>(*Amount);
        return getExtValuePtr(Shift->getOperand(0), Shift, 0);
      }
    }
  }

  Offset = 0;
  return Evidence;
}

std::optional<PackedStorageFieldReadMatch>
getPackedReadFromExtractedValue(llvm::CallBase &Load, llvm::Value *Extracted,
                                const StorageValueFieldPrefixMatch &FieldPrefix) {
  unsigned MaskWidth = 0;
  llvm::Value *Masked = getBitwiseAndOtherOperand(Extracted, MaskWidth);
  if (Masked == nullptr) {
    return std::nullopt;
  }

  unsigned Offset = 0;
  llvm::Value *Shifted = getShiftedValue(Masked, Offset);
  if (!notdec::passes::evm::detail::isSameValue(Shifted, &Load)) {
    return std::nullopt;
  }

  return PackedStorageFieldReadMatch{
      .FieldName = FieldPrefix.FieldName + ".packed@" +
                   std::to_string(Offset) + ":" + std::to_string(MaskWidth),
      .Value = Extracted,
      .KeyConstraints = FieldPrefix.KeyConstraints,
  };
}

std::optional<PackedStorageFieldReadMatch>
getPackedStorageFieldReadMatch(llvm::CallBase &Load) {
  auto FieldPrefix = getStorageValueFieldPrefixMatch(
      Load, Load.getArgOperand(0), /*AccessKind=*/1,
      /*UseField0ForMappingBase=*/true);
  if (!FieldPrefix.has_value()) {
    return std::nullopt;
  }

  for (llvm::User *User : Load.users()) {
    auto *DirectValue = dyn_cast<llvm::Value>(User);
    if (auto Match =
            getPackedReadFromExtractedValue(Load, DirectValue, *FieldPrefix)) {
      return Match;
    }

    for (llvm::User *NestedUser : User->users()) {
      auto *NestedValue = dyn_cast<llvm::Value>(NestedUser);
      if (auto Match =
              getPackedReadFromExtractedValue(Load, NestedValue, *FieldPrefix)) {
        return Match;
      }
    }
  }
  return std::nullopt;
}

std::optional<BytesStorageLengthMatch>
getBytesStorageLengthMatch(llvm::CallBase &Load) {
  auto FieldPrefix = getStorageValueFieldPrefixMatch(
      Load, Load.getArgOperand(0), /*AccessKind=*/1);

  if (!FieldPrefix.has_value()) {
    if (auto Array = getArrayStorageFieldMatch(Load, Load.getArgOperand(0),
                                               /*AccessKind=*/1)) {
      FieldPrefix = StorageValueFieldPrefixMatch{
          .FieldName = Array->Prefix + ".elem",
          .KeyConstraints = std::move(Array->KeyConstraints),
      };
      if (getStorageEvidenceValue(Array->Index) != nullptr) {
        FieldPrefix->KeyConstraints.push_back(MappingStorageKeyConstraint{
            .FieldName = Array->Prefix + ".index",
            .Key = Array->Index,
        });
      }
    } else if (auto StaticArray =
                   getStaticArrayStorageFieldMatch(Load.getArgOperand(0))) {
      FieldPrefix = StorageValueFieldPrefixMatch{
          .FieldName = StaticArray->Prefix + ".elem",
      };
      FieldPrefix->KeyConstraints.push_back(MappingStorageKeyConstraint{
          .FieldName = StaticArray->Prefix + ".index",
          .Key = StaticArray->Index,
      });
    }
  }

  if (!FieldPrefix.has_value()) {
    return std::nullopt;
  }

  llvm::SmallVector<llvm::Value *, 8> Worklist;
  llvm::SmallPtrSet<llvm::Value *, 16> Seen;
  for (llvm::User *User : Load.users()) {
    if (auto *V = dyn_cast<llvm::Value>(User)) {
      Worklist.push_back(V);
    }
  }

  unsigned Steps = 0;
  while (!Worklist.empty() && Steps++ < 32) {
    llvm::Value *V = Worklist.pop_back_val();
    if (V == nullptr || !Seen.insert(V).second) {
      continue;
    }
    if (matchStorageBytesLengthValue(V, &Load)) {
      return BytesStorageLengthMatch{
          .FieldName = FieldPrefix->FieldName + ".bytes.length",
          .Length = V,
          .KeyConstraints = std::move(FieldPrefix->KeyConstraints),
      };
    }
    for (llvm::User *User : V->users()) {
      if (auto *UserValue = dyn_cast<llvm::Value>(User)) {
        Worklist.push_back(UserValue);
      }
    }
  }
  return std::nullopt;
}

bool functionHasBytesLengthDecodeForBaseSlot(llvm::Function *F,
                                             llvm::Value *BaseSlot);

bool isBytesShortDataMask(llvm::Value *V) {
  auto *CI = dyn_cast_or_null<llvm::ConstantInt>(V);
  if (CI == nullptr) {
    return false;
  }

  const llvm::APInt &Mask = CI->getValue();
  if (Mask.getBitWidth() <= 8) {
    return false;
  }
  for (unsigned I = 0; I < Mask.getBitWidth(); ++I) {
    bool Expected = I >= 8;
    if (Mask[I] != Expected) {
      return false;
    }
  }
  return true;
}

bool matchStorageBytesShortDataValue(llvm::Value *Data, llvm::Value *Slot) {
  auto *And = dyn_cast_or_null<llvm::BinaryOperator>(Data);
  if (And == nullptr || And->getOpcode() != llvm::Instruction::And) {
    return false;
  }

  return (notdec::passes::evm::detail::isSameValue(And->getOperand(0), Slot) &&
          isBytesShortDataMask(And->getOperand(1))) ||
         (notdec::passes::evm::detail::isSameValue(And->getOperand(1), Slot) &&
          isBytesShortDataMask(And->getOperand(0)));
}

std::optional<BytesStorageShortDataMatch>
getBytesStorageShortDataMatch(llvm::CallBase &Load) {
  auto FieldPrefix = getStorageValueFieldPrefixMatch(
      Load, Load.getArgOperand(0), /*AccessKind=*/1);

  if (!FieldPrefix.has_value()) {
    if (auto Array = getArrayStorageFieldMatch(Load, Load.getArgOperand(0),
                                               /*AccessKind=*/1)) {
      FieldPrefix = StorageValueFieldPrefixMatch{
          .FieldName = Array->Prefix + ".elem",
          .KeyConstraints = std::move(Array->KeyConstraints),
      };
      if (getStorageEvidenceValue(Array->Index) != nullptr) {
        FieldPrefix->KeyConstraints.push_back(MappingStorageKeyConstraint{
            .FieldName = Array->Prefix + ".index",
            .Key = Array->Index,
        });
      }
    } else if (auto StaticArray =
                   getStaticArrayStorageFieldMatch(Load.getArgOperand(0))) {
      FieldPrefix = StorageValueFieldPrefixMatch{
          .FieldName = StaticArray->Prefix + ".elem",
      };
      FieldPrefix->KeyConstraints.push_back(MappingStorageKeyConstraint{
          .FieldName = StaticArray->Prefix + ".index",
          .Key = StaticArray->Index,
      });
    }
  }

  if (!FieldPrefix.has_value() ||
      !functionHasBytesLengthDecodeForBaseSlot(Load.getFunction(),
                                               Load.getArgOperand(0))) {
    return std::nullopt;
  }

  llvm::SmallVector<llvm::Value *, 8> Worklist;
  llvm::SmallPtrSet<llvm::Value *, 16> Seen;
  for (llvm::User *User : Load.users()) {
    if (auto *V = dyn_cast<llvm::Value>(User)) {
      Worklist.push_back(V);
    }
  }

  unsigned Steps = 0;
  while (!Worklist.empty() && Steps++ < 32) {
    llvm::Value *V = Worklist.pop_back_val();
    if (V == nullptr || !Seen.insert(V).second) {
      continue;
    }
    if (matchStorageBytesShortDataValue(V, &Load)) {
      return BytesStorageShortDataMatch{
          .FieldName = FieldPrefix->FieldName + ".bytes.short_data",
          .Data = V,
          .KeyConstraints = std::move(FieldPrefix->KeyConstraints),
      };
    }
    for (llvm::User *User : V->users()) {
      if (auto *UserValue = dyn_cast<llvm::Value>(User)) {
        Worklist.push_back(UserValue);
      }
    }
  }
  return std::nullopt;
}

bool functionHasBytesLengthDecodeForBaseSlot(llvm::Function *F,
                                             llvm::Value *BaseSlot) {
  if (F == nullptr) {
    return false;
  }

  for (llvm::BasicBlock &BB : *F) {
    for (llvm::Instruction &Inst : BB) {
      auto *Load = dyn_cast<llvm::CallBase>(&Inst);
      if (Load == nullptr ||
          !notdec::passes::evm::detail::isCallTo(Load, "evm_sload") ||
          Load->arg_size() != 1 ||
          !notdec::passes::evm::detail::isSameValue(Load->getArgOperand(0),
                                                    BaseSlot)) {
        continue;
      }

      llvm::SmallVector<llvm::Value *, 8> Worklist;
      llvm::SmallPtrSet<llvm::Value *, 16> Seen;
      for (llvm::User *User : Load->users()) {
        if (auto *V = dyn_cast<llvm::Value>(User)) {
          Worklist.push_back(V);
        }
      }

      unsigned Steps = 0;
      while (!Worklist.empty() && Steps++ < 32) {
        llvm::Value *V = Worklist.pop_back_val();
        if (V == nullptr || !Seen.insert(V).second) {
          continue;
        }
        if (matchStorageBytesLengthValue(V, Load)) {
          return true;
        }
        for (llvm::User *User : V->users()) {
          if (auto *UserValue = dyn_cast<llvm::Value>(User)) {
            Worklist.push_back(UserValue);
          }
        }
      }
    }
  }
  return false;
}

std::optional<BytesStorageLongElemMatch>
getBytesStorageLongElemMatch(llvm::CallBase &Access, llvm::Value *StorageSlot,
                             uint64_t AccessKind) {
  auto Match = notdec::passes::evm::detail::matchStorageArrayDataAccess(
      Access, StorageSlot, AccessKind);
  if (!Match.has_value() ||
      !functionHasBytesLengthDecodeForBaseSlot(Access.getFunction(),
                                               getStorageEvidenceValue(
                                                   Match->BaseSlot))) {
    return std::nullopt;
  }

  auto MatchBaseSlot = getStorageEvidenceValue(Match->BaseSlot);
  auto Base = getStorageFieldPrefixFromBaseSlot(MatchBaseSlot);
  auto BaseValue = getStorageValueFieldPrefixFromSlotExpression(
      MatchBaseSlot, /*UseField0ForMappingBase=*/false);
  if (!Base.has_value() && !BaseValue.has_value()) {
    return std::nullopt;
  }

  std::string Prefix;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
  if (BaseValue.has_value()) {
    Prefix = BaseValue->FieldName;
    KeyConstraints = std::move(BaseValue->KeyConstraints);
  } else {
    Prefix = Base->Prefix;
    KeyConstraints = std::move(Base->KeyConstraints);
  }

  return BytesStorageLongElemMatch{
      .Prefix = Prefix + ".bytes",
      .Index = getDynamicArrayIndex(StorageSlot, Match->DataHash),
      .KeyConstraints = std::move(KeyConstraints),
  };
}

llvm::CallBase *getSameSlotSLoad(llvm::Value *V, llvm::Value *StorageSlot) {
  auto *Load = dyn_cast_or_null<llvm::CallBase>(V);
  if (Load == nullptr ||
      !notdec::passes::evm::detail::isCallTo(Load, "evm_sload") ||
      Load->arg_size() != 1) {
    return nullptr;
  }
  if (!notdec::passes::evm::detail::isSameValue(Load->getArgOperand(0),
                                                StorageSlot)) {
    return nullptr;
  }
  return Load;
}

std::optional<PackedStorageFieldWriteMatch>
getPackedStorageFieldWriteMatch(llvm::Value *StorageSlot,
                                llvm::Value *StoredValue,
                                llvm::CallBase &Store) {
  auto FieldPrefix = getStorageValueFieldPrefixMatch(
      Store, StorageSlot, /*AccessKind=*/2,
      /*UseField0ForMappingBase=*/true);
  if (!FieldPrefix.has_value()) {
    return std::nullopt;
  }

  llvm::Value *OrLHS = nullptr;
  llvm::Value *OrRHS = nullptr;
  ExtValuePtr OrLHSEvidence = static_cast<llvm::Value *>(nullptr);
  ExtValuePtr OrRHSEvidence = static_cast<llvm::Value *>(nullptr);
  if (!getBitwiseOrOperands(StoredValue, OrLHS, OrLHSEvidence, OrRHS,
                            OrRHSEvidence)) {
    return std::nullopt;
  }

  auto tryMatch = [&](llvm::Value *ClearPart, llvm::Value *WritePart,
                      const ExtValuePtr &WritePartEvidence)
      -> std::optional<PackedStorageFieldWriteMatch> {
    PackedBitRange Range;
    llvm::Value *OldValue =
        getBitwiseAndOtherOperandForClearMask(ClearPart, Range);
    if (getSameSlotSLoad(OldValue, StorageSlot) == nullptr) {
      return std::nullopt;
    }

    unsigned Offset = 0;
    ExtValuePtr Value = getShiftedWriteValue(WritePart, WritePartEvidence,
                                             Offset);
    if (Offset != Range.Offset || Range.Width == 0) {
      return std::nullopt;
    }

    return PackedStorageFieldWriteMatch{
        .FieldName = FieldPrefix->FieldName + ".packed@" +
                     std::to_string(Range.Offset) + ":" +
                     std::to_string(Range.Width),
        .Value = Value,
        .KeyConstraints = FieldPrefix->KeyConstraints,
    };
  };

  if (auto Match = tryMatch(OrLHS, OrRHS, OrRHSEvidence)) {
    return Match;
  }
  return tryMatch(OrRHS, OrLHS, OrLHSEvidence);
}

bool isPackedWritePreservedWordLoad(llvm::CallBase &Load) {
  if (!notdec::passes::evm::detail::isCallTo(&Load, "evm_sload") ||
      Load.arg_size() != 1) {
    return false;
  }

  for (llvm::User *AndUser : Load.users()) {
    PackedBitRange Range;
    if (getBitwiseAndOtherOperandForClearMask(dyn_cast<llvm::Value>(AndUser),
                                              Range) == nullptr) {
      continue;
    }
    for (llvm::User *OrUser : AndUser->users()) {
      auto *StoredValue = dyn_cast<llvm::Value>(OrUser);
      if (StoredValue == nullptr) {
        continue;
      }
      for (llvm::User *StoreUser : OrUser->users()) {
        auto *Store = dyn_cast<llvm::CallBase>(StoreUser);
        if (Store == nullptr ||
            !notdec::passes::evm::detail::isCallTo(Store, "evm_sstore") ||
            Store->arg_size() != 2 ||
            !notdec::passes::evm::detail::isSameValue(Store->getArgOperand(0),
                                                      Load.getArgOperand(0)) ||
            !notdec::passes::evm::detail::isSameValue(Store->getArgOperand(1),
                                                      StoredValue)) {
          continue;
        }
        if (getPackedStorageFieldWriteMatch(Store->getArgOperand(0),
                                            Store->getArgOperand(1), *Store)) {
          return true;
        }
      }
    }
  }
  return false;
}

void ConstraintsGenerator::MLsubVisitor::addEVMRuntimeSemanticConstraints(
    llvm::CallBase &I) {
  auto *F = I.getCalledFunction();
  if (F == nullptr) {
    return;
  }

  auto markArg = [&](unsigned Index, llvm::StringRef SemanticName) {
    if (Index < I.arg_size() &&
        I.getArgOperand(Index)->getType()->isIntegerTy(256)) {
      addEVMSemanticConstraint(
          cg, getExtValuePtr(I.getArgOperand(Index), &I, Index), SemanticName);
    }
  };
  auto markRet = [&](llvm::StringRef SemanticName) {
    if (!I.getType()->isVoidTy() && I.getType()->isIntegerTy(256)) {
      addEVMSemanticConstraint(cg, &I, SemanticName);
    }
  };

  StringRef Name = F->getName();
  if (Name == "evm_address" || Name == "evm_caller" ||
      Name == "evm_origin" || Name == "evm_coinbase") {
    markRet("address");
    return;
  }
  if (Name == "evm_create" || Name == "evm_create2") {
    markRet("address");
    markArg(0, "integer");
    markArg(2, "integer");
    if (Name == "evm_create2") {
      markArg(3, "integer");
    }
    return;
  }
  if (Name == "evm_balance" || Name == "evm_extcodesize" ||
      Name == "evm_extcodehash") {
    markArg(0, "address");
    markRet("integer");
    return;
  }
  if (Name == "evm_extcodecopy") {
    markArg(1, "address");
    markArg(3, "integer");
    markArg(4, "integer");
    return;
  }
  if (Name == "evm_sload") {
    markArg(0, "storage_key");
    if (I.arg_size() == 1 && !I.getType()->isVoidTy()) {
      if (auto BytesLength = getBytesStorageLengthMatch(I)) {
        addStorageKeyConstraints(cg, BytesLength->KeyConstraints);

        auto FieldTy = cg.getOrCreateStorageField(BytesLength->FieldName, 256);
        if (auto LengthTy = getStorageEvidenceType(cg, BytesLength->Length)) {
          cg.addSubtype(FieldTy, *LengthTy);
        }

        if (auto ShortData = getBytesStorageShortDataMatch(I)) {
          addStorageKeyConstraints(cg, ShortData->KeyConstraints);

          auto ShortDataFieldTy =
              cg.getOrCreateStorageField(ShortData->FieldName, 256);
          if (auto DataTy = getStorageEvidenceType(cg, ShortData->Data)) {
            cg.addSubtype(ShortDataFieldTy, *DataTy);
          }
        }
      } else if (auto ArrayLength = getArrayLengthStorageFieldMatch(I)) {
        addStorageKeyConstraints(cg, ArrayLength->KeyConstraints);

        auto FieldTy =
            cg.getOrCreateStorageField(ArrayLength->Prefix + ".length");
        auto ResultTy = cg.getOrInsertNode(&I);
        cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
      } else if (isPackedWritePreservedWordLoad(I)) {
        return;
      } else if (auto BytesLong =
                     getBytesStorageLongElemMatch(I, I.getArgOperand(0),
                                                  /*AccessKind=*/1)) {
        addStorageKeyConstraints(cg, BytesLong->KeyConstraints);
        if (getStorageEvidenceValue(BytesLong->Index) != nullptr) {
          addStorageValueEvidenceConstraint(
              cg, BytesLong->Prefix + ".long_index", BytesLong->Index);
        }

        auto FieldTy =
            cg.getOrCreateStorageField(BytesLong->Prefix + ".long_elem");
        auto ResultTy = cg.getOrInsertNode(&I);
        cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
      } else if (auto Mapping =
                     getMappingStorageFieldMatch(I, I.getArgOperand(0),
                                                 /*AccessKind=*/1)) {
        if (auto Packed = getPackedStorageFieldReadMatch(I)) {
          addStorageKeyConstraints(cg, Packed->KeyConstraints);

          auto FieldTy = cg.getOrCreateStorageField(Packed->FieldName, 256);
          if (auto ValueTy = getStorageEvidenceType(cg, Packed->Value)) {
            cg.addSubtype(FieldTy, *ValueTy);
          }
        } else {
          addStorageKeyConstraints(cg, Mapping->KeyConstraints);

          auto FieldTy = cg.getOrCreateStorageField(Mapping->ValueFieldName);
          auto ResultTy = cg.getOrInsertNode(&I);
          cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
        }
      } else if (auto Array =
                     getArrayStorageFieldMatch(I, I.getArgOperand(0),
                                               /*AccessKind=*/1)) {
        addStorageKeyConstraints(cg, Array->KeyConstraints);
        if (getStorageEvidenceValue(Array->Index) != nullptr) {
          addStorageValueEvidenceConstraint(cg, Array->Prefix + ".index",
                                            Array->Index);
        }

        auto FieldTy = cg.getOrCreateStorageField(Array->Prefix + ".elem");
        auto ResultTy = cg.getOrInsertNode(&I);
        cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
      } else if (auto StaticArray =
                     getStaticArrayStorageFieldMatch(I.getArgOperand(0))) {
        addStorageValueEvidenceConstraint(cg, StaticArray->Prefix + ".index",
                                          StaticArray->Index);

        auto FieldTy =
            cg.getOrCreateStorageField(StaticArray->Prefix + ".elem");
        auto ResultTy = cg.getOrInsertNode(&I);
        cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
      } else if (auto Packed = getPackedStorageFieldReadMatch(I)) {
        auto FieldTy = cg.getOrCreateStorageField(Packed->FieldName, 256);
        if (auto ValueTy = getStorageEvidenceType(cg, Packed->Value)) {
          cg.addSubtype(FieldTy, *ValueTy);
        }
      } else if (auto FieldName =
                     getDirectStorageSlotFieldName(I.getArgOperand(0))) {
        auto FieldTy = cg.getOrCreateStorageField(*FieldName);
        auto ResultTy = cg.getOrInsertNode(&I);
        cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
      }
    }
    return;
  }
  if (Name == "evm_sstore") {
    markArg(0, "storage_key");
    if (I.arg_size() == 2) {
      if (auto Mapping = getMappingStorageFieldMatch(I, I.getArgOperand(0),
                                                     /*AccessKind=*/2)) {
        if (auto Packed = getPackedStorageFieldWriteMatch(
                I.getArgOperand(0), I.getArgOperand(1), I)) {
          addStorageKeyConstraints(cg, Packed->KeyConstraints);

          auto FieldTy = cg.getOrCreateStorageField(Packed->FieldName, 256);
          if (auto ValueTy = getStorageEvidenceType(cg, Packed->Value)) {
            cg.addSubtype(*ValueTy, FieldTy);
          }
        } else {
          addStorageKeyConstraints(cg, Mapping->KeyConstraints);

          auto FieldTy = cg.getOrCreateStorageField(Mapping->ValueFieldName);
          auto ValueTy =
              cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(1), &I, 1));
          cg.addSubtype(FieldTy, binarysub::make_ptr_store(ValueTy, 256));
        }
      } else if (auto BytesLong =
                     getBytesStorageLongElemMatch(I, I.getArgOperand(0),
                                                  /*AccessKind=*/2)) {
        addStorageKeyConstraints(cg, BytesLong->KeyConstraints);
        if (getStorageEvidenceValue(BytesLong->Index) != nullptr) {
          addStorageValueEvidenceConstraint(
              cg, BytesLong->Prefix + ".long_index", BytesLong->Index);
        }

        auto FieldTy =
            cg.getOrCreateStorageField(BytesLong->Prefix + ".long_elem");
        auto ValueTy =
            cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(1), &I, 1));
        cg.addSubtype(FieldTy, binarysub::make_ptr_store(ValueTy, 256));
      } else if (auto Array =
                     getArrayStorageFieldMatch(I, I.getArgOperand(0),
                                               /*AccessKind=*/2)) {
        addStorageKeyConstraints(cg, Array->KeyConstraints);
        if (getStorageEvidenceValue(Array->Index) != nullptr) {
          addStorageValueEvidenceConstraint(cg, Array->Prefix + ".index",
                                            Array->Index);
        }

        auto FieldTy = cg.getOrCreateStorageField(Array->Prefix + ".elem");
        auto ValueTy =
            cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(1), &I, 1));
        cg.addSubtype(FieldTy, binarysub::make_ptr_store(ValueTy, 256));
      } else if (auto StaticArray =
                     getStaticArrayStorageFieldMatch(I.getArgOperand(0))) {
        addStorageValueEvidenceConstraint(cg, StaticArray->Prefix + ".index",
                                          StaticArray->Index);

        auto FieldTy =
            cg.getOrCreateStorageField(StaticArray->Prefix + ".elem");
        auto ValueTy =
            cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(1), &I, 1));
        cg.addSubtype(FieldTy, binarysub::make_ptr_store(ValueTy, 256));
      } else if (auto Packed = getPackedStorageFieldWriteMatch(
                     I.getArgOperand(0), I.getArgOperand(1), I)) {
        addStorageKeyConstraints(cg, Packed->KeyConstraints);

        auto FieldTy = cg.getOrCreateStorageField(Packed->FieldName, 256);
        if (auto ValueTy = getStorageEvidenceType(cg, Packed->Value)) {
          cg.addSubtype(*ValueTy, FieldTy);
        }
      } else if (auto FieldName =
                     getDirectStorageSlotFieldName(I.getArgOperand(0))) {
        auto FieldTy = cg.getOrCreateStorageField(*FieldName);
        auto ValueTy =
            cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(1), &I, 1));
        cg.addSubtype(FieldTy, binarysub::make_ptr_store(ValueTy, 256));
      }
    }
    return;
  }
  if (Name == "evm_call" || Name == "evm_callcode") {
    markArg(3, "integer");
    markArg(4, "address");
    markArg(5, "integer");
    markArg(7, "integer");
    markArg(9, "integer");
    markRet("integer");
    return;
  }
  if (Name == "evm_delegatecall" || Name == "evm_staticcall") {
    markArg(3, "integer");
    markArg(4, "address");
    markArg(6, "integer");
    markArg(8, "integer");
    markRet("integer");
    return;
  }
}

bool ConstraintsGenerator::MLsubVisitor::handleIntrinsicCall(
    llvm::CallBase &I) {
  auto Target = I.getCalledFunction();
  if (!Target->isIntrinsic()) {
    return false;
  }
  auto ID = Target->getIntrinsicID();
  switch (ID) {
  case Intrinsic::memset:
  case Intrinsic::memcpy:
  case Intrinsic::memmove:
    return true;
  default:
    break;
  }
  if (ID == Intrinsic::assume) {
    return true;
  }
  if (isMinMaxIntrinsic(ID)) {
    assert(I.arg_size() == 2 && "min/max intrinsic must have two operands");
    auto RetVar = cg.getOrInsertNode(&I);
    auto Src1 = getExtValuePtr(I.getArgOperand(0), &I, 0);
    auto Src2 = getExtValuePtr(I.getArgOperand(1), &I, 1);
    auto Src1Var = cg.getOrInsertNode(Src1);
    auto Src2Var = cg.getOrInsertNode(Src2);
    // LLVM 22 folds integer icmp+select into min/max intrinsics. Keep the
    // old frozen-IR behavior by modeling them as a select-like join.
    cg.setNonPointer(Src1);
    cg.setNonPointer(Src2);
    cg.setNonPointer(&I);
    cg.addSubtype(Src1Var, RetVar);
    cg.addSubtype(Src2Var, RetVar);
    return true;
  }
  // auto ID = Target->getIntrinsicID();
  if (I.getType()->isAggregateType()) {
    // ignore this call and handle the value in visitExtractValue
    return true;
  }
  return false;
}

void ConstraintsGenerator::MLsubVisitor::visitCallBase(CallBase &I) {
  auto Target = I.getCalledFunction();
  if (Target) {
    if (Target->getName().starts_with("llvm.dbg") ||
        Target->getName().starts_with("llvm.lifetime")) {
      return;
    }
  }

  if (Target == nullptr) {
    // TODO indirect call
    std::cerr
        << __FILE__ << ":" << __LINE__ << ": "
        << "Warn: ConstraintsGenerator: indirect call not supported yet\n";
    return;
  }

  // TODO if is allocation function, treat as alloca inst
  if (isHeapAllocationCall(I)) {
    cg.createNode(&I);
    // set as pointer type
    cg.setPointer(&I);
    cg.ContraVariantValues.insert(&I);
  } else if (handleEVMMarkerCall(I)) {
    return;
  } else if (shouldIgnoreRuntimeCall(I)) {
    addEVMRuntimeSemanticConstraints(I);
    return;
  } else if (handleIntrinsicCall(I)) {
    return;
  } else {
    // Call within the SCC:
    auto Func = Target;
    // metadata 实参只出现在带 metadata 的 intrinsic（如
    // llvm.experimental.noalias.scope.decl）里；这类调用没有可推理的
    // 类型信息，直接跳过，避免 convertSimpleTypeVal 对 metadata 求
    // size=0 崩溃。
    for (unsigned i = 0; i < I.arg_size(); ++i) {
      if (I.getArgOperand(i)->getType()->isMetadataTy()) {
        return;
      }
    }
    std::vector<SimpleType> Args;
    for (unsigned i = 0; i < I.arg_size(); ++i) {
      auto ValVar = cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(i), &I, i));
      Args.push_back(ValVar);
    }
    SimpleType Ret = nullptr;
    if (!I.getType()->isVoidTy()) {
      if (I.getType()->isAggregateType()) {
        Ret = cg.makeCallAggregateReturnRecord(I);
      } else {
        Ret = cg.getOrInsertNode(&I);
      }
    }
    // Import semantics from the JSON policy: "ret_alias_of_arg": {"memcpy": 0}
    // declares that the returned value is the same object as argument k.  Two
    // same-level variables connected in both directions are merged by the
    // solver, which reproduces what a lifted body encodes by returning the
    // result of the import call that received argument k.  Declarations have no
    // body, so without this the callee's parameter and result never join and it
    // cannot act as a merge hub for pointer arguments.
    // "pointer_args": the annotated positions are pointers to caller-owned
    // objects; an empty record bound is this codebase's representation of
    // "pointer to an object with no known fields yet" (see
    // makeAggregateReturnFieldSeed), and it satisfies hasPointerLikeEvidence.
    for (unsigned int PointerArg : lookupPointerArgs(*Target)) {
      if (PointerArg < Args.size() && Args[PointerArg] != nullptr) {
        cg.addSubtype(Args[PointerArg], binarysub::make_record({}));
      }
    }
    if (Ret != nullptr && lookupPointerRet(*Target)) {
      cg.addSubtype(Ret, binarysub::make_record({}));
    }
    // Copy semantics: for memcpy/strcpy/... the destination and the source have
    // the same pointee type; two same-level variables connected in both
    // directions are merged by the solver.  This is the piece the lifted stub
    // body used to provide through the recognized llvm.memcpy intrinsic.
    for (auto [LHS, RHS] : lookupArgAliases(*Target)) {
      if (LHS < Args.size() && RHS < Args.size() && Args[LHS] != nullptr &&
          Args[RHS] != nullptr) {
        cg.addSubtype(Args[LHS], Args[RHS]);
        cg.addSubtype(Args[RHS], Args[LHS]);
      }
    }
    if (Ret != nullptr) {
      if (auto AliasArg = lookupRetAliasOfArg(*Target)) {
        if (*AliasArg < Args.size() && Args[*AliasArg] != nullptr) {
          cg.addSubtype(Args[*AliasArg], Ret);
          // Bidirectional by default (the solver merges the two variables);
          // NOTDEC_IMPORT_SEMANTICS=ret1 keeps it one-way, so the returned value
          // inherits the destination's shape without dragging the caller's
          // object into the destination's class.
          static const bool OneWay = [] {
            const char *V = std::getenv("NOTDEC_IMPORT_SEMANTICS");
            return V != nullptr && llvm::StringRef(V) == "ret1";
          }();
          if (!OneWay) {
            cg.addSubtype(Ret, Args[*AliasArg]);
          }
        }
      }
    }
    auto ActualFunc = binarysub::make_function(Args, Ret);
    if (cg.SCCs.count(Target)) {
      auto F = cg.getNodeOrNull(Func);
      std::vector<SimpleType> FormalArgs;
      FormalArgs.reserve(Func->arg_size());
      for (auto &Arg : Func->args()) {
        FormalArgs.push_back(cg.getNodeOrNull(&Arg));
      }
      SimpleType FormalRet = nullptr;
      if (Ret != nullptr && !Func->getReturnType()->isAggregateType()) {
        FormalRet = cg.getNodeOrNull(ReturnValue{.Func = Func});
      }
      auto FormalFunc = binarysub::make_function(FormalArgs, FormalRet);
      cg.deferCallConstraint(I, *Func, F, ActualFunc, FormalFunc);
    } else {
      // create and save to CallToInstance map. instance with summary later
      auto It = cg.unhandledCalls.insert({&I, ActualFunc});
      assert(It.second && "Insert unhandledCalls failed!");
    }
  }
}

void ConstraintsGenerator::MLsubVisitor::visitReturnInst(ReturnInst &I) {
  auto *SrcVal = I.getReturnValue();
  if (SrcVal == nullptr) { // ret void.
    return;
  }
  if (SrcVal->getType()->isAggregateType()) {
    cg.addAggregateReturnConstraints(SrcVal, I);
    return;
  }
  auto Src = cg.getOrInsertNode(getExtValuePtr(SrcVal, &I, 0));
  auto Dst = cg.getNodeOrNull(ReturnValue{.Func = I.getFunction()});
  // src is a subtype of dest
  cg.addSubtype(Src, Dst);
  // Polymorphic wrapper: the marker recorded "ret == arg i" (!notdec.mlsub
  // .local_alias) for a function that returns the object it was handed.  Tie
  // the returned value to that argument HERE, inside the wrapper, so the
  // relation holds per instance instead of leaking through the callee's shared
  // formal.
  if (auto *MD = I.getFunction()->getMetadata("notdec.mlsub.local_alias")) {
    if (MD->getNumOperands() == 1) {
      if (auto *Str = llvm::dyn_cast<llvm::MDString>(MD->getOperand(0))) {
        llvm::StringRef Spec = Str->getString();
        unsigned ArgNo = 0;
        if (!Spec.consume_front("ret=arg") ||
            Spec.getAsInteger(10, ArgNo) ||
            ArgNo >= I.getFunction()->arg_size()) {
          // Malformed or out of range: ignore, the annotation is advisory.
        } else {
          auto *ArgVal = I.getFunction()->getArg(ArgNo);
          auto ArgNode = cg.getNodeOrNull(getExtValuePtr(ArgVal, &I, 0));
          if (ArgNode != nullptr) {
            cg.addSubtype(Src, ArgNode);
            cg.addSubtype(ArgNode, Src);
          }
        }
      }
    }
  }
  cg.ReturnValueMergeCandidates.push_back(
      ConstraintsGenerator::ReturnValueMergeCandidate{
          .Return = &I, .Operand = Src, .FunctionReturn = Dst});
}

void ConstraintsGenerator::MLsubVisitor::visitPHINode(PHINode &I) {
  // A predecessor block may appear before the PHI block in the function's
  // block list. In that case visiting the predecessor has already created
  // the PHI node through an operand lookup, so reuse it here.
  cg.getOrInsertNode(&I);
  // Defer constraints generation (and unification) to handlePHINodes
  phiNodes.push_back(&I);
}

void ConstraintsGenerator::MLsubVisitor::handlePHINodes() {
  for (auto I : phiNodes) {
    auto P = cg.getNodeOrNull(I);
    for (long i = 0; i < I->getNumIncomingValues(); i++) {
      auto *Src = I->getIncomingValue(i);
      auto SrcVar = cg.getOrInsertNode(getExtValuePtr(Src, I, i));
      cg.addSubtype(SrcVar, P);
    }
  }
}

unsigned ConstraintsGenerator::getLLVMTypeSize(Type *ElemTy) {
  if (ElemTy->isPointerTy()) {
    assert(PointerSize != 0);
    return PointerSize;
  }
  const auto Size = ElemTy->getPrimitiveSizeInBits();
  assert(Size != 0);
  return Size;
}

void ConstraintsGenerator::MLsubVisitor::visitLoadInst(LoadInst &I) {
  // A lane split out of a wide access by the Scalarizer is not a source-level
  // member access.  Keep the loaded value typeable for its users, but do not
  // create pointer/pointee evidence for the lane.
  if (I.getMetadata(notdec::mlsub::KIND_SCALARIZER_LANE) != nullptr) {
    cg.getOrInsertNode(&I);
    return;
  }
  // if this is access to table, then we ignore the type, and return func ptr.
  auto Node = cg.getNodeOrNull(getExtValuePtr(I.getPointerOperand(), &I, 0));
  if (!Node) {
    if (lowerConstantExprGEPAddress(cg, I.getPointerOperand(), I, 0,
                                    I.getModule()->getDataLayout())) {
      Node = cg.getNodeOrNull(getExtValuePtr(I.getPointerOperand(), &I, 0));
    }
  }
  if (!Node) {
    if (auto CE = dyn_cast<ConstantExpr>(I.getPointerOperand())) {
      if (CE->getOpcode() == Instruction::BitCast) {
        if (auto CE2 = dyn_cast<ConstantExpr>(CE->getOperand(0))) {
          CE = CE2;
        }
      }
      if (CE->getOpcode() == Instruction::GetElementPtr) {
        return;
      }
    }
  }

  auto PtrVal = cg.getOrInsertNode(getExtValuePtr(I.getPointerOperand(), &I, 0));
  auto RetVal = cg.getOrInsertNode(&I);
  auto BitSize = cg.getLLVMTypeSize(I.getType());
  auto Addr = getExtValuePtr(I.getPointerOperand(), &I, 0);

  // A canary slot is frame padding; do not turn its load/store into a field.
  const bool CanaryAccess = isStackCanarySlotValue(Addr);
  if (!CanaryAccess) {
    cg.addConstantMemoryField(Addr);
    ConstraintsGenerator::BoundOriginScope LoadOrigin(
        cg, "load:" + toStableString(Addr) + "[" + std::to_string(BitSize) +
                "]");
    cg.addSubtype(PtrVal,
                  ptrAccessCapability(RetVal, BitSize, /*Store=*/false));
  }
}

void ConstraintsGenerator::MLsubVisitor::visitStoreInst(StoreInst &I) {
  // See visitLoadInst: a scalarizer lane must not define struct members.  The
  // stored value keeps its own node so its producers stay typed.
  if (I.getMetadata(notdec::mlsub::KIND_SCALARIZER_LANE) != nullptr) {
    cg.getOrInsertNode(getExtValuePtr(I.getValueOperand(), &I, 0));
    return;
  }
  // if this is access to table, then we ignore the type, and return func ptr.
  auto Node = cg.getNodeOrNull(getExtValuePtr(I.getPointerOperand(), &I, 1));
  if (!Node) {
    if (lowerConstantExprGEPAddress(cg, I.getPointerOperand(), I, 1,
                                    I.getModule()->getDataLayout())) {
      Node = cg.getNodeOrNull(getExtValuePtr(I.getPointerOperand(), &I, 1));
    }
  }
  if (!Node) {
    if (auto CE = dyn_cast<ConstantExpr>(I.getPointerOperand())) {
      if (CE->getOpcode() == Instruction::BitCast) {
        if (auto CE2 = dyn_cast<ConstantExpr>(CE->getOperand(0))) {
          CE = CE2;
        }
      }
      if (CE->getOpcode() == Instruction::GetElementPtr) {
        return;
      }
    }
  }

  auto PtrVal = cg.getOrInsertNode(getExtValuePtr(I.getPointerOperand(), &I, 1));
  auto BitSize = cg.getLLVMTypeSize(I.getValueOperand()->getType());
  auto StoreVal = cg.getOrInsertNode(getExtValuePtr(I.getValueOperand(), &I, 0));
  auto Addr = getExtValuePtr(I.getPointerOperand(), &I, 1);

  // Stack-canary stores write ABI padding into the frame, not an object member.
  // Skip both the constant-address field and the pointer store capability so
  // the slot cannot reappear as a field after object merge.
  const bool CanaryAccess = isStackCanarySlotValue(Addr) ||
                            isOpaqueSegmentReadValue(I.getValueOperand());
  if (!CanaryAccess) {
    cg.recordStore(Addr, StoreVal, BitSize, &I);
    cg.addConstantMemoryField(Addr);
    ConstraintsGenerator::BoundOriginScope StoreOrigin(
        cg, "store:" + toStableString(Addr) + "[" + std::to_string(BitSize) +
                "]");
    cg.addSubtype(PtrVal,
                  ptrAccessCapability(StoreVal, BitSize, /*Store=*/true));
  }
}

void ConstraintsGenerator::MLsubVisitor::visitAllocaInst(AllocaInst &I) {
  cg.createNode(&I);
  // set as pointer type
  cg.setPointer(&I);
  cg.ContraVariantValues.insert(&I);
}

void ConstraintsGenerator::MLsubVisitor::visitGetElementPtrInst(
    GetElementPtrInst &Gep) {
  // A lane address (base + element) created by the Scalarizer must not become a
  // field of the base pointer's pointee: the base is a byte pointer used by a
  // copy, not a struct pointer.  Keep the address value itself typeable.
  if (Gep.getMetadata(notdec::mlsub::KIND_SCALARIZER_LANE) != nullptr) {
    cg.getOrInsertNode(&Gep);
    return;
  }
  // supress warnings for table gep
  if (Gep.getPointerOperand()->getName().starts_with("table_")) {
    return;
  }

  if (lowerGEPOperatorAsPtrAdd(cg, cast<GEPOperator>(Gep), Gep,
                               Gep.getModule()->getDataLayout(), &Gep)) {
    return;
  }

  std::cerr << "Warning: MLsubVisitor::visitGetElementPtrInst: "
               "skip unsupported GEP offset lowering: "
            << toStableString(&Gep) << "\n";
}

void ConstraintsGenerator::addCmpConstraint(const ExtValuePtr LHS,
                                            const ExtValuePtr RHS,
                                            llvm::ICmpInst *I) {
  auto Left = LHS;
  auto Right = RHS;
  llvmValue2ExtVal(Left, I, 0);
  llvmValue2ExtVal(Right, I, 1);
  getOrInsertNode(Left);
  getOrInsertNode(Right);
  getPNINode(Left).unify(getPNINode(Right));
}

void ConstraintsGenerator::addAddConstraint(ExtValuePtr LHS, ExtValuePtr RHS,
                                            llvm::BinaryOperator *I) {
  llvmValue2ExtVal(LHS, I, 0);
  llvmValue2ExtVal(RHS, I, 1);
  auto Left = &getOrInsertPNINode(LHS);
  auto Right = &getOrInsertPNINode(RHS);
  getOrInsertPNINode(I);
  // addAddCons 的求解规则表只覆盖 PN related 两侧（i/I/p/P）；一侧
  // NotPN 时规则无法应用，跳过约束（与断言及 solve 前提保持一致）。
  if (Left->isPNRelated() && Right->isPNRelated()) {
    PG.addAddCons(getPNIValue(LHS), getPNIValue(RHS), getPNIValue(I), I);
  }
}
void ConstraintsGenerator::addSubConstraint(ExtValuePtr LHS, ExtValuePtr RHS,
                                            llvm::BinaryOperator *I) {
  llvmValue2ExtVal(LHS, I, 0);
  llvmValue2ExtVal(RHS, I, 1);
  auto Left = &getOrInsertPNINode(LHS);
  auto Right = &getOrInsertPNINode(RHS);
  getOrInsertPNINode(I);
  if (Left->isPNRelated() && Right->isPNRelated()) {
    PG.addSubCons(getPNIValue(LHS), getPNIValue(RHS), getPNIValue(I), I);
  }
}

void ConstraintsGenerator::MLsubVisitor::visitICmpInst(ICmpInst &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addCmpConstraint(Src1, Src2, &I);

  // type the inst as bool?
  assert(I.getType()->isIntegerTy(1));
  cg.createNode(&I);
}

void ConstraintsGenerator::MLsubVisitor::visitSelectInst(SelectInst &I) {
  auto DstVar = cg.getOrInsertNode(&I);
  auto *Src1 = I.getTrueValue();
  auto *Src2 = I.getFalseValue();
  auto Src1Var = cg.getOrInsertNode(getExtValuePtr(Src1, &I, 1));
  auto Src2Var = cg.getOrInsertNode(getExtValuePtr(Src2, &I, 2));
  // Not generate boolean constraints. Because it must be i1.
  cg.addSubtype(Src1Var, DstVar);
  cg.addSubtype(Src2Var, DstVar);
}

void ConstraintsGenerator::MLsubVisitor::visitAdd(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addAddConstraint(Src1, Src2, &I);
}

void ConstraintsGenerator::MLsubVisitor::visitSub(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addSubConstraint(Src1, Src2, &I);
}

void ConstraintsGenerator::MLsubVisitor::visitAnd(BinaryOperator &I) {
  // llvm::errs() << "visiting " << __FUNCTION__ << " \n";
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  unsigned Src1Ind = 0;
  unsigned Src2Ind = 1;
  ensureSequence(Src1, Src2, Src1Ind, Src2Ind);

  auto Src1Node = cg.getOrInsertNode(getExtValuePtr(Src1, &I, Src1Ind));
  auto Src2Node = cg.getOrInsertNode(getExtValuePtr(Src2, &I, Src2Ind));
  auto RetNode = cg.getOrInsertNode(&I);

  if (auto CI = dyn_cast<ConstantInt>(Src2)) {
    // at least most of the bits are passed, View as pointer alignment.
    if (constantHasAllPointerTagMiddleBits(*CI)) {
      // act as simple assignment
      cg.addSubtype(RetNode, Src1Node);
      return;
    }
  } else {
    // llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
    //              << "Warn: And op without constant: " << I << "\n";
  }
  cg.setNonPointer(getExtValuePtr(Src1, &I, Src1Ind));
  cg.setNonPointer(getExtValuePtr(Src2, &I, Src2Ind));
  cg.setNonPointer(&I);
  return;
}

void ConstraintsGenerator::MLsubVisitor::visitOr(BinaryOperator &I) {
  // llvm::errs() << "Visiting " << __FUNCTION__ << " \n";
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  unsigned Src1Ind = 0;
  unsigned Src2Ind = 1;
  ensureSequence(Src1, Src2, Src1Ind, Src2Ind);

  auto Src1Node = cg.getOrInsertNode(getExtValuePtr(Src1, &I, Src1Ind));
  auto Src2Node = cg.getOrInsertNode(getExtValuePtr(Src2, &I, Src2Ind));
  auto RetNode = cg.getOrInsertNode(&I);

  if (auto CI = dyn_cast<ConstantInt>(Src2)) {
    // at least most of the bits are passed, View as pointer alignment.
    if (constantHasNoPointerTagMiddleBits(*CI)) {
      // act as simple assignment
      cg.addSubtype(RetNode, Src1Node);
      return;
    }
  } else {
    // llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
    //              << "Warn: Or op without constant: " << I << "\n";
  }
  // view as numeric operation?
  cg.setNonPointer(getExtValuePtr(Src1, &I, Src1Ind));
  cg.setNonPointer(getExtValuePtr(Src2, &I, Src2Ind));
  cg.setNonPointer(&I);
  return;
}

void ConstraintsGenerator::MLsubVisitor::visitInstruction(Instruction &I) {
  bool UnhandledReturn = false;
  std::vector<unsigned> UnhandledOperands;

  // return value
  if (I.getType()->isVoidTy()) {
    // skip void type
  } else if (ConstraintsGenerator::opTypes.count(I.getOpcode()) &&
             ConstraintsGenerator::opTypes.at(I.getOpcode())
                 .addRetConstraint(&I, cg)) {
    // good
  } else {
    UnhandledReturn = true;
  }
  // for each op
  for (unsigned Ind = 0; Ind < I.getNumOperands(); ++Ind) {
    auto Op = I.getOperand(Ind);
    if (Op->getType()->isVoidTy()) {
      // skip void type
    } else if (ConstraintsGenerator::opTypes.count(I.getOpcode()) &&
               ConstraintsGenerator::opTypes.at(I.getOpcode())
                   .addOpConstraint(Ind, &I, cg)) {
      // good
    } else {
      UnhandledOperands.push_back(Ind);
    }
  }

  if (!UnhandledReturn && UnhandledOperands.empty()) {
    return;
  }

  // Printing an Instruction constructs an LLVM SlotTracker for the whole
  // module. On large DebugInfo inputs that made warnings cost more than the
  // constraint work itself, and the old operand loop printed the same
  // instruction repeatedly. Keep one compact, stable warning per instruction.
  llvm::errs() << "WARN: MLsubGenerator: unhandled instruction function="
               << I.getFunction()->getName() << " opcode=" << I.getOpcodeName();
  if (I.hasName()) {
    llvm::errs() << " value=" << I.getName();
  }
  llvm::errs() << " parts=";
  bool NeedComma = false;
  if (UnhandledReturn) {
    llvm::errs() << "return";
    NeedComma = true;
  }
  for (unsigned Ind : UnhandledOperands) {
    if (NeedComma) {
      llvm::errs() << ',';
    }
    llvm::errs() << "op" << Ind;
    NeedComma = true;
  }
  llvm::errs() << '\n';
}

// Helper function for string comparison
static bool strEq(const char *S1, const char *S2) {
  return strcmp(S1, S2) == 0;
}

// PcodeOpType method implementations
bool ConstraintsGenerator::PcodeOpType::addRetConstraint(
    Instruction *I, ConstraintsGenerator &cg) const {
  // only create Covariant constraints, use addSubtype to handle contra-variant.
  auto N = cg.createNode(I);
  if (I->getType()->isVoidTy()) {
    return false;
  }
  const char *ty = output;
  if (ty == nullptr) { // no action
    return true;
  } else if (strEq(ty, "sint")) {
    cg.setNonPointer(I);
    auto SintNode = binarysub::make_primitive("sint", cg.getSize(I));
    cg.addSubtype(SintNode, N);
    return true;
  } else if (strEq(ty, "uint")) {
    cg.setNonPointer(I);
    if (skipUintPrimitiveConstraints()) {
      return true;
    }
    auto UintNode = binarysub::make_primitive("uint", cg.getSize(I));
    cg.addSubtype(UintNode, N);
    return true;
  } else if (strEq(ty, "int")) {
    cg.setNonPointer(I);
    return true;
  } else if (strEq(ty, "float")) {
    auto FloatNode = binarysub::make_primitive("float", cg.getSize(I));
    cg.addSubtype(FloatNode, N);
    return true;
  }

  return false;
}

bool ConstraintsGenerator::PcodeOpType::addOpConstraint(
    unsigned Index, Instruction *I, ConstraintsGenerator &cg) const {
  assert(size >= 0 && static_cast<unsigned>(size) == I->getNumOperands() &&
         "input size not match");
  auto Op = I->getOperand(Index);
  if (Op->getType()->isVoidTy()) {
    return false;
  }
  auto N = cg.getOrInsertNode(getExtValuePtr(Op, I, Index));
  const char *ty = inputs[Index];
  if (ty == nullptr) {
    return true;
  } else if (strEq(ty, "sint")) {
    cg.setNonPointer(getExtValuePtr(Op, I, Index));
    auto SintNode = binarysub::make_primitive(
        "sint", cg.getSize(getExtValuePtr(Op, I, Index)));
    cg.addSubtype(N, SintNode);
    return true;
  } else if (strEq(ty, "uint")) {
    cg.setNonPointer(getExtValuePtr(Op, I, Index));
    if (skipUintPrimitiveConstraints()) {
      return true;
    }
    auto UintNode = binarysub::make_primitive(
        "uint", cg.getSize(getExtValuePtr(Op, I, Index)));
    cg.addSubtype(N, UintNode);
    return true;
  } else if (strEq(ty, "int")) {
    cg.setNonPointer(getExtValuePtr(Op, I, Index));
    return true;
  } else if (strEq(ty, "float")) {
    auto FloatNode =
        binarysub::make_primitive("float",
                                  cg.getSize(getExtValuePtr(Op, I, Index)));
    cg.addSubtype(N, FloatNode);
    return true;
  }
  return false;
}

// Static member definition for opTypes
const std::map<unsigned, ConstraintsGenerator::PcodeOpType>
    ConstraintsGenerator::opTypes = {
        // for Trunc, ZExt, SExt
        {Instruction::SExt, {"sint", 1, (const char *[1]){"sint"}}},
        {Instruction::ZExt, {"uint", 1, (const char *[1]){"uint"}}},
        {Instruction::Trunc, {"int", 1, (const char *[1]){"int"}}},

        // other cast insts: FPToUIInst, FPToSIInst, UIToFPInst, SIToFPInst
        {Instruction::FPToUI, {"uint", 1, (const char *[1]){nullptr}}},
        {Instruction::FPToSI, {"sint", 1, (const char *[1]){nullptr}}},
        {Instruction::UIToFP, {nullptr, 1, (const char *[1]){"uint"}}},
        {Instruction::SIToFP, {nullptr, 1, (const char *[1]){"sint"}}},
        {Instruction::FAdd, {"float", 2, (const char *[2]){"float", "float"}}},
        {Instruction::FSub, {"float", 2, (const char *[2]){"float", "float"}}},
        {Instruction::FMul, {"float", 2, (const char *[2]){"float", "float"}}},
        {Instruction::FDiv, {"float", 2, (const char *[2]){"float", "float"}}},

        {Instruction::UDiv, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::SDiv, {"sint", 2, (const char *[2]){"sint", "sint"}}},
        {Instruction::URem, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::SRem, {"sint", 2, (const char *[2]){"sint", "sint"}}},
        {Instruction::Xor, {"uint", 2, (const char *[2]){"uint", "uint"}}},

        // {Instruction::Add, {"int", (const char *[2]){"int", "int"}, 2}},
        // {Instruction::Sub, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::Mul, {"int", 2, (const char *[2]){"int", "int"}}},

        {Instruction::Shl, {"int", 2, (const char *[2]){"int", "int"}}},
        {Instruction::LShr, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::AShr, {"sint", 2, (const char *[2]){"sint", "uint"}}},
        // {Instruction::And, {"int", (const char *[2]){"int", "int"}, 2}},
        // {Instruction::Or, {"int", (const char *[2]){"int", "int"}, 2}},
};

// #endregion ConstraintsGenerator::MLsubVisitor

} // namespace notdec::mlsub
