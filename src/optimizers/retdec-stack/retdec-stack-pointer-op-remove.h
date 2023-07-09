/**
* @file include/retdec/bin2llvmir/optimizations/stack_pointer_ops/stack_pointer_ops.h
* @brief Remove the remaining stack pointer operations.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_BIN2LLVMIR_OPTIMIZATIONS_STACK_POINTER_OPS_STACK_POINTER_OPS_H
#define RETDEC_BIN2LLVMIR_OPTIMIZATIONS_STACK_POINTER_OPS_STACK_POINTER_OPS_H

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

#include "optimizers/retdec-stack/retdec-abi.h"

namespace retdec {
namespace bin2llvmir {

using namespace llvm;

class StackPointerOpsRemove : public llvm::PassInfoMixin<StackPointerOpsRemove>
{
	public:
		StackPointerOpsRemove(Abi* abi): _abi(abi) {}
		PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
		bool runOnModuleCustom(llvm::Module& m, Abi* a);

	private:
		bool run();
		bool removeStackPointerStores();
		bool removePreservationStores();

	private:
		llvm::Module* _module = nullptr;
		Abi* _abi = nullptr;
};

} // namespace bin2llvmir
} // namespace retdec

#endif
