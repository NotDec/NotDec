#ifndef _NOTDEC_PASSES_RETYPD_GENERATE_H_
#define _NOTDEC_PASSES_RETYPD_GENERATE_H_

#include <cassert>
#include <clang/AST/Type.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/FormattedStream.h>

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/DotSummaryParser.h"
#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/PointerNumberIdentification.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/TRContext.h"
#include "Utils/DSUMap.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"

#ifdef NOTDEC_ENABLE_LLVM2C
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Interface/ExtValuePtr.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#endif

namespace notdec {

using retypd::CGEdge;
using retypd::CGNode;
using retypd::DerivedTypeVariable;
using retypd::FieldLabel;
using retypd::TRContext;
using retypd::TypeVariable;

struct ConstraintsGenerator;

const char *getTRDebugDir();
std::optional<std::string> getSCCDebugDir(std::size_t SCCIndex);
std::optional<int64_t> getAllocSize(ExtValuePtr Val);
std::string getUniquePath(const std::string &basePath, const char *suffix);

struct SCCSignatureTypes {
  std::shared_ptr<ConstraintsGenerator> SignatureGenerator;
  std::map<llvm::Function *, retypd::CGNode *> FuncNodeMap;

  void instantiate(ConstraintsGenerator &To);
};

struct SCCTypeResult {
  std::map<ExtValuePtr, ast::HType *> ValueTypes;
  std::map<ExtValuePtr, ast::HType *> ValueTypesLowerBound;
};

struct SCCData {
  std::vector<llvm::CallGraphNode *> Nodes;
  std::string SCCName;
  std::set<llvm::Function *> SCCSet;
  SCCSignatureTypes SigTy;
  std::shared_ptr<ConstraintsGenerator> BottomUpGenerator;
  std::shared_ptr<ConstraintsGenerator> TopDownGenerator;
  std::shared_ptr<ConstraintsGenerator> SketchGenerator;
  std::shared_ptr<SCCTypeResult> TypeResult;

  void onIRChanged() {
    BottomUpGenerator.reset();
    TopDownGenerator.reset();
    SketchGenerator.reset();
    TypeResult.reset();
  }
};

struct AllGraphs {
  std::vector<SCCData> AllSCCs;
  std::map<llvm::CallGraphNode *, std::size_t> Func2SCCIndex;
  // Graph for global variables.
  std::shared_ptr<ConstraintsGenerator> Global;
  // Sketch graph for global variables.
  std::shared_ptr<ConstraintsGenerator> GlobalSketch;
  std::map<llvm::CallGraphNode *,
           std::vector<std::pair<llvm::CallBase *, llvm::CallGraphNode *>>>
      FuncCallers;
  llvm::CallGraph *CG = nullptr;
};

struct TypeRecovery {
  // Specify the result type of this analysis pass.
  using Result = ::notdec::llvm2c::HTypeResult;

  const llvm::Module &Mod;
  std::shared_ptr<retypd::TRContext> TRCtx;
  std::shared_ptr<ast::HTypeContext> HTCtx;

  llvm::Value *StackPointer;
  std::string data_layout;

  // Map from SCC to initial constraint graph.
  std::map<llvm::Function *, std::shared_ptr<ConstraintsGenerator>>
      FuncSummaries;

  AllGraphs AG;

  std::shared_ptr<BytesManager> MemoryBytes;
  std::map<std::set<llvm::Function *>, std::shared_ptr<ConstraintsGenerator>>
      SummaryOverride;
  std::map<llvm::Function *, std::shared_ptr<ConstraintsGenerator>>
      SignatureOverride;
  std::map<llvm::CallBase *, std::shared_ptr<ConstraintsGenerator>>
      CallsiteSummaryOverride;
  std::unique_ptr<llvm::CallGraph> CallG;
  // for recreating alloca range that is eliminated as dead code.
  std::unique_ptr<std::map<llvm::Function *,
                           std::vector<std::pair<SimpleRange, std::string>>>>
      AllocaRanges;
  bool hasAllocaRanges() { return AllocaRanges != nullptr; }
  std::vector<std::pair<SimpleRange, std::string>> &
  getOrCreateFuncAllocaRange(llvm::Function *Func) {
    assert(Func != nullptr);
    if (AllocaRanges == nullptr) {
      AllocaRanges = std::make_unique<
          std::map<llvm::Function *,
                   std::vector<std::pair<SimpleRange, std::string>>>>();
    }
    return (*AllocaRanges)[Func];
  }

  std::function<bool(llvm::Function *)> isPolymorphic = [](llvm::Function *F) {
    if (auto Env = std::getenv("NOTDEC_DEFAULT_POLY")) {
      if (std::strcmp(Env, "1")) {
        return true;
      }
    }
    return false;
  };

  unsigned pointer_size = 0;

  std::unique_ptr<Result> ResultVal;
  void run(llvm::Module &M1, llvm::ModuleAnalysisManager &MAM);
  // on demand running
  std::unique_ptr<Result> &getResult(llvm::Module &M1,
                                     llvm::ModuleAnalysisManager &MAM) {
    assert((!AG.AllSCCs.empty()) && "function run() is not called!");
    if (ResultVal == nullptr) {
      genASTTypes(M1);
    }
    return ResultVal;
  }
  // Prepare topological order of SCC in AG.AllSCCs
  void prepareSCC(llvm::CallGraph &CG);
  void bottomUpPhase();
  std::shared_ptr<ConstraintsGenerator>
  getBottomUpGraph(SCCData &Data,
                   std::optional<std::string> SCCDebugPath = std::nullopt);
  std::shared_ptr<ConstraintsGenerator>
  getTopDownGraph(SCCData &Data,
                  std::optional<std::string> SCCDebugPath = std::nullopt);
  std::shared_ptr<ConstraintsGenerator>
  getSketchGraph(SCCData &Data,
                 std::optional<std::string> SCCDebugPath = std::nullopt);
  std::shared_ptr<SCCTypeResult>
  getASTTypes(SCCData &Data,
              std::optional<std::string> DebugDir = std::nullopt);
  void topDownPhase();
  void handleGlobals();
  void genASTTypes(llvm::Module &M);

  // NOTDEC_SUMMARY_OVERRIDE
  const char *SummaryFile;
  // NOTDEC_SIGNATURE_OVERRIDE
  const char *SigFile;
  // NOTDEC_TYPE_RECOVERY_TRACE_IDS
  const char *Traces;
  // NOTDEC_TYPE_RECOVERY_NO_SCC
  bool NoSCC = false;
  llvm::Optional<llvm::raw_fd_ostream> SCCsCatalog;

  TypeRecovery(std::shared_ptr<retypd::TRContext> TRCtx,
               std::shared_ptr<ast::HTypeContext> HTCtx, llvm::Module &M)
      : Mod(M), TRCtx(TRCtx), HTCtx(HTCtx),
        SummaryFile(std::getenv("NOTDEC_SUMMARY_OVERRIDE")),
        SigFile(std::getenv("NOTDEC_SIGNATURE_OVERRIDE")),
        Traces(std::getenv("NOTDEC_TYPE_RECOVERY_TRACE_IDS")) {
    if (const char *path = std::getenv("NOTDEC_TYPE_RECOVERY_NO_SCC")) {
      if ((std::strcmp(path, "1") == 0)) {
        llvm::errs() << "Warning: Splitting SCCs to single function!\n";
        NoSCC = true;
      }
    }
  }

  static std::shared_ptr<ConstraintsGenerator>
  postProcess(ConstraintsGenerator &G,
              std::optional<std::string> DebugDir = std::nullopt);
  // merge nodes to current graph by determinize.
  static retypd::CGNode *
  multiGraphDeterminizeTo(ConstraintsGenerator &CurrentTypes,
                          std::set<retypd::CGNode *> &StartNodes,
                          const char *NamePrefix);
  void loadSummaryFile(llvm::Module &M, const char *path);
  void loadSignatureFile(llvm::Module &M, const char *path);
  void print(llvm::Module &M, std::string path);
  void printAnnotatedModule(const llvm::Module &M, std::string path, int level);
};

/// The ConstraintsGenerator class is responsible for generating constraints.
/// It represents a constraint graph and mappings from LLVM values to nodes in
/// the graph.
struct ConstraintsGenerator {
  TypeRecovery &Ctx;

  // LLVM值到那边节点的映射关系由转TypeVar的函数决定。这里其实只需要保证不重复，其次是保留名字的意义？
  // 因为转换过程有临时变量参与，所以这里到Key的映射要缓存。这里使用的就是Val2Node做缓存
  // std::map<ExtValuePtr, retypd::NodeKey> Val2Node;
  // TODO
  // 使用getPreferredVariance合并两个map。key是ExtValuePtr带上variance的pair。
  DSUMap<ExtValuePtr, CGNode *> V2N;
  DSUMap<ExtValuePtr, CGNode *> V2NContra;
  void removeNode(retypd::CGNode &N);
  // void removeNode(const retypd::NodeKey &K);

  retypd::ConstraintGraph CG;
  retypd::PNIGraph *PG;
  std::set<llvm::Function *> SCCs;
  std::map<llvm::CallBase *, std::pair<retypd::CGNode *, retypd::CGNode *>>
      CallToInstance;
  // unhandled due to not having function body.
  // TODO: If reachable from function node, then it makes summary incorrect.
  std::map<llvm::CallBase *, std::pair<retypd::CGNode *, retypd::CGNode *>>
      UnhandledCalls;

  DSUMap<std::pair<std::string, llvm::Type *>, CGNode *> PrimMap;

  CGNode &getOrCreatePrim(std::string Name, llvm::Type *LowType) {
    auto Key = std::make_pair(Name, LowType);
    auto It = PrimMap.find(Key);
    if (It != PrimMap.end()) {
      return *It->second;
    } else {
      auto [N, NC] = CG.createNodePair(
          retypd::NodeKey(TypeVariable::CreatePrimitive(*CG.Ctx, Name),
                          retypd::Covariant),
          LowType);
      PrimMap.insert(Key, &N);
      return N;
    }
  }

  bool isPrimitive(const CGNode &N) const {
    return const_cast<ConstraintsGenerator *>(this)->PrimMap.count(
        const_cast<CGNode *>(&N));
  }

  // retypd::CGNode &getNode(const retypd::NodeKey &Key) {
  //   auto *N = getNodeOrNull(Key);
  //   assert(N != nullptr && "getNode: Node not found");
  //   return *N;
  // }

  void addMergeNode(CGNode &From, CGNode &To) {
    V2N.merge(&From, &To);
    V2NContra.merge(&From, &To);
  }

  std::shared_ptr<ConstraintsGenerator>
  genSummary(std::optional<std::string> DebugDir = std::nullopt);
  void fixSCCFuncMappings();
  static std::shared_ptr<ConstraintsGenerator>
  fromDotSummary(TypeRecovery &Ctx, std::set<llvm::Function *> SCCs,
                 retypd::DotGraph &G);
  static std::shared_ptr<ConstraintsGenerator>
  fromConstraints(TypeRecovery &Ctx, std::set<llvm::Function *> SCCs,
                  const retypd::ConstraintSummary &Summary);
  void instantiateSummary(llvm::CallBase *Inst, llvm::Function *Target,
                          const ConstraintsGenerator &Summary);
  // std::shared_ptr<retypd::Sketch> solveType(const TypeVariable &Node);
  // void instantiateSketchAsSub(ExtValuePtr Val,
  //                             std::shared_ptr<retypd::Sketch> Sk);
  // void instantiateSketchAsSup(ExtValuePtr Val,
  //                             std::shared_ptr<retypd::Sketch> Sk);

  // for determinization: extended powerset construction
  std::map<std::set<retypd::CGNode *>, retypd::CGNode *> DTrans;

  void preSimplify();
  // void determinizeStructEqual();
  void eliminateCycle(std::optional<std::string> DebugDir = std::nullopt);
  void mergeOnlySubtype();
  void mergeAfterDeterminize();
  // determinization algorithm from all v2n nodes that keeps all reachable
  // nodes.
  void determinize();
  // minimize to reduce type node count after determinization.
  std::shared_ptr<ConstraintsGenerator>
  minimizeShared(std::map<const retypd::CGNode *, retypd::CGNode *> &Old2New);
  void minimizeTo(ConstraintsGenerator &Target,
                  std::map<const retypd::CGNode *, retypd::CGNode *> &Old2New);
  void linkNodes();
  void recoverNodes(ConstraintsGenerator &From);
  // remove nodes that is unreachable from nodes in Val2Node map.
  void removeUnreachable();
  void linkContraToCovariant();

  bool checkSymmetry();
  // void makeSymmetry();

  void dumpV2N();

  std::map<retypd::CGNode *, TypeInfo> TypeInfos;
  std::map<retypd::CGNode *, TypeInfo> organizeTypes();
  void mergeArrayUnions();
  void elimSingleStruct();
  void mergeArrayWithMember();
  void mergeNodeAndType(retypd::CGNode &From, retypd::CGNode &To);
  void mergeFixTypeInfo(retypd::CGNode &From, retypd::CGNode &To);

  void run();
  // clone CG and maintain value map.
  // ConstraintsGenerator
  // clone(std::map<const retypd::CGNode *, retypd::CGNode *> &Old2New);
  void cloneTo(ConstraintsGenerator &Target,
               std::map<const retypd::CGNode *, retypd::CGNode *> &Old2New,
               bool isMergeClone = false,
               std::function<retypd::ConstraintGraph::SubtypeRelation(
                   retypd::CGNode &, retypd::CGNode &)>
                   ConflictKeyRelation = nullptr);
  std::shared_ptr<ConstraintsGenerator>
  cloneShared(std::map<const retypd::CGNode *, retypd::CGNode *> &Old2New,
              bool isMergeClone = false);
  ConstraintsGenerator(TypeRecovery &Ctx, std::string Name,
                       std::set<llvm::Function *> SCCs = {})
      : Ctx(Ctx), CG(Ctx.TRCtx, Ctx.pointer_size, Name, false), PG(CG.PG.get()),
        SCCs(SCCs) {}

public:
  retypd::CGNode &addVarSubtype(llvm::Value *Val, retypd::CGNode &dtv) {
    auto &Node = getOrInsertNode(Val, nullptr, -1);
    addSubtype(dtv, Node);
    return Node;
  }

  const CGEdge *addConstraint(CGNode &From, CGNode &To,
                              retypd::FieldLabel Label) {
    return CG.addRecallEdge(From, To, Label);
  }

  void addSubtype(retypd::CGNode &SubNode, retypd::CGNode &SupNode) {
    assert(SubNode.getVariance() == SupNode.getVariance());
    auto &Sub = SubNode.key.Base;
    auto &Sup = SupNode.key.Base;
    if (Sub.isPrimitive() && Sup.isPrimitive()) {
      // TODO check if this is correct
      if (Sub.getPrimitiveName() != Sup.getPrimitiveName()) {
        std::cerr << "addSubtype: relation between primitive types: "
                  << toString(Sub) << " <= " << toString(Sup) << "\n";
        // assert(sub.getPrimitiveName() == sup.getPrimitiveName() &&
        //        "addConstraint: different primitive types !?");
      }
      return;
    }
    CG.addEdgeDualVariance(SubNode, SupNode, {retypd::One{}});
  }

  retypd::CGNode &getNode(ExtValuePtr Val, llvm::User *User, long OpInd,
                          retypd::Variance V);
  const retypd::CGNode &getNode(ExtValuePtr Val, llvm::User *User, long OpInd,
                                retypd::Variance V) const {
    return const_cast<ConstraintsGenerator *>(this)->getNode(Val, User, OpInd,
                                                             V);
  }

  retypd::CGNode *getNodeOrNull(ExtValuePtr Val, llvm::User *User, long OpInd,
                                retypd::Variance V);
  const retypd::CGNode *getNodeOrNull(ExtValuePtr Val, llvm::User *User,
                                      long OpInd, retypd::Variance V) const {
    return const_cast<ConstraintsGenerator *>(this)->getNodeOrNull(Val, User,
                                                                   OpInd, V);
  }

  // Create Node of both variance
  std::pair<retypd::CGNode &, retypd::CGNode &>
  createNode(ExtValuePtr Val, llvm::User *User, long OpInd);
  retypd::CGNode &createNodeCovariant(ExtValuePtr Val, llvm::User *User,
                                      long OpInd) {
    auto [N, NC] = createNode(Val, User, OpInd);
    return N;
  }

  retypd::CGNode &getOrInsertNode(ExtValuePtr Val, llvm::User *User, long OpInd,
                                  retypd::Variance V = retypd::Covariant);

  const TypeVariable &getTypeVar(ExtValuePtr val, llvm::User *User, long OpInd);
  // convert the value to a type variable.
  TypeVariable convertTypeVar(ExtValuePtr Val, llvm::User *User = nullptr,
                              long OpInd = -1);
  TypeVariable convertTypeVarVal(Value *Val, llvm::User *User = nullptr,
                                 long OpInd = -1);
  void addAddConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        llvm::BinaryOperator *Result);
  void addSubConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        llvm::BinaryOperator *Result);
  void addCmpConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        llvm::ICmpInst *I);

  TypeVariable addOffset(TypeVariable &dtv, OffsetRange Offset);
  unsigned getPointerElemSize(llvm::Type *ty);
  static inline bool is_cast(Value *Val) {
    return llvm::isa<llvm::AddrSpaceCastInst, llvm::BitCastInst,
                     llvm::PtrToIntInst, llvm::IntToPtrInst>(Val);
  }

  std::map<const retypd::CGEdge *, const retypd::CGEdge *>
  mergeNodeTo(retypd::CGNode &From, retypd::CGNode &To,
              bool NoSelfLoop = false);

  void setPointer(retypd::CGNode &Node) { CG.setPointer(Node); }

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
  class RetypdGeneratorVisitor
      : public llvm::InstVisitor<RetypdGeneratorVisitor> {
    ConstraintsGenerator &cg;
    // defer phi node constraints
    std::vector<llvm::PHINode *> phiNodes;

  public:
    RetypdGeneratorVisitor(ConstraintsGenerator &cg) : cg(cg) {}

    bool handleIntrinsicCall(llvm::CallBase &I);
    // overloaded visit functions
    void visitExtractValueInst(llvm::ExtractValueInst &I);
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

void inline ensureSequence(Value *&Src1, Value *&Src2) {
  if (llvm::isa<llvm::ConstantInt>(Src1) &&
      llvm::isa<llvm::ConstantInt>(Src2)) {
    assert(false && "Constant at both sides. Run Optimization first!");
  }
  if (llvm::isa<llvm::ConstantInt>(Src1) &&
      !llvm::isa<llvm::ConstantInt>(Src2)) {
    // because of InstCombine canonical form, this should not happen?
    assert(false &&
           "Constant cannot be at the left side. Run InstCombine first.");
    std::swap(Src1, Src2);
  }
}

std::string inline getFuncTvName(llvm::Function *Func) {
  if (Func->isIntrinsic()) {
    return llvm::Intrinsic::getBaseName(Func->getIntrinsicID()).str();
  }
  return ValueNamer::getName(*Func, ValueNamer::FuncPrefix);
}

inline TypeVariable getCallArgTV(TypeVariable &TV, int32_t Index) {
  return TV.pushLabel({retypd::InLabel{std::to_string(Index)}});
}

inline TypeVariable getCallRetTV(TypeVariable &TV) {
  return TV.pushLabel({retypd::OutLabel{}});
}

inline FieldLabel getCallArgLabel(int32_t Index) {
  return {retypd::InLabel{std::to_string(Index)}};
}

inline FieldLabel getCallRetLabel() { return {retypd::OutLabel{}}; }

// #region FunctionTypeRecovery

struct TypeRecoveryMain : llvm::PassInfoMixin<TypeRecoveryMain> {

  TypeRecovery &TR;
  TypeRecoveryMain(TypeRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {
    TR.run(M, MAM);
    return llvm::PreservedAnalyses::all();
  }
};

// Currently only break stack.
struct TypeRecoveryOpt : llvm::PassInfoMixin<TypeRecoveryOpt> {

  TypeRecovery &TR;
  TypeRecoveryOpt(TypeRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
};

// re-create alloca according to data in TR.
struct RecoverDeadAlloca : llvm::PassInfoMixin<RecoverDeadAlloca> {

  TypeRecovery &TR;
  RecoverDeadAlloca(TypeRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
  void recoverAlloca(llvm::Function &F,
                     std::vector<std::pair<SimpleRange, std::string>> &Vec);
};

struct InvalidateAllTypes : llvm::PassInfoMixin<InvalidateAllTypes> {
  TypeRecovery &TR;
  InvalidateAllTypes(TypeRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {
    for (auto &Data : TR.AG.AllSCCs) {
      Data.onIRChanged();
    }
    const char *DebugDir = getTRDebugDir();
    if (DebugDir) {
      printModule(M, join(DebugDir, "TRFinal-InvalidateAllTypes.ll").c_str());
    }
    return llvm::PreservedAnalyses::all();
  }
};

struct FunctionTypeRecovery : llvm::AnalysisInfoMixin<FunctionTypeRecovery> {

  struct FuncTypeResult {};

  // TODO
  using Result = FuncTypeResult;
  static inline llvm::AnalysisKey Key; // NOLINT
  friend llvm::AnalysisInfoMixin<FunctionTypeRecovery>;

  TypeRecovery &TR;
  FunctionTypeRecovery(TypeRecovery &TR) : TR(TR) {}

  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM) {
    // TODO, if function generator is not invalidated.
    return {};
  }
};

// #endregion FunctionTypeRecovery

inline CGNode &getTarget(FieldEntry &F) {
  return const_cast<CGNode &>(F.Edge->getTargetNode());
}

} // namespace notdec

#endif
