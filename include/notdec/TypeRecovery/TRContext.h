#ifndef _NOTDEC_RETYPD_TRCONTEXT_H_
#define _NOTDEC_RETYPD_TRCONTEXT_H_

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Support/Allocator.h>
#include <utility>

#include "TypeRecovery/Schema.h"

namespace notdec::retypd {

using namespace llvm;

struct PTVCmp {

  bool operator()(PooledTypeVariable *const &a,
                  PooledTypeVariable *const &b) const {
    return *a < *b;
  }
};

struct TRContext {

  TRContext() = default;
  TRContext(TRContext &) = delete;
  TRContext &operator=(const TRContext &) = delete;

  // BumpPtrAllocator Alloc;
  std::set<PooledTypeVariable *, PTVCmp> TypeVars;
  // DenseMap<std::string, PooledTypeVariable *> BaseTypeVars;
  // DenseMap<std::string, PooledTypeVariable *> PrimitiveTypeVars;
  // DenseMap<BaseConstant, PooledTypeVariable *> ConstantTypeVars;
  // DenseMap<std::pair<FieldLabel, PooledTypeVariable *>, PooledTypeVariable *>
  //     DerivedTypeVars;

  ~TRContext() {
    for (auto *TV : TypeVars) {
      delete TV;
    }
    // for (auto &Ent : DerivedTypeVars) {
    //   delete Ent.getSecond();
    // }
    // for (auto &Ent : BaseTypeVars) {
    //   delete Ent.getSecond();
    // }
    // for (auto &Ent : PrimitiveTypeVars) {
    //   delete Ent.getSecond();
    // }
    // for (auto &Ent : ConstantTypeVars) {
    //   delete Ent.getSecond();
    // }
  }
};

} // namespace notdec::retypd
#endif
