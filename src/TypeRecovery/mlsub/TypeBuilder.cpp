#include "notdec/TypeRecovery/mlsub/TypeBuilder.h"
#include "binarysub/binarysub-core.h"
#include "binarysub/binarysub.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <llvm/Support/Debug.h>
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
using binarysub::UType;
using binarysub::UTypePtr;
using binarysub::UTypeVariable;
using binarysub::UUnion;

using FieldEntry = std::pair<SimpleRange, HType *>;

TypeBuilder::TypeBuilder(TypeBuilderContext &Parent)
    : Parent(Parent), Ctx(Parent.Ctx) {}

HType *TypeBuilder::getVoidPtr() {
  return Ctx.getPointerType(false, Parent.PointerSize * 8, nullptr);
}

HType *TypeBuilder::getTopType(std::uint32_t BitSize) {
  return Ctx.getTopType(false, BitSize);
}

HType *TypeBuilder::getBottomType(std::uint32_t BitSize) {
  return Ctx.getBottomType(false, BitSize);
}

HType *TypeBuilder::parsePrimitiveName(const std::string &Name,
                                       std::uint32_t BitSize) {
  // Integer types: "i8", "i16", "i32", "i64"
  if (Name == "sint") {
    return Ctx.getIntegerType(false, BitSize, false);
  }

  // Unsigned integer types: "u8", "u16", "u32", "u64"
  if (Name == "uint") {
    return Ctx.getIntegerType(false, BitSize, true);
  }

  // Floating point types
  if (Name == "float") {
    return Ctx.getFloatType(false, BitSize);
  }
  if (Name == "double") {
    assert(BitSize == 64);
    return Ctx.getFloatType(false, BitSize);
  }

  // Char and bool
  if (Name == "char") {
    return Ctx.getChar();
  }
  if (Name == "bool") {
    return Ctx.getBool();
  }

  assert(false && "Unknown primitive type!");
}

std::optional<ast::RecordDecl *>
TypeBuilder::getStructOrNull(binarysub::UTypePtr Ty) {
  // Check cache first
  auto CacheIt = TypeCache.find(Ty);
  if (CacheIt == TypeCache.end()) {
    return std::nullopt;
  }
  auto *HTy = CacheIt->second;
  if (HTy->isRecordType()) {
    return HTy->getAsRecordDecl();
  }
  return std::nullopt;
}

ast::RecordDecl *TypeBuilder::getOrCreateStruct(binarysub::UTypePtr Ty) {
  auto It = getStructOrNull(Ty);
  // create as struct ptr, if not in type cache
  if (!It.has_value()) {
    RecordDecl *Decl = RecordDecl::Create(Ctx, ValueNamer::getName("struct_"));
    HType *Ret = Ctx.getRecordPtrType(false, Decl);
    TypeCache[Ty] = Ret;
    It = Decl;
  }
  return *It;
}

HType *TypeBuilder::finalizeRecursiveType(const binarysub::UTypePtr &Ty,
                                          HType *Result) {
  auto ForceStructDecl = getStructOrNull(Ty);
  if (!ForceStructDecl) {
    return Result;
  }

  auto *Decl = ForceStructDecl.value();
  if (Result->isRecordType() && Result->getAsRecordDecl() == Decl) {
    return Result;
  }

  assert(Decl->getFields().empty());
  auto SizeBits = binarysub::get_size(Ty);
  auto SizeBytes = SizeBits == 0 ? 0 : (SizeBits + 7) / 8;
  if (SizeBytes == 0) {
    SizeBytes = Parent.PointerSize;
  }
  Decl->addField(ast::FieldDecl{
      .R = {.Start = 0, .Size = static_cast<OffsetTy>(SizeBytes)},
      .Type = Result,
      .Name = ValueNamer::getName("rec_"),
  });
  return TypeCache.at(Ty);
}

HType *TypeBuilder::convertFieldType(const binarysub::UTypePtr &Ty,
                                     std::optional<int64_t> FieldSizeBytes) {
  auto getFallbackBitSize = [&]() -> std::uint32_t {
    if (FieldSizeBytes.has_value()) {
      return static_cast<std::uint32_t>(*FieldSizeBytes * 8);
    }
    return binarysub::get_size(Ty);
  };
  auto convertPointerSide = [&](const UTypePtr &Side) -> HType * {
    if (!Side) {
      return nullptr;
    }
    return convertFieldType(Side, std::nullopt);
  };

  if (std::get_if<UTop>(&Ty->v)) {
    return getTopType(getFallbackBitSize());
  } else if (std::get_if<UBot>(&Ty->v)) {
    return getBottomType(getFallbackBitSize());
  } else if (auto *V = std::get_if<UPrimitiveType>(&Ty->v)) {
    return parsePrimitiveName(V->name, binarysub::get_size(Ty));
  } else if (auto *V = std::get_if<UTypeVariable>(&Ty->v)) {
    return convertVariable(*V);
  } else if (auto *V = std::get_if<UFunctionType>(&Ty->v)) {
    std::vector<HType *> Params;
    for (auto &P : V->args) {
      Params.push_back(convertFieldType(P, std::nullopt));
    }
    std::vector<HType *> RetTypes;
    if (V->result) {
      RetTypes.push_back(convertFieldType(V->result, std::nullopt));
    }
    auto FTy = Ctx.getFunctionType(false, RetTypes, Params);
    return getPtrTy(FTy);
  } else if (auto *V = std::get_if<UPointerType>(&Ty->v)) {
    // Preserve leaf pointer load/store semantics instead of approximating them
    // into a single member type. Under the current single-layer RecordPtr model,
    // plain PointerType still means "the field value is a normal C-like
    // pointer", while DualPointerType means "the field value is an address-like
    // object with separate load/store views".
    HType *LoadTy = convertPointerSide(V->load);
    HType *StoreTy = convertPointerSide(V->store);
    return Ctx.getDualPointerType(false, Parent.PointerSize * 8, LoadTy,
                                  StoreTy);
  } else if (auto *PT = std::get_if<URecordType>(&Ty->v)) {
    auto &T = *PT;
    std::vector<std::pair<OffsetRange, UTypePtr>> RawFields;
    for (const auto &field : T.fields) {
      RawFields.push_back({OffsetRange::fromStr(field.first), field.second});
    }
    return convertStruct(Ty, RawFields, FieldSizeBytes);
  } else if (auto *V = std::get_if<UUnion>(&Ty->v)) {
    return doUnion(convertFieldType(V->lhs, FieldSizeBytes),
                   convertFieldType(V->rhs, FieldSizeBytes));
  } else if (auto *V = std::get_if<UInter>(&Ty->v)) {
    return doInter(convertFieldType(V->lhs, FieldSizeBytes),
                   convertFieldType(V->rhs, FieldSizeBytes));
  } else if (auto *V = std::get_if<URecursiveType>(&Ty->v)) {
    return convertRecursive(Ty, *V);
  }
  assert(false && "Unhandled field UType variant");
}

HType *TypeBuilder::convertRecursive(const binarysub::UTypePtr &Ty,
                                     const binarysub::URecursiveType &T) {
  getOrCreateStruct(Ty);
  HType *Anchor = TypeCache.at(Ty);

  auto NameIt = RecursiveTypeNames.find(T.name);
  assert((NameIt == RecursiveTypeNames.end() || NameIt->second == Anchor) &&
         "Recursive type name rebound to a different anchor");
  RecursiveTypeNames[T.name] = Anchor;

  auto [_, Inserted] = InProgress.insert(Ty);
  assert(Inserted && "Recursive type should not be re-entered before caching");

  HType *Body = convert(T.body);

  InProgress.erase(Ty);
  RecursiveTypeNames.erase(T.name);
  return finalizeRecursiveType(Ty, Body);
}

HType *TypeBuilder::convert(UTypePtr Ty) {

  // Check cache first
  auto CacheIt = TypeCache.find(Ty);
  if (CacheIt != TypeCache.end()) {
    return CacheIt->second;
  }

  if (auto *V = std::get_if<URecursiveType>(&Ty->v)) {
    return convertRecursive(Ty, *V);
  }

  // Cycles must be anchored by an explicit URecursiveType.
  if (InProgress.count(Ty)) {
    llvm::errs() << "Unexpected recursive UType without URecursiveType: "
                 << binarysub::printType(Ty) << "\n";
    assert(false &&
           "Unexpected recursive UType without explicit URecursiveType");
  }

  InProgress.insert(Ty);

  HType *Result = nullptr;
  // 先处理非指针类型
  if (std::get_if<UTop>(&Ty->v)) {
    Result = getTopType(binarysub::get_size(Ty));
  } else if (std::get_if<UBot>(&Ty->v)) {
    Result = getBottomType(binarysub::get_size(Ty));
  } else if (auto *V = std::get_if<UPrimitiveType>(&Ty->v)) {
    // TODO 这里是不是应该用到ObjSize大小。而不是名字里面带大小。
    Result = parsePrimitiveName(V->name, binarysub::get_size(Ty));
  } else if (auto *V = std::get_if<UFunctionType>(&Ty->v)) {
    // 最后返回函数指针类型
    std::vector<HType *> Params;
    for (auto &P : V->args) {
      Params.push_back(convert(P));
    }
    std::vector<HType *> RetTypes;
    if (V->result) {
      RetTypes.push_back(convert(V->result));
    }
    auto FTy = Ctx.getFunctionType(false, RetTypes, Params);
    Result = getPtrTy(FTy);
  } else if (auto *V = std::get_if<UTypeVariable>(&Ty->v)) {
    Result = convertVariable(*V);
  } else if (auto *V = std::get_if<UUnion>(&Ty->v)) {
    HType *LhsTy = convert(V->lhs);
    HType *RhsTy = convert(V->rhs);

    Result = doUnion(LhsTy, RhsTy);
  } else if (auto *V = std::get_if<UInter>(&Ty->v)) {
    HType *LhsTy = convert(V->lhs);
    HType *RhsTy = convert(V->rhs);
    Result = doInter(LhsTy, RhsTy);
  } else if (std::get_if<UPointerType>(&Ty->v)) {
    Result = convertPointer(Ty);
  } else if (std::get_if<URecordType>(&Ty->v)) {
    Result = convertPointer(Ty);
  } else {
    assert(false && "Unhandled UType variant");
  }

  InProgress.erase(Ty);
  TypeCache[Ty] = Result;
  return Result;
}

int64_t TypeBuilder::accessedPointeeSizeInBits(const binarysub::UTypePtr &Ty) {
  if (std::get_if<UTop>(&Ty->v)) {
    assert(false && "Impossible UType variant");
  } else if (auto *V = std::get_if<UBot>(&Ty->v)) {
    return V->size;
  } else if (std::get_if<UPrimitiveType>(&Ty->v)) {
    assert(false && "Impossible UType variant");
  } else if (auto *V = std::get_if<UPointerType>(&Ty->v)) {
    // Stop recursive at the pointer type.
    return V->psize;
  } else if (std::get_if<UFunctionType>(&Ty->v)) {
    assert(false && "Impossible UType variant");
  } else if (auto *V = std::get_if<URecursiveType>(&Ty->v)) {
    return accessedPointeeSizeInBits(V->body);
  } else if (auto *V = std::get_if<UTypeVariable>(&Ty->v)) {
    return V->size;
  } else if (auto *V = std::get_if<UUnion>(&Ty->v)) {
    return std::max(accessedPointeeSizeInBits(V->lhs),
                    accessedPointeeSizeInBits(V->rhs));
  } else if (auto *V = std::get_if<UInter>(&Ty->v)) {
    return std::max(accessedPointeeSizeInBits(V->lhs),
                    accessedPointeeSizeInBits(V->rhs));
  } else if (auto *V = std::get_if<URecordType>(&Ty->v)) {
    auto MaxElem = 0;
    for (const auto &field : V->fields) {
      auto OR = OffsetRange::fromStr(field.first);
      auto PointeeSizeInBits = accessedPointeeSizeInBits(field.second);
      auto Size =
          OR.access.empty() ? OR.offset * 8 + PointeeSizeInBits
                            : OR.maxAccess() * 8;
      if (Size > MaxElem) {
        MaxElem = Size;
      }
    }
    return MaxElem;
  } else {
    assert(false && "Unhandled UType variant");
  }
}

// 根据要求的range创建结构体，按需插入padding。
// 1. 创建union panel的时候，创建匿名结构体。此时T==nullptr
// 2. 最后构造结构体的时候，按照range构造。此时T!=nullptr
// 如果指定了T，则会存入TypeCache。
HType *TypeBuilder::craftStruct(const std::vector<FieldEntry> &Fields,
                                std::optional<SimpleRange> ValidRange,
                                std::optional<std::string> Name,
                                const binarysub::UTypePtr *T) {
  if (!Name) {
    Name = ValueNamer::getName("struct_");
  }
  RecordDecl *PrevDecl = nullptr;
  if (T) {
    PrevDecl = getStructOrNull(*T).value_or(nullptr);
  }
  RecordDecl *Decl = PrevDecl;
  if (Decl == nullptr) {
    // 临时的结构体
    Decl = ast::RecordDecl::Create(Ctx, Name.value());
  }
  assert(Decl != nullptr);

  assert(Decl->getFields().empty());
  if (Fields.size() == 0) {
    if (!ValidRange) {
      assert(false);
    } else {
      // create a struct with only padding:
      auto FieldName = ValueNamer::getName("padding_");
      auto CurrentDecl = FieldDecl{
          .R = *ValidRange,
          .Type = Ctx.getArrayType(false, Ctx.getChar(), ValidRange->Size),
          .Name = FieldName,
          .Comment = "at offset: " + std::to_string(ValidRange->Start)};
      Decl->addField(CurrentDecl);
      return Ctx.getRecordPtrType(false, Decl);
    }
  }

  auto Current = Fields.front().first.Start;
  if (ValidRange) {
    Current = ValidRange->Start;
  }

  for (size_t i = 0; i < Fields.size(); i++) {
    auto &Ent = Fields[i];
    if (Ent.first.Size == 0) {
      llvm::errs() << "Warning: Skip zero sized field at offset: "
                   << Ent.first.Start << "\n";
      continue;
    }
    if (ValidRange) {
      auto IR = Ent.first.intersect(*ValidRange);
      if (IR != Ent.first && IR.Size == 0) {
        assert(false);
        // fully out of the range.
        llvm::errs() << "Warning: Skip field because of size or range";
        continue;
      }
    }

    HType *Ty = Ent.second;

    auto FieldName = ValueNamer::getName("field_");
    auto CurrentDecl =
        FieldDecl{.R = Ent.first,
                  .Type = Ty,
                  .Name = FieldName,
                  .Comment = "at offset: " + std::to_string(Ent.first.Start)};
    std::optional<FieldDecl> PaddingAfter = std::nullopt;

    // Try to calc expand end:
    // if no ValidRange, no next ent: ent.end()
    // if no valid range, has next ent: max(next.start, ent.end())
    // if has valid range, no next ent: max(ValidRang, ent.end())
    // if has valid range, has next end: min(ValidRange, next.start)
    auto ExpandEnd = Ent.first.end();
    // if there is space to next field
    if (i + 1 < Fields.size()) {
      auto &Next = Fields.at(i + 1);
      ExpandEnd = std::max(ExpandEnd, Next.first.Start);
    } else {
      // if no next field, use valid range.
      if (ValidRange) {
        ExpandEnd = ValidRange->end();
      }
    }

    // if it is char array, merge with elem
    if (Ty->isCharArrayType()) {
      auto j = i + 1;
      for (; j < Fields.size(); j++) {
        auto &EntJ = Fields[j];
        auto TyJ = EntJ.second;
        if (!TyJ->isCharType()) {
          break;
        }
        // merge to prev array.
        ExpandEnd = EntJ.first.end();

        if (j + 1 < Fields.size()) {
          auto &NextJ = Fields.at(j + 1);
          ExpandEnd = std::max(ExpandEnd, NextJ.first.Start);
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
      // TODO 检查ElemSize对不对。
      auto ElemSize = Ent.first.Size /
                      Ty->getAs<ast::ArrayType>()->getNumElements().value();
      auto NewCount = (ExpandEnd - Ent.first.Start) / ElemSize;
      CurrentDecl.R.Size = NewCount * ElemSize;
      auto OldArrTy = llvm::cast<ast::ArrayType>(CurrentDecl.Type);
      CurrentDecl.Type = OldArrTy->withSize(Ctx, NewCount);
    }

    // crop the type if intersecting
    if (ValidRange) {
      auto IR = Ent.first.intersect(*ValidRange);
      // intersecting member?
      if (IR.Size < Ent.first.Size) {
        assert(false && "TODO: intersecting member?");
        // llvm::errs() << "Warning: Field intersect with PointeeSize!! "
        //              << Decl->getName() << Ent.first.str() << "\n";
        // std::pair<HType *, SimpleRange> Ent1 = cutType(
        //     Ctx, Ty, Ent.first.Size,
        //     SimpleRange{.Start = IR.Start - Ent.first.Start, .Size =
        //     IR.Size});
        // if (Ent1.second.Size == 0) {
        //   llvm::dbgs() << "Warning: Crop field failed, Skipping: "
        //                << Decl->getName() << Ent.first.str() << "\n";
        //   continue;
        // }
        // Ent.first = Ent1.second;
        // Ty = Ent1.first;
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
          .R = SimpleRange{.Start = CurrentDecl.R.end(), .Size = PaddingSize},
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

  if (Decl->getFields().size() == 0) {
    llvm::errs() << "Error: Empty Struct?\n";
  }

  if (!PrevDecl && Decl->getFields().size() == 0) {
    return getVoidPtr();
  }
  return Ctx.getRecordPtrType(false, Decl);
}

HType *TypeBuilder::convertStruct(
    const binarysub::UTypePtr &T,
    std::vector<std::pair<OffsetRange, UTypePtr>> &RawFields,
    std::optional<int64_t> PointeeSize) {
  HType *Result = nullptr;

  std::vector<FieldEntry> Fields;

  // 1 处理所有数组类型的成员
  // 提取所有乘数项
  std::set<int64_t> AllStrides;
  std::vector<std::pair<OffsetRange, UTypePtr>> RemainingEntries = RawFields;
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
        HasStrideEntries.emplace(HasStrideEntries.end(), *It);
        RemainingEntries.erase(It);
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
      HasStrideEntries.erase(HasStrideEntries.begin() + RemoveStart.value(),
                             HasStrideEntries.begin() + RemoveEnd.value());
      assert(InitialSize == (HasStrideEntries.size() + InRangeEnts.size()));

      // 去掉所有的最大stride，转换为子问题，递归处理，作为数组类型
      std::vector<std::pair<OffsetRange, UTypePtr>> SubProblem;
      for (auto Ent : InRangeEnts) {
        auto NewOffsetRange = Ent.first;
        NewOffsetRange.offset -= RangeStart;
        auto &AccArr = NewOffsetRange.access;
        AccArr.erase(std::remove(AccArr.begin(), AccArr.end(), MaxStride),
                     AccArr.end());
        SubProblem.push_back({NewOffsetRange, Ent.second});
      }
      // 因为是数组，必须得是这个大小
      auto MemberTy = convertStruct(T, SubProblem, MaxStride);
      auto ArrTy = Ctx.getArrayType(false, MemberTy, std::nullopt);
      Fields.push_back(
          {SimpleRange{.Start = RangeStart, .Size = MaxStride}, ArrTy});
    }
  }
  // 已经没有任何数组访问模式，接下来创建结构体和union类型
  if (RemainingEntries.size() > 0) {
    for (auto &Ent : RemainingEntries) {
      // 肯定是非数组
      assert(Ent.first.access.empty());
      auto SizeInBits = accessedPointeeSizeInBits(Ent.second);
      auto Size = SizeInBits <= 0 ? 0 : (SizeInBits + 7) / 8;
      Fields.push_back({SimpleRange{.Start = Ent.first.offset, .Size = Size},
                        convertFieldType(Ent.second, Size)});
    }
  }

  auto IsOverlap = [](OffsetTy S1, OffsetTy E1, OffsetTy S2, OffsetTy E2) {
    assert(S1 < E1);
    assert(S2 < E2);
    if (std::max(S1, S2) < std::min(E1, E2)) {
      return true;
    }
    return false;
  };

  // Fields should not be mutated during the lifetime of ret vector
  auto FilterInRange =
      [&](const std::vector<std::pair<SimpleRange, HType *>> &Fields,
          OffsetTy Start, OffsetTy End) -> std::vector<size_t> {
    std::vector<size_t> Ret;
    for (size_t I = 0; I < Fields.size(); I++) {
      auto &F = Fields.at(I);
      auto FS = F.first.Start;
      auto FE = F.first.Size + FS;
      // overlaps
      if (IsOverlap(Start, End, FS, FE)) {
        Ret.push_back(I);
      }
    }
    return Ret;
  };

  while (true) {
    // 收集所有分割点
    std::set<OffsetTy> AllIndex;
    for (auto &F : Fields) {
      AllIndex.insert(F.first.Start);
      AllIndex.insert(F.first.end());
    }
    // 合并所有重叠：遍历所有最小范围区间，如果出现重叠则以此开始创建union类型。
    bool NoUpdate = true;
    for (auto It = AllIndex.begin(); It != AllIndex.end(); ++It) {
      auto NextIt = std::next(It);
      if (NextIt == AllIndex.end()) {
        break;
      }
      auto Start = *It;
      auto End = *NextIt;
      auto InRangeFieldIndex = FilterInRange(Fields, Start, End);
      if (InRangeFieldIndex.size() <= 1) {
        continue;
      }
      NoUpdate = false;

      // create a union here.
      decltype(InRangeFieldIndex.size()) OldSize = 1;
      auto NewSize = InRangeFieldIndex.size();
      while (
          NewSize >
          OldSize) { // 根据小的重叠区域，左右拓展找到需要处理创建union的所有Fields。
        OldSize = NewSize;
        for (auto Ind : InRangeFieldIndex) {
          auto &F = Fields.at(Ind);
          Start = std::min(Start, F.first.Start);
          End = std::max(End, F.first.end());
        }
        InRangeFieldIndex = FilterInRange(Fields, Start, End);
        NewSize = InRangeFieldIndex.size();
        assert(NewSize >= 2);
      }

      auto UnionStart = Start;
      auto UnionEnd = End;
      std::vector<FieldEntry> OtherFields;
      std::vector<FieldEntry> OverlapFields;
      for (auto &F : Fields) {
        if (IsOverlap(Start, End, F.first.Start, F.first.end())) {
          OverlapFields.push_back(FieldEntry{
              {.Start = F.first.Start - UnionStart, .Size = F.first.Size},
              F.second});
        } else {
          OtherFields.push_back(F);
        }
      }

      // #region build members using OverlapFields;
      // sort the entry by end offset.
      std::sort(OverlapFields.begin(), OverlapFields.end(),
                [](const FieldEntry &A, const FieldEntry &B) {
                  return A.first.end() < B.first.end();
                });
      // use std::min to find the min start offset.
      auto MinStartOff =
          std::min_element(OverlapFields.begin(), OverlapFields.end(),
                           [](const FieldEntry &A, const FieldEntry &B) {
                             return A.first.Start < B.first.Start;
                           })
              ->first.Start;
      auto MaxOff = OverlapFields.back().first.end();
      // unified start to 0
      assert(MinStartOff == 0);
      assert(MaxOff == (UnionEnd - UnionStart));
      // after determinize, there will not be nested struct. We assume
      // Offset
      // to struct == Min Start Offset. So set size as MaxOff - MinStartOff.
      auto OurSize = MaxOff - MinStartOff;
      std::vector<std::vector<FieldEntry>> UnionPanels;
      for (auto &F : OverlapFields) {
        bool inserted = false;
        for (auto &Panel : UnionPanels) {
          if (Panel.back().first.Start + Panel.back().first.Size <=
              F.first.Start) {
            Panel.push_back(F);
            inserted = true;
            break;
          }
        }
        if (!inserted) {
          UnionPanels.push_back({F});
        }
      }
      assert(UnionPanels.size() > 1);
      // create new node for each panel struct.
      std::vector<HType *> Members;
      for (auto &Panel : UnionPanels) {
        if (Panel.size() == 1) {
          // we do not need to create a struct
          Members.push_back(Panel.front().second);
          continue;
        }
        // directly create a struct here
        std::string Name = ValueNamer::getName("Us_");
        auto E1 = craftStruct(Panel, SimpleRange{.Start = 0, .Size = OurSize},
                              Name, nullptr);
        Members.push_back(E1);
      }
      if (Members.empty()) {
        llvm::errs() << "Warning: Empty union!\n";
      }
      // create union
      auto Name = ValueNamer::getName("union_");
      auto Decl = UnionDecl::Create(Ctx, Name);
      for (auto Ent : Members) {
        auto FieldName = ValueNamer::getName("field_");
        // Union需要起始大小是0，然后每一项大小都是OurSize。
        Decl->addMember(ast::FieldDecl{.R = {.Start = 0, .Size = OurSize},
                                       .Type = Ent,
                                       .Name = FieldName});
      }
      // push the merged union back to fields, and iterate again
      OtherFields.push_back({FieldEntry{
          SimpleRange{.Start = UnionStart + MinStartOff, .Size = OurSize},
          Ctx.getUnionType(false, Decl)}});
      assert(NoUpdate == false);
      // #endregion build members using OverlapFields;
      // reiterate with merged fields.
      Fields = OtherFields;
      break;
    }
    // no overlap, cannot create unions
    if (NoUpdate) {
      break;
    }
  } // end of while true
  // Now there is no overlap, create struct for Fields.
  if (Fields.empty()) {
    // TODO create empty struct?
  }
  // sort the entry by start offset.
  std::sort(Fields.begin(), Fields.end(),
            [](const FieldEntry &A, const FieldEntry &B) {
              return A.first.Start < B.first.Start;
            });
  auto MaxEndOff =
      std::max_element(Fields.begin(), Fields.end(),
                       [](const FieldEntry &A, const FieldEntry &B) {
                         return A.first.end() < B.first.end();
                       })
          ->first.end();
  // TODO
  // 是否会在递归的时候，要求当前大小？下面的大小就当做不知道多大的时候猜测出来的值。
  auto Size = MaxEndOff - Fields.front().first.Start;
  assert(Size >= 0);
  if (PointeeSize) {
    if (PointeeSize.value() > Size) {
      Size = PointeeSize.value();
    }
    assert(!(PointeeSize.value() < Size) && "TODO");
  }
  if (Fields.empty()) {
    llvm::errs() << "Warning: Empty struct!\n";
  }

  Result = craftStruct(
      Fields, SimpleRange{.Start = Fields.front().first.Start, .Size = Size},
      std::nullopt, &T);
  return Result;
}

HType *TypeBuilder::convertPointer(const binarysub::UTypePtr &Ty,
                                   std::optional<int64_t> PointeeSize) {

  HType *Ret = nullptr;
  if (std::holds_alternative<UBot>(Ty->v) ||
      std::holds_alternative<UTop>(Ty->v)) {
    return getVoidPtr();
  } else if (auto *V = std::get_if<UPointerType>(&Ty->v)) {
    auto convertPointerSide = [&](const UTypePtr &Side) -> HType * {
      if (!Side) {
        return nullptr;
      }
      if (std::get_if<UTop>(&Side->v) || std::get_if<UBot>(&Side->v)) {
        return nullptr;
      }
      return convert(Side);
    };
    HType *LoadTy = convertPointerSide(V->load);
    HType *StoreTy = convertPointerSide(V->store);
    Ret =
        Ctx.getDualPointerType(false, Parent.PointerSize * 8, LoadTy, StoreTy);
  } else if (auto *PT = std::get_if<URecordType>(&Ty->v)) {
    auto &T = *PT;
    if (T.fields.empty()) {
      return getVoidPtr();
    }

    // 把field字符串都实例化为OffsetRange。
    std::vector<std::pair<OffsetRange, UTypePtr>> RawFields;
    for (const auto &field : T.fields) {
      RawFields.push_back({OffsetRange::fromStr(field.first), field.second});
    }
    Ret = convertStruct(Ty, RawFields, PointeeSize);
  } else if (auto *V = std::get_if<UUnion>(&Ty->v)) {
    HType *LhsTy = convert(V->lhs);
    HType *RhsTy = convert(V->rhs);
    Ret = doUnion(LhsTy, RhsTy);
  } else if (auto *V = std::get_if<UInter>(&Ty->v)) {
    HType *LhsTy = convert(V->lhs);
    HType *RhsTy = convert(V->rhs);
    Ret = doInter(LhsTy, RhsTy);
  } else {
    assert(false && "Unhandled Pointer UType variant");
  }
  assert((Ret->isPointerType() || Ret->isDualPointerType() ||
          Ret->isRecordType()) &&
         "Pointer conversion must preserve address-like semantics");
  return Ret;
}

HType *TypeBuilder::doUnion(HType *LhsTy, HType *RhsTy) {
  // Keep obvious identities collapsed, but otherwise preserve the
  // set-theoretic union in HType instead of forcing an early backend-oriented
  // approximation.
  if (LhsTy == RhsTy) {
    return LhsTy;
  }
  if (LhsTy->getCanonicalType() == RhsTy->getCanonicalType()) {
    return LhsTy->getCanonicalType();
  }

  // Canonicalize operand order so semantically identical unions unique to the
  // same HType node.
  if (std::less<HType *>{}(RhsTy->getCanonicalType(),
                           LhsTy->getCanonicalType())) {
    std::swap(LhsTy, RhsTy);
  }
  return Ctx.getSetUnionType(false, LhsTy, RhsTy);
}

HType *TypeBuilder::doInter(HType *LhsTy, HType *RhsTy) {
  // Same rationale as doUnion(): preserve semantic information unless the
  // intersection is trivially identical.
  if (LhsTy == RhsTy) {
    return LhsTy;
  }
  if (LhsTy->getCanonicalType() == RhsTy->getCanonicalType()) {
    return LhsTy->getCanonicalType();
  }
  if (std::less<HType *>{}(RhsTy->getCanonicalType(),
                           LhsTy->getCanonicalType())) {
    std::swap(LhsTy, RhsTy);
  }
  return Ctx.getSetInterType(false, LhsTy, RhsTy);
}

HType *TypeBuilder::convertVariable(const binarysub::UTypeVariable &T) {
  // For type variables, check if we have a recursive binding
  auto It = RecursiveTypeNames.find(T.name);
  if (It != RecursiveTypeNames.end()) {
    return It->second;
  }

  // Otherwise, create a TypeVariableType to preserve the semantic information
  return Ctx.getTypeVariableType(false, T.name, T.size);
}

} // namespace notdec::mlsub
