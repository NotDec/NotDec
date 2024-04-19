#ifndef _NOTDEC_BACKEND_GOTO_H_
#define _NOTDEC_BACKEND_GOTO_H_

#include "backend/structural-analysis.h"
#include <clang/AST/Stmt.h>
#include <map>
#include <vector>

namespace notdec::backend {

class Goto : IStructuralAnalysis {

  std::map<clang::LabelDecl *, std::vector<clang::GotoStmt *>> labelUsers;

  clang::GotoStmt *createGotoStmt(clang::LabelDecl *label) {
    auto Goto = new (ctx.getASTContext()) clang::GotoStmt(
        label, clang::SourceLocation(), clang::SourceLocation());
    labelUsers[label].push_back(Goto);
    return Goto;
  }

  void replaceAllUsesWith(clang::LabelDecl *label, clang::LabelDecl *newLabel) {
    auto &users = labelUsers[label];
    for (auto user : users) {
      user->setLabel(newLabel);
    }
    labelUsers[newLabel].insert(labelUsers[newLabel].end(), users.begin(),
                                users.end());
    labelUsers.erase(label);
  }

public:
  Goto(SAFuncContext &ctx) : IStructuralAnalysis(ctx) {}

  void execute() override;
  void simplifyBlock(CFGBlock &Block);
};

} // namespace notdec::backend

#endif
