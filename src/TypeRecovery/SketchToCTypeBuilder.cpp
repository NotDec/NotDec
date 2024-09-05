#include "TypeRecovery/SketchToCTypeBuilder.h"
#include "Utils/Range.h"

namespace notdec::retypd {

clang::QualType fromLatticeElem(clang::ASTContext &Ctx, std::string Name,
                                unsigned BitSize) {
  if (Name == "top") {
    // TODO create typedef to unsigned int. e.g., typedef top32 uint32_t
    return Ctx.UnsignedIntTy;
  };
  if (BitSize == 1) {
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
SketchToCTypeBuilder::TypeBuilderImpl::visitType(const SketchNode &Node,
                                                 unsigned BitSize) {
  if (Visited.count(&Node)) {
    if (NodeTypeMap.count(&Node)) {
      return NodeTypeMap.at(&Node);
    } else {
      std::cerr << "Cyclic dependency forces the node become struct.\n";
      clang::RecordDecl *Decl = createStruct(Ctx);
      clang::QualType Ret = Ctx.getPointerType(Ctx.getRecordType(Decl));
      NodeTypeMap.emplace(&Node, Ret);
      return Ret;
    }
  }
  Visited.emplace(&Node);
  // TODO add attribute to contain the offset info.
  // Generally, a node represents a struct type

  // 1. no out edges, then it is a simple primitive type
  if (Node.outEdges.empty()) {
    return fromLatticeElem(Ctx, Node.Element, BitSize);
  }

  // std::map<OffsetLabel, SketchNode *> Off2Node;
  std::map<OffsetRange, clang::QualType> FieldMap;
  std::optional<std::pair<LoadLabel, SketchNode *>> Load2Node;
  std::optional<std::pair<StoreLabel, SketchNode *>> Store2Node;
  // handle all out edges
  for (auto &Edge : Node.outEdges) {
    auto &Target = const_cast<SketchNode &>(Edge.getTargetNode());
    if (auto *OL = std::get_if<OffsetLabel>(&Edge.getLabel())) {
      // handle offset edges:
      // must be struct type. Save to map early.
      if (!NodeTypeMap.count(&Node)) {
        // must be struct type
        clang::RecordDecl *Decl = createStruct(Ctx);
        //
        NodeTypeMap.emplace(&Node, Ctx.getPointerType(Ctx.getRecordType(Decl)));
        auto It = FieldMap.emplace(OL->range, visitType(Target, BitSize));
        assert(It.second && "Duplicate offset edge");
      }
    } else if (auto *LL = std::get_if<LoadLabel>(&Edge.getLabel())) {
      assert(!Load2Node);
      Load2Node.emplace(*LL, &Target);
    } else if (auto *SL = std::get_if<StoreLabel>(&Edge.getLabel())) {
      assert(!Store2Node);
      Store2Node.emplace(*SL, &Target);
    } else {
      assert(false && "Unknown edge type");
    }
  }

  // handle load/store edges
  clang::QualType LoadOrStoreTy;
  if (Load2Node && !Store2Node) {
    assert(Node.V == Covariant);
    LoadOrStoreTy = visitType(*Load2Node->second, Load2Node->first.Size);
  } else if (!Load2Node && Store2Node) {
    assert(Node.V == Contravariant);
    LoadOrStoreTy = visitType(*Store2Node->second, Store2Node->first.Size);
  } else if (Load2Node && Store2Node) {
    assert(false && "Is this case really exist?");
    // both load and store present, prefer load when covariant,
    if (Node.V == Covariant) {
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
