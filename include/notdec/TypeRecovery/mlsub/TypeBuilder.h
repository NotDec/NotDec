#ifndef _NOTDEC_MLSUB_TYPEBUILDER_H_
#define _NOTDEC_MLSUB_TYPEBUILDER_H_

#include "binarysub/binarysub.h"
#include "notdec-llvm2c/Interface/HType.h"
#include <llvm/IR/DataLayout.h>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace notdec::mlsub {

using notdec::ast::HType;
using notdec::ast::HTypeContext;

struct TypeBuilderContext {
  HTypeContext &Ctx;
  const llvm::DataLayout &DL;
  // Stored in bytes to match DataLayout and memory ranges.
  const unsigned PointerSize;
  const binarysub::StructMergeInfo *StructMerge;
  const std::map<std::string, std::vector<std::uint32_t>>
      *StructMergeRootGroups;

  TypeBuilderContext(HTypeContext &Ctx, const llvm::DataLayout &DL,
                     const binarysub::StructMergeInfo *StructMerge = nullptr,
                     const std::map<std::string, std::vector<std::uint32_t>>
                         *StructMergeRootGroups = nullptr)
      : Ctx(Ctx), DL(DL), PointerSize(DL.getPointerSize()),
        StructMerge(StructMerge), StructMergeRootGroups(StructMergeRootGroups) {}
};

class TypeBuilder {

  TypeBuilderContext &Parent;
  HTypeContext &Ctx;

  // Cache to avoid duplicate conversions
  std::map<binarysub::UTypePtr, HType *> TypeCache;

  // Handle explicit recursive bindings from UType. The binder identity is kept
  // separate from the C anchor decl so HType can preserve recursive back edges.
  std::map<std::string, ast::RecursiveBinder *> RecursiveTypeNames;
  std::set<binarysub::UTypePtr> InProgress; // cycle detection
  std::optional<std::string> CurrentRootDebugLabel;
  std::vector<std::string> CurrentDebugPath;
  unsigned ConvertStructTraceDepth = 0;
  std::map<std::uint32_t, std::set<std::uint32_t>> StructMergeGroupLabels;
  std::map<std::uint32_t, std::size_t> StructMergeGroupCandidateCounts;
  std::map<std::uint32_t, ast::RecordDecl *> StructMergeGroupDecls;
  std::optional<std::uint32_t> ActiveStructMergeGroup;
  // Late HType reuse cache. It only keys exact converted layouts, so it can
  // remove duplicate declarations after UType simplification without guessing
  // that two different layouts describe the same source struct.
  using RecordLayoutKey =
      std::pair<std::optional<SimpleRange>,
                std::vector<std::tuple<OffsetTy, OffsetTy, HType *>>>;
  std::map<RecordLayoutKey, ast::RecordDecl *> ExactRecordLayoutDecls;

public:
  TypeBuilder(TypeBuilderContext &Parent);
  void setDebugRootLabel(std::optional<std::string> Label);
  void pushDebugPath(std::string Frame);
  void popDebugPath();

  // Main conversion entry point
  HType *convert(binarysub::UTypePtr Ty);

protected:
  HType *convertRecursive(const binarysub::UTypePtr &Ty,
                          const binarysub::URecursiveType &T);
  HType *finalizeRecursiveType(const binarysub::UTypePtr &Ty, HType *Result);
  // 转换所有的Pointer类型
  // PointeeSize是专属于指针类型的，而且仅局限于指针类型的递归范围内。一旦指针被load或者store，PointeeSize就不再传递。
  HType *convertPointer(const binarysub::UTypePtr &T,
                        std::optional<int64_t> PointeeSize = std::nullopt);
  // 处理结构体指针访问
  // PreferElementType 只给 stride 数组元素递归使用：元素本身如果只是
  // offset 0 的单字段 shell，就直接返回字段类型，避免制造无意义结构体。
  HType *convertStruct(
      const binarysub::UTypePtr &T,
      std::vector<std::pair<OffsetRange, binarysub::UTypePtr>> &RawFields,
      std::optional<int64_t> PointeeSize,
      bool PreferElementType = false, bool IsCovariant = true);
  // 如果结构体成员已经递归转换为HType
  // *且处理了重叠问题，这个方法处理padding和size。
  HType *craftStruct(const std::vector<std::pair<SimpleRange, HType *>> &Fields,
                     std::optional<SimpleRange> ValidRange,
                     std::optional<std::string> Name,
                     const binarysub::UTypePtr *T,
                     bool IsCovariant = true);
  std::optional<ast::RecordDecl *> getStructOrNull(binarysub::UTypePtr Ty);
  ast::RecordDecl *getOrCreateStruct(binarysub::UTypePtr Ty);
  void initializeStructMergeInfo();
  std::optional<std::uint32_t>
  findStructMergeGroupForOrigins(const std::set<std::uint32_t> &Origins);
  std::optional<std::uint32_t>
  findStructMergeGroupForSet(const std::vector<binarysub::UTypePtr> &Terms);
  std::optional<std::uint32_t>
  findStructMergeGroupForType(const binarysub::UTypePtr &Ty);
  std::optional<std::uint32_t> findStructMergeGroupForCurrentRoot();
  ast::RecordDecl *getOrCreateStructMergeDecl(std::uint32_t GroupId);
  void bindStructMergeDecl(binarysub::UTypePtr Ty, std::uint32_t GroupId);
  RecordLayoutKey buildRecordLayoutKey(
      const std::vector<std::pair<SimpleRange, HType *>> &Fields,
      std::optional<SimpleRange> ValidRange) const;
  std::optional<ast::RecordDecl *> findExactRecordLayout(
      const std::vector<std::pair<SimpleRange, HType *>> &Fields,
      std::optional<SimpleRange> ValidRange) const;
  void rememberExactRecordLayout(
      ast::RecordDecl *Decl,
      const std::vector<std::pair<SimpleRange, HType *>> &Fields,
      std::optional<SimpleRange> ValidRange);
  int64_t accessedPointeeSizeInBits(const binarysub::UTypePtr &Ty);
  HType *convertFieldType(const binarysub::UTypePtr &Ty,
                          std::optional<int64_t> FieldSizeBytes,
                          bool IsCovariant = true);
  HType *getFieldValueTy(HType *FieldTy, bool IsCovariant = true);
  HType *doUnion(HType *Lhs, HType *Rhs);
  HType *doInter(HType *Lhs, HType *Rhs);
  HType *convertVariable(const binarysub::UTypeVariable &T);

  // Helper to get void pointer
  HType *getVoidPtr();
  HType *getTopType(std::uint32_t BitSize);
  HType *getBottomType(std::uint32_t BitSize);
  // Helper to parse primitive type name
  HType *parsePrimitiveName(const std::string &Name, std::uint32_t BitSize);

  HType *getPtrTy(HType *Pointee) {
    return Ctx.getPointerType(false, Parent.PointerSize * 8, Pointee);
  }
  HType *wrapFieldStorageTy(HType *ValueTy) { return getPtrTy(ValueTy); }
};

} // namespace notdec::mlsub

#endif
