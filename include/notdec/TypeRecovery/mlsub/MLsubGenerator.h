#ifndef _BINARYSUB_MLSUBGENERATOR_H_
#define _BINARYSUB_MLSUBGENERATOR_H_

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
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
#include <llvm/Support/Casting.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/JSON.h>

#include <clang/AST/Type.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>

#include "TypeRecovery/mlsub/MLsubGraph.h"
#include "TypeRecovery/mlsub/PNDiff.h"
#include "binarysub/binarysub-core.h"

#ifdef NOTDEC_ENABLE_LLVM2C
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Interface/ExtValuePtr.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"
#endif

namespace notdec::mlsub {

using binarysub::SimpleType;

struct ConstraintsGenerator;

struct ConstraintsGenerator {
  long PointerSize = 0;
  std::string Name;
  // ConstraintGraph CG;
  PNIGraph PG;
  const std::set<llvm::Function *> &SCCs;
  int lvl = 0;
  SimpleType MemoryType = nullptr;

  DSUMap<ExtValuePtr, SimpleType> V2N;
  std::map<ExtValuePtr, ast::HType *> ValueTypes;
  std::map<llvm::CallBase *, SimpleType> unhandledCalls;
  std::set<ExtValuePtr> ContraVariantValues;
  std::set<ExtValuePtr> SnapshotContraVariantValues;
  bool EnablePNDiffTypeVariableClosureUnification = true;
  std::ostream *TraceStream = nullptr;

  void addMergeNode(SimpleType From, SimpleType To) { V2N.merge(From, To); }

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
  }

  void run() {
    for (const llvm::Function *Func1 : SCCs) {
      auto Func = const_cast<llvm::Function *>(Func1);
      // create function nodes
      auto F = createNode(Func, nullptr, -1);
      std::vector<SimpleType> Args;
      for (unsigned i = 0; i < Func->arg_size(); ++i) {
        auto Arg = createNode(Func->getArg(i), nullptr, i);
        // Contra-variant.
        Args.push_back(Arg);
      }
      SimpleType Ret = nullptr;
      if (!Func->getReturnType()->isVoidTy()) {
        Ret = createNode(ReturnValue{.Func = Func}, nullptr, -1);
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
      auto F = getNodeOrNull(Func, nullptr, -1);
      assert(F->getAsVariableState() != nullptr);
    }
    PG.solve();
  }
  void genTypes(ast::HTypeContext &HCtx, const llvm::DataLayout &DL,
                bool SolveMemory = false);
  void releaseBinarysubState();

  SimpleType convertSimpleType(ExtValuePtr Val, llvm::User *User, long OpInd);
  SimpleType convertSimpleTypeVal(Value *Val, llvm::User *User, long OpInd);
  void maybeUnifyPNDiffTypeVariablePair(const SimpleType &Lhs,
                                        const SimpleType &Rhs);

public:
  // Create Node of both variance
  SimpleType createNode(ExtValuePtr Val, llvm::User *User, long OpInd) {
    llvmValue2ExtVal(Val, User, OpInd);
    // auto N = binarysub::make_variable(lvl);
    auto N = convertSimpleType(Val, User, OpInd);
    auto It = V2N.insert(Val, N);
    if (!It.second) {
      llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                   << "createNode: Value already mapped to "
                   << It.first->second->str() << ", but now set to "
                   << toString(Val) << "\n";
      std::abort();
    }
    // Create PNI
    PG.getOrInsertPNINode(Val, User, OpInd);
    // if the value is constant addr, we set ptr and link to memory
    if (std::get_if<ConstantAddr>(&Val)) {
      setPointer(Val, User, OpInd);
    }
    return N;
  }

  SimpleType getNodeOrNull(ExtValuePtr Val, llvm::User *User, long OpInd) {
    llvmValue2ExtVal(Val, User, OpInd);

    if (V2N.count(Val)) {
      return V2N.at(Val);
    }
    return nullptr;
  }

  SimpleType getOrInsertNode(ExtValuePtr Val, llvm::User *User, long OpInd) {
    llvmValue2ExtVal(Val, User, OpInd);
    auto Node = getNodeOrNull(Val, User, OpInd);
    if (Node != nullptr) {
      return Node;
    }
    auto N = createNode(Val, User, OpInd);
    return N;
  }

  void addSubtype(SimpleType lhs, SimpleType rhs) {
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    binarysub::Cache cache;
    binarysub::constrain(lhs, rhs, cache,
                         [this](const SimpleType &Lhs, const SimpleType &Rhs) {
                           maybeUnifyPNDiffTypeVariablePair(Lhs, Rhs);
                         });
  }

  SimpleType addVarSubtype(llvm::Value *Val, SimpleType dtv) {
    auto Node = getOrInsertNode(Val, nullptr, -1);
    addSubtype(dtv, Node);
    return Node;
  }

  SimpleType addRemapType(ExtValuePtr Val, llvm::User *User, long OpInd,
                          SimpleType ty) {
    assert(ty != nullptr);
    auto N = getNodeOrNull(Val, User, OpInd);
    if (N == ty) {
      return N;
    }
    auto It = V2N.insert(Val, ty);
    if (!It.second) {
      llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                   << "setTypeVar: Value already mapped to "
                   << It.first->second->str() << ", but now set to "
                   << toString(Val) << "\n";
      std::abort();
    }
    return It.first->second;
  }

  unsigned getPointerElemSize(llvm::Type *ty);
  static inline bool is_cast(Value *Val) {
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

  void setPointer(ExtValuePtr Val, llvm::User *User, long OpInd) {
    llvmValue2ExtVal(Val, User, OpInd);
    if (auto N = PG.getPNIVarOrNull(Val)) {
      N->setPtr();
    }
  }
  void setNonPointer(ExtValuePtr Val, llvm::User *User, long OpInd) {
    llvmValue2ExtVal(Val, User, OpInd);
    if (auto N = PG.getPNIVarOrNull(Val)) {
      N->setNonPtrIfRelated();
    }
  }
  unsigned getSize(ExtValuePtr Val, llvm::User *User = nullptr, long OpInd = -1) {
    llvmValue2ExtVal(Val, User, OpInd);
    return notdec::getSize(Val, PointerSize);
  }

  void onUpdatePNType(ExtValuePtr Val) {}
  void setAsPtrAdd(ExtValuePtr basePtr, ExtValuePtr result, OffsetRange Off) {
    auto BaseNode = getOrInsertNode(basePtr, nullptr, -1);
    auto ResultNode = getOrInsertNode(result, nullptr, -1);
    std::vector<std::pair<std::string, SimpleType>> fields;
    fields.push_back({Off.str(), ResultNode});
    addSubtype(BaseNode, binarysub::make_record(std::move(fields)));
    addSubtype(ResultNode, binarysub::make_record({}));
    PG.unifyVar(basePtr, result);
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
  llvm::Optional<llvm::raw_fd_ostream> SCCsCatalog;
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
  const llvm::json::Value *getExtraConstraintsSpec(
      const llvm::Function &Func) const;
  const llvm::json::Value *getSummaryOverrideSpec(
      const llvm::Function &Func) const;
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
