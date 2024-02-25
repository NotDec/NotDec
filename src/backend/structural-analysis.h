#ifndef _NOTDEC_BACKEND_STRUCTURAL_H_
#define _NOTDEC_BACKEND_STRUCTURAL_H_

#include "backend/CFG.h"
#include "optimizers/retdec-stack/retdec-utils.h"
#include "utils.h"
#include <clang/AST/ASTContext.h>
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
  TypeBuilder TB;

  clang::FunctionDecl *FD = nullptr;
  std::unique_ptr<CFG> Cfg;
  // For checking local names (of variables, labels) are used or not.
  std::unique_ptr<llvm::StringSet<>> Names;

  unsigned tempVarID = 0;

public:
  SAFuncContext(SAContext &ctx, llvm::Function &func)
      : ctx(ctx), func(func), TB(getASTContext()) {
    Cfg = std::make_unique<CFG>();
    Names = std::make_unique<llvm::StringSet<>>();
  }

  clang::IdentifierInfo *getIdentifierInfo(llvm::StringRef Name) {
    return getNewIdentifierInfo(*Names, getASTContext().Idents, Name);
  }
  clang::FunctionDecl *getFunctionDecl() { return FD; }
  llvm::Function &getFunction() { return func; }
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
  static clang::StorageClass getStorageClass(llvm::GlobalValue &GV);
};

/// Main data structures for structural analysis
class SAContext {
protected:
  llvm::Module &mod;
  std::map<llvm::Function *, SAFuncContext> funcContexts;

  // Clang AST
  std::unique_ptr<clang::ASTUnit> ASTunit;

public:
  SAContext(llvm::Module &mod) : mod(mod) {
    // TODO: set target arch by cmdline or input arch, so that TargetInfo is set
    // and int width is correct.

    // follow llvm unittests/Analysis/CFGTest.cpp, so we don't need to create
    // ASTContext.
    ASTunit = clang::tooling::buildASTFromCode("", "decompiled.c");
  }

  clang::ASTContext &getASTContext() { return ASTunit->getASTContext(); }

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

/// Build expressions instead of creating stmts for instructions and IR
/// expressions.
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

/// Convert instructions with side effects to a statement.
/// Other instructions are waited to be folded.
/// while visiting instructions, statements created are inserted into the
/// region.
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

  enum Conversion {
    None,
    Signed,
    Unsigned,
    Arithmetic, // make left op signed
    Logical,    // make left op unsigned
  };

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

  clang::Expr *castSigned(clang::Expr *E) {
    if (E->getType()->isUnsignedIntegerType()) {
      auto ty = makeSigned(E->getType());
      return clang::CStyleCastExpr::Create(
          Ctx, ty, clang::VK_PRValue, clang::CK_IntegralCast, E, nullptr,
          clang::FPOptionsOverride(), Ctx.CreateTypeSourceInfo(ty),
          clang::SourceLocation(), clang::SourceLocation());
    }
    return E;
  }

  clang::Expr *castUnsigned(clang::Expr *E) {
    if (E->getType()->isSignedIntegerType()) {
      auto ty = makeUnsigned(E->getType());
      return clang::CStyleCastExpr::Create(
          Ctx, ty, clang::VK_PRValue, clang::CK_IntegralCast, E, nullptr,
          clang::FPOptionsOverride(), Ctx.CreateTypeSourceInfo(ty),
          clang::SourceLocation(), clang::SourceLocation());
    }
    return E;
  }

  void visitBinaryOperator(llvm::BinaryOperator &I) {
    clang::Optional<clang::BinaryOperatorKind> op = convertOp(I.getOpcode());
    if (op.hasValue()) {
      Conversion cv = getSignedness(I.getOpcode());
      // insert conversion if needed
      clang::Expr *lhs = EB.visitValue(I.getOperand(0));
      if (cv == Signed || cv == Arithmetic) {
        lhs = castSigned(lhs);
      } else if (cv == Unsigned || cv == Logical) {
        lhs = castUnsigned(lhs);
      }
      clang::Expr *rhs = EB.visitValue(I.getOperand(1));
      if (cv == Signed) {
        rhs = castSigned(rhs);
      } else if (cv == Unsigned) {
        rhs = castUnsigned(rhs);
      }

      clang::Expr *binop = clang::BinaryOperator::Create(
          Ctx, lhs, rhs, op.getValue(), visitType(*I.getType()),
          clang::VK_PRValue, clang::OK_Ordinary, clang::SourceLocation(),
          clang::FPOptionsOverride());
      FCtx.addExpr(I, *binop);
      return;
    }
    llvm_unreachable("CFGBuilder.visitBinaryOperator: unexpected op type");
  }

  void visitReturnInst(llvm::ReturnInst &I) {
    clang::Stmt *ret;
    ret = clang::ReturnStmt::Create(Ctx, clang::SourceLocation(),
                                    EB.visitValue(I.getReturnValue()), nullptr);
    FCtx.addStmt(I, *ret);
    Blk->setTerminator(CFGTerminator(ret));
  }
  void visitUnreachableInst(llvm::UnreachableInst &I) {
    // TODO create call statement to something like abort.
  }

  CFGBuilder(SAFuncContext &FCtx)
      : Ctx(FCtx.getASTContext()), FCtx(FCtx), EB(FCtx) {}
  void setBlock(CFGBlock *blk) { Blk = blk; }
};

} // namespace notdec::backend

#endif
