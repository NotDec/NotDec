
#ifndef _NOTDEC_BACKEND_REGION_H_
#define _NOTDEC_BACKEND_REGION_H_

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <variant>
#include <vector>
namespace notdec::backend {

class IfStmt;

class WhileStmt;
class DoWhileStmt;

class BreakStmt;
class ContinueStmt;
class ReturnStmt;

// 用 std::holds_alternative<IfStmt>(v) 判断是哪种类型的
using Statement = std::variant<llvm::Instruction, IfStmt, WhileStmt>;
using Statements = std::vector<Statement>;


class IfStmt{
    llvm::Instruction* inst; // 原本的跳转指令
    llvm::Value* condition; // 没想好，要不直接从跳转指令里取？
    bool invert; // 是否翻转条件
    Statements then;
    Statements els;
};

class WhileStmt{
    Statements body;
    llvm::Instruction* inst; // 原本的跳转指令
    llvm::Value* condition; // 没想好，要不直接从跳转指令里取？
    bool invert; // 是否翻转条件
};

class DoWhileStmt {
    Statements body;
    llvm::Instruction* inst; // 原本的跳转指令
    llvm::Value* condition; // 没想好，要不直接从跳转指令里取？
    bool invert; // 是否翻转条件
};

// 可以默认假设true的是跳走的？如果invert了就反过来。
class BreakStmt {
    llvm::Instruction* inst; // 原本的跳转指令
    bool invert; // 是否翻转条件
};

// 可以默认假设true的是跳走的？如果invert了就反过来。
class ContinueStmt {
    llvm::Instruction* inst; // 原本的跳转指令
    bool invert; // 是否翻转条件
};

class ReturnStmt {
    llvm::Instruction* inst; // 原本的跳转指令
};


}

#endif
