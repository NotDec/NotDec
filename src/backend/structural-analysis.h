#ifndef _NOTDEC_BACKEND_STRUCTURAL_H_
#define _NOTDEC_BACKEND_STRUCTURAL_H_

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Analysis/CFG.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>

#include "optimizers/retdec-stack/retdec-utils.h"
#include "utils.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
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
};

/// Structural analysis context for a function.
class SAFuncContext {
  SAContext &ctx;
  llvm::Function &func;
  // map from llvm inst to clang stmt
  std::map<llvm::Value *, clang::Stmt *> StmtMap;
  // map from llvm inst to clang expr
  std::map<llvm::Value *, clang::Expr *> ExprMap;
  // map from llvm block to CFGBlock
  std::map<llvm::BasicBlock *, clang::CFGBlock *> ll2cfg;
  std::map<clang::CFGBlock *, llvm::BasicBlock *> cfg2ll;
  TypeBuilder TB;

  clang::FunctionDecl *FD = nullptr;
  std::unique_ptr<clang::CFG> CFG;
  // For checking local names (of variables, labels) are used or not.
  std::unique_ptr<llvm::StringSet<>> Names;

public:
  SAFuncContext(SAContext &ctx, llvm::Function &func)
      : ctx(ctx), func(func), TB(getASTContext()) {
    CFG = std::make_unique<clang::CFG>();
    Names = std::make_unique<llvm::StringSet<>>();
  }

  clang::IdentifierInfo *getIdentifierInfo(llvm::StringRef Name) {
    return getNewIdentifierInfo(*Names, getASTContext().Idents, Name);
  }
  clang::FunctionDecl *getFunctionDecl() { return FD; }
  llvm::Function &getFunction() { return func; }
  clang::ASTContext &getASTContext();
  clang::CFG &getCFG() { return *CFG; }
  TypeBuilder &getTypeBuilder() { return TB; }
  bool isExpr(llvm::Value &v) { return ExprMap.count(&v) > 0; }
  bool isStmt(llvm::Value &v) { return StmtMap.count(&v) > 0; }
  clang::Stmt *getStmt(llvm::Value &v) { return StmtMap.at(&v); }
  void addStmt(llvm::Value &v, clang::Stmt &s) { StmtMap[&v] = &s; }
  clang::Expr *getExpr(llvm::Value &v) { return ExprMap.at(&v); }
  void addExpr(llvm::Value &v, clang::Expr &s) { ExprMap[&v] = &s; }
  clang::CFGBlock *getBlock(llvm::BasicBlock &bb) { return ll2cfg.at(&bb); }
  void addBlock(llvm::BasicBlock &bb, clang::CFGBlock &b) {
    ll2cfg[&bb] = &b;
    cfg2ll[&b] = &bb;
  }
  llvm::BasicBlock *getBlock(clang::CFGBlock &bb) { return cfg2ll.at(&bb); }
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
  clang::LabelDecl *getBlockLabel(clang::CFGBlock *blk);
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
  clang::CFGBlock *Blk = nullptr;
  ExprBuilder EB;

  clang::QualType visitType(llvm::Type &Ty) {
    return FCtx.getTypeBuilder().visitType(Ty);
  }

public:
  clang::CFGBlock *run(llvm::BasicBlock &BB) {
    auto ret = FCtx.getCFG().createBlock();
    if (BB.isEntryBlock()) {
      FCtx.getCFG().setEntry(ret);
    }
    // maintain block mapping
    FCtx.addBlock(BB, *ret);
    // set current block so that visitor for terminator can insert CFGTerminator
    setBlock(ret);
    llvm::InstVisitor<CFGBuilder>::visit(BB);
    return ret;
  }
  // void visitBinaryOperator(llvm::BinaryOperator &I) {}
  void visitReturnInst(llvm::ReturnInst &I) {
    clang::Stmt *ret;
    ret = clang::ReturnStmt::Create(Ctx, clang::SourceLocation(),
                                    EB.visitValue(I.getReturnValue()), nullptr);
    FCtx.addStmt(I, *ret);
    Blk->setTerminator(clang::CFGTerminator(ret));
  }
  void visitUnreachableInst(llvm::UnreachableInst &I) {
    // TODO create call statement to something like abort.
  }

  CFGBuilder(SAFuncContext &FCtx)
      : Ctx(FCtx.getASTContext()), FCtx(FCtx), EB(FCtx) {}
  void setBlock(clang::CFGBlock *blk) { Blk = blk; }
};

} // namespace notdec::backend

#endif
