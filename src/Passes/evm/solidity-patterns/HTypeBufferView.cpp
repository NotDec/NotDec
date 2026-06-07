#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Casting.h>
#include <notdec-llvm2c/Interface.h>

using namespace llvm;

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
  if (Ty->isPointerType()) {
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

} // namespace

HTypeBufferView getHTypeBufferView(llvm2c::HTypeResult &HTypes, Value *Base,
                                   CallBase &Use, unsigned ArgIndex) {
  ast::HType *Ty =
      HTypes.getDefaultValueType(getExtValuePtr(Base, &Use, ArgIndex));
  ast::RecordDecl *Record = getRecordPointerPointee(Ty);
  if (Record != nullptr) {
    return HTypeBufferView{Record, Ty, HTypeBufferGap::None};
  }
  if (!containsPointerType(Ty)) {
    return HTypeBufferView{nullptr, Ty, HTypeBufferGap::NoPointerType};
  }
  return HTypeBufferView{nullptr, Ty, HTypeBufferGap::NonRecordPointerType};
}

} // namespace notdec::passes::evm::detail
