#ifndef _NOTDEC_RETYPD_SK2C_H
#define _NOTDEC_RETYPD_SK2C_H

#include "TypeRecovery/Schema.h"
#include "TypeRecovery/Sketch.h"
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
  std::unique_ptr<clang::ASTUnit> ASTUnit;

  // Todo: Change filename or remove the argument
  SketchToCTypeBuilder(llvm::StringRef FileName)
      : ASTUnit(clang::tooling::buildASTFromCode("", "decompilation.c")) {}

  clang::QualType buildType(const Sketch &Sk, unsigned BitSize) {
    TypeBuilderImpl Builder{ASTUnit->getASTContext()};
    return Builder.visitType(*Sk.Root, BitSize);
  }

  struct TypeBuilderImpl {
    clang::ASTContext &Ctx;
    std::map<const SketchNode *, clang::QualType> NodeTypeMap;
    std::set<const SketchNode *> Visited;
    clang::QualType visitType(const SketchNode &Node, unsigned BitSize);
  };
};

} // namespace notdec::retypd
#endif
