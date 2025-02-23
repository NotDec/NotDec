#ifndef _NOTDEC_RETYPD_SK2C_H
#define _NOTDEC_RETYPD_SK2C_H

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/Schema.h"
#include "notdec-llvm2c/Range.h"
#include "notdec-llvm2c/StructManager.h"
#include "Utils/ValueNamer.h"
#include <clang/AST/ASTContext.h> 
#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "notdec-llvm2c/Utils.h"

namespace notdec::retypd {

clang::RecordDecl *createStruct(clang::ASTContext &Ctx, const char *prefix = "struct_");

struct SketchToCTypeBuilder {
  struct TypeBuilderImpl {
    SketchToCTypeBuilder& Parent;
    clang::ASTContext &Ctx;
    std::map<const CGNode *, clang::QualType> NodeTypeMap;
    std::set<const CGNode *> Visited;
    // Main interface: recursively visit the node and build the type
    clang::QualType visitType(const CGNode &Node, unsigned ArraySize = 0);
    TypeBuilderImpl(SketchToCTypeBuilder& Parent) :Parent(Parent), Ctx(Parent.ASTUnit->getASTContext()) {}

    std::map<std::string, clang::QualType> TypeDefs;
    clang::QualType getUndef(unsigned BitSize) {
      assert(BitSize > 0);
      std::string Name = "undef" + std::to_string(BitSize);
      if (TypeDefs.count(Name)) {
        return TypeDefs.at(Name);
      }
      auto Decl = clang::TypedefDecl::Create(
          Ctx, Ctx.getTranslationUnitDecl(), clang::SourceLocation(),
          clang::SourceLocation(), &Ctx.Idents.get(Name),
          Ctx.CreateTypeSourceInfo(Ctx.getIntTypeForBitwidth(BitSize, false)));
      Ctx.getTranslationUnitDecl()->addDecl(Decl);
      auto Ret = Ctx.getTypedefType(Decl);
      TypeDefs.emplace(Name, Ret);
      return Ret;
    }

    clang::QualType getBot(unsigned BitSize) {
      assert(BitSize > 0);
      std::string Name = "bottom" + std::to_string(BitSize);
      if (TypeDefs.count(Name)) {
        return TypeDefs.at(Name);
      }
      auto Decl = clang::TypedefDecl::Create(
          Ctx, Ctx.getTranslationUnitDecl(), clang::SourceLocation(),
          clang::SourceLocation(), &Ctx.Idents.get(Name),
          Ctx.CreateTypeSourceInfo(Ctx.getIntTypeForBitwidth(BitSize, false)));
      Ctx.getTranslationUnitDecl()->addDecl(Decl);
      auto Ret = Ctx.getTypedefType(Decl);
      TypeDefs.emplace(Name, Ret);
      return Ret;
    }

    clang::QualType fromLowTy(LowTy LTy, unsigned BitSize);
    clang::QualType fromLatticeTy(LowTy Low, std::optional<LatticeTy> LTy, unsigned BitSize);
  };
  std::unique_ptr<clang::ASTUnit> ASTUnit;
  std::map<clang::Decl *, std::string> DeclComments;
  std::map<clang::Decl *, StructInfo> StructInfos;
  TypeBuilderImpl Builder;

  // Todo: Change filename or remove the argument
  SketchToCTypeBuilder(llvm::StringRef FileName)
      : ASTUnit(llvm2c::buildAST("decompilation.c")), Builder(*this) {}

  clang::QualType buildType(const CGNode &Root) {
    return Builder.visitType(Root);
  }
};

} // namespace notdec::retypd
#endif
