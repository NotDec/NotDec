
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
#include <memory>
#include <optional>
#include <variant>

#include "Passes/ConstraintGenerator.h"
#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/LowTy.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/SketchToCTypeBuilder.h"
#include "Utils/Utils.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include "notdec-llvm2c/StructuralAnalysis.h"

namespace notdec::retypd {

using notdec::ast::FieldDecl;
using notdec::ast::HType;
using notdec::ast::RecordDecl;
using notdec::ast::UnionDecl;

HType *TypeBuilder::fromLatticeTy(LowTy Low, LatticeTy *LTy, unsigned BitSize) {
  if (!LTy) {
    return fromLowTy(Low, BitSize);
  }
  return LTy->buildType(Ctx);
}

HType *TypeBuilder::fromLowTy(LowTy LTy, unsigned BitSize) {
  auto Name = LTy.latticeStr();
  if (BitSize == 1 || Name == "bool") {
    assert(BitSize == 1);
    return Ctx.getBool();
  }
  if (BitSize == 8 || Name == "char") {
    return Ctx.getChar();
  }
  if (Name == "top" || Name == "unk") {
    // TODO create typedef to unsigned int. e.g., typedef top32 uint32_t
    return getUndef(BitSize);
  }
  if (Name == "bottom") {
    return getBot(BitSize);
  }
  if (Name == "float") {
    return Ctx.getFloatType(false, 32);
  }
  if (Name == "double") {
    return Ctx.getFloatType(false, 64);
  }
  if (Name == "ptr") {
    return Ctx.getPointerType(false, Parent.PointerSize, nullptr);
  }
  if (Name == "int" || Name == "uint" || Name == "sint") {
    // default to signed
    bool Signed = true;
    if (Name == "uint") {
      Signed = false;
    }
    auto ret = Ctx.getIntegerType(false, BitSize, Signed);
    // if (ret.isNull()) {
    //   llvm::errs() << "Warning: cannot find exact type for Int of size "
    //                << BitSize << "\n";
    //   ret = Ctx.getBitIntType(!Signed, BitSize);
    // }
    return ret;
  }
  assert(false && "TODO: fromLatticeElem: Unknown lattice element");
}

// static bool isRecordOrUnionPointer(HType *Ty) {
//   if (!Ty->isPointerType()) {
//     return false;
//   }
//   auto Pointee = Ty->getPointeeType();
//   return Pointee->isRecordType() || Pointee->isUnionType();
// }

HType *TypeBuilder::buildType(const CGNode &Node, Variance V,
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
      std::cerr << "Cyclic dependency forces the node become struct/union.\n";
      if (std::holds_alternative<UnionInfo>(
              TypeInfos.at(const_cast<CGNode *>(&Node)).Info)) {
        UnionDecl *Decl = UnionDecl::Create(Ctx, ValueNamer::getName(prefix));
        HType *Ret = Ctx.getPointerType(false, Parent.PointerSize,
                                        Ctx.getUnionType(false, Decl));
        NodeTypeMap.emplace(&Node, Ret);
        return Ret;
      } else {
        RecordDecl *Decl = RecordDecl::Create(Ctx, ValueNamer::getName(prefix));
        HType *Ret = Ctx.getPointerType(false, Parent.PointerSize,
                                        Ctx.getRecordType(false, Decl));
        NodeTypeMap.emplace(&Node, Ret);
        return Ret;
      }
    }
  }
  Visited.emplace(&Node);

  HType *Ret = nullptr;
  bool hasSetNodeMap = false;
  auto ETy = Node.getPNIVar()->getLatticeTy();

  if (Node.outEdges.empty()) {
    // no info from graph, just use LatticeTy.
    Ret = fromLowTy(Node.getPNIVar()->getLatticeTy(), BitSize);
  } else if (!Node.isPNIPointer()) {
    // No out edges.
    std::optional<std::shared_ptr<LatticeTy>> LT;
    for (auto &Edge : Node.outEdges) {
      if (const auto *FB = std::get_if<ForgetBase>(&Edge.getLabel())) {
        if (FB->Base.isPrimitive()) {
          auto L1 = createLatticeTy(ETy, V, FB->Base.getPrimitiveName());
          if (V == Covariant) {
            meet(LT, L1);
          } else {
            join(LT, L1);
          }
        }
      }
    }
    if (!LT) {
      LT = createLatticeTy(ETy, V, "");
    }

    Ret = fromLatticeTy(ETy, &**LT, BitSize);
  } else {
    auto &TI = TypeInfos.at(const_cast<CGNode *>(&Node));
    if (std::holds_alternative<SimpleTypeInfo>(TI.Info)) {
      auto &Info = std::get<SimpleTypeInfo>(TI.Info);
      assert(Info.Edge != nullptr && "TODO");
      // simple pointer type
      auto PointeeTy = buildType(Info.Edge->getTargetNode(), V, *TI.Size);
      Ret = getPtrTy(PointeeTy);
    } else if (std::holds_alternative<ArrayInfo>(TI.Info)) {
      auto &Info = std::get<ArrayInfo>(TI.Info);
      auto EdgeOff = getOffsetLabel(Info.Edge->getLabel());
      assert(EdgeOff->range.offset == 0);
      auto EdgeAcc = EdgeOff->range.access.front();

      auto ElemTy = buildType(Info.Edge->getTargetNode(), V, EdgeAcc.Size);
      if (!ElemTy->isPointerType()) {
        assert(false && "Array out edge must be pointer type");
      }
      ElemTy = ElemTy->getPointeeType();

      auto Count = *TI.Size / EdgeAcc.Size;
      assert(Count >= 1);
      auto ArrayTy = Ctx.getArrayType(false, ElemTy, Count);
      Ret = getPtrTy(ArrayTy);
    } else if (std::holds_alternative<StructInfo>(TI.Info)) {
      auto &Info = std::get<StructInfo>(TI.Info);

      // forward declare struct type, by inserting into the map.
      RecordDecl *Decl;
      if (NodeTypeMap.count(&Node)) {
        Ret = NodeTypeMap.at(&Node);
        Decl = Ret->getPointeeType()->getAsRecordDecl();
      } else {
        auto Name = ValueNamer::getName(prefix);
        Decl = RecordDecl::Create(Ctx, Name);
        Ret = getPtrTy(Ctx.getRecordType(false, Decl));
        NodeTypeMap.emplace(&Node, Ret);
      }
      hasSetNodeMap = true;

      for (size_t i = 0; i < Info.Fields.size(); i++) {
        auto &Ent = Info.Fields[i];
        auto Target = const_cast<CGNode *>(Ent.Target);
        HType *Ty;

        // for each field/edge
        Ty = buildType(*Target, V, Ent.R.Size);

        if (!Ty->isPointerType()) {
          assert(false && "Offset edge must be pointer type");
        }
        // the node is a field pointer type. get the field type.
        Ty = Ty->getPointeeType();

        auto FieldName = ValueNamer::getName("field_");
        Decl->addField(
            FieldDecl{.R = Ent.R,
                      .Type = Ty,
                      .Name = FieldName,
                      .Comment = "at offset: " + std::to_string(Ent.R.Start)});

        // Ent.Decl = Field;
      }
    } else if (std::holds_alternative<UnionInfo>(TI.Info)) {
      auto &Info = std::get<UnionInfo>(TI.Info);

      // forward declare union type, by inserting into the map.
      UnionDecl *Decl;
      if (NodeTypeMap.count(&Node)) {
        Ret = NodeTypeMap.at(&Node);
        Decl = Ret->getPointeeType()->getAsUnionDecl();
      } else {
        auto Name = ValueNamer::getName(prefix);
        Decl = UnionDecl::Create(Ctx, Name);
        Ret = getPtrTy(Ctx.getUnionType(false, Decl));
        NodeTypeMap.emplace(&Node, Ret);
      }
      hasSetNodeMap = true;

      auto R = Info.R;
      for (size_t i = 0; i < Info.Members.size(); i++) {
        auto &Edge = Info.Members[i];
        auto &Target = const_cast<CGNode &>(Edge->getTargetNode());
        HType *Ty;

        // for each field/edge
        Ty = buildType(Target, V, R.Size);

        if (!Ty->isPointerType()) {
          assert(false && "Offset edge must be pointer type");
        }
        // the node is a field pointer type. get the field type.
        Ty = Ty->getPointeeType();

        auto FieldName = ValueNamer::getName("field_");
        Decl->addMember(
            FieldDecl{.R = R,
                      .Type = Ty,
                      .Name = FieldName,
                      .Comment = "at offset: " + std::to_string(0)});
      }
    } else {
      assert(false && "Unknown TypeInfo");
    }
  }

  assert(Ret != nullptr);
  // // force the node to be struct type.
  // if (Node.isMemory() && !isRecordOrUnionPointer(Ret)) {
  //   assert(!hasSetNodeMap);
  //   assert(isPNIPtr);
  //   auto Name = ValueNamer::getName(prefix);
  //   auto Decl = RecordDecl::Create(Ctx, Name);
  //   if (! Node.outEdges.empty())
  //   Decl->addField()
  //   Ret = Ctx.getPointerType(false, Parent.PointerSize,
  //   Ctx.getRecordType(false, Decl));
  // }

  if (!hasSetNodeMap) {
    NodeTypeMap.emplace(&Node, Ret);
  }
  return Ret;
}

} // namespace notdec::retypd
