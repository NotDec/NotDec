#ifndef _BINARYSUB_MLSUBGENERATOR_H_
#define _BINARYSUB_MLSUBGENERATOR_H_

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/JSON.h>


#include "TypeRecovery/mlsub/MLsubGraph.h"
#include "TypeRecovery/mlsub/MergePolicyEval.h"
#include "binarysub/PNDiff.h"
#include "binarysub/binarysub-core.h"
#include "notdec-backends/Core/HTypeResult.h"
#include "notdec-llvm2c/Interface/ExtValuePtr.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"

namespace notdec::mlsub {

using binarysub::SimpleType;

// Workdir files repeat the same verbose value labels across SCC reports. Keep
// one cache for a complete top-down pass so label formatting is paid once per
// ExtValuePtr without giving the cache a lifetime beyond the LLVM module run.
using ExtValueLabelCache = std::map<ExtValuePtr, std::string>;

// Stack-protector canary helpers (FS/GS segment reads). A store of such a
// value into a pointer is ABI frame padding, not a source-level object field.
bool isOpaqueSegmentReadValue(const llvm::Value *V);
bool isStackCanarySlotValue(const ExtValuePtr &Slot);

// genTypes phases are sequential within one SCC. Keeping only microsecond
// counters lets solveAndLowerTypes() aggregate every SCC without retaining any
// type graph or debug-output state.
struct GenTypesPhaseTiming {
  std::uint64_t WallUs = 0;
  std::uint64_t CpuUs = 0;
  bool CpuAvailable = false;
};

struct GenTypesTiming {
  GenTypesPhaseTiming Prepare;
  GenTypesPhaseTiming Bulk;
  GenTypesPhaseTiming Lower;
  GenTypesPhaseTiming Upper;
  GenTypesPhaseTiming DebugOutput;
};

// bottomUpPhase is split only at existing sequential boundaries. Each phase
// keeps its solver counters so time changes can be compared with recursive
// constraint work instead of only top-level addSubtype calls.
struct BottomUpPhaseTiming {
  std::uint64_t WallUs = 0;
  std::uint64_t CpuUs = 0;
  bool CpuAvailable = false;
  long ResidentRssKb = 0;
  binarysub::ConstraintSolverStats ConstraintStats;
};

// These counters split the expensive policy stages without retaining any
// candidate data after an SCC finishes. WorkItems is the number of entries or
// candidates seen by that subphase; it is only a scale indicator, not a result
// count. The whole structure is populated only with NOTDEC_CONSTRAINT_DIAG.
struct BottomUpSubphaseTiming {
  std::uint64_t WallUs = 0;
  std::uint64_t CpuUs = 0;
  bool CpuAvailable = true;
  std::uint64_t WorkItems = 0;
};

enum class BottomUpStage : std::size_t {
  FunctionNodes,
  VisitorConstraints,
  SccDeferredCalls,
  PNDiffSolve,
  CallInterfaceMerge,
  StructSlotMerge,
  FieldFollowupMerge,
  Overrides,
  SummaryInstantiation,
  PostSummaryDeferredCalls,
  PostSummaryCallInterfaceMerge,
  PostSummaryFieldFollowupMerge,
  Count,
};

enum class BottomUpSubstage : std::size_t {
  DeferredCallSubtype,
  DeferredCallCandidateCollect,
  CallInterfacePlan,
  CallInterfacePrecheck,
  CallInterfaceTransaction,
  StructSlotCandidateCollect,
  StructSlotCandidateApply,
  FieldFollowupCandidateCollect,
  FieldFollowupCandidateApply,
  Count,
};

struct BottomUpTiming {
  std::array<BottomUpPhaseTiming,
             static_cast<std::size_t>(BottomUpStage::Count)>
      Phases;
  std::array<BottomUpSubphaseTiming,
             static_cast<std::size_t>(BottomUpSubstage::Count)>
      Subphases;
  // Cross-SCC summary instantiation reuses the same policy functions. Keep a
  // second aggregate so their work is not silently charged to the main SCC
  // stages with similar names.
  std::array<BottomUpSubphaseTiming,
             static_cast<std::size_t>(BottomUpSubstage::Count)>
      PostSummarySubphases;

  BottomUpPhaseTiming &at(BottomUpStage Stage) {
    return Phases.at(static_cast<std::size_t>(Stage));
  }
  const BottomUpPhaseTiming &at(BottomUpStage Stage) const {
    return Phases.at(static_cast<std::size_t>(Stage));
  }
  BottomUpSubphaseTiming &at(BottomUpSubstage Stage) {
    return Subphases.at(static_cast<std::size_t>(Stage));
  }
  const BottomUpSubphaseTiming &at(BottomUpSubstage Stage) const {
    return Subphases.at(static_cast<std::size_t>(Stage));
  }
  BottomUpSubphaseTiming &atPostSummary(BottomUpSubstage Stage) {
    return PostSummarySubphases.at(static_cast<std::size_t>(Stage));
  }
  const BottomUpSubphaseTiming &atPostSummary(BottomUpSubstage Stage) const {
    return PostSummarySubphases.at(static_cast<std::size_t>(Stage));
  }
};

// This control-flow signal unwinds the pass pipeline before an incomplete type
// map reaches lowering or an output backend. main translates it to exit 75.
class SimplifyDiagnosticStop final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

struct ConstraintsGenerator;

struct RecordedStore {
  ExtValuePtr Addr;
  SimpleType ValueTy = nullptr;
  unsigned BitSize = 0;
  llvm::Instruction *Source = nullptr;
};

// EVM semantic passes use this as field evidence after HType has already
// confirmed a buffer field.  It preserves the original store/value pair without
// exposing BinarySub's temporary type nodes past type recovery.
struct EVMStoreEvidence {
  ExtValuePtr Addr;
  llvm::Value *StoredValue = nullptr;
  unsigned BitSize = 0;
  llvm::Instruction *Source = nullptr;
};

/// EVM semantic passes need the original store instructions after BinarySub
/// state has been converted into HType. This is evidence collection only; it
/// does not model aliases or create any pointer-analysis constraints.
struct EVMStoreRecords {
  std::map<ExtValuePtr, std::vector<RecordedStore>> StoresByAddr;
};

#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
// One origin id space per recovery.  instantiate_type_graph copies types (and
// the origins carried on them) from one SCC into another, so a per-generator
// table would let a foreign id be read against the wrong kind table.  The
// description/kind tables are shared, so a type instantiated into another SCC
// never carries an id that would be read against the wrong table.
struct BoundOriginTable {
  std::vector<std::string> Descriptions;
  std::vector<std::uint8_t> Kinds;
};
#endif

struct ConstraintsGenerator {
  long PointerSize = 0;
  std::string Name;
  // ConstraintGraph CG;
  PNIGraph PG;
  const std::set<llvm::Function *> &SCCs;
  int lvl = 0;
  SimpleType MemoryType = nullptr;
  // EVM storage is one contract-global object.  Keep the root and field map
  // shared across SCC generators so polymorphic summaries cannot clone storage
  // slots into per-callsite variables.
  SimpleType StorageType = nullptr;
  std::map<std::string, SimpleType> *StorageFields = nullptr;

  DSUMap<ExtValuePtr, SimpleType> V2N;
  EVMStoreRecords EVMStoreAccesses;
  std::map<ExtValuePtr, ast::HType *> ValueTypesLower;
  // Keep both solved bounds per value. ValueTypesLower is the pos=true result;
  // ValueTypesUpper is the pos=false result.
  std::map<ExtValuePtr, ast::HType *> ValueTypesUpper;
  ast::HType *StorageHType = nullptr;
  ast::RecordDecl *StorageDecl = nullptr;
  std::map<llvm::CallBase *, SimpleType> unhandledCalls;
  // A direct call has two related type shapes: SubtypeLHS is the old left-hand
  // side of the call subtype constraint, while FormalFunc exposes the formal
  // argument/return slots used by the later merge policy.  Keeping both lets us
  // preserve the old subtype relation exactly, but delay every call relation
  // until instruction visiting has collected all local layout evidence.
  struct DeferredCallConstraint {
    llvm::CallBase *Call = nullptr;
    llvm::Function *Target = nullptr;
    SimpleType SubtypeLHS = nullptr;
    SimpleType ActualFunc = nullptr;
    SimpleType FormalFunc = nullptr;
  };
  std::vector<DeferredCallConstraint> DeferredCallConstraints;
  std::set<ExtValuePtr> ContraVariantValues;
  std::set<ExtValuePtr> SnapshotContraVariantValues;
  std::map<std::uint32_t, std::set<ExtValuePtr>> OriginalVariableSources;
  std::map<std::pair<llvm::CallBase *, unsigned>, SimpleType>
      AggregateCallReturnSlots;
  // PNDiff only stores opaque handles.  The real ExtValuePtr objects stay here
  // so LLVM and llvm2c details do not leak into the PNDiff module.
  std::map<ExtValuePtr, std::unique_ptr<ExtValuePtr>> PNDiffValueHandles;
  // Late merge candidates.  They are collected while visiting IR, then applied
  // after pointer/type evidence has been generated so the policy can stay
  // conservative.
  struct ReturnValueMergeCandidate {
    llvm::ReturnInst *Return = nullptr;
    llvm::CallBase *Call = nullptr;
    llvm::Function *Target = nullptr;
    SimpleType Operand = nullptr;
    SimpleType FunctionReturn = nullptr;
  };
  std::vector<ReturnValueMergeCandidate> ReturnValueMergeCandidates;
  bool EnablePNDiffTypeVariableClosureUnification = true;
  std::ostream *TraceStream = nullptr;
  std::shared_ptr<MergePolicyEval> MergeEval;
  bool EnableStructPtrLoadStoreMerge = false;

  // Pair of struct-pointer contents stored in the same pointer slot.  The
  // policy only merges targets after both already look like struct pointers, so
  // scalar pointers and byte buffers stay separate.
  struct StructPtrSlotMergeCandidate {
    SimpleType Pointer;
    SimpleType FromTarget;
    SimpleType IntoTarget;
    unsigned AccessSize = 0;
  };

  // One-level struct field evidence on a pointer-sized variable.  We only use
  // direct record fields on the variable bounds and never recurse into field
  // types here.
  struct StructFieldSlice {
    uint64_t Offset = 0;
    uint64_t SizeBytes = 0;
    bool operator<(const StructFieldSlice &Other) const {
      return std::tie(Offset, SizeBytes) <
             std::tie(Other.Offset, Other.SizeBytes);
    }
  };

  // Follow-up merge after two owner struct pointers are merged.  If both owners
  // have a struct-pointer member at the same offset, the member nodes should be
  // considered the same recursive field entry as well.
  struct StructFieldFollowupMergeCandidate {
    SimpleType FromOwner = nullptr;
    SimpleType IntoOwner = nullptr;
    uint64_t Offset = 0;
    SimpleType FromFieldTarget = nullptr;
    SimpleType IntoFieldTarget = nullptr;
  };
  std::vector<StructFieldFollowupMergeCandidate>
      StructFieldFollowupMergeCandidates;

  struct CallArgStructPtrMergeCandidate {
    llvm::CallBase *Call = nullptr;
    llvm::Function *Target = nullptr;
    unsigned ArgIndex = 0;
    SimpleType ActualArg = nullptr;
    SimpleType FormalArg = nullptr;
  };
  std::vector<CallArgStructPtrMergeCandidate> CallArgStructPtrMergeCandidates;

  // Snapshot one whole formal-slot decision before later policies can change
  // its roots.  Entries retain every callsite/return point in the group, while
  // Roots retain the layouts and per-root actions used for the all-or-nothing
  // check.  MLsubRecovery writes these records only when a workdir is enabled.
  struct CallSlotMergeDecision {
    std::uint64_t TransactionId = 0;
    std::string Target;
    std::string Policy;
    std::string Decision;
    std::string Reason;
    std::string Formal;
    std::size_t TouchedNodes = 0;
    std::vector<std::string> Entries;
    std::vector<std::string> Roots;
    std::vector<std::string> RecursiveMerges;
  };
  std::vector<CallSlotMergeDecision> CallSlotMergeDecisions;
  std::uint64_t NextCallMergeTransactionId = 1;

  // Diagnostic modes for testing whether local subtype edges can be replaced
  // by merges before they grow large bound graphs.  The default keeps the
  // current struct-pointer rule; broader modes are opt-in experiments because
  // a merge cannot be split when later constraints reveal a conflict.
  enum class LocalSubtypeMergeMode { StructPointer, Pointer, AllLocal };
  LocalSubtypeMergeMode LocalSubtypeMode = LocalSubtypeMergeMode::StructPointer;
  mutable std::size_t LocalSubtypeHookChecks = 0;
  // Two-round merge (NOTDEC_TWO_ROUND_MERGE=1).  Round one runs while the
  // cross-function subtype edges are being added (the visitor plus the
  // deferred call constraints): a merge is only accepted when both sides
  // already carry identical field evidence.  Refused pairs are remembered and
  // re-checked in round two, after every call edge and field access has been
  // seen, with the ordinary "no conflicting layouts" policy.
  bool TwoRoundMerge = false;
  bool TwoRoundMergeDiag = false;
  // True only while round two re-checks the deferred pairs; the round-one
  // gate is active everywhere else.
  bool RoundTwoRecheck = false;
  mutable std::vector<std::pair<SimpleType, SimpleType>> DeferredRoundOneMerges;
  mutable std::size_t RoundOneMergeRefused = 0;
  std::size_t RoundTwoMergeConsidered = 0;
  std::size_t RoundTwoMergeAccepted = 0;
  mutable std::size_t LocalSubtypeBasicCandidates = 0;
  mutable std::size_t LocalSubtypeSameFunctionCandidates = 0;
  mutable std::size_t LocalSubtypePointerCandidates = 0;
  mutable std::size_t LocalSubtypeAccepted = 0;
  std::size_t LocalSubtypeReplaceBoundMergeEvents = 0;
  std::size_t LocalSubtypeAuxiliaryMergeEvents = 0;
  // Experimental ordering: merge complete actual/formal slot groups before
  // adding their call subtype edges. This can eliminate temporary edges, but
  // the precheck only sees layout evidence available before those edges exist.
  bool EnableEarlyCallInterfaceMerge = false;
  std::size_t EarlyCallInterfaceMerged = 0;
  // addSubtype historically treats a rejected constraint as local evidence it
  // cannot add and continues. Count those failures so early-merge experiments
  // cannot look successful merely because a later conflicting edge was lost.
  std::map<std::string, std::size_t> SubtypeConstraintFailures;
  bool ConstraintDiagEnabled = false;
  binarysub::ConstraintSolverStats CurrentConstraintStats;
  // Local-retry mode diagnostics: how many addSubtype calls hit the
  // integer/structured coexistence conflict, how many were rolled back and
  // redone without the store->load coupling, and how many were kept because the
  // call merged variables (that fires hooks outside the transaction).
  std::size_t RuleLocalConflicts = 0;
  std::size_t RuleLocalRetries = 0;
  std::size_t RuleLocalSkippedMerged = 0;
  // Local-retry mode diagnostics (merge boundary): merges that
  // let an integer and a structured view coexist, and those redone without the
  // store->load coupling.
  std::size_t MergeLocalConflicts = 0;
  std::size_t MergeLocalRetries = 0;

  void addMergeNode(SimpleType From, SimpleType To);
  void configurePNDiffCallbacks();
  bool configureConstraintContext(binarysub::ConstraintContext &Context);
  bool tryMergeVariablesForPolicy(llvm::StringRef Policy, SimpleType From,
                                  SimpleType Into,
                                  const std::string &TraceDetail);
  std::size_t applyCallInterfaceMergePolicy();
  std::size_t applyReturnValueMergePolicy();
  const ExtValuePtr *getVariableExternalValue(SimpleType Ty) const;
  void attachExternalValueHandle(SimpleType Ty, const ExtValuePtr &Val);
  llvm::Function *getExtValueFunction(const ExtValuePtr &Val) const;
  llvm::Function *getVariableOwningFunction(SimpleType Ty) const;
  binarysub::expected<bool, binarysub::Error>
  shouldMergeSameFunctionStructPtrSubtype(SimpleType LHS, SimpleType RHS) const;
  // Round-one gate for NOTDEC_TWO_ROUND_MERGE: true when merging is currently
  // allowed (switch off, or we are in round two, or both sides already carry
  // identical field slices).  Refused pairs are queued for round two.
  bool roundOneMergeAllowed(SimpleType LHS, SimpleType RHS) const;
  void deferRoundOneMergePair(SimpleType LHS, SimpleType RHS) const;
  // Round two: re-check the pairs round one refused, with full evidence.
  void recheckDeferredRoundOneMerges();
  // ---- Stack-region marking (default on) ---------------------------------
  // A value is frame derived when it addresses the shared "%frame" alloca of a
  // function, directly or through GEP/cast chains; such a value denotes a
  // location inside that function stack region.  The mark vetoes type-node
  // merges that would unify a stack region with something unrelated:
  //   NOTDEC_FRAME_NO_NONFRAME_MERGE        stack region vs non-stack region
  //   NOTDEC_FRAME_NO_CROSS_FUNCTION_MERGE  stack region of F vs stack region of G
  // Both are on by default (they remove the "object grows into a whole stack
  // frame" false members; see microsub2/logs 38/39/68).  Set either switch to
  // 0 to restore the pre-guard behaviour for an A/B run.
  const llvm::AllocaInst *frameOwnerOfValue(const llvm::Value *V) const;
  // What kind of object a value's address provably is, following the same
  // transparent chains as frameOwnerOfValue.  Object = a stack slot that is not
  // the frame, a global, or a parameter: an address we can attribute.  Unknown
  // = a loaded/returned/computed address, which may point anywhere (including
  // inside this very frame), so it must not be read as "provably not the frame".
  enum class AddressOrigin { Unknown, Frame, Object };
  AddressOrigin addressOriginOfValue(const llvm::Value *V) const;
  // nullopt: no value bookkeeping for this node, or (with
  // FrameVetoKnownObjectOnly) some value whose address is of unknown origin;
  // empty: known to be a non-frame object; non-empty: the stack regions the
  // node addresses.
  std::optional<std::set<const llvm::AllocaInst *>>
  frameOwnersOfNode(SimpleType Ty) const;
  bool frameMergeVeto(SimpleType LHS, SimpleType RHS,
                      const char **Why = nullptr) const;
  // Diagnostic helper for frameMergeVeto (NOTDEC_FRAME_VETO_DIAG).
  void dumpFrameVetoSide(const char *Tag, SimpleType Ty) const;
  bool FrameNoNonFrameMerge = true;
  bool FrameNoCrossFunctionMerge = true;
  bool FrameMarkIncludesLvars = false;
  // Only treat a node as "provably not a stack region" when every value behind
  // it is an attributable object (stack slot / global / parameter).  A loaded
  // or computed pointer is an unknown address, not evidence of a different
  // object, and must not block the merge.
  bool FrameVetoKnownObjectOnly = false;
  mutable std::map<const llvm::Value *, const llvm::AllocaInst *>
      FrameOwnerCache;
  mutable std::size_t FrameVetoNonFrame = 0;
  mutable std::size_t FrameVetoCrossFunction = 0;
  // Diagnostic: merges where one side is a stack region and the other side has
  // no value bookkeeping, so the veto above cannot decide (silent today).
  mutable std::size_t FrameVetoUnknownSide = 0;
  binarysub::expected<void, binarysub::Error> queueLocalSubtypeMerges(
      SimpleType Var, const binarysub::EnqueueMergeFn &EnqueueMerge) const;
  binarysub::expected<void, binarysub::Error> onVariableNonVarBoundAdded(
      SimpleType Var, SimpleType Bound, binarysub::BoundPolarity Polarity,
      const binarysub::EnqueueMergeFn &EnqueueMerge) const;
  binarysub::expected<void, binarysub::Error>
  onVariableNestedBoundRewritten(
      SimpleType User, SimpleType OldBound, SimpleType NewBound,
      const binarysub::EnqueueMergeFn &EnqueueMerge) const;
  void recordCallArgStructPtrMergeCandidate(llvm::CallBase &Call,
                                            llvm::Function &Target,
                                            unsigned ArgIndex,
                                            SimpleType ActualArg,
                                            SimpleType FormalArg);
  void recordCallArgStructPtrMergeCandidates(llvm::CallBase &Call,
                                             llvm::Function &Target,
                                             SimpleType ActualFunc,
                                             SimpleType FormalFunc);
  void recordCallReturnStructPtrMergeCandidate(llvm::CallBase &Call,
                                               llvm::Function &Target,
                                               SimpleType ActualRet,
                                               SimpleType FormalRet);
  void recordCallReturnStructPtrMergeCandidates(llvm::CallBase &Call,
                                                llvm::Function &Target,
                                                SimpleType ActualFunc,
                                                SimpleType FormalFunc);
  void deferCallConstraint(llvm::CallBase &Call, llvm::Function &Target,
                           SimpleType SubtypeLHS, SimpleType ActualFunc,
                           SimpleType FormalFunc);
  void applyDeferredCallConstraints();
  std::optional<std::vector<StructFieldSlice>>
  collectOneLevelStructFieldSlices(SimpleType Ty) const;
  // Field record entries only prove an address.  The slice width must come from
  // real direct load/store evidence on that field address.
  std::optional<uint64_t>
  collectMaxDirectFieldAccessSizeBytes(SimpleType FieldAddrTy) const;
  // Only direct loads/stores on the field address prove the field content.
  // Record fields by themselves only prove that an address exists.
  std::vector<SimpleType>
  collectDirectStructPointerAccessTargets(SimpleType FieldAddrTy) const;
  // After an owner merge, multiple targets at one offset are the recursive field
  // entries we may need to merge next.
  std::map<uint64_t, std::vector<SimpleType>>
  collectOneLevelStructPtrFieldTargetGroups(SimpleType Ty) const;
  std::vector<StructFieldFollowupMergeCandidate>
  collectStructPtrFieldFollowupMergeCandidatesForMergedOwner(
      SimpleType Owner) const;
  std::size_t applyStructPtrFieldFollowupMergePolicy();
  bool hasNonConflictingStructFieldSlices(SimpleType LHS, SimpleType RHS,
                                          bool RequireEvidence) const;
  // OverlapMode: -1 = use NOTDEC_LAYOUT_OVERLAP_MODE; 0 = strict (any
  // overlapping non-identical slice vetoes); 1 = allow nested (containment)
  // overlaps; 2 = allow every overlap.
  std::optional<std::string>
  explainStructFieldSliceCompatibilityFailure(SimpleType LHS, SimpleType RHS,
                                              bool RequireEvidence,
                                              int OverlapMode = -1) const;
  bool hasStructPointerEvidenceInBound(SimpleType Bound) const;
  bool hasPointerLikeEvidenceBeforeNonVarBound(
      SimpleType Ty, binarysub::BoundPolarity Polarity) const;
  bool hasStructPointerEvidenceBeforeNonVarBound(
      SimpleType Ty, binarysub::BoundPolarity Polarity) const;
  bool hasStructPointerEvidence(SimpleType Ty) const;
  bool hasPointerLikeEvidence(SimpleType Ty) const;
  // hasStructPointerEvidence 的版本化 memo。evidence 只依赖变量的
  // ptrLoad/ptrStore 这类非变量 bound；merge 只把这些 bound 从 From
  // 转移到 Into，其他变量的 evidence 不变，所以只需在 merge 成功时
  // 失效 (From, Into)，新增/重写非变量 bound 时失效对应变量即可。
  // slot-merge 循环里旧实现每合并一对就全清所有 false 缓存，巨型 SCC
  // 下剩余候选全部重算 bound 证据，redis 32 万候选的轮次数分钟无进展。
  mutable std::map<const binarysub::TypeNode *, bool> StructEvidenceMemo;
  std::size_t applyCallArgStructPtrMergePolicy();
  std::vector<StructPtrSlotMergeCandidate>
  collectStructPtrSameAccessKindMergeCandidates(bool CollectLoads) const;
  std::vector<StructPtrSlotMergeCandidate>
  collectStructPtrSameLoadMergeCandidates() const;
  std::vector<StructPtrSlotMergeCandidate>
  collectStructPtrSameStoreMergeCandidates() const;
  std::vector<StructPtrSlotMergeCandidate>
  collectStructPtrLoadStoreMergeCandidates() const;
  std::size_t applyStructPtrSlotMergeCandidates(llvm::StringRef Policy);
  std::size_t applyStructPtrLoadStoreMergePolicy();

  void instantiateSummary(llvm::CallBase *Inst, llvm::Function *Target,
                          const ConstraintsGenerator &Summary);

  // Bodies of these functions are excluded from constraint generation.  The
  // summary override still supplies their interface, so callers keep a useful
  // signature without paying for the body's internal constraints.
  std::set<llvm::Function *> OpaqueBodies;

  ConstraintsGenerator(std::string Name, unsigned int pointer_size,
                       const std::set<llvm::Function *> &SCCs,
                       SimpleType MemoryType, SimpleType StorageType = nullptr,
                       std::map<std::string, SimpleType> *StorageFields =
                           nullptr,
                       int lvl = 0,
                       std::ostream *TraceStream = nullptr,
                       std::shared_ptr<MergePolicyEval> MergeEval = nullptr,
                       bool EnableStructPtrLoadStoreMerge = false,
                       std::set<llvm::Function *> OpaqueBodies = {})
      : PointerSize(pointer_size), Name(Name), PG(Name, pointer_size),
        SCCs(SCCs), lvl(lvl), MemoryType(MemoryType),
        StorageType(StorageType), StorageFields(StorageFields),
        TraceStream(TraceStream), MergeEval(std::move(MergeEval)),
        EnableStructPtrLoadStoreMerge(EnableStructPtrLoadStoreMerge),
        OpaqueBodies(std::move(OpaqueBodies)) {
    PG.TraceStream = TraceStream;
    configurePNDiffCallbacks();
    if (auto *Mode = std::getenv("NOTDEC_LOCAL_SUBTYPE_MERGE_MODE")) {
      if (std::strcmp(Mode, "pointer") == 0) {
        LocalSubtypeMode = LocalSubtypeMergeMode::Pointer;
      } else if (std::strcmp(Mode, "all-local") == 0) {
        LocalSubtypeMode = LocalSubtypeMergeMode::AllLocal;
      } else if (std::strcmp(Mode, "struct-pointer") != 0) {
        llvm::errs() << "Warning: unknown NOTDEC_LOCAL_SUBTYPE_MERGE_MODE='"
                     << Mode << "', using struct-pointer.\n";
      }
    }
    if (auto *Enabled = std::getenv("NOTDEC_EARLY_CALL_INTERFACE_MERGE")) {
      EnableEarlyCallInterfaceMerge = std::strcmp(Enabled, "1") == 0;
    }
    // Default on: round one only merges nodes whose field evidence already
    // agrees, round two re-checks the rest once every call edge and field
    // access has been seen.  "=0" restores the single-round behaviour.
    TwoRoundMerge = true;
    if (auto *Enabled = std::getenv("NOTDEC_TWO_ROUND_MERGE")) {
      TwoRoundMerge = std::strcmp(Enabled, "0") != 0;
    }
    TwoRoundMergeDiag = std::getenv("NOTDEC_TWO_ROUND_DIAG") != nullptr;
    // Default on.  See frameOwnerOfValue: a stack region must not be unified
    // with an unrelated region.  "=0" restores the pre-guard behaviour.
    if (auto *Enabled = std::getenv("NOTDEC_FRAME_NO_NONFRAME_MERGE")) {
      FrameNoNonFrameMerge = std::strcmp(Enabled, "0") != 0;
    }
    if (auto *Enabled = std::getenv("NOTDEC_FRAME_NO_CROSS_FUNCTION_MERGE")) {
      FrameNoCrossFunctionMerge = std::strcmp(Enabled, "0") != 0;
    }
    if (auto *Enabled = std::getenv("NOTDEC_FRAME_MARK_INCLUDES_LVARS")) {
      FrameMarkIncludesLvars = std::strcmp(Enabled, "1") == 0;
    }
    // Relaxation of the veto condition: only an attributable non-frame object
    // counts as "provably not a stack region"; unknown addresses do not.
    FrameVetoKnownObjectOnly =
        std::getenv("NOTDEC_FRAME_VETO_KNOWN_OBJECT_ONLY") != nullptr;
    ConstraintDiagEnabled =
        std::getenv("NOTDEC_CONSTRAINT_DIAG") != nullptr;
  }

  BottomUpTiming run();
  GenTypesTiming genTypes(ast::HTypeContext &HCtx, unsigned PointerSizeBytes,
                          bool SolveGlobals,
                          ExtValueLabelCache *DebugLabelCache);
  void releaseBinarysubState();

  SimpleType convertSimpleType(ExtValuePtr Val);
  SimpleType convertSimpleTypeVal(Value *Val, llvm::User *User, long OpInd);
  SimpleType makeAggregateReturnRecord(
      llvm::Type *Ty,
      const std::function<SimpleType(unsigned, llvm::Type *)> &MakeField);
  SimpleType makeFunctionAggregateReturnRecord(llvm::Function &Func);
  SimpleType makeCallAggregateReturnRecord(llvm::CallBase &Call);
  SimpleType getOrCreateCallAggregateReturnSlot(llvm::CallBase &Call,
                                                unsigned Index);
  void addAggregateReturnConstraints(llvm::Value *Agg, llvm::ReturnInst &Ret);
  void maybeUnifyPNDiffTypeVariablePair(const SimpleType &Lhs,
                                        const SimpleType &Rhs);
  void unifyPNDiffValueGroups(llvm::ArrayRef<ExtValuePtr> LeftValues,
                              llvm::ArrayRef<ExtValuePtr> RightValues,
                              const SimpleType &Lhs, const SimpleType &Rhs);
  void emitMappingTrace(llvm::StringRef Event, ExtValuePtr Val,
                        const SimpleType &Ty);
  void emitRemapTrace(llvm::StringRef Event, ExtValuePtr Val,
                      ExtValuePtr Target, const SimpleType &Ty);
  void emitMergeTrace(llvm::StringRef Event, SimpleType From, SimpleType To,
                      llvm::ArrayRef<ExtValuePtr> MovedValues);
  void emitTypeRecoveryTrace(const std::string &Message);
  PNIValue getPNIValue(const ExtValuePtr &Val);
  const ExtValuePtr &getPNIExtValue(PNIValue Val) const;
  PNTy getPNILatticeType(const ExtValuePtr &Val) const;
  PNINode *getPNINodeOrNull(ExtValuePtr Val);
  PNINode &getPNINode(ExtValuePtr Val);
  PNINode &getOrInsertPNINode(ExtValuePtr Val);
  void remapPNINode(ExtValuePtr Val, ExtValuePtr Target);
  void unifyPNIValues(ExtValuePtr V1, ExtValuePtr V2);

  public:
  // Create Node of both variance
  SimpleType createNode(ExtValuePtr Val);

  // 约束生成中间点（函数内部约束已生成、跨函数调用边未连接）按函数统计
  // 约束图规模：节点数、边数、自引用节点数。用于评估"哪些函数把约束图
  // 撑大"，预警类型推理爆炸。由 NOTDEC_CONSTRAINT_STATS=1 开启。
  void emitFunctionConstraintStats() const;

  SimpleType getNodeOrNull(ExtValuePtr Val) {
    if (V2N.count(Val)) {
      return V2N.at(Val);
    }
    return nullptr;
  }

  SimpleType getOrInsertNode(ExtValuePtr Val) {
    auto Node = getNodeOrNull(Val);
    if (Node != nullptr) {
      return Node;
    }
    auto N = createNode(Val);
    return N;
  }

  // Class of the IR operation that introduced a bound origin.  Filters match on
  // this class, so extending the matcher set does not touch the layout side.
  enum class BoundOriginKind : std::uint8_t {
    Access = 0,
    Gep = 1,
    PtrAdd = 2,
    ConstAddr = 3,
    // Recognized bulk memory operation: memcpy family, or an inlined copy loop
    // whose pointer-add run covers a contiguous span.
    BulkCopy = 4,
    OpaqueHelper = 5,
  };
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
  // Origin id space shared by every SCC generator of this recovery (see
  // BoundOriginTable).  Lazy so a generator built outside an MLsubRecovery
  // still has a usable table.
  std::shared_ptr<BoundOriginTable> BoundOrigins;
  BoundOriginTable &boundOriginTable() {
    if (!BoundOrigins) {
      BoundOrigins = std::make_shared<BoundOriginTable>();
    }
    return *BoundOrigins;
  }
  // Origin ids attached to the bounds this generator's current addSubtype()
  // carries.  Descriptions/kinds are indexed by the shared id space.
  std::shared_ptr<const binarysub::SmallIdSet> CurrentBoundOrigins;
#endif

  // Attribute every constraint added inside this scope to Description.  The id
  // travels to binarysub through ConstraintContext::boundOrigins.
  class BoundOriginScope {
  public:
    BoundOriginScope(ConstraintsGenerator &CG, llvm::StringRef Description,
                     BoundOriginKind Kind = BoundOriginKind::Access)
        : CG(CG) {
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
      Previous = std::move(CG.CurrentBoundOrigins);
      if (!Description.empty()) {
        auto &Table = CG.boundOriginTable();
        auto Id = static_cast<std::uint32_t>(Table.Descriptions.size());
        Table.Descriptions.push_back(Description.str());
        Table.Kinds.push_back(static_cast<std::uint8_t>(Kind));
        CurrentId = Id;
        CG.CurrentBoundOrigins =
            std::make_shared<const binarysub::SmallIdSet>(
                binarysub::SmallIdSet{Id});
      }
#else
      (void)CG;
      (void)Description;
      (void)Kind;
#endif
    }
    ~BoundOriginScope() {
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
      CG.CurrentBoundOrigins = std::move(Previous);
#endif
    }
    BoundOriginScope(const BoundOriginScope &) = delete;
    BoundOriginScope &operator=(const BoundOriginScope &) = delete;

    // Origin id allocated by this scope (kNoOriginId when nothing was
    // attributed); the caller records extra evidence for it.
    std::uint32_t originId() const {
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
      return CurrentId;
#else
      return kNoOriginId;
#endif
    }
    static constexpr std::uint32_t kNoOriginId = 0xFFFFFFFFu;

  private:
    ConstraintsGenerator &CG;
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
    std::shared_ptr<const binarysub::SmallIdSet> Previous;
    std::uint32_t CurrentId = kNoOriginId;
#endif
  };

  // Partially-monomorphic positions create one extruded "shared" node per
  // (callee, position).  That node replaces the callee's formal in the call
  // slot, so it must count as a node of the CALLER: the struct-pointer merge
  // policy only pairs variables owned by one llvm::Function
  // (shouldMergeSameFunctionStructPtrSubtype), and the caller's other copies of
  // the same C object are what carry the members no other call path of the
  // callee touches.  Wrapper because the value->variable handle itself stays a
  // private detail of the generator.
  void attachSharedNodeOwner(SimpleType Ty, const ExtValuePtr &Val);

  // Collect the extruded (shared) node together with the other level-0 views of
  // the same C object inside its caller, so that the same-function
  // struct-pointer merge policy can unify them BEFORE the summary is extruded
  // and instantiated again.  The policy is edge-driven (it is consulted from
  // queueLocalSubtypeMerges for constraint neighbours only) and the extruded
  // node has no edge to the caller's other copies of the object, so without an
  // explicit collection the union of the object's members never reaches it.
  void collectSharedNodeSiblings(SimpleType Shared, llvm::Value *Actual);


  // True while an IR instruction is being visited: constraints added in that
  // window correspond to IR operations.  Everything added outside it comes from
  // a merge policy or from type-graph bookkeeping and is traced instead.
  bool InsideIRVisit = false;

  void addSubtype(SimpleType lhs, SimpleType rhs) {
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    const bool IRDerived = InsideIRVisit;
    if (!IRDerived && std::getenv("NOTDEC_NON_IR_EDGE_TRACE") != nullptr) {
      auto NonIROwner = [&](SimpleType T) -> std::string {
        if (auto *O = getVariableOwningFunction(T)) {
          return O->getName().str();
        }
        return "<none>";
      };
      llvm::errs() << "[non-ir-edge] " << binarysub::debug_string(lhs)
                   << " owner=" << NonIROwner(lhs) << "  ->  "
                   << binarysub::debug_string(rhs) << " owner=" << NonIROwner(rhs)
                   << "\n";
    }
    if (MergeEval) {
      MergeEval->observeAddSubtype();
    }
    if (const char *Watch = std::getenv("NOTDEC_FATGROW_DIAG")) {
      // Trace the growth of one node: every constraint that makes some value a
      // subtype of it (i.e. adds a lower bound), with the origin of the edge.
      static std::size_t Printed = 0;
      if (Printed < 400) {
        auto Match = [&](SimpleType Side) -> bool {
          auto Root = binarysub::resolve_variable(Side);
          auto *VS = Root ? Root->getAsVariableState() : nullptr;
          if (VS == nullptr) {
            return false;
          }
          if (VS->lowerBounds.size() < 20) {
            return false;
          }
          bool Found = false;
          for (const auto &Ent : V2N.rev()) {
            if (binarysub::resolve_variable(Ent.first).get() != Root.get()) {
              continue;
            }
            for (const auto &Val : Ent.second) {
              llvm::Function *Owner = nullptr;
              if (auto *VP = std::get_if<llvm::Value *>(&Val)) {
                if (auto *Inst = llvm::dyn_cast<llvm::Instruction>(*VP)) {
                  Owner = Inst->getFunction();
                } else if (auto *Arg = llvm::dyn_cast<llvm::Argument>(*VP)) {
                  Owner = Arg->getParent();
                }
              }
              if (Owner != nullptr && Owner->getName() == Watch) {
                Found = true;
                break;
              }
            }
            if (Found) {
              break;
            }
          }
          return Found;
        };
        const bool LhsIsFat = Match(lhs);
        const bool RhsIsFat = Match(rhs);
        if (LhsIsFat || RhsIsFat) {
          ++Printed;
          SimpleType Other = LhsIsFat ? rhs : lhs;
          auto OtherRoot = binarysub::resolve_variable(Other);
          auto *OtherVS = OtherRoot ? OtherRoot->getAsVariableState() : nullptr;
          llvm::errs() << "[fat-grow] #" << Printed
                       << (LhsIsFat ? " lhs=fat" : " rhs=fat")
                       << " other_lb="
                       << (OtherVS ? OtherVS->lowerBounds.size() : 0)
                       << " other_ub="
                       << (OtherVS ? OtherVS->upperBounds.size() : 0)
                       << " via=" << (InsideIRVisit ? "ir" : "policy");
          auto It = V2N.rev().find(OtherRoot);
          if (It != V2N.rev().end()) {
            for (std::size_t I = 0; I < It->second.size() && I < 2; ++I) {
              llvm::errs() << " v=" << toStableString(It->second[I]);
            }
          } else {
            llvm::errs() << " v=<none>";
          }
          llvm::errs() << "\n";
        }
      }
    }
    if (std::getenv("NOTDEC_FATNODE_DIAG") != nullptr) {
      for (SimpleType Side : {lhs, rhs}) {
        auto Root = binarysub::resolve_variable(Side);
        auto *VS = Root ? Root->getAsVariableState() : nullptr;
        static const std::size_t MinBounds = [] {
          const char *V = std::getenv("NOTDEC_FATNODE_MIN");
          return V == nullptr ? std::size_t{1000}
                              : static_cast<std::size_t>(std::atoi(V));
        }();
        static const std::string OnlyFunc = [] {
          const char *V = std::getenv("NOTDEC_FATNODE_FUNC");
          return std::string(V == nullptr ? "" : V);
        }();
        if (VS == nullptr ||
            VS->upperBounds.size() + VS->lowerBounds.size() < MinBounds) {
          continue;
        }
        static std::size_t PrintedForFunc = 0;
        std::vector<ExtValuePtr> Members;
        for (const auto &Ent : V2N.rev()) {
          if (binarysub::resolve_variable(Ent.first).get() == Root.get()) {
            Members.insert(Members.end(), Ent.second.begin(), Ent.second.end());
          }
        }
        if (!OnlyFunc.empty()) {
          if (PrintedForFunc >= 4) {
            continue;
          }
          bool Match = false;
          for (const auto &Val : Members) {
            if (auto *O = getExtValueFunction(Val);
                O != nullptr && O->getName() == OnlyFunc) {
              Match = true;
              break;
            }
          }
          if (!Match) {
            continue;
          }
          ++PrintedForFunc;
        }
        llvm::errs() << "[fat-node] node=" << binarysub::debug_string(Root)
                     << " lb=" << VS->lowerBounds.size()
                     << " ub=" << VS->upperBounds.size()
                     << " values=" << Members.size() << "\n";
        for (std::size_t I = 0; I < Members.size() && I < 50; ++I) {
          llvm::errs() << "    v=" << toStableString(Members[I]);
          if (auto *O = getExtValueFunction(Members[I])) {
            llvm::errs() << " in=" << O->getName();
          }
          llvm::errs() << "\n";
        }
        {
          // Every bound carries the origin ids that were active when it was
          // added (parallel vectors maintained by binarysub), so the operation
          // that generated the copies can be named directly.
          const auto &Table = boundOriginTable();
          auto Name = [&](std::uint32_t Id) -> std::string {
            return Id < Table.Descriptions.size() ? Table.Descriptions[Id]
                                                  : std::string("?");
          };
          std::map<std::string, unsigned> Hist;
          for (std::size_t I = 0; I < VS->lowerBounds.size(); ++I) {
            if (I >= VS->lowerBoundOrigins.size()) {
              ++Hist["<no-origin-slot>"];
              continue;
            }
            const auto &Origins = VS->lowerBoundOrigins[I];
            if (!Origins || Origins->empty()) {
              ++Hist["<no-origin>"];
              continue;
            }
            for (std::uint32_t Id : *Origins) {
              ++Hist[Name(Id)];
            }
          }
          llvm::errs() << "[lb-origin-hist] node="
                       << binarysub::debug_string(Root)
                       << " bounds=" << VS->lowerBounds.size()
                       << " slots=" << VS->lowerBoundOrigins.size()
                       << "\n";
          for (const auto &[N, C] : Hist) {
            llvm::errs() << "    " << C << "\t" << N << "\n";
          }
          unsigned Sample = 0;
          for (std::size_t I = 0;
               I < VS->lowerBounds.size() && Sample < 12; ++I, ++Sample) {
            llvm::errs() << "    lb[" << I << "]="
                         << binarysub::debug_string(VS->lowerBounds[I])
                         << " origins=[";
            if (I < VS->lowerBoundOrigins.size() &&
                VS->lowerBoundOrigins[I]) {
              unsigned C = 0;
              for (std::uint32_t Id : *VS->lowerBoundOrigins[I]) {
                if (C++ >= 3) {
                  llvm::errs() << ",…";
                  break;
                }
                llvm::errs() << Name(Id) << ";";
              }
            }
            llvm::errs() << "]\n";
          }
        }
        unsigned ShownLb = 0;
        for (const auto &B : VS->lowerBounds) {
          if (ShownLb++ >= 40) {
            break;
          }
          llvm::errs() << "    lb: " << binarysub::debug_string(B);
          if (auto *O = getVariableOwningFunction(B)) {
            llvm::errs() << " owner=" << O->getName();
          }
          auto It = V2N.rev().find(binarysub::resolve_variable(B));
          if (It != V2N.rev().end()) {
            for (std::size_t I = 0; I < It->second.size() && I < 2; ++I) {
              llvm::errs() << " v=" << toStableString(It->second[I]);
            }
          }
          llvm::errs() << "\n";
        }
        unsigned Shown = 0;
        for (const auto &B : VS->upperBounds) {
          if (Shown++ >= 8) {
            break;
          }
          llvm::errs() << "    ub: " << binarysub::debug_string(B);
          if (auto *O = getVariableOwningFunction(B)) {
            llvm::errs() << " owner=" << O->getName();
          }
          llvm::errs() << "\n";
        }
      }
    }
    binarysub::Cache cache;
    binarysub::ConstraintContext Context;
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
    Context.boundOrigins = CurrentBoundOrigins;
#endif
    binarysub::ConstraintContext *ContextPtr =
        configureConstraintContext(Context) ? &Context : nullptr;
    // Diagnostic: a pair of variables connected in BOTH directions is what the
    // solver's reverse_edge rule later merges; print the first such pairs so the
    // cascade's precondition can be traced back to a function/position.
    if (std::getenv("NOTDEC_BIDIR_DIAG") != nullptr) {
      auto L = binarysub::resolve_variable(lhs);
      auto R = binarysub::resolve_variable(rhs);
      auto *LV = L ? L->getAsVariableState() : nullptr;
      auto *RV = R ? R->getAsVariableState() : nullptr;
      if (LV != nullptr && RV != nullptr) {
        bool Reverse = false;
        for (const auto &B : RV->upperBounds) {
          if (binarysub::resolve_variable(B).get() == L.get()) {
            Reverse = true;
            break;
          }
        }
        if (!Reverse) {
          for (const auto &B : LV->lowerBounds) {
            if (binarysub::resolve_variable(B).get() == R.get()) {
              Reverse = true;
              break;
            }
          }
        }
        if (Reverse) {
          auto OwnerName = [&](SimpleType T) -> std::string {
            if (auto *O = getVariableOwningFunction(T)) {
              return O->getName().str();
            }
            return "<none>";
          };
          auto Values = [&](SimpleType T) -> std::string {
            std::string Out;
            auto It = V2N.rev().find(T);
            if (It == V2N.rev().end()) {
              return Out;
            }
            for (std::size_t I = 0; I < It->second.size() && I < 4; ++I) {
              if (I != 0) {
                Out += ", ";
              }
              Out += toStableString(It->second[I]);
            }
            return Out;
          };
          llvm::errs() << "[bidir-edge] " << binarysub::debug_string(L)
                       << " owner=" << OwnerName(L) << " values=[" << Values(L)
                       << "]  <->  " << binarysub::debug_string(R)
                       << " owner=" << OwnerName(R) << " values=[" << Values(R)
                       << "] via=" << (IRDerived ? "ir" : "policy") << "\n";
        }
      }
    }
    auto RunConstrain = [&](binarysub::ConstraintContext *Ctx,
                            binarysub::Cache &Seen) {
      return binarysub::constrain(
          lhs, rhs, Seen,
          [this](const SimpleType &Lhs, const SimpleType &Rhs) {
            maybeUnifyPNDiffTypeVariablePair(Lhs, Rhs);
          },
          Ctx);
    };
    auto ReportFailure = [&](const binarysub::expected<void, binarysub::Error>
                                 &Result) {
      if (Result) {
        return;
      }
      ++SubtypeConstraintFailures[Result.error().msg];
      if (TraceStream != nullptr) {
        emitTypeRecoveryTrace(
            "[subtype:rejected] lhs=" + binarysub::debug_string(lhs) + " rhs=" +
            binarysub::debug_string(rhs) + " reason=" + Result.error().msg);
      }
    };
    // NOTDEC_NO_INT_STRUCT_COEXIST=no-store-load: the addSubtype boundary is a
    // quiescent point -- constrain()'s worklist and merge queue are local and
    // drained before it returns -- so this single call can be rolled back and
    // redone.  When the store->load coupling lets an integer view and a
    // structured view coexist on one value, redo the call with that coupling
    // suppressed instead of dropping the rule everywhere
    // (NOTDEC_MEM_NO_STORE_TO_LOAD) or rejecting coexistence globally
    // (NOTDEC_NO_INT_STRUCT_COEXIST).
    const bool LocalRetry =
        binarysub::intStructCoexistPolicy() ==
        binarysub::IntStructCoexistPolicy::LocalRetry;
    if (LocalRetry && ContextPtr != nullptr) {
      // A merge runs the host hooks (value bookkeeping, audit) outside the
      // transaction, so only calls that merged nothing are redone.
      const std::uint64_t MergesBefore =
          CurrentConstraintStats.mergeVariableCalls;
      const auto FailuresBefore = SubtypeConstraintFailures;
      bool Coexist = false;
      Context.onIntStructCoexist = [&Coexist](const SimpleType &,
                                              const SimpleType &) {
        Coexist = true;
      };
      Context.stats = &CurrentConstraintStats;
      binarysub::SimpleTypeTransaction Tx;
      auto Result = RunConstrain(ContextPtr, cache);
      if (Coexist && CurrentConstraintStats.mergeVariableCalls ==
                         MergesBefore) {
        Tx.rollback();
        SubtypeConstraintFailures = FailuresBefore;
        ++RuleLocalConflicts;
        ++RuleLocalRetries;
        binarysub::Cache RetryCache;
        Context.suppressStoreToLoad = true;
        Result = RunConstrain(&Context, RetryCache);
      } else {
        if (Coexist) {
          ++RuleLocalConflicts;
          ++RuleLocalSkippedMerged;
        }
        Tx.commit();
      }
      ReportFailure(Result);
      return;
    }
    auto Result = RunConstrain(ContextPtr, cache);
    ReportFailure(Result);
  }

  SimpleType addVarSubtype(llvm::Value *Val, SimpleType dtv) {
    auto Node = getOrInsertNode(Val);
    addSubtype(dtv, Node);
    return Node;
  }

  SimpleType addRemapType(ExtValuePtr Val, ExtValuePtr Target);

  unsigned getLLVMTypeSize(llvm::Type *ElemTy);
  static inline bool is_cast(llvm::Value *Val) {
    return llvm::isa<llvm::AddrSpaceCastInst, llvm::BitCastInst,
                     llvm::PtrToIntInst, llvm::IntToPtrInst>(Val);
  }

  // Interface functions for module interaction

  void addAddConstraint(ExtValuePtr LHS, ExtValuePtr RHS,
                        llvm::BinaryOperator *Result);
  void addSubConstraint(ExtValuePtr LHS, ExtValuePtr RHS,
                        llvm::BinaryOperator *Result);
  void addCmpConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        llvm::ICmpInst *I);

  void setPointer(ExtValuePtr Val) {
    if (auto N = getPNINodeOrNull(Val)) {
      N->setPtr();
    }
  }
  void setNonPointer(ExtValuePtr Val) {
    if (auto N = getPNINodeOrNull(Val)) {
      N->setNonPtrIfRelated();
    }
  }
  unsigned getSize(ExtValuePtr Val) {
    return notdec::getSize(Val, PointerSize);
  }
  void recordStore(ExtValuePtr Addr, SimpleType ValueTy, unsigned BitSize,
                   llvm::Instruction *Source);
  void addConstantMemoryField(ExtValuePtr Addr);
  SimpleType getOrCreateStorageField(llvm::StringRef FieldName,
                                     unsigned BitSize = 0);

  void onUpdatePNType(ExtValuePtr Val) {}
  // The llvm::Value behind an ExtValuePtr, when the variant carries one.
  static const llvm::Value *extValueAsValue(const ExtValuePtr &Val) {
    if (const auto *Plain = std::get_if<llvm::Value *>(&Val)) {
      return *Plain;
    }
    if (const auto *Const = std::get_if<UConstant>(&Val)) {
      return Const->Val;
    }
    if (const auto *Stack = std::get_if<StackObject>(&Val)) {
      return Stack->Allocator;
    }
    if (const auto *Heap = std::get_if<HeapObject>(&Val)) {
      return Heap->Allocator;
    }
    return nullptr;
  }

  void setAsPtrAdd(ExtValuePtr basePtr, ExtValuePtr result, OffsetRange Off,
                   llvm::StringRef Origin = {}) {
    auto BaseNode = getOrInsertNode(basePtr);
    auto ResultNode = getOrInsertNode(result);
    unifyPNIValues(basePtr, result);
    // Negative offsets can appear in pointer arithmetic, but they are not safe
    // to materialize as object fields. Keep the P/N relation and skip the field.
    if (PointerSize == 256 && Off.hasNegativeBaseOffset()) {
      return;
    }
    // A stack-canary slot (the address that stores readfsqword(0x28)) is frame
    // padding, not a member of whatever object the frame is later merged into.
    // Keep the P/N relation above; do not emit an object field for it.
    if (isStackCanarySlotValue(result)) {
      return;
    }
    // Experimental (NOTDEC_NO_PTRADD_FIELD_CONSTRAINT=1): an offset that comes
    // from raw pointer arithmetic on an integer (PNDiff matched a
    // ptrtoint/add/inttoptr walk, Origin="ptradd:...") is a cursor position in
    // a byte walk, not evidence that the object has a member there.  Keep the
    // pointer/number relation above and the object marker below, but do not
    // create a member field on the base.  GEP-derived offsets ("gep:...") keep
    // the old behaviour.  See microsub2/logs 11/12 (a layout-side cursor filter
    // cannot work: the oversized fields have mixed origins) and 68/69.
    static const bool DropPtrAddFields = [] {
      const char *Value = std::getenv("NOTDEC_NO_PTRADD_FIELD_CONSTRAINT");
      return Value != nullptr && Value[0] != '\0' && Value[0] != '0';
    }();
    if (DropPtrAddFields && !Origin.starts_with("gep:")) {
      addSubtype(ResultNode, binarysub::make_record({}));
      return;
    }
    // Experimental (NOTDEC_NO_FRAME_MEMBER_FIELDS=1): the shared "%frame"
    // alloca is the function's raw stack storage, not a struct.  Today every
    // "frame + constant" walk emits a member field on it, so the frame becomes
    // "one record with one member per stack offset" (and the dynamic-index
    // slices at 920/928/936... add more).  A later merge between that frame and
    // an object then imports the whole stack slot table.  Skip the field for
    // frame-relative bases: the named locals keep their own records through
    // their own slices, and the pointer/number relation above is unchanged.
    static const bool DropFrameFields = [] {
      const char *Value = std::getenv("NOTDEC_NO_FRAME_MEMBER_FIELDS");
      return Value != nullptr && Value[0] != '\0' && Value[0] != '0';
    }();
    if (DropFrameFields &&
        frameOwnerOfValue(extValueAsValue(basePtr)) != nullptr) {
      addSubtype(ResultNode, binarysub::make_record({}));
      return;
    }
    std::vector<std::pair<std::string, SimpleType>> fields;
    fields.emplace_back(Off.str(), ResultNode);
    // The field cell below is this constraint's field evidence; name its
    // origin so the layout can be traced back to the access that produced it.
    const auto Kind = Origin.starts_with("gep:") ? BoundOriginKind::Gep
                                                   : BoundOriginKind::PtrAdd;
    BoundOriginScope OriginScope(*this, Origin, Kind);
    // Stamp the field entry itself (not only the bound edge) so the origin
    // survives into CompactType/UType even when the variables behind the
    // field are dissolved during solving.
    std::vector<std::vector<std::uint32_t>> FieldOrigins;
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
    if (OriginScope.originId() != BoundOriginScope::kNoOriginId) {
      FieldOrigins.push_back({OriginScope.originId()});
    }
#endif
    addSubtype(BaseNode, binarysub::make_record(std::move(fields),
                                                std::move(FieldOrigins)));
    addSubtype(ResultNode, binarysub::make_record({}));
  }

public:
  struct PcodeOpType {
    // We only care about number or non-number, and signedness.
    // Size is not included.
    // so nullptr / int / sint / uint
    const char *output;
    // allow for uniform initializations
    int size;
    const char **inputs;

  public:
    PcodeOpType(const char *output, int size, const char *inputs[])
        : output(output), size(size), inputs(inputs) {}
    bool addRetConstraint(llvm::Instruction *I, ConstraintsGenerator &cg) const;
    bool addOpConstraint(unsigned Index, llvm::Instruction *I,
                         ConstraintsGenerator &cg) const;
  };

  static const std::map<unsigned, PcodeOpType> opTypes;

protected:
  // visitor class
  // Visit each basic block in topo order. Then handle dataflow of Phi nodes.
  // After visiting each instruction, it must be assigned a type variable.
  // Often visitor will immediately add a subtype constraint. If the primitive
  // type is final, then it will directly map as the known type.
  class MLsubVisitor : public llvm::InstVisitor<MLsubVisitor> {
    ConstraintsGenerator &cg;
    // defer phi node constraints
    std::vector<llvm::PHINode *> phiNodes;

  public:
    MLsubVisitor(ConstraintsGenerator &cg) : cg(cg) {}

    // Bring the base class's visit(Function&)/visit(BasicBlock&) overloads back
    // into scope; the derived visit(Instruction&) below would otherwise hide
    // them.
    using llvm::InstVisitor<MLsubVisitor>::visit;

    // Every instruction goes through this entry point (InstVisitor CRTP), so it
    // marks the window in which constraints are IR-derived.
    void visit(llvm::Instruction &I) {
      const bool Saved = cg.InsideIRVisit;
      cg.InsideIRVisit = true;
      llvm::InstVisitor<MLsubVisitor>::visit(I);
      cg.InsideIRVisit = Saved;
    }

    static bool isHeapAllocationCall(llvm::CallBase &I);
    static bool shouldIgnoreRuntimeCall(llvm::CallBase &I);
    bool handleEVMMarkerCall(llvm::CallBase &I);
    void addEVMRuntimeSemanticConstraints(llvm::CallBase &I);
    bool handleIntrinsicCall(llvm::CallBase &I);
    // overloaded visit functions
    void visitExtractValueInst(llvm::ExtractValueInst &I);
    void visitInsertValueInst(llvm::InsertValueInst &I);
    void visitCastInst(llvm::CastInst &I);
    void visitCallBase(llvm::CallBase &I);
    void visitReturnInst(llvm::ReturnInst &I);
    void visitPHINode(llvm::PHINode &I);
    void visitLoadInst(llvm::LoadInst &I);
    void visitStoreInst(llvm::StoreInst &I);
    void visitAllocaInst(llvm::AllocaInst &I);
    void visitGetElementPtrInst(llvm::GetElementPtrInst &I);
    void visitICmpInst(llvm::ICmpInst &I);
    void visitSelectInst(llvm::SelectInst &I);

    void visitAdd(llvm::BinaryOperator &I);
    void visitSub(llvm::BinaryOperator &I);

    // handle sth like
    // 1. Alignment/ Use lowest bits in pointer: And %x, 0xfffffff0.
    // 2. set lowest bits in the pointer: Or %x, 0x7
    void visitAnd(llvm::BinaryOperator &I);
    void visitOr(llvm::BinaryOperator &I);

    // ignore control flow related instructions
    void visitUnreachableInst(llvm::UnreachableInst &I) {}
    void visitBranchInst(llvm::BranchInst &I) {}
    void visitSwitchInst(llvm::SwitchInst &I) {}

    void handlePHINodes();
    // use opTypes to handle other insts.
    void visitInstruction(llvm::Instruction &I);
  };
};

struct SCCData {
  std::vector<llvm::CallGraphNode *> Nodes;
  std::string SCCName;
  std::set<llvm::Function *> SCCSet;
  std::shared_ptr<ConstraintsGenerator> Generator;
  unsigned int level = 0;

  void onIRChanged() { Generator.reset(); }
  // TODO disable copy constructor
};

struct AllGraphs {
  std::vector<SCCData> AllSCCs;
  std::map<llvm::CallGraphNode *, std::size_t> Func2SCCIndex;
  std::map<llvm::CallGraphNode *, std::set<llvm::CallGraphNode *>>
      Callee2Callers;
  llvm::CallGraph *CG = nullptr;
};

class MLsubRecovery {
  const llvm::Module &Mod;
  llvm::ModuleAnalysisManager &MAM;

  std::string data_layout = Mod.getDataLayoutStr();
  unsigned int PointerSize = Mod.getDataLayout().getPointerSizeInBits();

  AllGraphs AG;
#ifdef BINARYSUB_ENABLE_BOUND_ORIGINS
  // One origin id space for every SCC generator of this recovery; the kinds
  // and descriptions stay valid when a type is instantiated across SCCs.
  std::shared_ptr<BoundOriginTable> RecoveryBoundOrigins =
      std::make_shared<BoundOriginTable>();
#endif
  std::unique_ptr<llvm::CallGraph> CallG;
  std::optional<llvm::raw_fd_ostream> SCCsCatalog;
  std::unique_ptr<std::ofstream> BinarysubTraceFile;
  const char *SummaryFile = std::getenv("NOTDEC_SUMMARY_OVERRIDE");
  const char *SignatureFile = std::getenv("NOTDEC_SIGNATURE_OVERRIDE");
  const char *ExtraConstraintsFile = std::getenv("NOTDEC_EXTRA_CONSTRAINTS");
  std::string MergeEvalDir;
  std::string EmitPostConstraintStateDir;
  std::string LoadPostConstraintStateDir;
  int TypeRecoveryLevel = 0;
  bool ThrowOnDiagnosticStop = true;
  std::string FailureReason;
  bool LoadedPostConstraintState = false;
  std::shared_ptr<MergePolicyEval> MergeEval;
  bool EnableStructPtrLoadStoreMerge = false;
  // Generic allocator wrappers must become SCC summary boundaries before
  // prepareSCC(), otherwise callers and the wrapper body share one monotype SCC.
  std::set<llvm::Function *> DetectedMallocWrappers;
  // Buffer and generic-context APIs reuse one declaration for unrelated caller
  // types. Mark them polymorphic before SCC partitioning so each callsite gets
  // fresh argument variables instead of sharing one `read::arg1`-style node.
  std::set<llvm::Function *> DetectedPolymorphicBufferFunctions;
  // Set by PolymorphicMarkingPass; when true MLsubRecovery::run does not run
  // the recognition sources again (the pass already did, before the call
  // graph).
  bool PolymorphicMarkingDone = false;
  // Set once the summary/signature override documents and the primitive
  // semantic lattices are registered.
  bool OverridesLoaded = false;
  llvm::json::Value SummaryOverrideDoc = nullptr;
  std::set<llvm::Function *> SummaryOverrideFuncs;
  llvm::json::Value SignatureOverrideDoc = nullptr;
  std::set<llvm::Function *> SignatureOverrideFuncs;
  llvm::json::Value ExtraConstraintsDoc = nullptr;
  std::set<llvm::Function *> ExtraConstraintsFuncs;
  bool WrotePNDiffOverrideWarningHeader = false;
  // std::map<llvm::Function *, binarysub::TypeScheme> PolySchemes;
  SimpleType MemoryType = nullptr;
  SimpleType StorageType = nullptr;
  std::map<std::string, SimpleType> StorageFields;

  // HTypeContext for type building
  std::shared_ptr<ast::HTypeContext> HCtx;

  std::function<bool(llvm::Function *)> isPolymorphic = [](llvm::Function *F) {
    if (auto Env = std::getenv("NOTDEC_DEFAULT_POLY")) {
      if (std::strcmp(Env, "1") == 0) {
        return true;
      }
    }
    return false;
  };

public:
  struct OverrideTypeRecipe {
    SimpleType Root = nullptr;
    std::vector<std::pair<SimpleType, SimpleType>> Constraints;
  };

  struct OverrideBuildContext {
    ConstraintsGenerator &Generator;
    llvm::Function &Func;
    std::map<int64_t, SimpleType> Vars;
    std::map<int64_t, std::uint32_t> VarBitWidths;
  };

  MLsubRecovery(llvm::Module &Mod, llvm::ModuleAnalysisManager &MAM)
      : Mod(Mod), MAM(MAM) {}

  void setMergeEvalDir(std::string Dir) { MergeEvalDir = std::move(Dir); }
  void setPostConstraintCheckpointDirectories(std::string EmitDir,
                                              std::string LoadDir) {
    EmitPostConstraintStateDir = std::move(EmitDir);
    LoadPostConstraintStateDir = std::move(LoadDir);
  }
  void setTypeRecoveryLevel(int Level) { TypeRecoveryLevel = Level; }
  void setThrowOnDiagnosticStop(bool Throw) {
    ThrowOnDiagnosticStop = Throw;
  }
  bool failed() const { return !FailureReason.empty(); }
  const std::string &failureReason() const { return FailureReason; }
  const llvm::Module &getCheckpointModule() const { return Mod; }
  llvm::StringRef getCheckpointDataLayout() const { return data_layout; }
  unsigned getCheckpointPointerSize() const { return PointerSize; }
  int getCheckpointTypeRecoveryLevel() const { return TypeRecoveryLevel; }
  bool getCheckpointStructMergeEnabled() const {
    return EnableStructPtrLoadStoreMerge;
  }
  const llvm::json::Value &getCheckpointSummaryDoc() const {
    return SummaryOverrideDoc;
  }
  const llvm::json::Value &getCheckpointSignatureDoc() const {
    return SignatureOverrideDoc;
  }
  const llvm::json::Value &getCheckpointExtraConstraintsDoc() const {
    return ExtraConstraintsDoc;
  }
  // The level hook (isPolymorphic) is private; the marking pass reports it.
  bool isLevelPolymorphic(llvm::Function *F) const { return isPolymorphic(F); }
  bool isPolymorphicMarkingDone() const { return PolymorphicMarkingDone; }
  void setPolymorphicMarkingDone(bool Value = true) {
    PolymorphicMarkingDone = Value;
  }
  const std::set<llvm::Function *> &getCheckpointMallocWrappers() const {
    return DetectedMallocWrappers;
  }
  const std::set<llvm::Function *> &
  getCheckpointPolymorphicBufferFunctions() const {
    return DetectedPolymorphicBufferFunctions;
  }
  void setMergeStructPtrLoadStore(bool Enable) {
    EnableStructPtrLoadStoreMerge = Enable;
  }
  void run();
  void runImpl();
  void loadSummaryFile(llvm::Module &M, const char *path,
                       bool StrictValidation = true);
  void loadSignatureFile(llvm::Module &M, const char *path,
                         bool StrictValidation = true);
  void validateExtraConstraintsFile(llvm::Module &M, const char *Path,
                                    llvm::StringRef ModuleSHA256Hex);
  void emitTRInputArtifacts(llvm::Module &M, llvm::StringRef OutputPath);
  void loadOverridesAndPrimitives(llvm::Module &M);
  void detectMallocWrappers(llvm::Module &M);
  void markBuiltinPolymorphicBufferFunctions(llvm::Module &M);
  const llvm::json::Value *getExtraConstraintsSpec(
      const llvm::Function &Func) const;
  const llvm::json::Value *getSummaryOverrideSpec(
      const llvm::Function &Func) const;
  bool isSummaryOverridePolymorphic(const llvm::Function &Func) const;
  bool isOpaqueBody(const llvm::Function &Func) const;
  const llvm::json::Value *getSignatureOverrideSpec(
      const llvm::Function &Func) const;
  OverrideTypeRecipe buildOverrideType(const llvm::json::Value &Expr,
                                       OverrideBuildContext &Ctx,
                                       llvm::StringRef Path,
                                       bool AllowNull = false);
  void applyOverrideRecipe(ConstraintsGenerator &G,
                           const OverrideTypeRecipe &Recipe);
  void applySummaryOverride(ConstraintsGenerator &G, llvm::Function &Func,
                            const llvm::json::Value &Spec);
  void applyUpperBoundSignatureOverride(ConstraintsGenerator &G,
                                        llvm::Function &Func,
                                        const llvm::json::Value &Spec);
  void applyExtraConstraints(ConstraintsGenerator &G, llvm::Function &Func,
                             const llvm::json::Value &Spec);
  // 形成单独分析的SCC群。（按需复制多态函数）
  void prepareSCC(llvm::CallGraph &CG);
  void bottomUpPhase();
  // Reserved for a future caller-to-callee constraint propagation pass. The
  // current MLsub pipeline has no top-down graph mutation yet, so this phase
  // intentionally remains an explicit no-op between bottom-up generation and
  // CompactType/HType solving.
  void topDownConstraintPhase();
  void savePostConstraintState(llvm::StringRef Directory,
                               llvm::StringRef ModuleSHA256Hex);
  void loadPostConstraintState(llvm::StringRef Directory,
                               llvm::StringRef ModuleSHA256Hex);
  // Solve the completed SimpleType graph and lower each SCC result to HType.
  void solveAndLowerTypes();

  using Result = ::notdec::llvm2c::HTypeResult;
  std::unique_ptr<Result> ResultVal;
  std::vector<EVMStoreEvidence> EVMStores;
  std::unique_ptr<Result> &getResult(llvm::Module &M1,
                                     llvm::ModuleAnalysisManager &MAM) {
    if (ResultVal == nullptr) {
      genASTTypes(M1);
    }
    return ResultVal;
  }
  llvm::ArrayRef<EVMStoreEvidence> getEVMStoreEvidence() const {
    return EVMStores;
  }
  void genASTTypes(llvm::Module &M);
};

struct MLsubRecoveryMain : llvm::PassInfoMixin<MLsubRecoveryMain> {

  MLsubRecovery &TR;
  MLsubRecoveryMain(MLsubRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {
    TR.run();
    return llvm::PreservedAnalyses::all();
  }
};

// Currently only break stack.
struct MLsubRecoveryOpt : llvm::PassInfoMixin<MLsubRecoveryOpt> {

  MLsubRecovery &TR;
  MLsubRecoveryOpt(MLsubRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {
    assert(false && "TODO");
  }
};

} // namespace notdec::mlsub

#endif
