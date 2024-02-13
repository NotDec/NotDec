
#ifndef _NOTDEC_BACKEND_REGION_H_
#define _NOTDEC_BACKEND_REGION_H_

#include "backend/exprs.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <variant>
#include <vector>
namespace notdec::backend {

// control related statements
struct IfStmt;
struct WhileStmt;
struct DoWhileStmt;
struct BreakStmt;
struct ContinueStmt;
struct ReturnStmt;

// 用 std::holds_alternative<IfStmt>(v) 判断是哪种类型的
using Statement = std::variant<IfStmt, WhileStmt>;
using Statements = std::vector<Statement>;

struct IfStmt {
  Expr *cond;
  bool invert;
  Statements then;
  Statements els;
};

struct WhileStmt {
  Statements body;
  llvm::Instruction *inst; // 原本的跳转指令
  llvm::Value *condition;  // 没想好，要不直接从跳转指令里取？
  bool invert;             // 是否翻转条件
};

struct DoWhileStmt {
  Statements body;
  llvm::Instruction *inst; // 原本的跳转指令
  llvm::Value *condition;  // 没想好，要不直接从跳转指令里取？
  bool invert;             // 是否翻转条件
};

// 可以默认假设true的是跳走的？如果invert了就反过来。
struct BreakStmt {
  llvm::Instruction *inst; // 原本的跳转指令
  bool invert;             // 是否翻转条件
};

// 可以默认假设true的是跳走的？如果invert了就反过来。
struct ContinueStmt {
  llvm::Instruction *inst; // 原本的跳转指令
  bool invert;             // 是否翻转条件
};

struct ReturnStmt {
  llvm::Instruction *inst; // 原本的跳转指令
};

} // namespace notdec::backend

#endif
