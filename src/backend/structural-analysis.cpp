

#include "backend/structural-analysis.h"
#include "backend/goto.h"
#include "backend/phoenix.h"
#include "utils.h"
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Analysis/CFG.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <utility>

namespace notdec::backend {

int LogLevel = level_debug;

ExprBuilder::ExprBuilder(SAFuncContext &FCtx)
    : Ctx(FCtx.getASTContext()), FCtx(FCtx) {}

clang::ASTContext &SAFuncContext::getASTContext() {
  return ctx.getASTContext();
}

std::string printBasicBlock(const llvm::BasicBlock *b) {
  return llvmObjToString(b);
}

/// Decompile the module to c and print to a file.
void decompileModule(llvm::Module &M, llvm::raw_fd_ostream &OS) {
  SAContext Ctx(const_cast<llvm::Module &>(M));
  for (const llvm::Function &F : M) {
    SAFuncContext &FuncCtx =
        Ctx.getFuncContext(const_cast<llvm::Function &>(F));
    FuncCtx.run();
  }
  Ctx.getASTContext().getTranslationUnitDecl()->print(OS);
}

std::string printFunction(const llvm::Function *F) {
  std::string str;
  llvm::raw_string_ostream Out(str);
  if (F->isDeclaration()) {
    Out << "declare ";
    Out << ' ';
  } else {
    Out << "define ";
  }

  Out << F->getName();
  Out << " {";
  // Output all of the function's basic blocks.
  for (const llvm::BasicBlock &BB : *F)
    Out << printBasicBlock(&BB);

  Out << "}\n\n";
  return Out.str();
}

bool usedInBlock(llvm::Instruction &inst, llvm::BasicBlock &bb) {
  for (llvm::User *U : inst.users()) {
    if (llvm::Instruction *UI = llvm::dyn_cast<llvm::Instruction>(U)) {
      if (UI->getParent() == &bb) {
        return true;
      }
    }
  }
  return false;
}

clang::StorageClass SAFuncContext::getStorageClass(llvm::GlobalValue &GV) {
  return GV.isDeclaration() ? clang::SC_Extern
         : GV.getLinkage() == llvm::GlobalValue::LinkageTypes::InternalLinkage
             ? clang::SC_Static
             : clang::SC_None;
}

void SAFuncContext::run() {
  // 1. build the CFGBlocks
  CFGBuilder Builder(*this);

  // the first block will be implicitly registered as the exit block.
  Exit = CFG->createBlock();
  assert(Exit == &CFG->getExit());

  // create function decl
  clang::IdentifierInfo *II = &getASTContext().Idents.get(func.getName());
  clang::FunctionProtoType::ExtProtoInfo EPI;
  EPI.Variadic = func.isVarArg();
  FD = clang::FunctionDecl::Create(
      getASTContext(), getASTContext().getTranslationUnitDecl(),
      clang::SourceLocation(), clang::SourceLocation(), II,
      TB.visitFunctionType(*func.getFunctionType(), EPI), nullptr,
      getStorageClass(func));

  if (!func.isDeclaration()) {
    for (llvm::BasicBlock &bb : func) {
      // convert each instructions to stmt, and fill the StmtMap
      auto block = Builder.run(bb);

      // insert correct stmts to form a block.
      // iterate all insts, if not referenced by any inst in current block, or
      // has side effect, then put in the CFGBlock.
      for (llvm::Instruction &inst : bb) {
        if (inst.isTerminator()) {

        } else if (isStmt(inst)) {
          auto *stmt = getStmt(inst);
          block->appendStmt(stmt, CFG->getBumpVectorContext());
        } else if (isExpr(inst)) {
          // check if the expr is used by other insts in the same block
          if (!usedInBlock(inst, bb)) {
            std::abort();
            // auto *expr = getExpr(inst);
            // clang::IdentifierInfo *II2 =
            //     &getASTContext().Idents.get(inst.getNameOrAsOperand());
            // clang::VarDecl *decl = clang::VarDecl::Create(
            //     getASTContext(), FD, clang::SourceLocation(),
            //     clang::SourceLocation(), II2, TB.visitType(*inst.getType()),
            //     nullptr, clang::SC_None);
            // block->appendStmt(decl, CFG->getBumpVectorContext());
          }
        } else {
          llvm::errs()
              << __FILE__ << ":" << __LINE__ << ": "
              << "ERROR: SAFuncContext::init: Instruction not converted: "
              << inst << "\n";
          std::abort();
        }
      }
    }

    // TODO: create structural analysis according to cmdline
    Goto SA(*this);
    SA.execute();

    CFG->print(llvm::errs(), getASTContext().getLangOpts(), true);

    // Finalize steps
    // After structural analysis, the CFG is expected to have only one linear
    // block.
    clang::CFGBlock &entry = CFG->getEntry();
    llvm::SmallVector<clang::Stmt *> Stmts;
    for (auto elem : entry) {
      // if is stmt, then add to FD
      auto stmt = elem.getAs<clang::CFGStmt>();
      if (stmt.hasValue()) {
        Stmts.push_back(const_cast<clang::Stmt *>(stmt->getStmt()));
      }
    }
    auto term = entry.getTerminator();
    if (term.isStmtBranch()) {
      if (auto t = term.getStmt()) {
        Stmts.push_back(t);
      }
    }
    // create a compound stmt as function body
    auto CS = clang::CompoundStmt::Create(getASTContext(), Stmts,
                                          clang::SourceLocation(),
                                          clang::SourceLocation());
    FD->setBody(CS);
  }

  getASTContext().getTranslationUnitDecl()->addDecl(FD);
}

clang::Expr *ExprBuilder::visitValue(llvm::Value *Val) {
  if (Val == nullptr) {
    return nullptr;
  }
  // cache
  if (FCtx.isExpr(*Val)) {
    return FCtx.getExpr(*Val);
  }
  if (llvm::Instruction *Inst = llvm::dyn_cast<llvm::Instruction>(Val)) {
    return visit(*Inst);
  } else if (llvm::Constant *C = llvm::dyn_cast<llvm::Constant>(Val)) {
    return visitConstant(*C);
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "UnImplemented: ExprBuilder.visitValue cannot handle: "
                 << Val << "\n";
    std::abort();
  }
}

clang::QualType
TypeBuilder::visitFunctionType(llvm::FunctionType &Ty,
                               clang::FunctionProtoType::ExtProtoInfo &EPI) {
  llvm::SmallVector<clang::QualType, 16> Args(Ty.getNumParams());
  for (unsigned i = 0; i < Ty.getNumParams(); i++) {
    Args[i] = visitType(*Ty.getParamType(i));
  }
  clang::QualType RetTy = visitType(*Ty.getReturnType());
  RetTy.print(llvm::errs(), Ctx.getPrintingPolicy());
  return Ctx.getFunctionType(RetTy, Args, EPI);
}

clang::QualType TypeBuilder::visitType(llvm::Type &Ty) {
  if (Ty.isFunctionTy()) {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "ERROR: TypeBuilder::visitType cannot handle FunctionTy, "
                    "use visitFunctionType instead: "
                 << Ty << "\n";
    std::abort();
  }
  // else if (Ty.isStructTy()) {
  //   return visitStructType(Ty);
  // }
  // for simple primitive types
  if (Ty.isFloatTy()) {
    return Ctx.FloatTy;
  } else if (Ty.isDoubleTy()) {
    return Ctx.DoubleTy;
  } else if (Ty.isVoidTy()) {
    return Ctx.VoidTy;
  }

  if (Ty.isIntegerTy()) {
    // TODO get signed or unsigned
    auto ret = Ctx.getIntTypeForBitwidth(Ty.getIntegerBitWidth(), false);
    if (ret.isNull()) {
      if (LogLevel >= level_warning) {
        llvm::errs() << "Warning: cannot find exact type for: " << Ty << "\n";
      }
      ret = Ctx.getBitIntType(true, Ty.getIntegerBitWidth());
    }
    return ret;
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "UnImplemented: TypeBuilder.visitType cannot handle: " << Ty
                 << "\n";
    std::abort();
  }
}

clang::Expr *ExprBuilder::visitConstant(llvm::Constant &C) {
  if (llvm::ConstantInt *CI = llvm::dyn_cast<llvm::ConstantInt>(&C)) {
    return clang::IntegerLiteral::Create(Ctx, CI->getValue(),
                                         visitType(*CI->getType()),
                                         clang::SourceLocation());
  } else if (llvm::ConstantFP *CFP = llvm::dyn_cast<llvm::ConstantFP>(&C)) {
    return clang::FloatingLiteral::Create(Ctx, CFP->getValueAPF(), true,
                                          visitType(*CFP->getType()),
                                          clang::SourceLocation());
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "UnImplemented: ExprBuilder.visitConstant cannot handle: "
                 << C << "\n";
    std::abort();
  }
}

} // namespace notdec::backend
