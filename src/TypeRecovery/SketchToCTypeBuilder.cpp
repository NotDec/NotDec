
#include "clang/AST/Attr.h"
#include "clang/AST/Comment.h"
#include <cassert>
#include <clang/AST/ASTFwd.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <cstddef>
#include <llvm/ADT/APSInt.h>
#include <optional>
#include <variant>

#include "Passes/ConstraintGenerator.h"
#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/LowTy.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/SketchToCTypeBuilder.h"
#include "Utils/Utils.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include "notdec-llvm2c/StructuralAnalysis.h"

namespace notdec::retypd {

clang::QualType SketchToCTypeBuilder::TypeBuilderImpl::fromLatticeTy(
    LowTy Low, std::optional<LatticeTy> LTy, unsigned BitSize) {
  if (!LTy.has_value()) {
    return fromLowTy(Low, BitSize);
  }
  return LTy->buildType(Ctx);
}

clang::QualType
SketchToCTypeBuilder::TypeBuilderImpl::fromLowTy(LowTy LTy, unsigned BitSize) {
  auto Name = LTy.latticeStr();
  if (BitSize == 1 || Name == "bool") {
    assert(BitSize == 1);
    return Ctx.BoolTy;
  }
  if (BitSize == 8 || Name == "char") {
    return Ctx.CharTy;
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
    bool Signed = true;
    if (Name == "uint") {
      Signed = false;
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
SketchToCTypeBuilder::TypeBuilderImpl::visitType(const CGNode &Node,
                                                 unsigned ArraySize) {
  unsigned BitSize = Node.getSize();
  const char *prefix = "struct_";
  if (Node.isMemory()) {
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
  if (!Node.isMemory() && Node.outEdges.empty()) {
    // no info from graph, just use LatticeTy.
    auto Ret = fromLowTy(Node.getPNIVar()->getLatticeTy(), BitSize);
    NodeTypeMap.emplace(&Node, Ret);
    return Ret;
  }

  if (!Node.isMemory() && !isPNIPtr) {
    // if only edges to #End with forget primitive, then it is a simple
    // primitive type.
    std::optional<LatticeTy> LT = createLatticeTy(ETy, "");
    for (auto &Edge : Node.outEdges) {
      if (const auto *FB = std::get_if<ForgetBase>(&Edge.getLabel())) {
        if (FB->Base.isPrimitive()) {
          auto L1 = createLatticeTy(ETy, FB->Base.getPrimitiveName());
          if (Node.key.SuffixVariance == Covariant) {
            meet(LT, L1);
          } else {
            join(LT, L1);
          }
        }
      }
    }

    auto Ret = fromLatticeTy(ETy, LT, BitSize);
    NodeTypeMap.emplace(&Node, Ret);
    return Ret;
  }

  auto FI = ConstraintsGenerator::getFieldInfo(Node);
  // Check for simple pointer type and array type
  if (!Node.isMemory() && FI.Fields.size() == 1) {
    auto &Field = FI.Fields[0];
    if (Field.R.Start == 0) {
      if (Field.R.Start.access.size() == 0) {
        // simple pointer type
        auto PointeeTy = visitType(Field.Edge->getTargetNode());
        auto Ty = Ctx.getPointerType(PointeeTy);
        NodeTypeMap.emplace(&Node, Ty);
        return Ty;
      } else {
        auto Count = ArraySize / Field.R.Start.access.at(0).Size;
        auto ElemTy = visitType(Field.Edge->getTargetNode(),
                                Field.R.Start.access.at(0).Size);
        auto ArrayTy = Ctx.getConstantArrayType(
            ElemTy->getPointeeType(), llvm::APInt(32, Count),
            clang::IntegerLiteral::Create(Ctx, llvm::APInt(32, Count),
                                          Ctx.IntTy, clang::SourceLocation()),
            Count == 0 ? clang::ArrayType::Star : clang::ArrayType::Normal, 0);
        auto FinalTy = Ctx.getPointerType(ArrayTy);
        NodeTypeMap.emplace(&Node, FinalTy);
        return FinalTy;
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
    Decl = createStruct(Ctx, prefix);
    NodeTypeMap.emplace(&Node, Ctx.getPointerType(Ctx.getRecordType(Decl)));
    Ret = Ctx.getPointerType(Ctx.getRecordType(Decl));
  }
  Parent.StructInfos.insert_or_assign(Decl, FI);
  auto &Info = Parent.StructInfos.at(Decl);
  Info.Decl = Decl;
  // Info.addPaddings();
  Decl->startDefinition();
  for (size_t i = 0; i < Info.Fields.size(); i++) {
    auto &Ent = Info.Fields[i];

    // for merging elem to prev array
    FieldEntry *Prev = nullptr;
    if (i > 0) {
      Prev = &Info.Fields[i - 1];
    }

    // add padding, expand array accordingly
    FieldEntry *Next = nullptr;
    if (i + 1 < Info.Fields.size()) {
      Next = &Info.Fields[i + 1];
    }

    clang::QualType Ty;
    auto ArraySize = Ent.R.Size;
    // expand the array size to fill up the padding
    // set to Ent.R.Size later if this is really an array.
    if (Next != nullptr &&
        Ent.R.Start + Ent.R.Size < Next->R.Start) {
      ArraySize = Next->R.Start - Ent.R.Start;
    }
    // for each field/edge
    Ty = visitType(Ent.Edge->getTargetNode(), ArraySize);
    if (isLoadOrStore(Ent.Edge->getLabel())) {
      // no operation
    } else {
      // assert(Ty->isPointerType() ||
      //        Ty->isArrayType() && "Offset edge must be pointer or array
      //        type");
      if (Ty->isPointerType()) {
        // the node is a field pointer type. get the field type.
        Ty = Ty->getPointeeType();
      } else {
        assert(false && "Offset edge must be pointer type");
      }
      if (Ent.R.Start.access.size() > 0) {
        auto Count = ArraySize / Ent.R.Start.access.at(0).Size;
        // create array type
        Ty = Ctx.getConstantArrayType(
            Ty, llvm::APInt(32, Count), nullptr,
            Count == 0 ? clang::ArrayType::Star : clang::ArrayType::Normal, 0);
      }
    }
    // set to Ent.R.Size if this is really an array.
    if (Ty->isArrayType()) {
      Ent.R.Size = ArraySize;
    }

    // if prev is array
    if (Prev != nullptr && Prev->Decl != nullptr &&
        Prev->Decl->getType()->isArrayType()) {
      auto ElemTy = Prev->Decl->getType()->getArrayElementTypeNoTypeQual();
      // if current ty is char
      if (ElemTy == Ty.getTypePtr() && Ty->isCharType()) {
        if (Prev->R.Start + Prev->R.Size == Ent.R.Start) {
          // merge to prev
          Prev->R.Size += Ent.R.Size;
          // Update array Size
          Prev->Decl->setType(Ctx.getConstantArrayType(
              Ty, llvm::APInt(32, Prev->R.Size),
              clang::IntegerLiteral::Create(Ctx, llvm::APInt(32, Prev->R.Size),
                                            Ctx.IntTy, clang::SourceLocation()),
              clang::ArrayType::Normal, 0));
          Info.Fields.erase(Info.Fields.begin() + i);
          i--;
          continue;
        }
        // merge to prev
      }
    }

    auto *FII = &Ctx.Idents.get(ValueNamer::getName("field_"));
    clang::FieldDecl *Field = clang::FieldDecl::Create(
        Ctx, Decl, clang::SourceLocation(), clang::SourceLocation(), FII,
        llvm2c::toLValueType(Ctx, Ty), nullptr, nullptr, false,
        clang::ICIS_CopyInit);

    Parent.DeclComments[Field] =
        "at offset: " + std::to_string(Ent.R.Start);
    // bool useAnno = false;
    // if (useAnno) {
    //   Field->addAttr(clang::AnnotateAttr::Create(
    //       Ctx, "off:" + std::to_string(Ent.R.Start), nullptr, 0,
    //       clang::AttributeCommonInfo(clang::SourceRange())));
    // }

    Ent.Decl = Field;
    Decl->addDecl(Field);

    // add padding?
    auto End = Ent.R.Start + Ent.R.Size;
    if (Next != nullptr && End < Next->R.Start) {
      auto PaddingSize = Next->R.Start - End;
      if (PaddingSize != 0) {
        Ty = Ctx.getConstantArrayType(
            Ctx.CharTy, llvm::APInt(32, PaddingSize),
            clang::IntegerLiteral::Create(Ctx, llvm::APInt(32, PaddingSize),
                                          Ctx.IntTy, clang::SourceLocation()),
            clang::ArrayType::Normal, 0);

        auto *FII = &Ctx.Idents.get(ValueNamer::getName("padding_"));
        clang::FieldDecl *Field = clang::FieldDecl::Create(
            Ctx, Decl, clang::SourceLocation(), clang::SourceLocation(), FII,
            Ty, nullptr, nullptr, false, clang::ICIS_CopyInit);

        Parent.DeclComments[Field] = "at offset: " + std::to_string(End);
        Decl->addDecl(Field);
      }
    }
  }
  Decl->completeDefinition();
  // Let backend to insert into the translation unit.
  Parent.AllDecls.insert(Decl);
  // Ctx.getTranslationUnitDecl()->addDecl(Decl);
  return Ret;
}

} // namespace notdec::retypd
