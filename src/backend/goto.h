#ifndef _NOTDEC_BACKEND_GOTO_H_
#define _NOTDEC_BACKEND_GOTO_H_

#include "backend/structural-analysis.h"
#include <clang/AST/Stmt.h>
#include <map>
#include <vector>

namespace notdec::backend {

class Goto : IStructuralAnalysis {

  std::map<clang::LabelDecl *, std::vector<clang::GotoStmt *>> labelUsers;

public:
  Goto(SAFuncContext &ctx) : IStructuralAnalysis(ctx) {}

  void execute() override;
  void simplifyBlock(CFGBlock &Block);
};

} // namespace notdec::backend

#endif
