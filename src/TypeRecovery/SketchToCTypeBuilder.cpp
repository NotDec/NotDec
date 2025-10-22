
#include "clang/AST/Attr.h"
#include "clang/AST/Comment.h"
#include <cassert>
#include <clang/AST/ASTFwd.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <cstddef>
#include <cstdint>
#include <llvm/ADT/APSInt.h>
#include <llvm/Support/Debug.h>
#include <memory>
#include <optional>
#include <utility>
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
#include "notdec-llvm2c/Interface/ValueNamer.h"
#include "notdec-llvm2c/StructuralAnalysis.h"

#define DEBUG_TYPE "sketchtypebuilder"

namespace notdec::retypd {

using notdec::ast::FieldDecl;
using notdec::ast::HType;
using notdec::ast::RecordDecl;
using notdec::ast::UnionDecl;

HType *TypeBuilder::fromLatticeTy(PNTy Low, LatticeTy *LTy, unsigned BitSize) {
  if (!LTy) {
    return fromLowTy(Low, BitSize);
  }
  return LTy->buildType(Ctx);
}

HType *TypeBuilder::fromLowTy(PNTy LTy, unsigned BitSize) {
  auto Name = LTy.latticeStr();
  if (BitSize == 1 || startswith(Name, "bool")) {
    assert(BitSize == 1);
    return Ctx.getBool();
  }
  if (BitSize == 8 || startswith(Name, "char")) {
    return Ctx.getChar();
  }
  if (startswith(Name, "top") || startswith(Name, "unk")) {
    // TODO create typedef to unsigned int. e.g., typedef top32 uint32_t
    return getUndef(BitSize);
  }
  if (startswith(Name, "bottom")) {
    return getBot(BitSize);
  }
  if (startswith(Name, "float")) {
    return Ctx.getFloatType(false, 32);
  }
  if (startswith(Name, "double")) {
    return Ctx.getFloatType(false, 64);
  }
  if (startswith(Name, "ptr")) {
    return Ctx.getPointerType(false, Parent.PointerSize, nullptr);
  }
  if (startswith(Name, "int") || startswith(Name, "short") ||
      startswith(Name, "longlong") || startswith(Name, "uint") ||
      startswith(Name, "sint") || decodeSi(Name) || decodeUi(Name)) {
    // default to signed
    bool Signed = true;
    if (startswith(Name, "uint") || decodeUi(Name)) {
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

void TypeBuilder::buildNodeSizeHintMap(ConstraintsGenerator &G2) {
  NodeSizeHint = std::make_unique<std::map<const CGNode *, uint64_t>>();
  for (auto &Ent : G2.V2N) {
    auto *Node = Ent.second;
    if (Node == nullptr) {
      continue;
    }
    auto AS = getAllocSize(Ent.first);
    if (AS) {
      (*NodeSizeHint)[Node] = *AS;
    }
  }
  for (auto &Ent : G2.V2NContra) {
    if (G2.V2N.count(Ent.first)) {
      continue;
    }
    auto *Node = Ent.second;
    if (Node == nullptr) {
      continue;
    }
    auto AS = getAllocSize(Ent.first);
    if (AS) {
      (*NodeSizeHint)[Node] = *AS;
    }
  }
}

std::pair<HType *, SimpleRange> cutType(ast::HTypeContext &Ctx, HType *HT,
                                        OffsetTy OldSize, SimpleRange R) {
  std::pair<HType *, SimpleRange> Ret = {
      nullptr, SimpleRange{.Start = R.Start, .Size = 0}};
  if (HT->isIntType() || HT->isFloatType()) {
    return Ret;
  } else if (auto AT = HT->getAs<ast::ArrayType>()) {
    assert(AT->getNumElements() && "TODO: How to get array size?");
    auto ElemSize = OldSize / *AT->getNumElements();
    auto ElemCount = R.Size / ElemSize;
    // round up to array border
    auto Start = (R.Start + (ElemSize - 1)) / ElemSize * ElemSize;
    return {AT->withSize(Ctx, ElemCount),
            SimpleRange{.Start = Start, .Size = ElemCount * ElemSize}};
  } else {
    assert(false && "TODO");
  }

  return Ret;
}

HType *TypeBuilder::buildType(const CGNode &Node, Variance V,
                              std::optional<int64_t> PointeeSize) {
  unsigned BitSize = Node.getSize();
  const char *prefix = nullptr;
  if (Node.isMemory()) {
    prefix = "MEMORY_";
  }
  if (Visited.count(&Node)) {
    if (NodeTypeMap.count(&Node)) {
      return NodeTypeMap.at(&Node);
    } else {
      // Visited, but have not set a type (in DFS progress, i.e. visiting
      // dependency nodes).
      // TODO: remove debug output.
      std::cerr << "Cyclic dependency forces the node become struct/union: "
                << toString(Node.key) << "\n";
      if (std::holds_alternative<UnionInfo>(
              TypeInfos.at(const_cast<CGNode *>(&Node)).Info)) {
        UnionDecl *Decl = UnionDecl::Create(
            Ctx, ValueNamer::getName(prefix != nullptr ? prefix : "union_"));
        HType *Ret = Ctx.getPointerType(false, Parent.PointerSize,
                                        Ctx.getUnionType(false, Decl));
        NodeTypeMap.emplace(&Node, Ret);
        return Ret;
      } else {
        RecordDecl *Decl = RecordDecl::Create(
            Ctx, ValueNamer::getName(prefix != nullptr ? prefix : "struct_"));
        HType *Ret = Ctx.getPointerType(false, Parent.PointerSize,
                                        Ctx.getRecordType(false, Decl));
        NodeTypeMap.emplace(&Node, Ret);
        return Ret;
      }
    }
  }
  Visited.emplace(&Node);

  if (TraceIds.count(Node.getId())) {
    PRINT_TRACE(Node.getId())
        << "Building HType for " << toString(Node.key) << "...\n";
  }

  // get from previous size hint map.
  std::optional<int64_t> PointeeSize2 = getNodeSizeHint(&Node);
  if (PointeeSize && PointeeSize2) {
    assert(*PointeeSize == PointeeSize2);
  }
  if (!PointeeSize) {
    PointeeSize = PointeeSize2;
  }

  HType *Ret = nullptr;
  bool hasSetNodeMap = false;
  auto ETy = Node.getPNIVar()->getLatticeTy();

  if (!Node.isPNIPointer()) {
    if (!PointeeSize) {
      // No out edges.
      std::optional<std::shared_ptr<LatticeTy>> LT = std::nullopt;
      for (auto &Edge : Node.outEdges) {
        if (const auto *FB = Edge.getLabel().getAs<ForgetBase>()) {
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

      if (LT) {
        Ret = fromLatticeTy(ETy, &**LT, BitSize);
      } else {
        Ret = fromLowTy(Node.getPNIVar()->getLatticeTy(), BitSize);
      }
    } else {
      assert(BitSize == (Parent.PointerSize * 8));
      assert(PointeeSize > 0);
      // force as pointer.
      // Array type always cannot exist alone, only array pointer type.
      Ret = Ctx.getArrayType(false, Ctx.getChar(), PointeeSize);
      Ret = Ctx.getPointerType(false, Parent.PointerSize, Ret);
    }
  } else if (Node.outEdges.empty()) {
    assert(BitSize == (Parent.PointerSize * 8));
    if (!PointeeSize) {
      // no info from graph, just use LatticeTy. (void pointer)
      Ret = fromLowTy(Node.getPNIVar()->getLatticeTy(), BitSize);
    } else {
      assert(PointeeSize > 0);
      // Array type always cannot exist alone, only array pointer type.
      Ret = Ctx.getArrayType(false, Ctx.getChar(), PointeeSize);
      Ret = Ctx.getPointerType(false, Parent.PointerSize, Ret);
    }
  } else {
    auto &TI = TypeInfos.at(const_cast<CGNode *>(&Node));
    if (std::holds_alternative<SimpleTypeInfo>(TI.Info)) {
      auto &Info = std::get<SimpleTypeInfo>(TI.Info);
      if (Info.Edge == nullptr) {
        // void pointer?
        return getPtrTy(nullptr);
      }
      // assert(!PointeeSize || ((*PointeeSize) == *TI.Size));
      // simple pointer type
      auto PointeeTy = buildType(Info.Edge->getTargetNode(), V);
      Ret = getPtrTy(PointeeTy);
    } else if (std::holds_alternative<ArrayInfo>(TI.Info)) {
      auto &Info = std::get<ArrayInfo>(TI.Info);
      auto EdgeOff = getOffsetLabel(Info.Edge->getLabel());
      assert(EdgeOff->range.offset == 0);

      auto ElemTy = buildType(Info.Edge->getTargetNode(), V, *Info.ElemSize);
      if (!ElemTy->isPointerType()) {
        assert(false && "Array out edge must be pointer type");
      }
      ElemTy = ElemTy->getPointeeType();

      auto Count = *TI.Size / *Info.ElemSize;
      assert(Count >= 1);

      if (PointeeSize) {
        assert(PointeeSize > 0);
        if ((*PointeeSize % *Info.ElemSize) != 0) {
          llvm::errs() << "Warning: Non-aligned array type?\n";
        }
        Count = *PointeeSize / *Info.ElemSize;
      }

      auto ArrayTy = Ctx.getArrayType(false, ElemTy, Count);
      Ret = getPtrTy(ArrayTy);
    } else if (std::holds_alternative<StructInfo>(TI.Info)) {
      auto &Info = std::get<StructInfo>(TI.Info);

      std::optional<SimpleRange> ValidRange;
      if (PointeeSize) {
        if (*PointeeSize > 0) {
          ValidRange = {0, *PointeeSize};
        } else {
          assert(*PointeeSize != 0);
          ValidRange = {*PointeeSize, -*PointeeSize};
        }
      }

      // forward declare struct type, by inserting into the map.
      RecordDecl *Decl;
      if (NodeTypeMap.count(&Node)) {
        Ret = NodeTypeMap.at(&Node);
        Decl = Ret->getPointeeType()->getAsRecordDecl();
      } else {
        auto Name = ValueNamer::getName(prefix != nullptr ? prefix : "struct_");
        Decl = RecordDecl::Create(Ctx, Name);
        Ret = getPtrTy(Ctx.getRecordType(false, Decl));
        NodeTypeMap.emplace(&Node, Ret);
      }
      hasSetNodeMap = true;

      std::map<FieldEntry *, FieldDecl *> E2D;
      auto Current = Info.Fields.front().R.Start;
      if (ValidRange) {
        Current = ValidRange->Start;
      }
      for (size_t i = 0; i < Info.Fields.size(); i++) {
        // copy the entry
        auto Ent = Info.Fields[i];
        if (ValidRange) {
          auto IR = Ent.R.intersect(*ValidRange);
          if (IR.Size == 0) {
            // fully out of the range.
            LLVM_DEBUG(llvm::dbgs()
                       << "Warning: Skip field because of size or range");
            continue;
          }
        }

        auto Target = const_cast<CGNode *>(&Ent.Edge->getTargetNode());
        HType *Ty = nullptr;

        // for each field/edge
        Ty = buildType(*Target, V, Ent.R.Size);

        if (!Ty->isPointerType()) {
          assert(false && "Offset edge must be pointer type");
        }
        // the node is a field pointer type. get the field type.
        Ty = Ty->getPointeeType();

        // merge the char with previous char array
        // because this requires array expanding, cannot be separated to a new
        // pass.
        // FieldEntry *Prev = nullptr;
        // if (i > 0) {
        //   Prev = &Info.Fields[i - 1];
        // }
        // if (Prev != nullptr && Ty->isCharType() &&
        //     Decl->getFields().size() > 0) {
        //   auto &LastField = Decl->getFields().back();
        //   if (!LastField.isPadding && LastField.Type->isCharArrayType()) {
        //     if (LastField.R.end() == Ent.R.Start) {
        //       // merge into prev field
        //       LastField.R.Size += Ent.R.Size;
        //       auto* OldArrTy = llvm::cast<ArrayType>(LastField.Type);
        //       auto NewCount = LastField.R.Size /
        //       auto NewArrTy = OldArrTy.withSize(Ctx,);

        //     }
        //   }
        // }

        auto FieldName = ValueNamer::getName("field_");
        auto CurrentDecl =
            FieldDecl{.R = Ent.R,
                      .Type = Ty,
                      .Name = FieldName,
                      .Comment = "at offset: " + std::to_string(Ent.R.Start)};
        std::optional<FieldDecl> PaddingAfter = std::nullopt;

        // Try to calc expand end:
        // if no ValidRange, no next ent: ent.end()
        // if no valid range, has next ent: max(next.start, ent.end())
        // if has valid range, no next ent: max(ValidRang, ent.end())
        // if has valid range, has next end: min(ValidRange, next.start)
        auto ExpandEnd = Ent.R.end();
        // if there is space to next field
        if (i + 1 < Info.Fields.size()) {
          auto &Next = Info.Fields.at(i + 1);
          ExpandEnd = std::max(ExpandEnd, Next.R.Start);
        } else {
          // if no next field, use valid range.
          if (ValidRange) {
            ExpandEnd = ValidRange->end();
          }
        }

        // if it is char array and merge with elem
        if (Ty->isCharArrayType()) {
          auto j = i + 1;
          for (; j < Info.Fields.size(); j++) {
            auto &EntJ = Info.Fields[j];
            auto TargetJ = const_cast<CGNode *>(&EntJ.Edge->getTargetNode());
            auto TyJ = buildType(*TargetJ, V, EntJ.R.Size);
            TyJ = TyJ->getPointeeType();
            if (!TyJ->isCharType()) {
              break;
            }
            // merge to prev array.
            ExpandEnd = EntJ.R.end();

            if (j + 1 < Info.Fields.size()) {
              auto &NextJ = Info.Fields.at(j + 1);
              ExpandEnd = std::max(ExpandEnd, NextJ.R.Start);
            } else {
              if (ValidRange) {
                ExpandEnd = ValidRange->end();
              }
            }
          }
          // adjust i to skip some field
          i = j - 1;
        }
        // must less than valid range.
        if (ValidRange) {
          ExpandEnd = std::min(ExpandEnd, ValidRange->end());
        }

        // Try to expand array size.
        if (Ty->isArrayType()) {
          const ArrayInfo &TInfo =
              std::get<ArrayInfo>(TypeInfos.at(Target).Info);
          // expand the array size to the range
          assert(TInfo.ElemSize);
          auto ElemSize = *TInfo.ElemSize;
          auto NewCount = (ExpandEnd - Ent.R.Start) / ElemSize;
          CurrentDecl.R.Size = NewCount * ElemSize;
          auto OldArrTy = llvm::cast<ast::ArrayType>(CurrentDecl.Type);
          CurrentDecl.Type = OldArrTy->withSize(Ctx, NewCount);
        }

        // crop the type if intersecting
        if (ValidRange) {
          auto IR = Ent.R.intersect(*ValidRange);
          // intersecting member?
          if (IR.Size < Ent.R.Size) {
            LLVM_DEBUG(llvm::dbgs()
                       << "Warning: Field intersect with PointeeSize!! "
                       << Decl->getName() << Ent.R.str());
            std::pair<HType *, SimpleRange> Ent1 = cutType(
                Ctx, Ty, Ent.R.Size,
                SimpleRange{.Start = IR.Start - Ent.R.Start, .Size = IR.Size});
            if (Ent1.second.Size == 0) {
              LLVM_DEBUG(llvm::dbgs()
                         << "Warning: Crop field failed, Skipping: "
                         << Decl->getName() << Ent.R.str());
              continue;
            }
            Ent.R = Ent1.second;
            Ty = Ent1.first;
          }
        }

        std::optional<FieldDecl> PaddingBefore = std::nullopt;
        // Add padding at the beginning
        if (Current < CurrentDecl.R.Start) {
          auto PaddingSize = CurrentDecl.R.Start - Current;
          PaddingBefore = FieldDecl{
              .R = SimpleRange{.Start = Current, .Size = PaddingSize},
              .Type = Ctx.getArrayType(false, Ctx.getChar(), PaddingSize),
              .Name = ValueNamer::getName("padding_"),
              .Comment = "at offset: " + std::to_string(Current),
              .isPadding = true,
          };
          Current = CurrentDecl.R.Start;
        }

        Current = CurrentDecl.R.end();

        // add padding after if there is space
        if (CurrentDecl.R.end() < ExpandEnd) {
          auto PaddingSize = ExpandEnd - CurrentDecl.R.end();
          PaddingAfter = FieldDecl{
              .R = SimpleRange{.Start = CurrentDecl.R.end(),
                               .Size = PaddingSize},
              .Type = Ctx.getArrayType(false, Ctx.getChar(), PaddingSize),
              .Name = ValueNamer::getName("padding_"),
              .Comment = "at offset: " + std::to_string(CurrentDecl.R.end()),
              .isPadding = true,
          };
          Current = ExpandEnd;
        }
        if (PaddingBefore) {
          Decl->addField(*PaddingBefore);
        }
        Decl->addField(CurrentDecl);
        if (PaddingAfter) {
          Decl->addField(*PaddingAfter);
        }
      }
      if (Decl->getFields().size() == 0) {
        // add only padding
        if (ValidRange) {
          if (Current < ValidRange->end()) {
            auto PaddingSize = ValidRange->end() - Current;
            auto PaddingOnly = FieldDecl{
                .R = SimpleRange{.Start = Current, .Size = PaddingSize},
                .Type = Ctx.getArrayType(false, Ctx.getChar(), PaddingSize),
                .Name = ValueNamer::getName("padding_"),
                .Comment = "at offset: " + std::to_string(Current),
                .isPadding = true,
            };
            Current = ValidRange->end();
            Decl->addField(PaddingOnly);
          }
        }
      }

      // if there is only one field, return the field's type
      if (Decl->getFields().size() == 0) {
        Ret = getPtrTy(nullptr);
        if (hasSetNodeMap) {
          NodeTypeMap.insert_or_assign(&Node, Ret);
        }
      } else if (Decl->getFields().size() == 1 &&
                 Decl->getFields().front().R.Start == 0) {
        Ret = getPtrTy(Decl->getFields().front().Type);
        if (hasSetNodeMap) {
          NodeTypeMap.insert_or_assign(&Node, Ret);
        }
      }
    } else if (std::holds_alternative<UnionInfo>(TI.Info)) {
      auto &Info = std::get<UnionInfo>(TI.Info);

      // forward declare union type, by inserting into the map, in case of
      // cyclic dependency
      UnionDecl *Decl;
      if (NodeTypeMap.count(&Node)) {
        Ret = NodeTypeMap.at(&Node);
        Decl = Ret->getPointeeType()->getAsUnionDecl();
      } else {
        auto Name = ValueNamer::getName(prefix != nullptr ? prefix : "union_");
        Decl = UnionDecl::Create(Ctx, Name);
        Ret = getPtrTy(Ctx.getUnionType(false, Decl));
        auto It = NodeTypeMap.emplace(&Node, Ret);
        assert(It.second);
      }
      hasSetNodeMap = true;

      auto Size = PointeeSize ? PointeeSize : TI.Size;
      if (PointeeSize) {
        assert(*PointeeSize > 0);
      }
      for (size_t i = 0; i < Info.Members.size(); i++) {
        auto &Edge = Info.Members[i];
        auto &Target = const_cast<CGNode &>(Edge->getTargetNode());
        HType *Ty;

        // for each field/edge
        Ty = buildType(Target, V, Size);

        if (!Ty->isPointerType()) {
          assert(false && "Offset edge must be pointer type");
        }
        // the node is a field pointer type. get the field type.
        Ty = Ty->getPointeeType();

        auto FieldName = ValueNamer::getName("field_");
        Decl->addMember(FieldDecl{
            .R = {.Start = 0, .Size = *Size}, .Type = Ty, .Name = FieldName});
      }
    } else {
      assert(false && "Unknown TypeInfo");
    }
  }

  if (TraceIds.count(Node.getId())) {
    PRINT_TRACE(Node.getId()) << "HType for " << toString(Node.key) << ": "
                              << Ret->getAsString() << "\n";
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
    // not struct or union
    if (Ret->isPointerType()) {
      if (Ret->getPointeeType()) {
        assert(!Ret->getPointeeType()->isRecordType());
        assert(!Ret->getPointeeType()->isUnionType());
      }
    }
    if (NodeTypeMap.count(&Node)) {
      // if forced to be a struct
      auto StructPtrTy = NodeTypeMap.at(&Node);
      RecordDecl *Decl = StructPtrTy->getPointeeType()->getAsRecordDecl();
      assert(BitSize % 8 == 0);
      Decl->addField(FieldDecl{
          .R = SimpleRange{.Start = 0, .Size = BitSize / 8},
          .Type = Ret,
          .Name = ValueNamer::getName("field_"),
          .Comment = "at offset: 0",
      });
    } else {
      auto It = NodeTypeMap.emplace(&Node, Ret);
      assert(It.second);
    }
  }
  return Ret;
}

} // namespace notdec::retypd
