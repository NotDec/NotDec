#ifndef _NOTDEC_BACKEND_STRUCTURAL_EXPR_EXPR_H_
#define _NOTDEC_BACKEND_STRUCTURAL_EXPR_EXPR_H_

#include <llvm/IR/Type.h>

namespace notdec::backend {

class Expr {
  llvm::Type *type;
  bool useFuncStyle = false;

public:
  Expr(llvm::Type *type) : type(type) {}
  llvm::Type *getType() { return type; }
  bool getFuncStyle() { return useFuncStyle; }
  virtual void print(llvm::raw_string_ostream &os) const = 0;
};

} // namespace notdec::backend

#endif
