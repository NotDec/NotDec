#ifndef _NOTDEC_BACKEND_STRUCTURAL_H_
#define _NOTDEC_BACKEND_STRUCTURAL_H_

#include "backend/CFG.h"
#include "optimizers/retdec-stack/retdec-utils.h"
#include "utils.h"
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
#include <llvm/ADT/StringSet.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/ErrorHandling.h>
#include <memory>

namespace notdec::backend {

void decompileModule(llvm::Module &M, llvm::raw_fd_ostream &os);
std::string printBasicBlock(const llvm::BasicBlock *b);
std::string printFunction(const llvm::Function *F);

template <class SetTy>
clang::IdentifierInfo *getNewIdentifierInfo(SetTy &Names,
                                            clang::IdentifierTable &Idents,
                                            llvm::StringRef Name);

class SAContext;
class SAFuncContext;

class TypeBuilder {
  clang::ASTContext &Ctx;

public:
  TypeBuilder(clang::ASTContext &Ctx) : Ctx(Ctx) {}
  clang::QualType visitType(llvm::Type &Ty);
  clang::QualType
  visitFunctionType(llvm::FunctionType &Ty,
                    clang::FunctionProtoType::ExtProtoInfo &EPI);
  clang::QualType makeSigned(clang::QualType Ty) {
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
  clang::QualType makeUnsigned(clang::QualType Ty) {
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

/// Structural analysis context for a function.
class SAFuncContext {
  SAContext &ctx;
  llvm::Function &func;
  // map from llvm inst to clang stmt
  std::map<llvm::Value *, clang::Stmt *> StmtMap;
  // map from llvm inst to clang expr
  std::map<llvm::Value *, clang::Expr *> ExprMap;
  // map from llvm block to CFGBlock. Store the iterator to facilitate deletion
  std::map<llvm::BasicBlock *, CFG::iterator> ll2cfg;
  std::map<CFGBlock *, llvm::BasicBlock *> cfg2ll;
  TypeBuilder &TB;

  clang::FunctionDecl *FD = nullptr;
  std::unique_ptr<CFG> Cfg;
  // For checking local names (of variables, labels) are used or not.
  std::unique_ptr<llvm::StringSet<>> Names;

  unsigned tempVarID = 0;

public:
  SAFuncContext(SAContext &ctx, llvm::Function &func);

  clang::IdentifierInfo *getIdentifierInfo(llvm::StringRef Name) {
    return getNewIdentifierInfo(*Names, getASTContext().Idents, Name);
  }
  clang::FunctionDecl *getFunctionDecl() { return FD; }
  llvm::Function &getFunction() { return func; }
  SAContext &getSAContext() { return ctx; }
  clang::ASTContext &getASTContext();
  class CFG &getCFG() { return *Cfg; }
  TypeBuilder &getTypeBuilder() { return TB; }
  bool isExpr(llvm::Value &v) { return ExprMap.count(&v) > 0; }
  bool isStmt(llvm::Value &v) { return StmtMap.count(&v) > 0; }
  clang::Stmt *getStmt(llvm::Value &v) { return StmtMap.at(&v); }
  void addStmt(llvm::Value &v, clang::Stmt &s) { StmtMap[&v] = &s; }
  clang::Expr *getExpr(llvm::Value &v) { return ExprMap.at(&v); }
  void addExpr(llvm::Value &v, clang::Expr &s) { ExprMap[&v] = &s; }
  CFG::iterator &getBlock(llvm::BasicBlock &bb) { return ll2cfg.at(&bb); }
  CFG::iterator createBlock(llvm::BasicBlock &bb) {
    CFG::iterator b = getCFG().createBlock();
    ll2cfg[&bb] = b;
    cfg2ll[&*b] = &bb;
    return b;
  }
  llvm::BasicBlock *getBlock(CFGBlock &bb) { return cfg2ll.at(&bb); }
  void run();
};

/// Main data structures for structural analysis
class SAContext {
protected:
  llvm::Module &M;
  std::map<llvm::Function *, SAFuncContext> funcContexts;
  std::map<llvm::Function *, clang::FunctionDecl *> funcDecls;

  // Clang AST should be placed first, so that it is initialized first.
  std::unique_ptr<clang::ASTUnit> ASTunit;

  TypeBuilder TB;

public:
  // The usage of `clang::tooling::buildASTFromCode` follows llvm
  // unittests/Analysis/CFGTest.cpp, so we don't need to create ASTContext.
  SAContext(llvm::Module &mod)
      : M(mod), ASTunit(clang::tooling::buildASTFromCode("", "decompiled.c")),
        TB(getASTContext()) {
    // TODO: set target arch by cmdline or input arch, so that TargetInfo is set
    // and int width is correct.
  }
  void createDecls();

  clang::ASTContext &getASTContext() { return ASTunit->getASTContext(); }
  TypeBuilder &getTypeBuilder() { return TB; }

  static clang::StorageClass getStorageClass(llvm::GlobalValue &GV);
  clang::IdentifierInfo *getIdentifierInfo(llvm::StringRef Name) {
    auto &Idents = getASTContext().Idents;
    return getNewIdentifierInfo(Idents, Idents, Name);
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
    return funcDecls.at(&F);
  }

  static const llvm::StringSet<> Keywords;
  static bool isKeyword(llvm::StringRef Name);
};

/// Main interface for different structural analysis.
// TODO: add const to the module and function?
class IStructuralAnalysis {
protected:
  SAFuncContext &ctx;

public:
  IStructuralAnalysis(SAFuncContext &ctx) : ctx(ctx) {}

  virtual ~IStructuralAnalysis() = default;
  virtual void execute() = 0;
  clang::LabelDecl *getBlockLabel(CFGBlock *blk);
  void mergeBlock(llvm::BasicBlock &bb, llvm::BasicBlock &next);
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
  clang::ASTContext &Ctx;
  SAFuncContext &FCtx; // TODO only reference part of the context

  clang::Expr *visitConstant(llvm::Constant &I);
  clang::QualType visitType(llvm::Type &Ty) {
    return FCtx.getTypeBuilder().visitType(Ty);
  }

public:
  ExprBuilder(SAFuncContext &FCtx);
  clang::Expr *visitInstruction(llvm::Instruction &I) { return nullptr; }
  // The main interface to convert a llvm::Value to Expr.
  clang::Expr *visitValue(llvm::Value *Val);
};

/// Convert visited instructions and register the result to the expr or
/// statement map using `FCtx.addExpr` or `FCtx.AddStmt`.
/// Instructions that has side effects are converted to statements, or expr
/// otherwise.
class CFGBuilder : public llvm::InstVisitor<CFGBuilder> {
protected:
  clang::ASTContext &Ctx;
  SAFuncContext &FCtx; // TODO only reference part of the context
  CFGBlock *Blk = nullptr;
  ExprBuilder EB;

  clang::QualType visitType(llvm::Type &Ty) {
    return FCtx.getTypeBuilder().visitType(Ty);
  }
  clang::QualType makeSigned(clang::QualType Ty) {
    return FCtx.getTypeBuilder().makeSigned(Ty);
  }
  clang::QualType makeUnsigned(clang::QualType Ty) {
    return FCtx.getTypeBuilder().makeUnsigned(Ty);
  }

public:
  CFGBlock *run(llvm::BasicBlock &BB) {
    auto ret = FCtx.createBlock(BB);
    if (BB.isEntryBlock()) {
      FCtx.getCFG().setEntry(&*ret);
    }
    // set current block so that visitor for terminator can insert CFGTerminator
    setBlock(&*ret);
    llvm::InstVisitor<CFGBuilder>::visit(BB);
    return &*ret;
  }

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
  Conversion getSignedness(llvm::Instruction::BinaryOps op) {
    switch (op) {
    case llvm::Instruction::Add:
    case llvm::Instruction::Sub:
    case llvm::Instruction::Mul:
    case llvm::Instruction::And:
    case llvm::Instruction::Or:
    case llvm::Instruction::Xor:
    case llvm::Instruction::Shl:
      return None;
    case llvm::Instruction::LShr:
      return Logical;
    case llvm::Instruction::AShr:
      return Arithmetic;
    case llvm::Instruction::UDiv:
    case llvm::Instruction::URem:
      return Unsigned;
    case llvm::Instruction::SDiv:
    case llvm::Instruction::SRem:
      return Signed;
    case llvm::Instruction::FAdd:
    case llvm::Instruction::FSub:
    case llvm::Instruction::FMul:
    case llvm::Instruction::FDiv:
    case llvm::Instruction::FRem:
      return None;
    default:
      return None;
    }
  }

  /// Convert the LLVM binary operator to a Clang binary operator op.
  clang::Optional<clang::BinaryOperatorKind>
  convertOp(llvm::Instruction::BinaryOps op) {
    switch (op) {
    case llvm::Instruction::Add:
    case llvm::Instruction::FAdd:
      return clang::BO_Add;
    case llvm::Instruction::Sub:
    case llvm::Instruction::FSub:
      return clang::BO_Sub;
    case llvm::Instruction::Mul:
    case llvm::Instruction::FMul:
      return clang::BO_Mul;
    case llvm::Instruction::UDiv:
    case llvm::Instruction::SDiv:
      return clang::BO_Div;
    case llvm::Instruction::URem:
    case llvm::Instruction::SRem:
    case llvm::Instruction::FRem:
      return clang::BO_Rem;

    case llvm::Instruction::Shl:
      return clang::BO_Shl;
    case llvm::Instruction::LShr:
      return clang::BO_Shr;
    case llvm::Instruction::AShr:
      return clang::BO_Shl;

    case llvm::Instruction::And:
      return clang::BO_And;
    case llvm::Instruction::Or:
      return clang::BO_Or;
    case llvm::Instruction::Xor:
      return clang::BO_Xor;
    default:
      return clang::None;
    }
  }

  /// Ensure the expression is signed, or insert a cast.
  clang::Expr *castSigned(clang::Expr *E);
  /// Ensure the expression is unsigned, or insert a cast.
  clang::Expr *castUnsigned(clang::Expr *E);
  clang::DeclRefExpr *
  makeDeclRefExpr(clang::ValueDecl *D,
                  bool RefersToEnclosingVariableOrCapture = false);
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

  void visitBinaryOperator(llvm::BinaryOperator &I);
  void visitReturnInst(llvm::ReturnInst &I);
  void visitUnreachableInst(llvm::UnreachableInst &I) {
    // TODO create call statement to something like abort.
  }
  void visitCallInst(llvm::CallInst &I);

  CFGBuilder(SAFuncContext &FCtx)
      : Ctx(FCtx.getASTContext()), FCtx(FCtx), EB(FCtx) {}
  void setBlock(CFGBlock *blk) { Blk = blk; }
};

} // namespace notdec::backend

#endif
