
#include "backend/statements.h"
#include <llvm/IR/Type.h>
namespace notdec::backend {

class TypePrinter {
  Statements declarations;

  std::string printType(llvm::Type *ty);
  DeclareStmt declareType(llvm::Type *ty);
};

} // namespace notdec::backend
