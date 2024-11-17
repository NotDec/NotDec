#include "TypeRecovery/SketchToCTypeBuilder.h"
#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/Schema.h"
#include "Utils/Range.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Comment.h"
#include <clang/AST/ASTFwd.h>
#include <clang/Basic/SourceLocation.h>

namespace notdec::retypd {

// TODO a better lattice representation
clang::QualType fromLatticeElem(clang::ASTContext &Ctx, std::string Name,
                                unsigned BitSize) {
  if (Name == "top") {
    // TODO create typedef to unsigned int. e.g., typedef top32 uint32_t
    return Ctx.UnsignedIntTy;
  };
  if (BitSize == 1 || Name == "bool") {
    return Ctx.BoolTy;
  }
  if (Name == "float") {
    return Ctx.FloatTy;
  }
  if (Name == "double") {
    return Ctx.DoubleTy;
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

clang::RecordDecl *createStruct(clang::ASTContext &Ctx) {
  auto *II = &Ctx.Idents.get(ValueNamer::getName("struct_"));
  // Create a Struct type for it.
  clang::RecordDecl *decl = clang::RecordDecl::Create(
      Ctx, clang::TagDecl::TagKind::TTK_Struct, Ctx.getTranslationUnitDecl(),
      clang::SourceLocation(), clang::SourceLocation(), II, nullptr);
  return decl;
}

clang::QualType
SketchToCTypeBuilder::TypeBuilderImpl::visitType(const CGNode &Node,
                                                 unsigned BitSize) {
  if (Visited.count(&Node)) {
    if (NodeTypeMap.count(&Node)) {
      return NodeTypeMap.at(&Node);
    } else {
      // Visited, but have not set a type (in progress, i.e. visiting dependency
      // nodes).
      // TODO: remove debug output.
      std::cerr << "Cyclic dependency forces the node become struct.\n";
      clang::RecordDecl *Decl = createStruct(Ctx);
      clang::QualType Ret = Ctx.getPointerType(Ctx.getRecordType(Decl));
      NodeTypeMap.emplace(&Node, Ret);
      return Ret;
    }
  }
  Visited.emplace(&Node);
  // Generally, a node represents a struct type

  // 1. no out edges, then it is top
  if (Node.outEdges.empty()) {
    // assert(false && "TODO");
    auto Ret = fromLatticeElem(Ctx, "top", BitSize);
    NodeTypeMap.emplace(&Node, Ret);
    return Ret;
  }
  // if only edges to #End with forget primitive, then it is a simple
  // primitive type
  std::string Var;
  bool AllForgetPrim = true;
  for (auto &Edge : Node.outEdges) {
    bool IsForgetPrim = false;
    if (const auto *FB = std::get_if<ForgetBase>(&Edge.getLabel())) {
      if (FB->Base.isPrimitive()) {
        IsForgetPrim = true;
        if (Var.empty()) {
          Var = FB->Base.getPrimitiveName();
        } else {
          if (Node.key.SuffixVariance == Covariant) {
            Var = meet(Var, FB->Base.getPrimitiveName());
          } else {
            Var = join(Var, FB->Base.getPrimitiveName());
          }
        }
      }
    }
    AllForgetPrim &= IsForgetPrim;
  }
  if (AllForgetPrim) {
    auto Ret = fromLatticeElem(Ctx, Var, BitSize);
    NodeTypeMap.emplace(&Node, Ret);
    return Ret;
  }

  // std::map<OffsetLabel, SketchNode *> Off2Node;
  std::map<OffsetRange, clang::QualType> FieldMap;
  std::optional<std::pair<LoadLabel, CGNode *>> Load2Node;
  std::optional<std::pair<StoreLabel, CGNode *>> Store2Node;
  // handle all out edges
  for (auto &Edge : Node.outEdges) {
    auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
    if (auto *RL = std::get_if<RecallLabel>(&Edge.getLabel())) {
      if (auto *OL = std::get_if<OffsetLabel>(&RL->label)) {
        // handle offset edges:
        // must be struct type. Save to map early.
        if (!NodeTypeMap.count(&Node)) {
          // must be struct type
          clang::RecordDecl *Decl = createStruct(Ctx);
          NodeTypeMap.emplace(&Node,
                              Ctx.getPointerType(Ctx.getRecordType(Decl)));
          auto It = FieldMap.emplace(OL->range, visitType(Target, BitSize));
          assert(It.second && "Duplicate offset edge");
        }
      } else if (auto *LL = std::get_if<LoadLabel>(&RL->label)) {
        // assert(!Load2Node);
        if (Load2Node) {
          llvm::errs()
              << "Warning: TypeBuilderImpl::visitType: multiple load edges: "
              << toString(Edge.getLabel()) << " and "
              << toString(Load2Node->first) << "\n";
        }
        Load2Node.emplace(*LL, &Target);
      } else if (auto *SL = std::get_if<StoreLabel>(&RL->label)) {
        if (Store2Node) {
          // TODO: create union?
          llvm::errs()
              << "Warning: TypeBuilderImpl::visitType: multiple load edges: "
              << toString(Edge.getLabel()) << " and "
              << toString(Store2Node->first) << "\n";
        }
        Store2Node.emplace(*SL, &Target);
      } else {
        assert(false && "Unknown edge type");
      }
    }
  }

  // handle load/store edges
  clang::QualType LoadOrStoreTy;
  if (Load2Node && !Store2Node) {
    // assert(Node.V == Covariant);
    LoadOrStoreTy = visitType(*Load2Node->second, Load2Node->first.Size);
  } else if (!Load2Node && Store2Node) {
    // assert(Node.V == Contravariant);
    LoadOrStoreTy = visitType(*Store2Node->second, Store2Node->first.Size);
  } else if (Load2Node && Store2Node) {
    // assert(false && "Is this case really exist?");
    // both load and store present, prefer load when covariant,
    if (Node.key.SuffixVariance == Covariant) {
      LoadOrStoreTy = visitType(*Load2Node->second, Load2Node->first.Size);
    } else {
      // prefer store when contravariant
      LoadOrStoreTy = visitType(*Store2Node->second, Store2Node->first.Size);
    }
  }

  // 3. has .load / .store edge, then it is a pointer type or struct pointer
  // type.
  // If inserted ahead of time, it must be a struct pointer type.
  if (NodeTypeMap.count(&Node)) {
    if (!LoadOrStoreTy.isNull()) {
      auto It = FieldMap.emplace(OffsetRange{}, LoadOrStoreTy);
      assert(It.second && "Duplicate zero offset");
    }

    // Get previous struct RecordDecl
    clang::QualType Ty = NodeTypeMap.at(&Node);
    clang::RecordDecl *Decl = Ty->getPointeeType()->getAsRecordDecl();
    // add all field into the struct, and finish struct declaration.
    Decl->startDefinition();
    for (auto &Ent : FieldMap) {
      // TODO add attribute to contain the offset info.
      // Generally, a node represents a struct type
      auto *FII = &Ctx.Idents.get(ValueNamer::getName("field_"));
      clang::FieldDecl *Field = clang::FieldDecl::Create(
          Ctx, Decl, clang::SourceLocation(), clang::SourceLocation(), FII,
          Ent.second, nullptr, nullptr, false, clang::ICIS_NoInit);

      bool useComment = false;
      if (useComment) {
        // TODO support comment. ASTWriter does not support comment?
        clang::comments::TextComment *TC =
            new (Ctx.getAllocator()) clang::comments::TextComment(
                clang::SourceLocation(), clang::SourceLocation(),
                "off:" + std::to_string(Ent.first.offset));
        clang::comments::ParagraphComment *PC =
            new (Ctx.getAllocator()) clang::comments::ParagraphComment(
                clang::ArrayRef<clang::comments::InlineContentComment *>(TC));
        clang::comments::FullComment *FC = new (Ctx.getAllocator())
            clang::comments::FullComment({PC}, nullptr);
        Ctx.ParsedComments[Field] = FC;
      } else {
        Field->addAttr(clang::AnnotateAttr::Create(
            Ctx, "off:" + std::to_string(Ent.first.offset), nullptr, 0,
            clang::AttributeCommonInfo(clang::SourceRange())));
      }

      Decl->addDecl(Field);
    }
    Decl->completeDefinition();
    Ctx.getTranslationUnitDecl()->addDecl(Decl);
    return Ty;
  } else {
    // else it is a simple pointer type
    assert(!LoadOrStoreTy.isNull());

    auto Ty = Ctx.getPointerType(LoadOrStoreTy);
    NodeTypeMap.emplace(&Node, Ty);
    return Ty;
  }
}

} // namespace notdec::retypd
