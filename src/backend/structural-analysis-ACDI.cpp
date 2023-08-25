#include "backend/structural-analysis-ACDI.h"

std::string printBasicBlock(const llvm::BasicBlock *b) {
    return llvmObjToString(b);
}

void printModule(const llvm::Module& M, llvm::raw_fd_ostream& os) {
    for (const llvm::Function &F : M)
        os << printFunction(&F);
}

std::string printFunction(const llvm::Function *F) {
    std::string str;
    llvm::raw_string_ostream Out(str);
    if (F->isDeclaration()) {
        Out << "declare ";
        Out << ' ';
    } else {
        Out << "define ";
    }

    Out << F->getName();
    Out << " {";
    // Output all of the function's basic blocks.
    for (const llvm::BasicBlock &BB : *F)
        Out << printBasicBlock(&BB);

    Out << "}\n\n";
    return Out.str();
}
