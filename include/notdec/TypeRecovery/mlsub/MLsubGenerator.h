#ifndef _BINARYSUB_MLSUBGENERATOR_H_
#define _BINARYSUB_MLSUBGENERATOR_H_

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
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

#include <clang/AST/Type.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>

#include "TypeRecovery/mlsub/MLsubGraph.h"
#include "TypeRecovery/mlsub/MergePolicyEval.h"
#include "binarysub/PNDiff.h"
#include "binarysub/binarysub-core.h"
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Interface/ExtValuePtr.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"

namespace notdec::mlsub {

using binarysub::SimpleType;

// Workdir files repeat the same verbose value labels across SCC reports. Keep
// one cache for a complete top-down pass so label formatting is paid once per
// ExtValuePtr without giving the cache a lifetime beyond the LLVM module run.
using ExtValueLabelCache = std::map<ExtValuePtr, std::string>;

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
  std::optional<std::string>
  explainStructFieldSliceCompatibilityFailure(SimpleType LHS, SimpleType RHS,
                                              bool RequireEvidence) const;
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
  }

  void run() {
    // 约束生成按函数名排序遍历：SCCs 是 std::set<llvm::Function*>，指针迭代
    // 顺序随 ASLR 变化，会让约束插入/求解顺序 run-to-run 不同（ValueTypes
    // 差异和分组方差都来自这里）。模块内函数名唯一，排序是确定性键。
    std::vector<const llvm::Function *> SortedSCCs(SCCs.begin(), SCCs.end());
    llvm::sort(SortedSCCs, [](const llvm::Function *A,
                              const llvm::Function *B) {
      return A->getName() < B->getName();
    });
    for (const llvm::Function *Func1 : SortedSCCs) {
      auto Func = const_cast<llvm::Function *>(Func1);
      // create function nodes
      auto F = createNode(Func);
      std::vector<SimpleType> Args;
      for (unsigned i = 0; i < Func->arg_size(); ++i) {
        auto Arg = createNode(Func->getArg(i));
        // Contra-variant.
        Args.push_back(Arg);
      }
      SimpleType Ret = nullptr;
      if (!Func->getReturnType()->isVoidTy()) {
        if (Func->getReturnType()->isAggregateType()) {
          Ret = makeFunctionAggregateReturnRecord(*Func);
        } else {
          Ret = createNode(ReturnValue{.Func = Func});
        }
      }
      addSubtype(binarysub::make_function(Args, Ret), F);
    }
    for (const llvm::Function *Func : SortedSCCs) {
      if (OpaqueBodies.count(const_cast<llvm::Function *>(Func)) != 0) {
        continue;
      }
      MLsubVisitor Visitor(*this);
      Visitor.visit(const_cast<llvm::Function *>(Func));
      Visitor.handlePHINodes();
    }
    applyDeferredCallConstraints();
    for (const llvm::Function *Func1 : SortedSCCs) {
      auto Func = const_cast<llvm::Function *>(Func1);
      auto F = getNodeOrNull(Func);
      assert(F->getAsVariableState() != nullptr);
    }
    PG.solve();
    {
      auto Merged = applyCallInterfaceMergePolicy();
      llvm::errs() << "Info: transactional call interface merge policy merged "
                   << Merged << " pair(s)\n";
    }
    if (EnableStructPtrLoadStoreMerge) {
      auto Merged = applyStructPtrLoadStoreMergePolicy();
      llvm::errs() << "Info: struct pointer slot merge policy merged "
                   << Merged << " pair(s)\n";
    }
    {
      auto Merged = applyStructPtrFieldFollowupMergePolicy();
      llvm::errs() << "Info: struct pointer field follow-up merge policy merged "
                   << Merged << " pair(s)\n";
    }
  }
  void genTypes(ast::HTypeContext &HCtx, unsigned PointerSizeBytes,
                bool SolveGlobals, ExtValueLabelCache *DebugLabelCache);
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

  void addSubtype(SimpleType lhs, SimpleType rhs) {
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    if (MergeEval) {
      MergeEval->observeAddSubtype();
    }
    binarysub::Cache cache;
    binarysub::ConstraintContext Context;
    binarysub::ConstraintContext *ContextPtr =
        configureConstraintContext(Context) ? &Context : nullptr;
    auto Result = binarysub::constrain(
        lhs, rhs, cache,
        [this](const SimpleType &Lhs, const SimpleType &Rhs) {
          maybeUnifyPNDiffTypeVariablePair(Lhs, Rhs);
        },
        ContextPtr);
    if (!Result) {
      ++SubtypeConstraintFailures[Result.error().msg];
      if (TraceStream != nullptr) {
        emitTypeRecoveryTrace(
            "[subtype:rejected] lhs=" + binarysub::debug_string(lhs) + " rhs=" +
            binarysub::debug_string(rhs) + " reason=" + Result.error().msg);
      }
    }
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
  void addEVMConstantMemoryField(ExtValuePtr Addr, SimpleType ValueTy);
  SimpleType getOrCreateStorageField(llvm::StringRef FieldName,
                                     unsigned BitSize = 0);

  void onUpdatePNType(ExtValuePtr Val) {}
  void setAsPtrAdd(ExtValuePtr basePtr, ExtValuePtr result, OffsetRange Off) {
    auto BaseNode = getOrInsertNode(basePtr);
    auto ResultNode = getOrInsertNode(result);
    unifyPNIValues(basePtr, result);
    // Negative offsets can appear in pointer arithmetic, but they are not safe
    // to materialize as object fields. Keep the P/N relation and skip the field.
    if (PointerSize == 256 && Off.hasNegativeBaseOffset()) {
      return;
    }
    std::vector<std::pair<std::string, SimpleType>> fields;
    fields.emplace_back(Off.str(), ResultNode);
    addSubtype(BaseNode, binarysub::make_record(std::move(fields)));
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
  std::unique_ptr<llvm::CallGraph> CallG;
  std::optional<llvm::raw_fd_ostream> SCCsCatalog;
  std::unique_ptr<std::ofstream> BinarysubTraceFile;
  const char *SummaryFile = std::getenv("NOTDEC_SUMMARY_OVERRIDE");
  const char *SignatureFile = std::getenv("NOTDEC_SIGNATURE_OVERRIDE");
  const char *ExtraConstraintsFile = std::getenv("NOTDEC_EXTRA_CONSTRAINTS");
  std::string MergeEvalDir;
  std::shared_ptr<MergePolicyEval> MergeEval;
  bool EnableStructPtrLoadStoreMerge = false;
  // Generic allocator wrappers must become SCC summary boundaries before
  // prepareSCC(), otherwise callers and the wrapper body share one monotype SCC.
  std::set<llvm::Function *> DetectedMallocWrappers;
  // Buffer and generic-context APIs reuse one declaration for unrelated caller
  // types. Mark them polymorphic before SCC partitioning so each callsite gets
  // fresh argument variables instead of sharing one `read::arg1`-style node.
  std::set<llvm::Function *> DetectedPolymorphicBufferFunctions;
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
  void setMergeStructPtrLoadStore(bool Enable) {
    EnableStructPtrLoadStoreMerge = Enable;
  }
  void run();
  void loadSummaryFile(llvm::Module &M, const char *path,
                       bool StrictValidation = true);
  void loadSignatureFile(llvm::Module &M, const char *path,
                         bool StrictValidation = true);
  void validateExtraConstraintsFile(llvm::Module &M, const char *Path,
                                    llvm::StringRef ModuleSHA256Hex);
  void emitTRInputArtifacts(llvm::Module &M, llvm::StringRef OutputPath);
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
  void topDownPhase();

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
