#ifndef _NOTDEC_MLSUB_TYPEBUILDER_H_
#define _NOTDEC_MLSUB_TYPEBUILDER_H_

#include "binarysub/binarysub.h"
#include "notdec-llvm2c/Interface/HType.h"
#include <llvm/IR/DataLayout.h>
#include <map>
#include <set>
#include <string>

namespace notdec::mlsub {

using notdec::ast::HType;
using notdec::ast::HTypeContext;

struct TypeBuilderContext {
  HTypeContext &Ctx;
  const llvm::DataLayout &DL;
  const unsigned PointerSize;

  TypeBuilderContext(HTypeContext &Ctx, const llvm::DataLayout &DL)
      : Ctx(Ctx), DL(DL), PointerSize(DL.getPointerSize()) {}
};

class TypeBuilder {

  TypeBuilderContext &Parent;
  HTypeContext &Ctx;

  // Cache to avoid duplicate conversions
  std::map<binarysub::UTypePtr, HType *> TypeCache;

  // Handle recursive types
  std::map<std::string, HType *> RecursiveTypeNames;
  std::set<binarysub::UTypePtr> InProgress; // cycle detection

public:
  TypeBuilder(TypeBuilderContext &Parent);

  // Main conversion entry point
  HType *convert(binarysub::UTypePtr Ty);

protected:
  // 转换所有的Pointer类型
  // PointeeSize是专属于指针类型的，而且仅局限于指针类型的递归范围内。一旦指针被load或者store，PointeeSize就不再传递。
  HType *convertPointer(const binarysub::UTypePtr &T,
                        std::optional<int64_t> PointeeSize = std::nullopt);
  // 处理结构体指针访问
  HType *convertStruct(
      const binarysub::UTypePtr &T,
      std::vector<std::pair<OffsetRange, binarysub::UTypePtr>> &RawFields,
      std::optional<int64_t> PointeeSize);
  // 如果结构体成员已经递归转换为HType
  // *且处理了重叠问题，这个方法处理padding和size。
  HType *craftStruct(const std::vector<std::pair<SimpleRange, HType *>> &Fields,
                     std::optional<int64_t> PointeeSize,
                     std::optional<std::string> Name,
                     std::optional<ast::RecordDecl *> RDecl);
  std::optional<ast::RecordDecl *> getStructOrNull(binarysub::UTypePtr Ty);
  int64_t accessedPointeeSize(const binarysub::UTypePtr &Ty);
  HType *doUnion(HType *Lhs, HType *Rhs);
  HType *doInter(HType *Lhs, HType *Rhs);
  HType *convertVariable(const binarysub::UTypeVariable &T);

  // Helper to get void pointer
  HType *getVoidPtr();
  HType *getIntPtr();
  // Helper to parse primitive type name
  HType *parsePrimitiveName(const std::string &Name, std::uint32_t BitSize);

  HType *getPtrTy(HType *Pointee) {
    return Ctx.getPointerType(false, Parent.PointerSize, Pointee);
  }
};

} // namespace notdec::mlsub

#endif
