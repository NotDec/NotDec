#ifndef _NOTDEC_BACKEND_STRUCTURAL_H_
#define _NOTDEC_BACKEND_STRUCTURAL_H_

#include "backend/exprs.h"
#include "backend/region-graph.h"
#include "backend/statements.h"
#include <clang/AST/ASTContext.h>
#include <llvm/ADT/BitVector.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>

#include "optimizers/retdec-stack/retdec-utils.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

namespace notdec::backend {

void decompileModule(llvm::Module &M, llvm::raw_fd_ostream &os);
std::string printBasicBlock(const llvm::BasicBlock *b);
std::string printFunction(const llvm::Function *F);

using ExprMap = std::map<llvm::Value *, Expr *>;
class SAContext;

/// Structural analysis context for a function.
class SAFuncContext {
  llvm::Function &func;
  RegionGraph rg;
  // map from block to region
  std::map<llvm::BasicBlock *, Region *> btor;
  // declarations at the beginning of the function
  std::vector<Statement *> decls;
  ExprMap exprMap;
  SAContext &ctx;

public:
  SAFuncContext(SAContext &ctx, llvm::Function &func) : func(func), ctx(ctx) {}

  void init();
};

/// Main data structures for structural analysis
class SAContext {
  llvm::Module &mod;
  ASTContext astCtx;
  std::map<llvm::Function *, SAFuncContext> funcContexts;

public:
  SAContext(llvm::Module &mod) : mod(mod) {}

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
  virtual Region execute() = 0;
};

/// Convert instructions with side effects to a statement.
/// Other instructions are waited to be folded.
/// while visiting instructions, statements created are inserted into the
/// region.
class LLVMRegionBuilder : public llvm::InstVisitor<LLVMRegionBuilder> {
  SAFuncContext &funcCtx;
  llvm::BasicBlock *bb = nullptr;
  Region *region = nullptr;

  static Region::RegionType successor2Rty(unsigned int numSuccessors) {
    static Region::RegionType Successor2Rty[3] = {
        Region::RegionType::Tail, Region::RegionType::Linear,
        Region::RegionType::Condition};
    if (numSuccessors > 2) {
      return Region::RegionType::Switch;
    } else {
      return Successor2Rty[numSuccessors];
    }
  }

public:
  // void visitBinaryOperator(llvm::BinaryOperator &I) {}
  void visitCallInst(llvm::CallInst &I) {}

  LLVMRegionBuilder(SAFuncContext &funcCtx) : funcCtx(funcCtx) {}
  Region build(llvm::BasicBlock &bb);
};

class LLVMExprBuilder : public llvm::InstVisitor<LLVMExprBuilder, Expr> {
  SAFuncContext &funcCtx;
  llvm::BasicBlock *bb = nullptr;
  Region *region = nullptr;
  LLVMExprBuilder(SAFuncContext &funcCtx) : funcCtx(funcCtx) {}
};

} // namespace notdec::backend

#endif
