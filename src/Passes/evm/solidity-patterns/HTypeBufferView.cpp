#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Debug.h>
#include <notdec-llvm2c/Interface.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm::detail {

namespace {

ast::RecordDecl *getRecordPointerPointee(ast::HType *Ty) {
  if (Ty == nullptr) {
    return nullptr;
  }
  if (Ty->isPointerType()) {
    ast::HType *Pointee = Ty->getPointeeType();
    return Pointee == nullptr ? nullptr : Pointee->getAsRecordDecl();
  }
  if (auto *Inter = dyn_cast<ast::SetInterType>(Ty)) {
    for (ast::HType *Term : Inter->getTypes()) {
      if (ast::RecordDecl *Record = getRecordPointerPointee(Term)) {
        return Record;
      }
    }
  }
  if (auto *Union = dyn_cast<ast::SetUnionType>(Ty)) {
    for (ast::HType *Term : Union->getTypes()) {
      if (ast::RecordDecl *Record = getRecordPointerPointee(Term)) {
        return Record;
      }
    }
  }
  return nullptr;
}

bool containsPointerType(ast::HType *Ty) {
  if (Ty == nullptr) {
    return false;
  }
  if (Ty->isPointerType() || Ty->isDualPointerType()) {
    return true;
  }
  if (auto *Inter = dyn_cast<ast::SetInterType>(Ty)) {
    return any_of(Inter->getTypes(), containsPointerType);
  }
  if (auto *Union = dyn_cast<ast::SetUnionType>(Ty)) {
    return any_of(Union->getTypes(), containsPointerType);
  }
  return false;
}

bool containsTransparentOffset0Field(ast::HType *Ty) {
  if (Ty == nullptr) {
    return false;
  }
  if (auto *DualPtr = dyn_cast<ast::DualPointerType>(Ty)) {
    return DualPtr->getAccessSize() == 256 &&
           DualPtr->getStoreType() != nullptr;
  }
  if (auto *Inter = dyn_cast<ast::SetInterType>(Ty)) {
    return any_of(Inter->getTypes(), containsTransparentOffset0Field);
  }
  if (auto *Union = dyn_cast<ast::SetUnionType>(Ty)) {
    return any_of(Union->getTypes(), containsTransparentOffset0Field);
  }
  return false;
}

HTypeBufferView makeHTypeBufferView(ast::HType *Ty) {
  ast::RecordDecl *Record = getRecordPointerPointee(Ty);
  bool HasTransparentOffset0Field = containsTransparentOffset0Field(Ty);
  if (Record != nullptr) {
    return HTypeBufferView{Record, Ty, HasTransparentOffset0Field,
                           HTypeBufferGap::None};
  }
  if (!containsPointerType(Ty)) {
    return HTypeBufferView{nullptr, Ty, HasTransparentOffset0Field,
                           HTypeBufferGap::NoPointerType};
  }
  return HTypeBufferView{nullptr, Ty, HasTransparentOffset0Field,
                         HTypeBufferGap::NonRecordPointerType};
}

} // namespace

HTypeBufferView getHTypeBufferView(llvm2c::HTypeResult &HTypes, Value *Base,
                                   CallBase &Use, unsigned ArgIndex) {
  ast::HType *Ty =
      HTypes.getDefaultValueType(getExtValuePtr(Base, &Use, ArgIndex));
  if (!containsPointerType(Ty) && !isa<Constant>(Base)) {
    if (ast::HType *BaseTy =
            HTypes.getDefaultValueType(getExtValuePtr(Base, nullptr))) {
      LLVM_DEBUG(dbgs() << "evm htype buffer: fallback base type "
                        << BaseTy->getAsString() << " for " << *Base << "\n");
      Ty = BaseTy;
    }
  }
  return makeHTypeBufferView(Ty);
}

HTypeBufferView getHTypeValueBufferView(llvm2c::HTypeResult &HTypes,
                                        Value *Base) {
  ast::HType *Ty =
      HTypes.getDefaultValueType(getExtValuePtr(Base, nullptr));
  return makeHTypeBufferView(Ty);
}

} // namespace notdec::passes::evm::detail
