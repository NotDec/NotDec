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
#include "TypeRecovery/mlsub/PNDiff.h"
#include "TypeRecovery/mlsub/PointerAnalysis.h"
#include "binarysub/binarysub-core.h"
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Interface/ExtValuePtr.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"

namespace notdec::mlsub {

using binarysub::SimpleType;

struct ConstraintsGenerator;

struct RecordedLoad {
  ExtValuePtr Addr;
  SimpleType ResultTy = nullptr;
  unsigned BitSize = 0;
  llvm::Instruction *Source = nullptr;
};

struct RecordedStore {
  ExtValuePtr Addr;
  SimpleType ValueTy = nullptr;
  unsigned BitSize = 0;
  llvm::Instruction *Source = nullptr;
};

/// Records IR memory events and gives each explicit MemoryLocKey a content
/// SimpleType. The records are kept outside PointerAnalysis so PA only answers
/// object identity, while BinarySub still owns the actual content type.
struct MemoryAccessRecords {
  std::map<MemoryLocKey, SimpleType> LocTypes;
  std::map<ExtValuePtr, std::vector<RecordedLoad>> LoadsByAddr;
  std::map<ExtValuePtr, std::vector<RecordedStore>> StoresByAddr;
  std::set<std::pair<MemoryLocKey, SimpleType>> EmittedLoadConstraints;
  std::set<std::pair<SimpleType, MemoryLocKey>> EmittedStoreConstraints;
  std::set<std::tuple<ExtValuePtr, SimpleType, unsigned>> EmittedLoadViews;
  std::set<std::tuple<ExtValuePtr, SimpleType, unsigned>> EmittedStoreViews;
  std::set<std::tuple<MemoryLocKey, llvm::Instruction *, llvm::Instruction *>>
      EmittedStoreLoadRelations;
  std::set<std::tuple<SimpleType, SimpleType, unsigned>> ObservedOldRelations;

  SimpleType getOrCreateLocType(ConstraintsGenerator &CG,
                                const MemoryLocKey &Loc);
};

struct ConstraintsGenerator {
  long PointerSize = 0;
  std::string Name;
  // ConstraintGraph CG;
  PNIGraph PG;
  PointerAnalysis PA;
  const std::set<llvm::Function *> &SCCs;
  int lvl = 0;
  SimpleType MemoryType = nullptr;

  DSUMap<ExtValuePtr, SimpleType> V2N;
  MemoryAccessRecords MemoryAccesses;
  std::map<ExtValuePtr, ast::HType *> ValueTypesLower;
  // Keep both solved bounds per value. ValueTypesLower is the pos=true result;
  // ValueTypesUpper is the pos=false result.
  std::map<ExtValuePtr, ast::HType *> ValueTypesUpper;
  std::map<llvm::CallBase *, SimpleType> unhandledCalls;
  std::set<ExtValuePtr> ContraVariantValues;
  std::set<ExtValuePtr> SnapshotContraVariantValues;
  std::map<std::uint32_t, std::set<ExtValuePtr>> OriginalVariableSources;
  std::map<std::pair<llvm::CallBase *, unsigned>, SimpleType>
      AggregateCallReturnSlots;
  bool EnablePNDiffTypeVariableClosureUnification = true;
  std::ostream *TraceStream = nullptr;
  PointerAnalysisMode PAMode = PointerAnalysisMode::Original;

  void addMergeNode(SimpleType From, SimpleType To);

  void instantiateSummary(llvm::CallBase *Inst, llvm::Function *Target,
                          const ConstraintsGenerator &Summary);

  ConstraintsGenerator(std::string Name, unsigned int pointer_size,
                       const std::set<llvm::Function *> &SCCs,
                       SimpleType MemoryType, int lvl = 0,
                       std::ostream *TraceStream = nullptr)
      : PointerSize(pointer_size), Name(Name), PG(*this, Name, pointer_size),
        SCCs(SCCs), lvl(lvl), MemoryType(MemoryType),
        TraceStream(TraceStream) {
    PG.TraceStream = TraceStream;
    if (auto *Mode = std::getenv("NOTDEC_POINTER_ANALYSIS_MODE")) {
      if (std::strcmp(Mode, "shadow") == 0) {
        PAMode = PointerAnalysisMode::Shadow;
      } else if (std::strcmp(Mode, "replace") == 0) {
        PAMode = PointerAnalysisMode::Replace;
      } else if (std::strcmp(Mode, "original") != 0) {
        llvm::errs() << "Warning: unknown NOTDEC_POINTER_ANALYSIS_MODE='"
                     << Mode << "', using original.\n";
      }
    }
  }

  void run() {
    for (const llvm::Function *Func1 : SCCs) {
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
    for (const llvm::Function *Func : SCCs) {
      MLsubVisitor Visitor(*this);
      Visitor.visit(const_cast<llvm::Function *>(Func));
      Visitor.handlePHINodes();
    }
    for (const llvm::Function *Func1 : SCCs) {
      auto Func = const_cast<llvm::Function *>(Func1);
      auto F = getNodeOrNull(Func);
      assert(F->getAsVariableState() != nullptr);
    }
    PG.solve();
    if (isPointerAnalysisEnabled()) {
      PA.solve();
      flushPointerDerivedTypeConstraints();
    }
  }
  void genTypes(ast::HTypeContext &HCtx, const llvm::DataLayout &DL,
                bool SolveMemory = false);
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
  void observeOldMemoryTypeEdge(const SimpleType &Lhs, const SimpleType &Rhs);
  void emitMappingTrace(llvm::StringRef Event, ExtValuePtr Val,
                        const SimpleType &Ty);
  void emitRemapTrace(llvm::StringRef Event, ExtValuePtr Val,
                      ExtValuePtr Target, const SimpleType &Ty);
  void emitMergeTrace(llvm::StringRef Event, SimpleType From, SimpleType To,
                      llvm::ArrayRef<ExtValuePtr> MovedValues);
  void emitPointerAnalysisTrace(const std::string &Message);

  public:
  // Create Node of both variance
  SimpleType createNode(ExtValuePtr Val);

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
    binarysub::Cache cache;
    binarysub::constrain(lhs, rhs, cache,
                         [this](const SimpleType &Lhs, const SimpleType &Rhs) {
                           maybeUnifyPNDiffTypeVariablePair(Lhs, Rhs);
                           observeOldMemoryTypeEdge(Lhs, Rhs);
                         });
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
    if (auto N = PG.getPNIVarOrNull(Val)) {
      N->setPtr();
    }
  }
  void setNonPointer(ExtValuePtr Val) {
    if (auto N = PG.getPNIVarOrNull(Val)) {
      N->setNonPtrIfRelated();
    }
  }
  unsigned getSize(ExtValuePtr Val) {
    return notdec::getSize(Val, PointerSize);
  }
  bool isPointerAnalysisEnabled() const {
    return PAMode != PointerAnalysisMode::Original;
  }
  bool isPointerAnalysisReplacingBinarysubMemory() const {
    return PAMode == PointerAnalysisMode::Replace;
  }
  bool shouldTracePointerAnalysis() const {
    return isPointerAnalysisEnabled() && TraceStream != nullptr;
  }
  MemoryLocKey withAccessSize(MemoryLocKey Loc, unsigned BitSize) const {
    Loc.BitSize = BitSize;
    return Loc;
  }
  MemoryLocKey getRootMemoryObject(ExtValuePtr Root, unsigned BitSize = 0) {
    return PA.getRootObject(Root, BitSize);
  }
  void addAddressOf(ExtValuePtr Dst, MemoryLocKey Obj) {
    if (isPointerAnalysisEnabled()) {
      PA.addAddrOf(Dst, Obj);
    }
  }
  void addPointerCopy(ExtValuePtr Dst, ExtValuePtr Src) {
    if (isPointerAnalysisEnabled()) {
      PA.addCopy(Dst, Src);
    }
  }
  void recordLoad(ExtValuePtr Addr, SimpleType ResultTy, unsigned BitSize,
                  llvm::Instruction *Source);
  void recordStore(ExtValuePtr Addr, SimpleType ValueTy, unsigned BitSize,
                   llvm::Instruction *Source);
  void addEVMConstantMemoryField(ExtValuePtr Addr, SimpleType ValueTy);
  void onPointsToDelta(ExtValuePtr Addr, MemoryLocKey Loc);
  void addPointerAccessViews();
  void flushPointerDerivedTypeConstraints();

  void onUpdatePNType(ExtValuePtr Val) {}
  void setAsPtrAdd(ExtValuePtr basePtr, ExtValuePtr result, OffsetRange Off) {
    auto BaseNode = getOrInsertNode(basePtr);
    auto ResultNode = getOrInsertNode(result);
    PG.unifyVar(basePtr, result);
    // Negative offsets can appear in pointer arithmetic, but they are not safe
    // to materialize as object fields. Keep the P/N relation and skip the field.
    if (PointerSize == 256 && Off.hasNegativeBaseOffset()) {
      return;
    }
    std::vector<std::pair<std::string, SimpleType>> fields;
    fields.emplace_back(Off.str(), ResultNode);
    addSubtype(BaseNode, binarysub::make_record(std::move(fields)));
    addSubtype(ResultNode, binarysub::make_record({}));
    if (isPointerAnalysisEnabled()) {
      PA.addField(result, basePtr, Off, getSize(result));
    }
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
  llvm::json::Value SummaryOverrideDoc = nullptr;
  std::set<llvm::Function *> SummaryOverrideFuncs;
  llvm::json::Value SignatureOverrideDoc = nullptr;
  std::set<llvm::Function *> SignatureOverrideFuncs;
  llvm::json::Value ExtraConstraintsDoc = nullptr;
  std::set<llvm::Function *> ExtraConstraintsFuncs;
  bool WrotePNDiffOverrideWarningHeader = false;
  // std::map<llvm::Function *, binarysub::TypeScheme> PolySchemes;
  SimpleType MemoryType = nullptr;

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

  void run();
  void loadSummaryFile(llvm::Module &M, const char *path,
                       bool StrictValidation = true);
  void loadSignatureFile(llvm::Module &M, const char *path,
                         bool StrictValidation = true);
  void validateExtraConstraintsFile(llvm::Module &M, const char *Path,
                                    llvm::StringRef ModuleSHA256Hex);
  void emitTRInputArtifacts(llvm::Module &M, llvm::StringRef OutputPath);
  const llvm::json::Value *getExtraConstraintsSpec(
      const llvm::Function &Func) const;
  const llvm::json::Value *getSummaryOverrideSpec(
      const llvm::Function &Func) const;
  bool isSummaryOverridePolymorphic(const llvm::Function &Func) const;
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
  std::unique_ptr<Result> &getResult(llvm::Module &M1,
                                     llvm::ModuleAnalysisManager &MAM) {
    if (ResultVal == nullptr) {
      genASTTypes(M1);
    }
    return ResultVal;
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
