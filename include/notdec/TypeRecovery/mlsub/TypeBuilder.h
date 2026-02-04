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

private:
  // Handlers for each UType variant
  HType *convertTop(const binarysub::UTop &T);
  HType *convertBot(const binarysub::UBot &T);
  HType *convertPrimitive(const binarysub::UPrimitiveType &T);
  HType *convertPointer(const binarysub::UPointerType &T);
  HType *convertFunction(const binarysub::UFunctionType &T);
  HType *convertRecord(const binarysub::URecordType &T);
  HType *convertUnion(const binarysub::UUnion &T);
  HType *convertInter(const binarysub::UInter &T);
  HType *convertRecursive(const binarysub::URecursiveType &T);
  HType *convertVariable(const binarysub::UTypeVariable &T);

  // Helper to get void pointer
  HType *getVoidPtr();
  // Helper to parse primitive type name
  HType *parsePrimitiveName(const std::string &name);
};

} // namespace notdec::mlsub

#endif
