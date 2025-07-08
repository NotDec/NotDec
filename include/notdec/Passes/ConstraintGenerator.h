#ifndef _NOTDEC_PASSES_RETYPD_GENERATE_H_
#define _NOTDEC_PASSES_RETYPD_GENERATE_H_

#include <cassert>
#include <clang/AST/Type.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <cstddef>
#include <deque>
#include <functional>
#include <iostream>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
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
#include "TypeRecovery/Sketch.h"
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

using namespace llvm;
using retypd::DerivedTypeVariable;
using retypd::TRContext;
using retypd::TypeVariable;

struct ConstraintsGenerator;

const char *getTRDebugDir();
std::optional<std::string> getSCCDebugDir(std::size_t SCCIndex);
std::optional<int64_t> getAllocSize(ExtValuePtr Val);
std::string getUniquePath(const std::string &basePath, const char *suffix);

struct SCCSignatureTypes {
  std::shared_ptr<ConstraintsGenerator> SignatureGenerator;
  std::map<llvm::Function *, CGNode *> FuncNodeMap;

  void instantiate(ConstraintsGenerator &To);
};

struct SCCTypeResult {
  std::map<ExtValuePtr, ast::HType *> ValueTypes;
  std::map<ExtValuePtr, ast::HType *> ValueTypesLowerBound;
};

struct SCCData {
  std::vector<CallGraphNode *> Nodes;
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
  std::map<CallGraphNode *, std::size_t> Func2SCCIndex;
  std::shared_ptr<ConstraintsGenerator> Global;
  std::shared_ptr<ConstraintsGenerator> GlobalSketch;
  std::map<CallGraphNode *,
           std::vector<std::pair<llvm::CallBase *, CallGraphNode *>>>
      FuncCallers;
  CallGraph *CG = nullptr;
};

struct TypeRecovery {
  // Specify the result type of this analysis pass.
  using Result = ::notdec::llvm2c::HTypeResult;

  const Module &Mod;
  std::shared_ptr<retypd::TRContext> TRCtx;
  std::shared_ptr<ast::HTypeContext> HTCtx;

  llvm::Value *StackPointer;
  std::string data_layout;

  // Map from SCC to initial constraint graph.
  std::map<llvm::Function *, std::shared_ptr<ConstraintsGenerator>>
      FuncSummaries;

  AllGraphs AG;

  std::shared_ptr<BytesManager> MemoryBytes;
  std::map<std::set<Function *>, std::shared_ptr<ConstraintsGenerator>>
      SummaryOverride;
  std::map<Function *, std::shared_ptr<ConstraintsGenerator>> SignatureOverride;
  std::map<CallBase *, std::shared_ptr<ConstraintsGenerator>>
      CallsiteSummaryOverride;
  std::unique_ptr<CallGraph> CallG;

  unsigned pointer_size = 0;

  std::unique_ptr<Result> ResultVal;
  void run(Module &M1, ModuleAnalysisManager &MAM);
  // on demand running
  std::unique_ptr<Result> &getResult(Module &M1, ModuleAnalysisManager &MAM) {
    assert((!AG.AllSCCs.empty()) && "function run() is not called!");
    if (ResultVal == nullptr) {
      genASTTypes(M1);
    }
    return ResultVal;
  }
  // Prepare topological order of SCC in AG.AllSCCs
  void prepareSCC(CallGraph &CG);
  void bottomUpPhase();
  std::shared_ptr<ConstraintsGenerator>
  getBottomUpGraph(SCCData &Data, std::optional<std::string> SCCDebugPath = std::nullopt);
  std::shared_ptr<ConstraintsGenerator> getTopDownGraph(SCCData &Data, std::optional<std::string> SCCDebugPath = std::nullopt);
  std::shared_ptr<SCCTypeResult> getASTTypes(SCCData &Data,
                                             std::optional<std::string> DebugDir = std::nullopt);
  void topDownPhase();
  void handleGlobals();
  void genASTTypes(Module &M);
  void gen_json(std::string OutputFilename);

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
               std::shared_ptr<ast::HTypeContext> HTCtx, Module &M)
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
  postProcess(ConstraintsGenerator &G, std::optional<std::string> DebugDir = std::nullopt);
  // merge nodes to current graph by determinize.
  static CGNode *multiGraphDeterminizeTo(ConstraintsGenerator &CurrentTypes,
                                         std::set<CGNode *> &StartNodes,
                                         const char *NamePrefix);
  void loadSummaryFile(Module &M, const char *path);
  void loadSignatureFile(Module &M, const char *path);
  void print(Module &M, std::string path);
  void printAnnotatedModule(const Module &M, std::string path, int level);
};

/// The ConstraintsGenerator class is responsible for generating constraints.
/// It represents a constraint graph and mappings from LLVM values to nodes in
/// the graph.
struct ConstraintsGenerator {
  TypeRecovery &Ctx;

  // LLVM值到那边节点的映射关系由转TypeVar的函数决定。这里其实只需要保证不重复，其次是保留名字的意义？
  // 因为转换过程有临时变量参与，所以这里到Key的映射要缓存。这里使用的就是Val2Node做缓存
  // std::map<ExtValuePtr, retypd::NodeKey> Val2Node;
  DSUMap<ExtValuePtr, retypd::NodeKey> V2N;
  DSUMap<ExtValuePtr, retypd::NodeKey> V2NContra;
  void removeNode(retypd::CGNode &N);
  void removeNode(const retypd::NodeKey &K);

  retypd::ConstraintGraph CG;
  retypd::PNIGraph *PG;
  std::set<llvm::Function *> SCCs;
  std::map<CallBase *, std::pair<CGNode *, CGNode *>> CallToInstance;
  // unhandled due to not having function body.
  // TODO: If reachable from function node, then it makes summary incorrect.
  std::map<CallBase *, std::pair<CGNode *, CGNode *>> UnhandledCalls;
  std::string DebugDir;

  retypd::CGNode &getNode(const retypd::NodeKey &Key) {
    auto *N = getNodeOrNull(Key);
    assert(N != nullptr && "getNode: Node not found");
    return *N;
  }

  retypd::CGNode *getNodeOrNull(const retypd::NodeKey &Key) {
    auto It2 = CG.Nodes.find(Key);
    if (It2 != CG.Nodes.end()) {
      return &It2->second;
    }
    return nullptr;
  }
  void addMergeNode(const retypd::NodeKey &From, const retypd::NodeKey &To) {
    V2N.merge(From, To);
    V2NContra.merge(From, To);
  }

  std::shared_ptr<ConstraintsGenerator> genSummary();
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
  std::map<std::set<CGNode *>, CGNode *> DTrans;

  void preSimplify();
  // void determinizeStructEqual();
  void eliminateCycle();
  void mergeOnlySubtype();
  void mergeAfterDeterminize();
  // determinization algorithm from all v2n nodes that keeps all reachable
  // nodes.
  void determinize();
  // remove nodes that is unreachable from nodes in Val2Node map.
  void removeUnreachable();
  void linkContraToCovariant();

  bool checkSymmetry();
  void makeSymmetry();

  void dumpV2N();

  std::map<CGNode *, TypeInfo> TypeInfos;
  std::map<CGNode *, TypeInfo> organizeTypes();
  void mergeArrayUnions();
  void elimSingleStruct();
  void mergeArrayWithMember();
  void mergeNodeAndType(CGNode &From, CGNode &To);
  void mergeFixTypeInfo(CGNode &From, CGNode &To);

  void run();
  // clone CG and maintain value map.
  ConstraintsGenerator clone(std::map<const CGNode *, CGNode *> &Old2New);
  void cloneTo(ConstraintsGenerator &Target,
               std::map<const CGNode *, CGNode *> &Old2New);
  std::shared_ptr<ConstraintsGenerator>
  cloneShared(std::map<const CGNode *, CGNode *> &Old2New);
  ConstraintsGenerator(TypeRecovery &Ctx, std::string Name,
                       std::set<llvm::Function *> SCCs = {})
      : Ctx(Ctx), CG(Ctx.TRCtx, Ctx.pointer_size, Name, false), PG(CG.PG.get()),
        SCCs(SCCs) {}

public:
  CGNode &addVarSubtype(llvm::Value *Val, CGNode &dtv) {
    auto &Node = getOrInsertNode(Val, nullptr, -1);
    addSubtype(dtv, Node);
    return Node;
  }
  void addSubtype(CGNode &SubNode, CGNode &SupNode) {
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
    CG.addConstraint(SubNode, SupNode);
  }
  std::map<const CGEdge *, const CGEdge *> mergeNodeTo(CGNode &From, CGNode &To,
                                                       bool NoSelfLoop = false);

  void setPointer(CGNode &Node) { CG.setPointer(Node); }

  retypd::CGNode &getNode(ExtValuePtr Val, User *User, long OpInd,
                          retypd::Variance V);
  const retypd::CGNode &getNode(ExtValuePtr Val, User *User, long OpInd,
                                retypd::Variance V) const {
    return const_cast<ConstraintsGenerator *>(this)->getNode(Val, User, OpInd,
                                                             V);
  }
  retypd::CGNode *getNodeOrNull(ExtValuePtr Val, User *User, long OpInd,
                                retypd::Variance V);
  const retypd::CGNode *getNodeOrNull(ExtValuePtr Val, User *User, long OpInd,
                                      retypd::Variance V) const {
    return const_cast<ConstraintsGenerator *>(this)->getNodeOrNull(Val, User,
                                                                   OpInd, V);
  }
  // Create Node of both variance
  std::pair<retypd::CGNode &, retypd::CGNode &>
  createNode(ExtValuePtr Val, User *User, long OpInd);
  retypd::CGNode &createNodeCovariant(ExtValuePtr Val, User *User, long OpInd) {
    auto [N, NC] = createNode(Val, User, OpInd);
    return N;
  }
  retypd::CGNode &getOrInsertNode(ExtValuePtr Val, User *User, long OpInd,
                                  retypd::Variance V = retypd::Covariant);

  const TypeVariable &getTypeVar(ExtValuePtr val, User *User, long OpInd);
  // convert the value to a type variable.
  TypeVariable convertTypeVar(ExtValuePtr Val, User *User = nullptr,
                              long OpInd = -1);
  TypeVariable convertTypeVarVal(Value *Val, User *User = nullptr,
                                 long OpInd = -1);
  void addAddConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        BinaryOperator *Result);
  void addSubConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        BinaryOperator *Result);
  void addCmpConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        ICmpInst *I);
  // void onEraseConstraint(const retypd::ConsNode *Cons);
  // void addSubTypeCons(retypd::SSGNode *LHS, retypd::SSGNode *RHS,
  //                     OffsetRange Offset);
  // void addSubTypeCons(llvm::Value *LHS, llvm::BinaryOperator *RHS,
  //                     OffsetRange Offset);

  TypeVariable addOffset(TypeVariable &dtv, OffsetRange Offset);
  TypeVariable deref(Value *Val, User *User, long OpInd, unsigned BitSize,
                     bool isLoad);
  unsigned getPointerElemSize(Type *ty);
  static inline bool is_cast(Value *Val) {
    return llvm::isa<AddrSpaceCastInst, BitCastInst, PtrToIntInst,
                     IntToPtrInst>(Val);
  }
  static std::string offset(APInt Offset, int Count = 0);

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
    bool addRetConstraint(Instruction *I, ConstraintsGenerator &cg) const;
    bool addOpConstraint(unsigned Index, Instruction *I,
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

    bool handleIntrinsicCall(CallBase &I);
    // overloaded visit functions
    void visitExtractValueInst(ExtractValueInst &I);
    void visitCastInst(CastInst &I);
    void visitCallBase(CallBase &I);
    void visitReturnInst(ReturnInst &I);
    void visitPHINode(PHINode &I);
    void visitLoadInst(LoadInst &I);
    void visitStoreInst(StoreInst &I);
    void visitAllocaInst(AllocaInst &I);
    void visitGetElementPtrInst(GetElementPtrInst &I);
    void visitICmpInst(ICmpInst &I);
    void visitSelectInst(SelectInst &I);

    void visitAdd(BinaryOperator &I);
    void visitSub(BinaryOperator &I);

    // handle sth like
    // 1. Alignment/ Use lowest bits in pointer: And %x, 0xfffffff0.
    // 2. set lowest bits in the pointer: Or %x, 0x7
    void visitAnd(BinaryOperator &I);
    void visitOr(BinaryOperator &I);

    // ignore control flow related instructions
    void visitUnreachableInst(UnreachableInst &I) {}
    void visitBranchInst(BranchInst &I) {}
    void visitSwitchInst(SwitchInst &I) {}

    void handlePHINodes();
    // use opTypes to handle other insts.
    void visitInstruction(Instruction &I);
  };
};

void inline ensureSequence(Value *&Src1, Value *&Src2) {
  if (isa<ConstantInt>(Src1) && isa<ConstantInt>(Src2)) {
    assert(false && "Constant at both sides. Run Optimization first!");
  }
  if (isa<ConstantInt>(Src1) && !isa<ConstantInt>(Src2)) {
    // because of InstCombine canonical form, this should not happen?
    assert(false &&
           "Constant cannot be at the left side. Run InstCombine first.");
    std::swap(Src1, Src2);
  }
}

std::string inline getFuncTvName(llvm::Function *Func) {
  return ValueNamer::getName(*Func, ValueNamer::FuncPrefix);
}

inline TypeVariable getCallArgTV(CGNode &Target, int32_t Index) {
  TypeVariable TV = Target.key.Base;
  return TV.pushLabel({retypd::InLabel{std::to_string(Index)}});
}

inline TypeVariable getCallRetTV(CGNode &Target) {
  TypeVariable TV = Target.key.Base;
  return TV.pushLabel({retypd::OutLabel{}});
}

// #region FunctionTypeRecovery

struct TypeRecoveryMain : PassInfoMixin<TypeRecoveryMain> {

  TypeRecovery &TR;
  TypeRecoveryMain(TypeRecovery &TR) : TR(TR) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
    TR.run(M, MAM);
    return PreservedAnalyses::all();
  }
};

struct TypeRecoveryOpt : PassInfoMixin<TypeRecoveryOpt> {

  TypeRecovery &TR;
  TypeRecoveryOpt(TypeRecovery &TR) : TR(TR) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

struct InvalidateAllTypes : PassInfoMixin<InvalidateAllTypes> {
  TypeRecovery &TR;
  InvalidateAllTypes(TypeRecovery &TR) : TR(TR) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
    for (auto &Data : TR.AG.AllSCCs) {
      Data.onIRChanged();
    }
    return PreservedAnalyses::all();
  }
};

struct FunctionTypeRecovery : AnalysisInfoMixin<FunctionTypeRecovery> {

  struct FuncTypeResult {};

  // TODO
  using Result = FuncTypeResult;
  static inline llvm::AnalysisKey Key; // NOLINT
  friend llvm::AnalysisInfoMixin<FunctionTypeRecovery>;

  TypeRecovery &TR;
  FunctionTypeRecovery(TypeRecovery &TR) : TR(TR) {}

  Result run(Function &F, FunctionAnalysisManager &FAM) {
    // TODO, if function generator is not invalidated.
    return {};
  }
};

// #endregion FunctionTypeRecovery

} // namespace notdec

#endif
