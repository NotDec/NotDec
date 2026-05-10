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

std::set<ast::RecordDecl *>
normalizeTailValueRecursiveRecords(ast::HTypeContext &Ctx) {
  // Convert the simplest invalid value-recursive shape into a tail array:
  //   struct R { T prefix; R tail; }  ->  struct R { T[] prefix; }
  // This intentionally handles only one real prefix field. Multiple-field
  // prefixes would need a synthetic array element record, which is riskier for
  // naming and struct-merge stability.
  unsigned Changed = 0;
  std::set<ast::RecordDecl *> Normalized;
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
    Normalized.insert(RD);
    ++Changed;
  }
  if (Changed != 0) {
    llvm::errs() << "Info: normalized " << Changed
                 << " tail value-recursive record(s) to arrays\n";
  }
  return Normalized;
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

void collectRecursiveBinders(ast::HType *Ty,
                             std::set<ast::RecursiveBinder *> &Binders,
                             std::set<ast::HType *> &VisitedTypes,
                             std::set<ast::TypedDecl *> &VisitedDecls,
                             std::set<ast::RecursiveBinder *> &VisitedBinders) {
  if (Ty == nullptr || !VisitedTypes.insert(Ty).second) {
    return;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecursiveBindingType>(Ty)) {
    auto *Binder = RT->getBinder();
    Binders.insert(Binder);
    if (VisitedBinders.insert(Binder).second) {
      collectRecursiveBinders(Binder->getBody(), Binders, VisitedTypes,
                              VisitedDecls, VisitedBinders);
    }
    return;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecursiveRefType>(Ty)) {
    auto *Binder = RT->getBinder();
    Binders.insert(Binder);
    if (VisitedBinders.insert(Binder).second) {
      collectRecursiveBinders(Binder->getBody(), Binders, VisitedTypes,
                              VisitedDecls, VisitedBinders);
    }
    return;
  }
  if (auto *PT = llvm::dyn_cast<ast::PointerType>(Ty)) {
    collectRecursiveBinders(PT->getPointeeType(), Binders, VisitedTypes,
                            VisitedDecls, VisitedBinders);
    return;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecordType>(Ty)) {
    auto *Decl = RT->getDecl();
    if (Decl != nullptr && VisitedDecls.insert(Decl).second) {
      for (auto &Field : Decl->getFields()) {
        collectRecursiveBinders(Field.Type, Binders, VisitedTypes, VisitedDecls,
                                VisitedBinders);
      }
    }
    return;
  }
  if (auto *UT = llvm::dyn_cast<ast::UnionType>(Ty)) {
    auto *Decl = UT->getDecl();
    if (Decl != nullptr && VisitedDecls.insert(Decl).second) {
      for (auto &Member : Decl->getMembers()) {
        collectRecursiveBinders(Member.Type, Binders, VisitedTypes,
                                VisitedDecls, VisitedBinders);
      }
    }
    return;
  }
  if (auto *AT = llvm::dyn_cast<ast::ArrayType>(Ty)) {
    collectRecursiveBinders(AT->getElementType(), Binders, VisitedTypes,
                            VisitedDecls, VisitedBinders);
    return;
  }
  if (auto *FT = llvm::dyn_cast<ast::FunctionType>(Ty)) {
    for (auto *RetTy : FT->getReturnType()) {
      collectRecursiveBinders(RetTy, Binders, VisitedTypes, VisitedDecls,
                              VisitedBinders);
    }
    for (auto *ParamTy : FT->getParamTypes()) {
      collectRecursiveBinders(ParamTy, Binders, VisitedTypes, VisitedDecls,
                              VisitedBinders);
    }
    return;
  }
  if (auto *DPT = llvm::dyn_cast<ast::DualPointerType>(Ty)) {
    collectRecursiveBinders(DPT->getLoadType(), Binders, VisitedTypes,
                            VisitedDecls, VisitedBinders);
    collectRecursiveBinders(DPT->getStoreType(), Binders, VisitedTypes,
                            VisitedDecls, VisitedBinders);
    return;
  }
  if (auto *Set = llvm::dyn_cast<ast::SetUnionType>(Ty)) {
    for (auto *Term : Set->getTypes()) {
      collectRecursiveBinders(Term, Binders, VisitedTypes, VisitedDecls,
                              VisitedBinders);
    }
    return;
  }
  if (auto *Set = llvm::dyn_cast<ast::SetInterType>(Ty)) {
    for (auto *Term : Set->getTypes()) {
      collectRecursiveBinders(Term, Binders, VisitedTypes, VisitedDecls,
                              VisitedBinders);
    }
    return;
  }
  if (auto *TT = llvm::dyn_cast<ast::TypedefType>(Ty)) {
    auto *Decl = TT->getDecl();
    if (Decl != nullptr && VisitedDecls.insert(Decl).second) {
      collectRecursiveBinders(Decl->getType(), Binders, VisitedTypes,
                              VisitedDecls, VisitedBinders);
    }
  }
}

std::set<ast::RecursiveBinder *>
collectRecursiveBinders(const llvm2c::HTypeResult &Result) {
  std::set<ast::RecursiveBinder *> Binders;
  std::set<ast::HType *> VisitedTypes;
  std::set<ast::TypedDecl *> VisitedDecls;
  std::set<ast::RecursiveBinder *> VisitedBinders;
  for (const auto &Ent : Result.ValueTypesLower) {
    collectRecursiveBinders(Ent.second, Binders, VisitedTypes, VisitedDecls,
                            VisitedBinders);
  }
  for (const auto &Ent : Result.ValueTypesUpper) {
    collectRecursiveBinders(Ent.second, Binders, VisitedTypes, VisitedDecls,
                            VisitedBinders);
  }
  collectRecursiveBinders(Result.MemoryType, Binders, VisitedTypes,
                          VisitedDecls, VisitedBinders);
  return Binders;
}

bool containsRecursiveBinder(ast::HType *Ty, ast::RecursiveBinder *Target,
                             std::set<ast::HType *> &VisitedTypes,
                             std::set<ast::TypedDecl *> &VisitedDecls,
                             std::set<ast::RecursiveBinder *> &VisitedBinders) {
  if (Ty == nullptr || Target == nullptr || !VisitedTypes.insert(Ty).second) {
    return false;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecursiveBindingType>(Ty)) {
    auto *Binder = RT->getBinder();
    if (Binder == Target) {
      return true;
    }
    return VisitedBinders.insert(Binder).second &&
           containsRecursiveBinder(Binder->getBody(), Target, VisitedTypes,
                                   VisitedDecls, VisitedBinders);
  }
  if (auto *RT = llvm::dyn_cast<ast::RecursiveRefType>(Ty)) {
    auto *Binder = RT->getBinder();
    if (Binder == Target) {
      return true;
    }
    return VisitedBinders.insert(Binder).second &&
           containsRecursiveBinder(Binder->getBody(), Target, VisitedTypes,
                                   VisitedDecls, VisitedBinders);
  }
  if (auto *PT = llvm::dyn_cast<ast::PointerType>(Ty)) {
    return containsRecursiveBinder(PT->getPointeeType(), Target, VisitedTypes,
                                   VisitedDecls, VisitedBinders);
  }
  if (auto *RT = llvm::dyn_cast<ast::RecordType>(Ty)) {
    auto *Decl = RT->getDecl();
    if (Decl == nullptr || !VisitedDecls.insert(Decl).second) {
      return false;
    }
    for (auto &Field : Decl->getFields()) {
      if (containsRecursiveBinder(Field.Type, Target, VisitedTypes,
                                  VisitedDecls, VisitedBinders)) {
        return true;
      }
    }
    return false;
  }
  if (auto *UT = llvm::dyn_cast<ast::UnionType>(Ty)) {
    auto *Decl = UT->getDecl();
    if (Decl == nullptr || !VisitedDecls.insert(Decl).second) {
      return false;
    }
    for (auto &Member : Decl->getMembers()) {
      if (containsRecursiveBinder(Member.Type, Target, VisitedTypes,
                                  VisitedDecls, VisitedBinders)) {
        return true;
      }
    }
    return false;
  }
  if (auto *AT = llvm::dyn_cast<ast::ArrayType>(Ty)) {
    return containsRecursiveBinder(AT->getElementType(), Target, VisitedTypes,
                                   VisitedDecls, VisitedBinders);
  }
  if (auto *FT = llvm::dyn_cast<ast::FunctionType>(Ty)) {
    for (auto *RetTy : FT->getReturnType()) {
      if (containsRecursiveBinder(RetTy, Target, VisitedTypes, VisitedDecls,
                                  VisitedBinders)) {
        return true;
      }
    }
    for (auto *ParamTy : FT->getParamTypes()) {
      if (containsRecursiveBinder(ParamTy, Target, VisitedTypes, VisitedDecls,
                                  VisitedBinders)) {
        return true;
      }
    }
    return false;
  }
  if (auto *DPT = llvm::dyn_cast<ast::DualPointerType>(Ty)) {
    return containsRecursiveBinder(DPT->getLoadType(), Target, VisitedTypes,
                                   VisitedDecls, VisitedBinders) ||
           containsRecursiveBinder(DPT->getStoreType(), Target, VisitedTypes,
                                   VisitedDecls, VisitedBinders);
  }
  if (auto *Set = llvm::dyn_cast<ast::SetUnionType>(Ty)) {
    for (auto *Term : Set->getTypes()) {
      if (containsRecursiveBinder(Term, Target, VisitedTypes, VisitedDecls,
                                  VisitedBinders)) {
        return true;
      }
    }
    return false;
  }
  if (auto *Set = llvm::dyn_cast<ast::SetInterType>(Ty)) {
    for (auto *Term : Set->getTypes()) {
      if (containsRecursiveBinder(Term, Target, VisitedTypes, VisitedDecls,
                                  VisitedBinders)) {
        return true;
      }
    }
    return false;
  }
  if (auto *TT = llvm::dyn_cast<ast::TypedefType>(Ty)) {
    auto *Decl = TT->getDecl();
    return Decl != nullptr && VisitedDecls.insert(Decl).second &&
           containsRecursiveBinder(Decl->getType(), Target, VisitedTypes,
                                   VisitedDecls, VisitedBinders);
  }
  return false;
}

bool containsRecursiveBinder(ast::HType *Ty, ast::RecursiveBinder *Target) {
  std::set<ast::HType *> VisitedTypes;
  std::set<ast::TypedDecl *> VisitedDecls;
  std::set<ast::RecursiveBinder *> VisitedBinders;
  return containsRecursiveBinder(Ty, Target, VisitedTypes, VisitedDecls,
                                 VisitedBinders);
}

// RecursiveBinderMap records binders whose bodies have already become plain
// non-recursive shapes after earlier normalization, so later users can rewrite
// both rec-binders and rec-refs straight to that body.
using RecursiveBinderMap = std::map<ast::RecursiveBinder *, ast::HType *>;

ast::HType *rewriteCollapsedRecursiveType(ast::HTypeContext &Ctx, ast::HType *Ty,
                                          const RecursiveBinderMap &Replacements,
                                          std::set<ast::RecursiveBinder *>
                                              &Resolving) {
  if (Ty == nullptr) {
    return nullptr;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecursiveBindingType>(Ty)) {
    auto It = Replacements.find(RT->getBinder());
    if (It == Replacements.end() ||
        !Resolving.insert(RT->getBinder()).second) {
      return Ty;
    }
    auto *Result = rewriteCollapsedRecursiveType(Ctx, It->second, Replacements,
                                                 Resolving);
    Resolving.erase(RT->getBinder());
    return Result;
  }
  if (auto *RT = llvm::dyn_cast<ast::RecursiveRefType>(Ty)) {
    auto It = Replacements.find(RT->getBinder());
    if (It == Replacements.end() ||
        !Resolving.insert(RT->getBinder()).second) {
      return Ty;
    }
    auto *Result = rewriteCollapsedRecursiveType(Ctx, It->second, Replacements,
                                                 Resolving);
    Resolving.erase(RT->getBinder());
    return Result;
  }
  if (auto *PT = llvm::dyn_cast<ast::PointerType>(Ty)) {
    auto *Pointee = rewriteCollapsedRecursiveType(Ctx, PT->getPointeeType(),
                                                  Replacements, Resolving);
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
          rewriteCollapsedRecursiveType(Ctx, RetTy, Replacements, Resolving);
      Changed |= NewTy != RetTy;
      RetTypes.push_back(NewTy);
    }
    for (auto *ParamTy : FT->getParamTypes()) {
      auto *NewTy =
          rewriteCollapsedRecursiveType(Ctx, ParamTy, Replacements, Resolving);
      Changed |= NewTy != ParamTy;
      ParamTypes.push_back(NewTy);
    }
    return Changed ? Ctx.getFunctionType(Ty->isConst(), RetTypes, ParamTypes)
                   : Ty;
  }
  if (auto *DPT = llvm::dyn_cast<ast::DualPointerType>(Ty)) {
    auto *LoadTy = rewriteCollapsedRecursiveType(Ctx, DPT->getLoadType(),
                                                 Replacements, Resolving);
    auto *StoreTy = rewriteCollapsedRecursiveType(Ctx, DPT->getStoreType(),
                                                  Replacements, Resolving);
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
          rewriteCollapsedRecursiveType(Ctx, Term, Replacements, Resolving);
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
          rewriteCollapsedRecursiveType(Ctx, Term, Replacements, Resolving);
      Changed |= NewTy != Term;
      Terms.push_back(NewTy);
    }
    return Changed ? Ctx.getSetInterType(Ty->isConst(), std::move(Terms)) : Ty;
  }
  if (auto *AT = llvm::dyn_cast<ast::ArrayType>(Ty)) {
    auto *Element = rewriteCollapsedRecursiveType(Ctx, AT->getElementType(),
                                                  Replacements, Resolving);
    if (Element == AT->getElementType()) {
      return Ty;
    }
    return Ctx.getArrayType(Ty->isConst(), Element, AT->getNumElements());
  }
  return Ty;
}

ast::HType *rewriteCollapsedRecursiveType(ast::HTypeContext &Ctx, ast::HType *Ty,
                                          const RecursiveBinderMap &Replacements) {
  std::set<ast::RecursiveBinder *> Resolving;
  return rewriteCollapsedRecursiveType(Ctx, Ty, Replacements, Resolving);
}

void refreshMemoryDecl(llvm2c::HTypeResult &Result) {
  Result.MemoryDecl = nullptr;
  if (Result.MemoryType != nullptr && Result.MemoryType->isRecordType()) {
    Result.MemoryDecl = Result.MemoryType->getAsRecordDecl();
  }
}

void normalizeCollapsedRecursiveBinders(
    llvm2c::HTypeResult &Result,
    const std::set<ast::RecordDecl *> &TailNormalizedDecls) {
  if (!Result.HTCtx) {
    return;
  }
  // Tail-array normalization may turn a previously recursive binder anchor into
  // a plain record shape. At that point value-side `rec*` users should first
  // collapse back to the anchor record type, so the later single-field record
  // rewrite can transparently replace `struct { T[]; }*` with `T[]*`.
  RecursiveBinderMap Replacements;
  for (auto *Binder : collectRecursiveBinders(Result)) {
    auto *AnchorDecl =
        Binder == nullptr ? nullptr
                          : llvm::dyn_cast_or_null<ast::RecordDecl>(
                                Binder->getAnchorDecl());
    if (Binder == nullptr || AnchorDecl == nullptr ||
        TailNormalizedDecls.count(AnchorDecl) == 0 ||
        Binder->getBody() == nullptr ||
        containsRecursiveBinder(Binder->getBody(), Binder)) {
      continue;
    }
    Replacements[Binder] = Result.HTCtx->getRecordType(false, AnchorDecl);
  }
  if (Replacements.empty()) {
    return;
  }

  for (auto &Ent : Result.ValueTypesLower) {
    Ent.second =
        rewriteCollapsedRecursiveType(*Result.HTCtx, Ent.second, Replacements);
  }
  for (auto &Ent : Result.ValueTypesUpper) {
    Ent.second =
        rewriteCollapsedRecursiveType(*Result.HTCtx, Ent.second, Replacements);
  }
  Result.MemoryType = rewriteCollapsedRecursiveType(*Result.HTCtx,
                                                    Result.MemoryType,
                                                    Replacements);

  for (const auto &Ent : Result.HTCtx->getDecls()) {
    if (auto *RD = llvm::dyn_cast<ast::RecordDecl>(Ent.second.get())) {
      for (auto &Field : RD->getFields()) {
        Field.Type = rewriteCollapsedRecursiveType(*Result.HTCtx, Field.Type,
                                                   Replacements);
      }
    } else if (auto *UD = llvm::dyn_cast<ast::UnionDecl>(Ent.second.get())) {
      for (auto &Member : UD->getMembers()) {
        Member.Type = rewriteCollapsedRecursiveType(*Result.HTCtx, Member.Type,
                                                    Replacements);
      }
    } else if (auto *TD = llvm::dyn_cast<ast::TypedefDecl>(Ent.second.get())) {
      TD->setType(rewriteCollapsedRecursiveType(*Result.HTCtx, TD->getType(),
                                                Replacements));
    }
  }

  refreshMemoryDecl(Result);
  llvm::errs() << "Info: collapsed " << Replacements.size()
               << " non-recursive binder(s)\n";
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

  refreshMemoryDecl(Result);
  llvm::errs() << "Info: normalized " << Replacements.size()
               << " transparent single-field record(s)\n";
}

} // namespace

void normalizeHTypeResult(llvm2c::HTypeResult &Result) {
  if (!Result.HTCtx) {
    return;
  }
  auto TailNormalizedDecls = normalizeTailValueRecursiveRecords(*Result.HTCtx);
  normalizeCollapsedRecursiveBinders(Result, TailNormalizedDecls);
  normalizeTransparentSingleFieldRecords(Result);
}

} // namespace notdec::mlsub
