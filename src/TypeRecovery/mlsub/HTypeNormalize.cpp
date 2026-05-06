#include "notdec/TypeRecovery/mlsub/HTypeNormalize.h"

#include "notdec-llvm2c/Interface/HType.h"

#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <map>
#include <set>
#include <vector>

namespace notdec::mlsub {
namespace {

bool isDirectValueSelfRecursiveField(ast::RecordDecl *Owner,
                                     ast::HType *Ty) {
  if (Owner == nullptr || Ty == nullptr || Ty->isPointerType() ||
      Ty->isDualPointerType()) {
    return false;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecordType>(Ty)) {
    return RT->getDecl() == Owner;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecursiveBindingType>(Ty)) {
    return RT->getBinder()->getAnchorDecl() == Owner;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecursiveRefType>(Ty)) {
    return RT->getBinder()->getAnchorDecl() == Owner;
  }
  return false;
}

bool isUnsizedArrayType(ast::HType *Ty) {
  if (auto *AT = llvm::dyn_cast_or_null<ast::ArrayType>(Ty)) {
    return !AT->getNumElements().has_value();
  }
  return false;
}

void normalizeTailValueRecursiveRecords(ast::HTypeContext &Ctx) {
  // Convert the simplest invalid value-recursive shape into a tail array:
  //   struct R { T prefix; R tail; }  ->  struct R { T[] prefix; }
  // This intentionally handles only one real prefix field. Multiple-field
  // prefixes would need a synthetic array element record, which is riskier for
  // naming and struct-merge stability.
  unsigned Changed = 0;
  for (const auto &Ent : Ctx.getDecls()) {
    auto *RD = llvm::dyn_cast<ast::RecordDecl>(Ent.second.get());
    if (RD == nullptr) {
      continue;
    }
    auto &Fields = RD->getFields();
    auto TailIndex = RD->getLastNonPaddingInd();
    if (!TailIndex ||
        !isDirectValueSelfRecursiveField(RD, Fields[*TailIndex].Type)) {
      continue;
    }

    if (Fields.size() != 2 || *TailIndex != 1 || Fields[0].isPadding) {
      continue;
    }

    auto &Prefix = Fields[0];
    auto &Tail = Fields[*TailIndex];
    if (Prefix.R.Start != 0 || Tail.R.Start <= Prefix.R.Start) {
      continue;
    }
    if (!isUnsizedArrayType(Prefix.Type)) {
      Prefix.Type = Ctx.getArrayType(false, Prefix.Type, std::nullopt);
    }
    Prefix.Comment = "tail value recursion normalized to array at offset: " +
                     std::to_string(Prefix.R.Start);
    Fields.erase(Fields.begin() + *TailIndex);
    ++Changed;
  }
  if (Changed != 0) {
    llvm::errs() << "Info: normalized " << Changed
                 << " tail value-recursive record(s) to arrays\n";
  }
}

void collectRecursiveAnchorDecls(ast::HType *Ty,
                                 std::set<ast::TypedDecl *> &Anchors,
                                 std::set<ast::HType *> &Visited) {
  if (Ty == nullptr || !Visited.insert(Ty).second) {
    return;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecursiveBindingType>(Ty)) {
    auto *Binder = RT->getBinder();
    if (auto *Anchor = Binder->getAnchorDecl()) {
      Anchors.insert(Anchor);
    }
    collectRecursiveAnchorDecls(Binder->getBody(), Anchors, Visited);
    return;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecursiveRefType>(Ty)) {
    if (auto *Anchor = RT->getBinder()->getAnchorDecl()) {
      Anchors.insert(Anchor);
    }
    return;
  }
  if (auto *PT = llvm::dyn_cast<ast::PointerType>(Ty)) {
    collectRecursiveAnchorDecls(PT->getPointeeType(), Anchors, Visited);
  } else if (auto *FT = llvm::dyn_cast<ast::FunctionType>(Ty)) {
    for (auto *RetTy : FT->getReturnType()) {
      collectRecursiveAnchorDecls(RetTy, Anchors, Visited);
    }
    for (auto *ParamTy : FT->getParamTypes()) {
      collectRecursiveAnchorDecls(ParamTy, Anchors, Visited);
    }
  } else if (auto *DPT = llvm::dyn_cast<ast::DualPointerType>(Ty)) {
    collectRecursiveAnchorDecls(DPT->getLoadType(), Anchors, Visited);
    collectRecursiveAnchorDecls(DPT->getStoreType(), Anchors, Visited);
  } else if (auto *Set = llvm::dyn_cast<ast::SetUnionType>(Ty)) {
    for (auto *Term : Set->getTypes()) {
      collectRecursiveAnchorDecls(Term, Anchors, Visited);
    }
  } else if (auto *Set = llvm::dyn_cast<ast::SetInterType>(Ty)) {
    for (auto *Term : Set->getTypes()) {
      collectRecursiveAnchorDecls(Term, Anchors, Visited);
    }
  } else if (auto *AT = llvm::dyn_cast<ast::ArrayType>(Ty)) {
    collectRecursiveAnchorDecls(AT->getElementType(), Anchors, Visited);
  } else if (auto *TT = llvm::dyn_cast<ast::TypedefType>(Ty)) {
    collectRecursiveAnchorDecls(TT->getDecl()->getType(), Anchors, Visited);
  }
}

std::set<ast::TypedDecl *>
collectRecursiveAnchorDecls(const llvm2c::HTypeResult &Result) {
  std::set<ast::TypedDecl *> Anchors;
  std::set<ast::HType *> Visited;
  for (const auto &Ent : Result.ValueTypesLower) {
    collectRecursiveAnchorDecls(Ent.second, Anchors, Visited);
  }
  for (const auto &Ent : Result.ValueTypesUpper) {
    collectRecursiveAnchorDecls(Ent.second, Anchors, Visited);
  }
  collectRecursiveAnchorDecls(Result.MemoryType, Anchors, Visited);
  if (Result.HTCtx) {
    for (const auto &Ent : Result.HTCtx->getDecls()) {
      if (auto *RD = llvm::dyn_cast<ast::RecordDecl>(Ent.second.get())) {
        for (auto &Field : RD->getFields()) {
          collectRecursiveAnchorDecls(Field.Type, Anchors, Visited);
        }
      } else if (auto *UD = llvm::dyn_cast<ast::UnionDecl>(Ent.second.get())) {
        for (auto &Member : UD->getMembers()) {
          collectRecursiveAnchorDecls(Member.Type, Anchors, Visited);
        }
      } else if (auto *TD = llvm::dyn_cast<ast::TypedefDecl>(Ent.second.get())) {
        collectRecursiveAnchorDecls(TD->getType(), Anchors, Visited);
      }
    }
  }
  return Anchors;
}

using TransparentRecordMap = std::map<ast::RecordDecl *, ast::HType *>;

ast::HType *
rewriteTransparentRecordType(ast::HTypeContext &Ctx, ast::HType *Ty,
                             const TransparentRecordMap &Replacements,
                             std::set<ast::RecordDecl *> &Resolving) {
  if (Ty == nullptr) {
    return nullptr;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecordType>(Ty)) {
    auto *Decl = RT->getDecl();
    auto It = Replacements.find(Decl);
    if (It == Replacements.end() || !Resolving.insert(Decl).second) {
      return Ty;
    }
    auto *Result =
        rewriteTransparentRecordType(Ctx, It->second, Replacements, Resolving);
    Resolving.erase(Decl);
    return Result;
  }
  if (auto *PT = llvm::dyn_cast<ast::PointerType>(Ty)) {
    auto *Pointee =
        rewriteTransparentRecordType(Ctx, PT->getPointeeType(), Replacements,
                                     Resolving);
    if (Pointee == PT->getPointeeType()) {
      return Ty;
    }
    return Ctx.getPointerType(Ty->isConst(), PT->getBitSize(), Pointee);
  }
  if (auto *FT = llvm::dyn_cast<ast::FunctionType>(Ty)) {
    std::vector<ast::HType *> RetTypes;
    std::vector<ast::HType *> ParamTypes;
    bool Changed = false;
    for (auto *RetTy : FT->getReturnType()) {
      auto *NewTy =
          rewriteTransparentRecordType(Ctx, RetTy, Replacements, Resolving);
      Changed |= NewTy != RetTy;
      RetTypes.push_back(NewTy);
    }
    for (auto *ParamTy : FT->getParamTypes()) {
      auto *NewTy =
          rewriteTransparentRecordType(Ctx, ParamTy, Replacements, Resolving);
      Changed |= NewTy != ParamTy;
      ParamTypes.push_back(NewTy);
    }
    return Changed ? Ctx.getFunctionType(Ty->isConst(), RetTypes, ParamTypes)
                   : Ty;
  }
  if (auto *DPT = llvm::dyn_cast<ast::DualPointerType>(Ty)) {
    auto *LoadTy =
        rewriteTransparentRecordType(Ctx, DPT->getLoadType(), Replacements,
                                     Resolving);
    auto *StoreTy =
        rewriteTransparentRecordType(Ctx, DPT->getStoreType(), Replacements,
                                     Resolving);
    if (LoadTy == DPT->getLoadType() && StoreTy == DPT->getStoreType()) {
      return Ty;
    }
    return Ctx.getDualPointerType(Ty->isConst(), DPT->getAccessSize(), LoadTy,
                                  StoreTy);
  }
  if (auto *Set = llvm::dyn_cast<ast::SetUnionType>(Ty)) {
    std::vector<ast::HType *> Terms;
    bool Changed = false;
    for (auto *Term : Set->getTypes()) {
      auto *NewTy =
          rewriteTransparentRecordType(Ctx, Term, Replacements, Resolving);
      Changed |= NewTy != Term;
      Terms.push_back(NewTy);
    }
    return Changed ? Ctx.getSetUnionType(Ty->isConst(), std::move(Terms)) : Ty;
  }
  if (auto *Set = llvm::dyn_cast<ast::SetInterType>(Ty)) {
    std::vector<ast::HType *> Terms;
    bool Changed = false;
    for (auto *Term : Set->getTypes()) {
      auto *NewTy =
          rewriteTransparentRecordType(Ctx, Term, Replacements, Resolving);
      Changed |= NewTy != Term;
      Terms.push_back(NewTy);
    }
    return Changed ? Ctx.getSetInterType(Ty->isConst(), std::move(Terms)) : Ty;
  }
  if (auto *AT = llvm::dyn_cast<ast::ArrayType>(Ty)) {
    auto *Element =
        rewriteTransparentRecordType(Ctx, AT->getElementType(), Replacements,
                                     Resolving);
    if (Element == AT->getElementType()) {
      return Ty;
    }
    return Ctx.getArrayType(Ty->isConst(), Element, AT->getNumElements());
  }
  return Ty;
}

ast::HType *
rewriteTransparentRecordType(ast::HTypeContext &Ctx, ast::HType *Ty,
                             const TransparentRecordMap &Replacements) {
  std::set<ast::RecordDecl *> Resolving;
  return rewriteTransparentRecordType(Ctx, Ty, Replacements, Resolving);
}

void normalizeTransparentSingleFieldRecords(llvm2c::HTypeResult &Result) {
  if (!Result.HTCtx) {
    return;
  }
  auto RecursiveAnchors = collectRecursiveAnchorDecls(Result);
  TransparentRecordMap Replacements;
  for (const auto &Ent : Result.HTCtx->getDecls()) {
    auto *RD = llvm::dyn_cast<ast::RecordDecl>(Ent.second.get());
    if (RD == nullptr || RD == Result.MemoryDecl ||
        RecursiveAnchors.count(RD) != 0) {
      continue;
    }
    auto &Fields = RD->getFields();
    if (Fields.size() != 1 || Fields.front().isPadding ||
        Fields.front().R.Start != 0 || Fields.front().Type == nullptr) {
      continue;
    }
    Replacements[RD] = Fields.front().Type;
  }
  if (Replacements.empty()) {
    return;
  }

  for (auto &Ent : Result.ValueTypesLower) {
    Ent.second =
        rewriteTransparentRecordType(*Result.HTCtx, Ent.second, Replacements);
  }
  for (auto &Ent : Result.ValueTypesUpper) {
    Ent.second =
        rewriteTransparentRecordType(*Result.HTCtx, Ent.second, Replacements);
  }
  Result.MemoryType = rewriteTransparentRecordType(
      *Result.HTCtx, Result.MemoryType, Replacements);

  for (const auto &Ent : Result.HTCtx->getDecls()) {
    if (auto *RD = llvm::dyn_cast<ast::RecordDecl>(Ent.second.get())) {
      for (auto &Field : RD->getFields()) {
        Field.Type = rewriteTransparentRecordType(*Result.HTCtx, Field.Type,
                                                  Replacements);
      }
    }
  }

  Result.MemoryDecl = nullptr;
  if (Result.MemoryType != nullptr && Result.MemoryType->isRecordType()) {
    Result.MemoryDecl = Result.MemoryType->getAsRecordDecl();
  }
  llvm::errs() << "Info: normalized " << Replacements.size()
               << " transparent single-field record(s)\n";
}

} // namespace

void normalizeHTypeResult(llvm2c::HTypeResult &Result) {
  if (!Result.HTCtx) {
    return;
  }
  normalizeTailValueRecursiveRecords(*Result.HTCtx);
  normalizeTransparentSingleFieldRecords(Result);
}

} // namespace notdec::mlsub
