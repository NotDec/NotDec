#include "notdec/TypeRecovery/mlsub/TypeBuilder.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace notdec::mlsub {

using notdec::SimpleRange;
using notdec::ValueNamer;
using notdec::ast::FieldDecl;
using notdec::ast::HType;
using notdec::ast::RecordDecl;
using notdec::ast::UnionDecl;

using binarysub::UBot;
using binarysub::UFunctionType;
using binarysub::UInter;
using binarysub::UPointerType;
using binarysub::UPrimitiveType;
using binarysub::URecordType;
using binarysub::URecursiveType;
using binarysub::UTop;
using binarysub::UTypePtr;
using binarysub::UTypeVariable;
using binarysub::UUnion;

TypeBuilder::TypeBuilder(TypeBuilderContext &Parent)
    : Parent(Parent), Ctx(Parent.Ctx) {}

HType *TypeBuilder::getVoidPtr() {
  return Ctx.getPointerType(false, Parent.PointerSize, nullptr);
}

HType *TypeBuilder::parsePrimitiveName(const std::string &name) {
  // Integer types: "i8", "i16", "i32", "i64"
  if (name == "i8" || name == "sint8") {
    return Ctx.getIntegerType(false, 8, false);
  }
  if (name == "i16" || name == "sint16") {
    return Ctx.getIntegerType(false, 16, false);
  }
  if (name == "i32" || name == "sint32" || name == "sint" || name == "int") {
    return Ctx.getIntegerType(false, 32, false);
  }
  if (name == "i64" || name == "sint64") {
    return Ctx.getIntegerType(false, 64, false);
  }

  // Unsigned integer types: "u8", "u16", "u32", "u64"
  if (name == "u8" || name == "uint8") {
    return Ctx.getIntegerType(false, 8, true);
  }
  if (name == "u16" || name == "uint16") {
    return Ctx.getIntegerType(false, 16, true);
  }
  if (name == "u32" || name == "uint32" || name == "uint") {
    return Ctx.getIntegerType(false, 32, true);
  }
  if (name == "u64" || name == "uint64") {
    return Ctx.getIntegerType(false, 64, true);
  }

  // Floating point types
  if (name == "f32" || name == "float") {
    return Ctx.getFloatType(false, 32);
  }
  if (name == "f64" || name == "double") {
    return Ctx.getFloatType(false, 64);
  }

  // Char and bool
  if (name == "char") {
    return Ctx.getChar();
  }
  if (name == "bool") {
    return Ctx.getBool();
  }

  assert(false && "Unknown primitive type!");
}

HType *TypeBuilder::convert(UTypePtr Ty, std::optional<int64_t> PointeeSize) {
  if (!Ty) {
    return getVoidPtr();
  }

  // Check cache first
  auto CacheIt = TypeCache.find(Ty);
  if (CacheIt != TypeCache.end()) {
    return CacheIt->second;
  }

  // In progress, but not in type cache
  if (InProgress.count(Ty)) {
    // TODO what if union
    RecordDecl *Decl = RecordDecl::Create(Ctx, ValueNamer::getName("struct_"));
    HType *Ret = Ctx.getPointerType(false, Parent.PointerSize,
                                    Ctx.getRecordType(false, Decl));
    TypeCache[Ty] = Ret;
    // Return a type variable for cyclic types
    return Ret;
  }

  InProgress.insert(Ty);
  std::optional<std::string> NameHint;

  HType *Result = nullptr;
  if (auto *V = std::get_if<UTop>(&Ty->v)) {
    Result = getVoidPtr();
  } else if (auto *V = std::get_if<UBot>(&Ty->v)) {
    Result = getVoidPtr();
  } else if (auto *V = std::get_if<UPrimitiveType>(&Ty->v)) {
    Result = parsePrimitiveName(V->name);
  } else if (auto *V = std::get_if<UPointerType>(&Ty->v)) {
    HType *PteTy = getVoidPtr();
    // TODO show load type.
    if (V->store) {
      PteTy = convert(V->store);
    }
    if ((PteTy == nullptr || PteTy->isVoidPtrType()) && V->load) {
      PteTy = convert(V->load);
    }
    Result = Ctx.getPointerType(false, Parent.PointerSize, PteTy);
  } else if (auto *V = std::get_if<UFunctionType>(&Ty->v)) {
    assert(false && "TODO support function type");
  } else if (auto *V = std::get_if<URecursiveType>(&Ty->v)) {
    Result = convert(V->body, PointeeSize);
    NameHint = V->name;
  } else if (auto *V = std::get_if<UTypeVariable>(&Ty->v)) {
    Result = convertVariable(*V);
    NameHint = V->name;
  } else if (auto *V = std::get_if<UUnion>(&Ty->v)) {
    HType *LhsTy = convert(V->lhs, PointeeSize);
    HType *RhsTy = convert(V->rhs, PointeeSize);
    Result = doUnion(LhsTy, RhsTy);
  } else if (auto *V = std::get_if<UInter>(&Ty->v)) {
    HType *LhsTy = convert(V->lhs, PointeeSize);
    HType *RhsTy = convert(V->rhs, PointeeSize);
    Result = doInter(LhsTy, RhsTy);
  } else if (auto *V = std::get_if<URecordType>(&Ty->v)) {
    Result = convertRecord(*V);
  } else {
    assert(false && "Unhandled UType variant");
  }

  InProgress.erase(Ty);
  TypeCache[Ty] = Result;
  return Result;
}

HType *TypeBuilder::convertRecord(const binarysub::URecordType &T,
                                  std::optional<int64_t> PointeeSize) {
  // TODO 把field字符串都实例化为AccessRange然后分析，生成结构体和数组类型。

  if (T.fields.empty()) {
    return getVoidPtr();
  }

  // convert to OffsetRange map
  std::map<OffsetRange, UTypePtr> RawFields;
  for (const auto &field : T.fields) {
    RawFields.insert({OffsetRange::fromStr(field.first), field.second});
  }

  // 递归函数
  std::function<HType *(std::map<OffsetRange, UTypePtr> &)> doConv =
      [&](std::map<OffsetRange, UTypePtr> &RawFields) -> HType * {
    HType *Result = nullptr;

    auto isSimpleArray = [&]() -> std::pair<bool, uint64_t> {};

    auto [isArray, SizeHint] = isSimpleArray();
    // 处理仅有一条offset=0的简单情况：看
    if (RawFields.size() == 1 && RawFields.begin()->first.isZero()) {
      Result = convert(RawFields.begin()->second, PointeeSize);
    } else if (isArray) { // 处理是简单的数组的情况
      // merge all elements
      assert(false && "TODO");
    } else { // this is a struct or union.

      std::vector<std::pair<SimpleRange, HType*>> Fields;

      // 1 处理所有数组类型的成员
      // 提取所有乘数项
      std::set<int64_t> AllStrides;
      std::map<OffsetRange, UTypePtr> RemainingEntries = RawFields;
      for (auto &E : RawFields) {
        for (auto &A : E.first.access) {
          if (A.Size > 0) {
            AllStrides.insert(A.Size);
          }
        }
      }

      while (!AllStrides.empty()) {
        // TODO 改成Vector
        auto MaxStride = *AllStrides.rbegin();
        assert(MaxStride > 0);
        AllStrides.erase(MaxStride);

        std::vector<std::pair<OffsetRange, UTypePtr>> HasStrideEntries;
        for (auto It = RemainingEntries.begin(); It != RemainingEntries.end();
             ++It) {
          if (std::find(It->first.access.begin(), It->first.access.end(),
                        MaxStride) != It->first.access.end()) {
            // move from RemainingEntries to HasStrideEntries
            auto Ext = RemainingEntries.extract(It);
            HasStrideEntries.emplace(
                HasStrideEntries.end(),
                std::pair<OffsetRange, UTypePtr>{Ext.key(), Ext.mapped()});
          }
        }

        // 按基址从小到大排序
        std::sort(HasStrideEntries.begin(), HasStrideEntries.end(),
                  [](const std::pair<OffsetRange, UTypePtr> &E1,
                     const std::pair<OffsetRange, UTypePtr> &E2) {
                    return E1.first.offset < E2.first.offset;
                  });
        while (!HasStrideEntries.empty()) {
          auto &FrontOffset = HasStrideEntries.front().first;
          auto RangeStart = FrontOffset.offset;
          auto RangeEnd = FrontOffset.offset + MaxStride;

          // 提取当前组中Base在当前范围内的模式 InRangeEnts
          auto InitialSize = HasStrideEntries.size();
          std::vector<std::pair<OffsetRange, UTypePtr>> InRangeEnts;
          std::optional<size_t> RemoveStart = std::nullopt;
          std::optional<size_t> RemoveEnd = HasStrideEntries.size();
          for (size_t I = 0; I < HasStrideEntries.size(); I++) {
            auto &Ent = HasStrideEntries.at(I);
            auto &CurrentOffset = Ent.first;
            if (CurrentOffset.offset >= RangeStart &&
                CurrentOffset.offset < RangeEnd) {
              InRangeEnts.push_back(Ent);
              if (!RemoveStart) {
                RemoveStart = I;
              }
            } else {
              RemoveEnd = I;
              break;
            }
          }

          assert(RemoveStart);
          HasStrideEntries.erase(HasStrideEntries.begin() + *RemoveStart,
                                 HasStrideEntries.begin() + *RemoveEnd);
          assert(InitialSize == (HasStrideEntries.size() + InRangeEnts.size()));

          // 去掉所有的最大stride，转换为子问题，递归处理，作为数组类型
          std::map<OffsetRange, UTypePtr> SubProblem;
          for (auto Ent : InRangeEnts) {
            auto NewOffsetRange = Ent.first;
            NewOffsetRange.offset -= RangeStart;
            auto &AccArr = NewOffsetRange.access;
            AccArr.erase(std::remove(AccArr.begin(), AccArr.end(), MaxStride), AccArr.end());
            SubProblem.insert({NewOffsetRange, Ent.second});
          }
          auto MemberTy = doConv(SubProblem);
          auto ArrTy = Ctx.getArrayType(false, MemberTy, std::nullopt);
          Fields.push_back({SimpleRange{.Start = RangeStart, .Size = MaxStride}, ArrTy});
        }
      }
      // 已经转换为了非数组类型，接下来创建结构体和union类型
      if (RemainingEntries.size() > 0) {
        for (auto &Ent: RemainingEntries) {
          // 肯定是非数组
          assert(Ent.first.access.empty());
          Fields.push_back({SimpleRange{.Start=Ent.first.offset, .Size=1}, convert(Ent.second)});
        }
      }
    }
    return Result;
  };

  // // Create the record declaration
  // RecordDecl *Decl = nullptr;

  // if (Decl == nullptr) {
  //   Decl = ast::RecordDecl::Create(Ctx, ValueNamer::getName("struct_"));
  // }

  // int64_t currentOffset = 0;
  // for (const auto &field : T.fields) {
  //   HType *FieldType = convert(field.second);
  //   int64_t fieldSize = Parent.PointerSize; // default to pointer size

  //   // Try to determine field size from type
  //   if (FieldType->isIntType()) {
  //     auto *IntTy = llvm::cast<ast::IntegerType>(FieldType);
  //     fieldSize = IntTy->getBitSize() / 8;
  //   } else if (FieldType->isFloatType()) {
  //     auto *FloatTy = llvm::cast<ast::FloatingType>(FieldType);
  //     fieldSize = FloatTy->getBitSize() / 8;
  //   } else if (FieldType->isPointerType()) {
  //     fieldSize = Parent.PointerSize;
  //   }

  //   ast::FieldDecl FieldDecl{
  //       .R = SimpleRange{.Start = currentOffset, .Size = fieldSize},
  //       .Type = FieldType,
  //       .Name =
  //           field.first.empty() ? ValueNamer::getName("field_") :
  //           field.first,
  //       .Comment = ""};
  //   Decl->addField(FieldDecl);
  //   currentOffset += fieldSize;
  // }

  // // TODO if cached in map, then forced to be wrapped in a struct.

  // return Ctx.getPointerType(false, Parent.PointerSize,
  //                           Ctx.getRecordType(false, Decl));
}

HType *TypeBuilder::doUnion(HType *LhsTy, HType *RhsTy) {
  // If both are the same type, return it
  if (LhsTy == RhsTy) {
    return LhsTy;
  }

  // If one is void*, return the other
  if (LhsTy->isPointerType() && LhsTy->getPointeeType() == nullptr) {
    return RhsTy;
  }
  if (RhsTy->isPointerType() && RhsTy->getPointeeType() == nullptr) {
    return LhsTy;
  }

  // Otherwise, return void* as the common supertype
  return getVoidPtr();
}

HType *TypeBuilder::convertInter(const binarysub::UInter &T) {
  // For intersection types, try to merge the two branches
  // If they are the same, return that type; otherwise return the more
  // specific one
  HType *LhsTy = convert(T.lhs);
  HType *RhsTy = convert(T.rhs);

  // If both are the same type, return it
  if (LhsTy == RhsTy) {
    return LhsTy;
  }

  // If one is void*, return the other (more specific)
  if (LhsTy->isPointerType() && LhsTy->getPointeeType() == nullptr) {
    return RhsTy;
  }
  if (RhsTy->isPointerType() && RhsTy->getPointeeType() == nullptr) {
    return LhsTy;
  }

  // Default to lhs
  return LhsTy;
}

HType *TypeBuilder::convertVariable(const binarysub::UTypeVariable &T) {
  // For type variables, check if we have a recursive binding
  auto It = RecursiveTypeNames.find(T.name);
  if (It != RecursiveTypeNames.end()) {
    return It->second;
  }

  // Otherwise, create a TypeVariableType to preserve the semantic information
  return Ctx.getTypeVariableType(false, T.name);
}

} // namespace notdec::mlsub
