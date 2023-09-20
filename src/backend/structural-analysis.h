#ifndef _NOTDEC_BACKEND_STRUCTURAL_H_
#define _NOTDEC_BACKEND_STRUCTURAL_H_

#include "backend/statements.h"
#include "backend/region-graph.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

#include "optimizers/retdec-stack/retdec-utils.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>


namespace notdec::backend {

void printModule(const llvm::Module& M, llvm::raw_fd_ostream& os);
std::string printBasicBlock(const llvm::BasicBlock *b);
std::string printFunction(const llvm::Function *F);


class IStructuralAnalysis
{
protected:
    llvm::Module& mod;
    llvm::Function& func;
public:

    IStructuralAnalysis(llvm::Module& mod, llvm::Function& func) : mod(mod), func(func) {}

    virtual ~IStructuralAnalysis() = default;
    virtual Region execute() = 0;
};



}


#endif
