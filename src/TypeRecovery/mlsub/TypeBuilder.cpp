#include "notdec/TypeRecovery/mlsub/TypeBuilder.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"
#include <cassert>
#include <iostream>
#include <variant>

namespace notdec::mlsub {

using notdec::SimpleRange;
using notdec::ValueNamer;

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

  // Default: treat as 32-bit integer
  return Ctx.getIntegerType(false, 32, false);
}

HType *TypeBuilder::convert(binarysub::UTypePtr Ty) {
  if (!Ty) {
    return getVoidPtr();
  }

  // Check cache first
  auto CacheIt = TypeCache.find(Ty);
  if (CacheIt != TypeCache.end()) {
    return CacheIt->second;
  }

  // Check for cycle
  if (InProgress.count(Ty)) {
    // Return a type variable for cyclic types
    return Ctx.getTypeVariableType(false, "cyclic");
  }

  InProgress.insert(Ty);

  HType *Result = std::visit(
      [this](auto &&arg) -> HType * {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, binarysub::UTop>) {
          return convertTop(arg);
        } else if constexpr (std::is_same_v<T, binarysub::UBot>) {
          return convertBot(arg);
        } else if constexpr (std::is_same_v<T, binarysub::UPrimitiveType>) {
          return convertPrimitive(arg);
        } else if constexpr (std::is_same_v<T, binarysub::UPointerType>) {
          return convertPointer(arg);
        } else if constexpr (std::is_same_v<T, binarysub::UFunctionType>) {
          return convertFunction(arg);
        } else if constexpr (std::is_same_v<T, binarysub::URecordType>) {
          return convertRecord(arg);
        } else if constexpr (std::is_same_v<T, binarysub::UUnion>) {
          return convertUnion(arg);
        } else if constexpr (std::is_same_v<T, binarysub::UInter>) {
          return convertInter(arg);
        } else if constexpr (std::is_same_v<T, binarysub::URecursiveType>) {
          return convertRecursive(arg);
        } else if constexpr (std::is_same_v<T, binarysub::UTypeVariable>) {
          return convertVariable(arg);
        } else {
          static_assert(sizeof(T) == 0, "Unhandled UType variant");
        }
      },
      Ty->v);

  InProgress.erase(Ty);
  TypeCache[Ty] = Result;
  return Result;
}

HType *TypeBuilder::convertTop(const binarysub::UTop &T) {
  // UTop represents the top of the type lattice - use void* as universal
  // pointer
  return getVoidPtr();
}

HType *TypeBuilder::convertBot(const binarysub::UBot &T) {
  // UBot represents the bottom of the type lattice - use void* as universal
  // pointer
  return getVoidPtr();
}

HType *TypeBuilder::convertPrimitive(const binarysub::UPrimitiveType &T) {
  return parsePrimitiveName(T.name);
}

HType *TypeBuilder::convertPointer(const binarysub::UPointerType &T) {
  // For pointer types, use the load type as pointee
  // If load type is available, convert it; otherwise use void*
  HType *Pointee = nullptr;
  if (T.load) {
    Pointee = convert(T.load);
  }
  return Ctx.getPointerType(false, Parent.PointerSize, Pointee);
}

HType *TypeBuilder::convertFunction(const binarysub::UFunctionType &T) {
  // For function types, we return a function pointer
  // Note: HType doesn't have full function type support yet, so we approximate
  // with void*
  return getVoidPtr();
}

HType *TypeBuilder::convertRecord(const binarysub::URecordType &T) {
  // For record types, create a struct
  // Since URecordType has field names and types, we create a RecordDecl
  if (T.fields.empty()) {
    return getVoidPtr();
  }

  // Create the record declaration
  auto Name = ValueNamer::getName("struct_");
  auto *Decl = ast::RecordDecl::Create(Ctx, Name);

  int64_t currentOffset = 0;
  for (const auto &field : T.fields) {
    HType *FieldType = convert(field.second);
    int64_t fieldSize = Parent.PointerSize; // default to pointer size

    // Try to determine field size from type
    if (FieldType->isIntType()) {
      auto *IntTy = llvm::cast<ast::IntegerType>(FieldType);
      fieldSize = IntTy->getBitSize() / 8;
    } else if (FieldType->isFloatType()) {
      auto *FloatTy = llvm::cast<ast::FloatingType>(FieldType);
      fieldSize = FloatTy->getBitSize() / 8;
    } else if (FieldType->isPointerType()) {
      fieldSize = Parent.PointerSize;
    }

    ast::FieldDecl FieldDecl{
        .R = SimpleRange{.Start = currentOffset, .Size = fieldSize},
        .Type = FieldType,
        .Name = field.first.empty() ? ValueNamer::getName("field_")
                                    : field.first,
        .Comment = ""};
    Decl->addField(FieldDecl);
    currentOffset += fieldSize;
  }

  return Ctx.getPointerType(false, Parent.PointerSize,
                            Ctx.getRecordType(false, Decl));
}

HType *TypeBuilder::convertUnion(const binarysub::UUnion &T) {
  // For union types, try to merge the two branches
  // If they are the same, return that type; otherwise return void*
  HType *LhsTy = convert(T.lhs);
  HType *RhsTy = convert(T.rhs);

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
  // If they are the same, return that type; otherwise return the more specific
  // one
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

HType *TypeBuilder::convertRecursive(const binarysub::URecursiveType &T) {
  // Check if we already have a type for this recursive name
  auto It = RecursiveTypeNames.find(T.name);
  if (It != RecursiveTypeNames.end()) {
    return It->second;
  }

  // Create a placeholder type variable for the recursive type
  HType *Placeholder = Ctx.getTypeVariableType(false, T.name);
  RecursiveTypeNames[T.name] = Placeholder;

  // Convert the body
  HType *BodyTy = convert(T.body);

  // Update the mapping with the actual type
  RecursiveTypeNames[T.name] = BodyTy;

  return BodyTy;
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
