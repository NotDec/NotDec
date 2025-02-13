
#include "clang/AST/Attr.h"
#include "clang/AST/Comment.h"
#include <cassert>
#include <clang/AST/ASTFwd.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/APSInt.h>
#include <optional>

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/SketchToCTypeBuilder.h"
#include "Utils/Range.h"
#include "optimizers/ConstraintGenerator.h"
#include "utils.h"

namespace notdec::retypd {

// TODO a better lattice representation
clang::QualType
SketchToCTypeBuilder::TypeBuilderImpl::fromLatticeElem(LatticeTy LTy,
                                                       unsigned BitSize) {
  auto Name = LTy.latticeStr();
  if (BitSize == 1 || Name == "bool") {
    assert(BitSize == 1);
    return Ctx.BoolTy;
  }
  if (Name == "top" || Name == "unk") {
    // TODO create typedef to unsigned int. e.g., typedef top32 uint32_t
    return getUndef(BitSize);
  }
  if (Name == "bottom") {
    return getBot(BitSize);
  }
  if (Name == "float") {
    return Ctx.FloatTy;
  }
  if (Name == "double") {
    return Ctx.DoubleTy;
  }
  if (Name == "ptr") {
    return Ctx.getPointerType(getUndef(BitSize));
  }
  if (Name == "int" || Name == "uint" || Name == "sint") {
    bool Signed = false;
    if (Name == "sint") {
      Signed = true;
    }
    auto ret = Ctx.getIntTypeForBitwidth(BitSize, Signed);
    if (ret.isNull()) {
      llvm::errs() << "Warning: cannot find exact type for Int of size "
                   << BitSize << "\n";
      ret = Ctx.getBitIntType(!Signed, BitSize);
    }
    return ret;
  }
  assert(false && "TODO: fromLatticeElem: Unknown lattice element");
}

clang::RecordDecl *createStruct(clang::ASTContext &Ctx, const char *prefix) {
  auto *II = &Ctx.Idents.get(ValueNamer::getName(prefix));
  // Create a Struct type for it.
  clang::RecordDecl *decl = clang::RecordDecl::Create(
      Ctx, clang::TagDecl::TagKind::TTK_Struct, Ctx.getTranslationUnitDecl(),
      clang::SourceLocation(), clang::SourceLocation(), II, nullptr);
  return decl;
}

clang::QualType
SketchToCTypeBuilder::TypeBuilderImpl::visitType(const CGNode &Node) {
  unsigned BitSize = Node.getSize();
  const char *prefix = "struct_";
  if (&Node == Node.Parent.getMemoryNode()) {
    prefix = "MEMORY_";
  }
  if (Visited.count(&Node)) {
    if (NodeTypeMap.count(&Node)) {
      return NodeTypeMap.at(&Node);
    } else {
      // Visited, but have not set a type (in progress, i.e. visiting
      // dependency nodes).
      // TODO: remove debug output.
      std::cerr << "Cyclic dependency forces the node become struct.\n";
      clang::RecordDecl *Decl = createStruct(Ctx, prefix);
      clang::QualType Ret = Ctx.getPointerType(Ctx.getRecordType(Decl));
      NodeTypeMap.emplace(&Node, Ret);
      return Ret;
    }
  }
  Visited.emplace(&Node);

  // 1. validate: if number, must have no offset/load/store edge.
  assert(Node.isPNIAndEdgeMatch());

  bool isPNIPtr = Node.isPNIPointer();

  auto ETy = Node.getPNIVar()->getLatticeTy();
  // No out edges.
  if (Node.outEdges.empty()) {
    // no info from graph, just use LatticeTy.
    auto Ret = fromLatticeElem(Node.getPNIVar()->getLatticeTy(), BitSize);
    NodeTypeMap.emplace(&Node, Ret);
    return Ret;
  }

  if (!isPNIPtr) {
    // if only edges to #End with forget primitive, then it is a simple
    // primitive type.
    for (auto &Edge : Node.outEdges) {
      if (const auto *FB = std::get_if<ForgetBase>(&Edge.getLabel())) {
        if (FB->Base.isPrimitive()) {
          if (Node.key.SuffixVariance == Covariant) {
            ETy.merge(Node.getPNIVar()->getLatticeTy(), false);
          } else {
            ETy.merge(Node.getPNIVar()->getLatticeTy(), true);
          }
        }
      }
    }

    auto Ret = fromLatticeElem(ETy, BitSize);
    NodeTypeMap.emplace(&Node, Ret);
    return Ret;
  }

  auto FI = ConstraintsGenerator::getFieldInfo(Node);
  // Check for simple pointer type and array type
  if (FI.Fields.size() == 1) {
    auto &Field = FI.Fields[0];
    if (Field.Start.offset == 0) {
      if (Field.Start.access.size() == 0) {
        // simple pointer type
        auto PointeeTy = visitType(Field.OutEdge->getTargetNode());
        auto Ty = Ctx.getPointerType(PointeeTy);
        NodeTypeMap.emplace(&Node, Ty);
        return Ty;
      } else {
        auto ElemTy = visitType(Field.OutEdge->getTargetNode());
        auto ArrayTy = Ctx.getConstantArrayType(
            ElemTy, llvm::APInt(32, 0), nullptr, clang::ArrayType::Star, 0);
        NodeTypeMap.emplace(&Node, ArrayTy);
        return ArrayTy;
      }
    }
  }

  // Create struct type.
  clang::QualType Ret;
  clang::RecordDecl *Decl;
  if (NodeTypeMap.count(&Node)) {
    Ret = NodeTypeMap.at(&Node);
    Decl = Ret->getPointeeType()->getAsRecordDecl();
  } else {
    Decl = createStruct(Ctx);
    NodeTypeMap.emplace(&Node, Ctx.getPointerType(Ctx.getRecordType(Decl)));
    Ret = Ctx.getPointerType(Ctx.getRecordType(Decl));
  }
  Decl->startDefinition();
  for (auto &Ent : FI.Fields) {
    auto Ty = visitType(Ent.OutEdge->getTargetNode());
    assert(Ty->isPointerType() ||
           Ty->isArrayType() && "Offset edge must be pointer or array type");
    if (Ty->isPointerType()) {
      // the node is a field pointer type. get the field type.
      Ty = Ty->getPointeeType();
    } else if (Ty->isArrayType()) {
      // do nothing. Keep array type.
    } else {
      assert(false && "Offset edge must be pointer or array type");
    }
    if (Ent.Start.access.size() > 0) {
      // create array type
      Ty = Ctx.getConstantArrayType(Ty, llvm::APInt(32, 0), nullptr,
                                    clang::ArrayType::Star, 0);
    }

    auto *FII = &Ctx.Idents.get(ValueNamer::getName("field_"));
    clang::FieldDecl *Field = clang::FieldDecl::Create(
        Ctx, Decl, clang::SourceLocation(), clang::SourceLocation(), FII, Ty,
        nullptr, nullptr, false, clang::ICIS_NoInit);

    Parent.DeclComments[Field] = "at offset: " + std::to_string(Ent.Start.offset);
    bool addAttr = true;
    if (addAttr) {
      Field->addAttr(clang::AnnotateAttr::Create(
          Ctx, "off:" + std::to_string(Ent.Start.offset), nullptr, 0,
          clang::AttributeCommonInfo(clang::SourceRange())));
    }

    Decl->addDecl(Field);
  }
  Decl->completeDefinition();
  Ctx.getTranslationUnitDecl()->addDecl(Decl);
  return Ret;
}

} // namespace notdec::retypd
