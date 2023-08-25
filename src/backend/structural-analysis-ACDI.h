#ifndef _NOTDEC_BACKEND_STRUCTURAL_ACDI_H_
#define _NOTDEC_BACKEND_STRUCTURAL_ACDI_H_


#include "optimizers/retdec-stack/retdec-utils.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

void printModule(const llvm::Module& M, llvm::raw_fd_ostream& os);
std::string printBasicBlock(const llvm::BasicBlock *b);
std::string printFunction(const llvm::Function *F);


#endif
