#ifndef _NOTDEC_RETYPD_SK2C_H
#define _NOTDEC_RETYPD_SK2C_H

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Schema.h"
#include "Utils/Range.h"
#include "Utils/ValueNamer.h"
#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>
#include <map>
#include <memory>
#include <optional>

namespace notdec::retypd {

clang::QualType fromLatticeElem(clang::ASTContext &Ctx, std::string Name,
                                unsigned BitSize);

clang::RecordDecl *createStruct(clang::ASTContext &Ctx);

struct SketchToCTypeBuilder {
  struct TypeBuilderImpl {
    clang::ASTContext &Ctx;
    std::map<const CGNode *, clang::QualType> NodeTypeMap;
    std::set<const CGNode *> Visited;
    clang::QualType visitType(const CGNode &Node, unsigned BitSize);
  };
  std::unique_ptr<clang::ASTUnit> ASTUnit;
  TypeBuilderImpl Builder{ASTUnit->getASTContext()};

  // Todo: Change filename or remove the argument
  SketchToCTypeBuilder(llvm::StringRef FileName)
      : ASTUnit(clang::tooling::buildASTFromCode("", "decompilation.c")) {}

  clang::QualType buildType(const CGNode &Root, unsigned BitSize) {
    return Builder.visitType(Root, BitSize);
  }
};

} // namespace notdec::retypd
#endif
