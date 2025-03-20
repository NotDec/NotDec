#ifndef _NOTDEC_RETYPD_SK2C_H
#define _NOTDEC_RETYPD_SK2C_H

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/Schema.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "notdec-llvm2c/StructuralAnalysis.h"
#include "notdec-llvm2c/Utils.h"

namespace notdec::retypd {

using notdec::ast::HType;
using notdec::ast::HTypeContext;
using notdec::ast::TypedefDecl;

struct TypeBuilderContext {
  // Contains build-specific data like map of CGNode.

  // std::map<clang::Decl *, std::string> DeclComments;
  // std::map<clang::Decl *, StructInfo> StructInfos;
  // std::set<clang::Decl *> AllDecls;
  // TypeBuilder Builder;
  HTypeContext &Ctx;
  const llvm::DataLayout &DL;
  const unsigned PointerSize;

  // Todo: Change filename or remove the argument
  TypeBuilderContext(HTypeContext &Ctx, llvm::StringRef FileName,
                     const llvm::DataLayout &DL)
      : Ctx(Ctx), DL(DL), PointerSize(DL.getPointerSize()) {}
};

struct TypeBuilder {
  TypeBuilderContext &Parent;
  HTypeContext &Ctx;
  std::map<const CGNode *, HType *> NodeTypeMap;
  std::set<const CGNode *> Visited;
  std::map<CGNode *, TypeInfo> TypeInfos;

  // Main interface: recursively visit the node and build the type
  HType *buildType(const CGNode &Node, Variance V, unsigned ArraySize = 0);

  TypeBuilder(TypeBuilderContext &Parent,
              std::map<CGNode *, TypeInfo> TypeInfos)
      : Parent(Parent), Ctx(Parent.Ctx), TypeInfos(TypeInfos) {}

  HType *getUndef(unsigned BitSize) {
    assert(BitSize > 0);
    std::string Name = "undef" + std::to_string(BitSize);
    if (auto *Decl = Ctx.getDecl(Name)) {
      return Ctx.getTypedefType(false, llvm::cast<TypedefDecl>(Decl));
    }

    // not exist, create it
    auto Decl = TypedefDecl::Create(Ctx, Name,
                                    Ctx.getIntegerType(false, BitSize, false));
    auto Ret = Ctx.getTypedefType(false, Decl);
    return Ret;
  }

  HType *getBot(unsigned BitSize) {
    assert(BitSize > 0);
    std::string Name = "bottom" + std::to_string(BitSize);
    if (auto *Decl = Ctx.getDecl(Name)) {
      return Ctx.getTypedefType(false, llvm::cast<TypedefDecl>(Decl));
    }

    // not exist, create it
    auto Decl = TypedefDecl::Create(Ctx, Name,
                                    Ctx.getIntegerType(false, BitSize, false));
    auto Ret = Ctx.getTypedefType(false, Decl);
    return Ret;
  }

  HType *getPtrTy(HType *Pointee) {
    return Ctx.getPointerType(false, Parent.PointerSize, Pointee);
  }

  HType *fromLowTy(LowTy LTy, unsigned BitSize);
  HType *fromLatticeTy(LowTy Low, LatticeTy *LTy, unsigned BitSize);
};

} // namespace notdec::retypd
#endif
