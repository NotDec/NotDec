

#include "backend/structural-analysis.h"
#include "backend/goto.h"
#include "backend/phoenix.h"
#include "utils.h"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Analysis/CFG.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <utility>

namespace notdec::backend {

int LogLevel = level_debug;

const llvm::StringSet<> SAContext::Keywords = {
#define KEYWORD(NAME, FLAGS) #NAME,
#include "clang/Basic/TokenKinds.def"
#undef KEYWORD
};

bool SAContext::isKeyword(llvm::StringRef Name) {
  return Keywords.find(Name) != Keywords.end();
}

template <class T>
clang::IdentifierInfo *getNewIdentifierInfo(T &Names,
                                            clang::IdentifierTable &Idents,
                                            llvm::StringRef Name) {
  llvm::SmallString<128> Buf;
  auto Iter = Names.find(Name);
  unsigned ID = 0;
  while (Iter != Names.end()) {
    // already exist, add suffix to the name
    Iter = Names.find((Name + llvm::Twine(ID)).toStringRef(Buf));
    ++ID;
  }
  return &Idents.get(Name);
}

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

  // Create the stub exit block.
  // The first block will be implicitly registered as the exit block.
  // TODO the exit block is currently not used. Edges to exit block are not
  // maintained.
  CFG::iterator Exit = Cfg->createBlock();
  assert(&*Exit == &Cfg->getExit());

  // create function decl
  clang::IdentifierInfo *II = ctx.getIdentifierInfo(func.getName());
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
          block->appendStmt(stmt);
        } else if (isExpr(inst)) {
          // check if the expr is used by other insts in the same block
          if (!usedInBlock(inst, bb)) {
            auto *expr = getExpr(inst);
            auto Name =
                (inst.hasName() && !SAContext::isKeyword(inst.getName()))
                    ? inst.getName()
                    : "temp_" + llvm::Twine(++tempVarID);

            llvm::SmallString<128> Buf;
            clang::IdentifierInfo *II2 =
                &getASTContext().Idents.get(Name.toStringRef(Buf));
            clang::VarDecl *decl = clang::VarDecl::Create(
                getASTContext(), FD, clang::SourceLocation(),
                clang::SourceLocation(), II2, TB.visitType(*inst.getType()),
                nullptr, clang::SC_None);
            clang::DeclRefExpr *ref = clang::DeclRefExpr::Create(
                getASTContext(), clang::NestedNameSpecifierLoc(),
                clang::SourceLocation(), decl, false,
                clang::DeclarationNameInfo(II2, clang::SourceLocation()),
                expr->getType(), clang::VK_LValue);
            // assign stmt
            clang::Stmt *DS = clang::BinaryOperator::Create(
                getASTContext(), ref, expr, clang::BO_Assign, expr->getType(),
                clang::VK_PRValue, clang::OK_Ordinary, clang::SourceLocation(),
                clang::FPOptionsOverride());
            block->appendStmt(DS);
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

    // connect the edges
    for (llvm::BasicBlock &bb : func) {
      for (auto succ : llvm::successors(&bb)) {
        auto src = getBlock(bb);
        auto dst = getBlock(*succ);
        src->addSuccessor(CFGBlock::AdjacentBlock(&*dst));
      }
    }

    if (LogLevel >= level_debug) {
      llvm::errs() << "========" << func.getName() << ": "
                   << "Before Structural Analysis ========"
                   << "\n";
      Cfg->dump(getASTContext().getLangOpts(), true);
    }

    // TODO: create structural analysis according to cmdline
    Goto SA(*this);
    SA.execute();

    if (LogLevel >= level_debug) {
      llvm::errs() << "========" << func.getName() << ": "
                   << "After Structural Analysis ========"
                   << "\n";
      Cfg->dump(getASTContext().getLangOpts(), true);
    }

    // Finalize steps
    // After structural analysis, the CFG is expected to have only one linear
    // block.
    CFGBlock &entry = Cfg->getEntry();
    llvm::SmallVector<clang::Stmt *> Stmts;
    // add labelStmt
    if (auto l = entry.getLabel()) {
      Stmts.push_back(l);
    }
    for (auto elem : entry) {
      // if is stmt, then add to FD
      auto stmt = elem.getAs<CFGStmt>();
      if (stmt.hasValue()) {
        Stmts.push_back(const_cast<clang::Stmt *>(stmt->getStmt()));
      }
    }
    auto term = entry.getTerminator();
    if (term.getStmt() != nullptr) {
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
  } else if (Ty.isIntegerTy(1)) {
    return Ctx.BoolTy;
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

/// get the label for the block. Create if not exist.
clang::LabelDecl *IStructuralAnalysis::getBlockLabel(CFGBlock *Blk) {
  if (auto label = Blk->getLabel()) {
    return llvm::cast<clang::LabelStmt>(label)->getDecl();
  } else {
    auto &astCtx = ctx.getASTContext();
    auto bb = ctx.getBlock(*Blk);
    auto Name = (bb->hasName() && !SAContext::isKeyword(bb->getName()))
                    ? bb->getName()
                    : "block_" + llvm::Twine(Blk->getBlockID());
    // create the label
    llvm::SmallString<128> Buf;
    clang::IdentifierInfo *II = ctx.getIdentifierInfo(Name.toStringRef(Buf));
    auto LabelDecl = clang::LabelDecl::Create(astCtx, ctx.getFunctionDecl(),
                                              clang::SourceLocation(), II);
    // create LabelStmt
    auto LabelStmt = new (astCtx)
        clang::LabelStmt(clang::SourceLocation(), LabelDecl,
                         new (astCtx) clang::NullStmt(clang::SourceLocation()));
    Blk->setLabel(LabelStmt);
    return LabelDecl;
  }
}

} // namespace notdec::backend
