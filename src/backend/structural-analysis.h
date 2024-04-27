#ifndef _NOTDEC_BACKEND_STRUCTURAL_H_
#define _NOTDEC_BACKEND_STRUCTURAL_H_

#include "backend/CFG.h"
#include "optimizers/retdec-stack/retdec-utils.h"
#include "utils.h"
#include <cassert>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Analysis/CFG.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalObject.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/ErrorHandling.h>
#include <map>
#include <memory>
#include <vector>

namespace notdec::backend {

// main interface
void decompileModule(llvm::Module &M, llvm::raw_fd_ostream &os);
std::string printBasicBlock(const llvm::BasicBlock *b);
std::string printFunction(const llvm::Function *F);

// utility functions
bool onlyUsedInBlock(llvm::Instruction &inst);
bool usedInBlock(llvm::Instruction &inst, llvm::BasicBlock &bb);
clang::DeclRefExpr *
makeDeclRefExpr(clang::ValueDecl *D,
                bool RefersToEnclosingVariableOrCapture = false);
struct ValueNamer {
  llvm::SmallString<128> Buf;
  unsigned int FuncCount = 0;
  unsigned int StructCount = 0;
  unsigned int GlobCount = 0;
  unsigned int TempCount = 0;
  unsigned int ArgCount = 0;
  llvm::StringRef getBlockName(llvm::BasicBlock &Val, unsigned int id) {
    return getValueName(Val, "block_", id);
  }
  llvm::StringRef getFuncName(llvm::Function &Val) {
    return getValueName(Val, "func_", FuncCount);
  }
  llvm::StringRef getGlobName(llvm::GlobalVariable &Val) {
    return getValueName(Val, "global_", GlobCount);
  }
  llvm::StringRef getStructName(llvm::StructType &Ty) {
    return getTypeName(Ty, "struct_", StructCount);
  }
  llvm::StringRef getTempName(llvm::Value &Val) {
    return getValueName(Val, "temp_", TempCount);
  }
  llvm::StringRef getArgName(llvm::Value &Val) {
    return getValueName(Val, "arg_", ArgCount);
  }
  // clear arg count and temp count
  void clearFuncCount() {
    ArgCount = 0;
    TempCount = 0;
  }
  // Currently LLVM field has no name
  llvm::StringRef getFieldName(unsigned int id) {
    Buf.clear();
    llvm::raw_svector_ostream OS(Buf);
    OS << "field_" << id;
    return OS.str();
  }

protected:
  llvm::StringRef getTypeName(llvm::StructType &Ty, const char *prefix,
                              unsigned int &id);
  llvm::StringRef getValueName(llvm::Value &Val, const char *prefix,
                               unsigned int &id);
  void escapeBuf();
};

template <class SetTy>
clang::IdentifierInfo *getNewIdentifierInfo(SetTy &Names,
                                            clang::IdentifierTable &Idents,
                                            llvm::StringRef Name);

// clang::IdentifierInfo *getGlobalIdentifierInfo(clang::ASTContext &Ctx,
//                                                llvm::StringRef Name);

class SAContext;
class SAFuncContext;

class TypeBuilder {
  clang::ASTContext &Ctx;
  // For creating names for structs.
  ValueNamer *VN = nullptr;
  // Map from llvm struct type to clang RecordDecl type.
  std::map<llvm::Type *, clang::Decl *> typeMap;
  llvm::StringSet<> &Names;

public:
  TypeBuilder(clang::ASTContext &Ctx, ValueNamer &VN, llvm::StringSet<> &Names)
      : Ctx(Ctx), VN(&VN), Names(Names) {}
  clang::RecordDecl *createRecordDecl(llvm::StructType &Ty, bool isDefinition,
                                      bool hasPrevDef);
  clang::QualType visitStructType(llvm::StructType &Ty);
  clang::QualType visitType(llvm::Type &Ty);
  clang::IdentifierInfo *getIdentifierInfo(llvm::StringRef Name) {
    return getNewIdentifierInfo(Names, Ctx.Idents, Name);
  }
  clang::QualType
  visitFunctionType(llvm::FunctionType &Ty,
                    const clang::FunctionProtoType::ExtProtoInfo &EPI);

  // TODO move to a dedicated file
  static clang::QualType makeSigned(clang::ASTContext &Ctx,
                                    clang::QualType Ty) {
    auto qty = Ty->getCanonicalTypeInternal();
    if (const auto *BT = llvm::dyn_cast<clang::BuiltinType>(qty)) {
      switch (BT->getKind()) {
      case clang::BuiltinType::WChar_U:
        return Ctx.getSignedWCharType();
      case clang::BuiltinType::UChar:
        return Ctx.SignedCharTy;
      case clang::BuiltinType::UShort:
        return Ctx.ShortTy;
      case clang::BuiltinType::UInt:
        return Ctx.IntTy;
      case clang::BuiltinType::ULong:
        return Ctx.LongTy;
      case clang::BuiltinType::ULongLong:
        return Ctx.LongLongTy;
      case clang::BuiltinType::UInt128:
        return Ctx.Int128Ty;
      default:
        llvm_unreachable("TypeBuilder.makeSigned: unexpected type");
      }
    }
    if (const auto *IT = llvm::dyn_cast<clang::BitIntType>(qty)) {
      if (IT->isUnsigned())
        return Ctx.getBitIntType(false, IT->getNumBits());
    }
    if (const auto *IT = llvm::dyn_cast<clang::DependentBitIntType>(qty)) {
      if (IT->isSigned())
        Ctx.getDependentBitIntType(false, IT->getNumBitsExpr());
    }
    llvm_unreachable("TypeBuilder.makeSigned: unexpected type");
  }
  static clang::QualType makeUnsigned(clang::ASTContext &Ctx,
                                      clang::QualType Ty) {
    auto qty = Ty->getCanonicalTypeInternal();
    if (const auto *BT = llvm::dyn_cast<clang::BuiltinType>(qty)) {
      switch (BT->getKind()) {
      case clang::BuiltinType::WChar_S:
        return Ctx.getUnsignedWCharType();
      case clang::BuiltinType::Char_S:
      case clang::BuiltinType::SChar:
        return Ctx.UnsignedCharTy;
      case clang::BuiltinType::Short:
        return Ctx.UnsignedShortTy;
      case clang::BuiltinType::Int:
        return Ctx.UnsignedIntTy;
      case clang::BuiltinType::Long:
        return Ctx.UnsignedLongTy;
      case clang::BuiltinType::LongLong:
        return Ctx.UnsignedLongLongTy;
      case clang::BuiltinType::Int128:
        return Ctx.UnsignedInt128Ty;
      default:
        llvm_unreachable("TypeBuilder.makeUnsigned: unexpected type");
      }
    }
    if (const auto *IT = llvm::dyn_cast<clang::BitIntType>(qty)) {
      if (IT->isSigned())
        return Ctx.getBitIntType(true, IT->getNumBits());
    }
    if (const auto *IT = llvm::dyn_cast<clang::DependentBitIntType>(qty)) {
      if (IT->isSigned())
        Ctx.getDependentBitIntType(true, IT->getNumBitsExpr());
    }
    llvm_unreachable("TypeBuilder.makeUnsigned: unexpected type");
  }
};

/// Build an expression that refers to the IR value. It does not create stmts
/// for instructions or IR expressions.
///
/// To have a comprehensive IR value visitor, refer to Value class hierarchy
/// https://llvm.org/doxygen/classllvm_1_1Value.html and
/// llvm/lib/IR/Verifier.cpp.
///
/// Don't use interfaces like visit(Inst), because we need to check for cache
/// first.
class ExprBuilder : public llvm::InstVisitor<ExprBuilder, clang::Expr *> {
protected:
  SAFuncContext *FCtx = nullptr; // TODO only reference part of the context
  SAContext &SCtx;
  clang::ASTContext &Ctx;
  TypeBuilder &TB;

  clang::Expr *visitConstant(llvm::Constant &I,
                             clang::QualType Ty = clang::QualType());
  clang::QualType visitType(llvm::Type &Ty) { return TB.visitType(Ty); }

public:
  ExprBuilder(SAContext &SCtx, clang::ASTContext &Ctx, TypeBuilder &TB)
      : SCtx(SCtx), Ctx(Ctx), TB(TB) {}
  ExprBuilder(SAFuncContext &FCtx);
  clang::Expr *visitInstruction(llvm::Instruction &I) { return nullptr; }
  // The main interface to convert a llvm::Value to Expr.
  clang::Expr *visitValue(llvm::Value *Val,
                          clang::QualType Ty = clang::QualType());
  clang::Expr *visitInitializer(llvm::Value *Val, clang::QualType Ty);
  clang::Expr *createCompoundLiteralExpr(llvm::Value *Val);
};

/// Structural analysis context for a function.
class SAFuncContext {
  SAContext &ctx;
  llvm::Function &Func;
  // map from llvm inst to clang expr
  std::map<llvm::Value *, clang::Expr *> ExprMap;
  // map from llvm block to CFGBlock. Store the iterator to facilitate deletion
  std::map<llvm::BasicBlock *, CFGBlock *> ll2cfg;
  std::map<CFGBlock *, llvm::BasicBlock *> cfg2ll;
  TypeBuilder &TB;

  clang::FunctionDecl *FD = nullptr;
  std::unique_ptr<CFG> Cfg;
  // For checking local names (of variables, labels) are used or not.
  std::unique_ptr<llvm::StringSet<>> Names;

public:
  SAFuncContext(SAContext &ctx, llvm::Function &func);

  // Main interface for CFGBuilder to add an expression for an instruction.
  void addExprOrStmt(llvm::Value &v, clang::Stmt &Stmt, CFGBlock &block);
  /// Directly register the mapping from llvm value to clang expr. Do not use
  /// this and use addExprOrStmt instead most of the time.
  void addMapping(llvm::Value &v, clang::Expr &Expr) { ExprMap[&v] = &Expr; }
  clang::IdentifierInfo *getIdentifierInfo(llvm::StringRef Name) {
    return getNewIdentifierInfo(*Names, getASTContext().Idents, Name);
  }
  clang::FunctionDecl *getFunctionDecl() { return FD; }
  llvm::Function &getFunction() { return Func; }
  SAContext &getSAContext() { return ctx; }
  clang::ASTContext &getASTContext();
  class CFG &getCFG() { return *Cfg; }
  TypeBuilder &getTypeBuilder() { return TB; }
  ValueNamer &getValueNamer();
  bool isExpr(llvm::Value &v) { return ExprMap.count(&v) > 0; }
  clang::Expr *getExpr(llvm::Value &v) { return ExprMap.at(&v); }
  CFGBlock *&getBlock(llvm::BasicBlock &bb) { return ll2cfg.at(&bb); }
  CFGBlock *createBlock(llvm::BasicBlock &bb) {
    CFG::iterator b = getCFG().createBlock();
    ll2cfg[&bb] = *b;
    cfg2ll[*b] = &bb;
    return *b;
  }
  llvm::BasicBlock *getBlock(CFGBlock &bb) { return cfg2ll.at(&bb); }
  void run();
};

/// Main data structures for structural analysis
class SAContext {
protected:
  std::unique_ptr<llvm::StringSet<>> Names;
  llvm::Module &M;
  std::map<llvm::Function *, SAFuncContext> funcContexts;
  // Stores the mapping from llvm global object to clang decls for Functions and
  // GlobalVariables.
  std::map<llvm::GlobalObject *, clang::ValueDecl *> globalDecls;

  // Clang AST should be placed first, so that it is initialized first.
  std::unique_ptr<clang::ASTUnit> ASTunit;

  TypeBuilder TB;
  ValueNamer VN;
  ExprBuilder EB; // for building initialize exprs

public:
  // The usage of `clang::tooling::buildASTFromCode` follows llvm
  // unittests/Analysis/CFGTest.cpp, so we don't need to create ASTContext.
  SAContext(llvm::Module &mod)
      : Names(std::make_unique<llvm::StringSet<>>()), M(mod),
        ASTunit(clang::tooling::buildASTFromCode("", "decompiled.c")),
        TB(getASTContext(), VN, *Names), EB(*this, getASTContext(), TB) {
    // TODO: set target arch by cmdline or input arch, so that TargetInfo is set
    // and int width is correct.
  }
  void createDecls();

  clang::ASTContext &getASTContext() { return ASTunit->getASTContext(); }
  ValueNamer &getValueNamer() { return VN; }
  TypeBuilder &getTypeBuilder() { return TB; }

  static clang::StorageClass getStorageClass(llvm::GlobalValue &GV);
  /// Also ensure the uniqueness of Names.
  clang::IdentifierInfo *getIdentifierInfo(llvm::StringRef Name) {
    return getNewIdentifierInfo(*Names, getASTContext().Idents, Name);
  }

  /// A new context is created if not exist
  SAFuncContext &getFuncContext(llvm::Function &func) {
    if (funcContexts.find(&func) == funcContexts.end()) {
      auto result = funcContexts.emplace(&func, SAFuncContext(*this, func));
      assert(result.second && "insertion failed");
      return result.first->second;
    }
    return funcContexts.at(&func);
  }
  clang::FunctionDecl *getFunctionDecl(llvm::Function &F) {
    return llvm::cast<clang::FunctionDecl>(globalDecls.at(&F));
  }
  clang::VarDecl *getGlobalVarDecl(llvm::GlobalVariable &GV) {
    return llvm::cast<clang::VarDecl>(globalDecls.at(&GV));
  }
  clang::ValueDecl *getGlobalDecl(llvm::GlobalObject &GO) {
    return globalDecls.at(&GO);
  }

  static const llvm::StringSet<> Keywords;
  static bool isKeyword(llvm::StringRef Name);
};

/// Main interface for different structural analysis.
// TODO: add const to the module and function?
class IStructuralAnalysis {
protected:
  SAFuncContext &FCtx;
  CFG &CFG;
  // initialize after FCtx
  clang::ASTContext &Ctx;

  clang::Expr *invertCond(clang::Expr *cond) {
    if (auto *BO = llvm::dyn_cast<clang::BinaryOperator>(cond)) {
      switch (BO->getOpcode()) {
      case clang::BO_EQ:
        BO->setOpcode(clang::BO_NE);
        return BO;
      case clang::BO_NE:
        BO->setOpcode(clang::BO_EQ);
        return BO;
      case clang::BO_LT:
        BO->setOpcode(clang::BO_GE);
        return BO;
      case clang::BO_GT:
        BO->setOpcode(clang::BO_LE);
        return BO;
      case clang::BO_LE:
        BO->setOpcode(clang::BO_GT);
        return BO;
      case clang::BO_GE:
        BO->setOpcode(clang::BO_LT);
        return BO;
      default:
        break;
      }
    }
    return createUnaryOperator(FCtx.getASTContext(), cond, clang::UO_LNot,
                               FCtx.getASTContext().IntTy, clang::VK_PRValue);
  }

public:
  IStructuralAnalysis(SAFuncContext &ctx)
      : FCtx(ctx), CFG(FCtx.getCFG()), Ctx(ctx.getASTContext()) {}

  virtual ~IStructuralAnalysis() = default;
  virtual void execute() = 0;
  clang::LabelDecl *getBlockLabel(CFGBlock *blk, bool prepend = false);
  void mergeBlock(llvm::BasicBlock &bb, llvm::BasicBlock &next);
  ValueNamer &getValueNamer() { return FCtx.getSAContext().getValueNamer(); }

  // utility functions
  /// Get the condition of a binary conditional block from the terminator and
  /// remove the terminator.
  clang::Expr *takeBinaryCond(CFGBlock &B) {
    assert(B.succ_size() == 2 &&
           "getBinaryCond: block should have 2 successors!");
    auto ret = llvm::cast<clang::Expr>(B.getTerminatorStmt());
    B.setTerminator(nullptr);
    return ret;
  }
  clang::CompoundStmt *makeCompoundStmt(CFGBlock *el, bool remove = false) {
    // convert to vector
    std::vector<clang::Stmt *> stmts;
    for (auto elem = el->begin(); elem != el->end(); ++elem) {
      if (auto stmt = elem->getAs<CFGStmt>()) {
        stmts.push_back(const_cast<clang::Stmt *>(stmt->getStmt()));
      }
    }
    if (remove) {
      el->clear();
    }
    return clang::CompoundStmt::Create(FCtx.getASTContext(), stmts,
                                       clang::SourceLocation(),
                                       clang::SourceLocation());
  }
  clang::Stmt *createWhileTrue(clang::Stmt *body) {
    auto &Ctx = FCtx.getASTContext();
    auto tru = clang::IntegerLiteral::Create(
        Ctx, llvm::APInt(32, 1, true), Ctx.IntTy, clang::SourceLocation());
    return clang::WhileStmt::Create(
        Ctx, nullptr, tru, body, clang::SourceLocation(),
        clang::SourceLocation(), clang::SourceLocation());
  }
  void removeEdge(CFGBlock *From, CFGBlock *To) {
    From->removeSucc(To);
    To->removePred(From);
  }
  /// Move all outgoing edges of From to To.
  void replaceSuccessors(CFGBlock *From, CFGBlock *Target) {
    for (auto &Succ : From->succs()) {
      Target->addSuccessor(Succ);
      // replace pred of succ
      Succ->replacePred(From, Target);
    }
    From->succ_clear();
  }
  // get the only successor or nullptr.
  CFGBlock *linearSuccessor(CFGBlock *Block) {
    if (Block->succ_size() != 1) {
      return nullptr;
    }
    return *Block->succ_begin();
  }
  // get the only predecessor
  CFGBlock *singlePredecessor(CFGBlock *Block) {
    if (Block->pred_size() != 1) {
      return nullptr;
    }
    return *Block->pred_begin();
  }
  // check if the block has only predecessor pred.
  bool onlyPred(CFGBlock *Block, CFGBlock *Pred) {
    for (auto &P : Block->preds()) {
      if (P.getBlock() != Pred) {
        return false;
      }
    }
    return true;
  }
};

// A -> B -> C, and
// 1. B is an empty block
// 2. B has only one succ.
// then remove B in the middle: A -> C
class CFGCleaner : IStructuralAnalysis {
public:
  CFGCleaner(SAFuncContext &Ctx) : IStructuralAnalysis(Ctx) {}

  void execute() override {
    std::vector<CFGBlock *> blks(CFG.begin(), CFG.end());
    for (auto Block : blks) {
      if (Block->size() == 0 && Block->succ_size() == 1 &&
          Block->pred_size() > 0) {
        auto succ = linearSuccessor(Block);
        succ->removePred(Block);
        for (auto pred : Block->preds()) {
          pred->replaceSucc(Block, succ);
          succ->addOnlyPredecessor(pred);
        }
        CFG.remove(Block);
      }
    }
  }
};

/// CFGBuilder builds ASTs for basic blocks. In contrast, ExprBuilder build ASTs
/// for expressions and does not insert new statements.
/// CFGBuilder converts visited instructions and register the result to the expr
/// or statement map using `FCtx.addExpr` or `FCtx.AddStmt`. Instructions that
/// has side effects are converted to statements, or expr otherwise.
class CFGBuilder : public llvm::InstVisitor<CFGBuilder> {
protected:
  clang::ASTContext &Ctx;
  SAFuncContext &FCtx; // TODO only reference part of the context
  CFGBlock *Blk = nullptr;
  ExprBuilder EB;

  clang::QualType visitType(llvm::Type &Ty) {
    return FCtx.getTypeBuilder().visitType(Ty);
  }

  void addExprOrStmt(llvm::Value &v, clang::Stmt &Stmt) {
    assert(Blk != nullptr && "Block can't be null!");
    FCtx.addExprOrStmt(v, Stmt, *Blk);
  }

public:
  void visitArgs() {
    llvm::SmallVector<clang::ParmVarDecl *, 16> Params;
    // create param var decl for arguments
    for (llvm::Argument &Arg : FCtx.getFunction().args()) {
      clang::IdentifierInfo *II =
          FCtx.getIdentifierInfo(FCtx.getValueNamer().getArgName(Arg));
      clang::ParmVarDecl *PD = clang::ParmVarDecl::Create(
          Ctx, FCtx.getFunctionDecl(), clang::SourceLocation(),
          clang::SourceLocation(), II,
          FCtx.getTypeBuilder().visitType(*Arg.getType()), nullptr,
          clang::SC_None, nullptr);
      addExprOrStmt(Arg, *makeDeclRefExpr(PD));
      FCtx.getFunctionDecl()->addDecl(PD);
      Params.push_back(PD);
    }
    // set params for function Decl
    FCtx.getFunctionDecl()->setParams(Params);
  }
  CFGBlock *run(llvm::BasicBlock &BB) {
    auto ret = FCtx.createBlock(BB);
    // set current block so that visitor for terminator can insert CFGTerminator
    setBlock(ret);
    if (BB.isEntryBlock()) {
      FCtx.getCFG().setEntry(ret);
      visitArgs();
    }
    llvm::InstVisitor<CFGBuilder>::visit(BB);
    return ret;
  }

  clang::ImplicitCastExpr *
  makeImplicitCast(clang::Expr *Arg, clang::QualType Ty, clang::CastKind CK) {
    return clang::ImplicitCastExpr::Create(
        Ctx, Ty,
        /* CastKind=*/CK,
        /* Expr=*/Arg,
        /* CXXCastPath=*/nullptr,
        /* ExprValueKind=*/clang::VK_PRValue,
        /* FPFeatures */ clang::FPOptionsOverride());
  }

  void visitReturnInst(llvm::ReturnInst &I);
  void visitUnreachableInst(llvm::UnreachableInst &I) {
    // TODO create call statement to something like abort.
  }
  void visitBranchInst(llvm::BranchInst &I) {
    if (I.isConditional()) {
      Blk->setTerminator(CFGTerminator(EB.visitValue(I.getCondition())));
    }
  }
  void visitBinaryOperator(llvm::BinaryOperator &I);
  void visitCallInst(llvm::CallInst &I);
  void visitCmpInst(llvm::CmpInst &I);
  void visitAllocaInst(llvm::AllocaInst &I);
  void visitStoreInst(llvm::StoreInst &I);
  void visitLoadInst(llvm::LoadInst &I);
  void visitGetElementPtrInst(llvm::GetElementPtrInst &I);
  void visitUnaryOperator(llvm::UnaryOperator &I);
  void visitCastInst(llvm::CastInst &I);
  void visitPhiNode(llvm::PHINode &I) {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "CFGBuilder.visitPhiNode: PHI node should be eliminated by "
                    "reg2mem first!\n";
    std::abort();
  }
  void visitSelectInst(llvm::SelectInst &I);

  CFGBuilder(SAFuncContext &FCtx)
      : Ctx(FCtx.getASTContext()), FCtx(FCtx), EB(FCtx) {}
  void setBlock(CFGBlock *blk) { Blk = blk; }
};

// ============
// Signedness
// ============

/// Conversion types for converting LLVM binary operator to C operations.
/// For example, if a LLVM add is signed, we need to ensure the operand
/// expression is signed or insert a cast.
/// The logical shift corresponds to the C unsigned number shift, and the
/// arithmetical shift corresponds to the C signed number shift.
enum Conversion {
  None,
  Signed,
  Unsigned,
  Arithmetic, // make left op signed
  Logical,    // make left op unsigned
};

/// Get the signedness of the binary operator.
Conversion getSignedness(llvm::Instruction::BinaryOps op);

Conversion getSignedness(llvm::CmpInst::Predicate op);

/// Convert the LLVM compare operator to Clang binary operator op.
clang::Optional<clang::BinaryOperatorKind>
convertOp(llvm::CmpInst::Predicate op);

/// Convert the LLVM binary operator to a Clang binary operator op.
clang::Optional<clang::BinaryOperatorKind>
convertOp(llvm::Instruction::BinaryOps op);
/// Ensure the expression is signed, or insert a cast.
clang::Expr *castSigned(clang::ASTContext &Ctx, TypeBuilder &TB,
                        clang::Expr *E);
/// Ensure the expression is unsigned, or insert a cast.
clang::Expr *castUnsigned(clang::ASTContext &Ctx, TypeBuilder &TB,
                          clang::Expr *E);

} // namespace notdec::backend

#endif
