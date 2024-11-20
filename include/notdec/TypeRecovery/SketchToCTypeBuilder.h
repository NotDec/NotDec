#ifndef _NOTDEC_RETYPD_SK2C_H
#define _NOTDEC_RETYPD_SK2C_H

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Schema.h"
#include "Utils/Range.h"
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

namespace notdec::retypd {

clang::RecordDecl *createStruct(clang::ASTContext &Ctx);

struct SketchToCTypeBuilder {
  struct TypeBuilderImpl {
    clang::ASTContext &Ctx;
    std::map<const CGNode *, clang::QualType> NodeTypeMap;
    std::set<const CGNode *> Visited;
    // Main interface: recursively visit the node and build the type
    clang::QualType visitType(const CGNode &Node, unsigned BitSize);
    TypeBuilderImpl(clang::ASTContext &Ctx) : Ctx(Ctx) {}

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

    clang::QualType fromLatticeElem(std::string Name, unsigned BitSize);
  };
  std::unique_ptr<clang::ASTUnit> ASTUnit;
  TypeBuilderImpl Builder{ASTUnit->getASTContext()};

  // Todo: Change filename or remove the argument
  SketchToCTypeBuilder(llvm::StringRef FileName)
      : ASTUnit(clang::tooling::buildASTFromCode("", "decompilation.c")) {}

  clang::QualType buildType(const CGNode &Root, unsigned BitSize) {
    assert(BitSize > 0);
    return Builder.visitType(Root, BitSize);
  }
};

} // namespace notdec::retypd
#endif
