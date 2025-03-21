/**
* @file include/retdec/bin2llvmir/optimizations/stack/stack.h
* @brief Reconstruct stack.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_BIN2LLVMIR_OPTIMIZATIONS_STACK_STACK_H
#define RETDEC_BIN2LLVMIR_OPTIMIZATIONS_STACK_STACK_H

#include <optional>
#include <unordered_set>

#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

#include "Passes/retdec-stack/retdec-symbolic-tree.h"
// #include "retdec/bin2llvmir/analyses/symbolic_tree.h"
// #include "retdec/bin2llvmir/providers/abi/abi.h"
// #include "retdec/bin2llvmir/providers/config.h"
// #include "retdec/bin2llvmir/providers/debugformat.h"

namespace retdec {
namespace bin2llvmir {
using namespace llvm;
class StackAnalysis : public llvm::PassInfoMixin<StackAnalysis>
{
	public:
		StackAnalysis(Abi* abi): _abi(abi) {}
		PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
		bool runOnModuleCustom(
				llvm::Module& m,
				Abi* abi
				);

	private:
		bool run();
		void handleInstruction(
				ReachingDefinitionsAnalysis& RDA,
				llvm::Instruction* inst,
				llvm::Value* val,
				llvm::Type* type,
				std::map<llvm::Value*, llvm::Value*>& val2val,
				std::map<int64_t, llvm::Value*>& off2alloca);
		std::optional<int> getBaseOffset(SymbolicTree &root);

	private:
		llvm::Module* _module = nullptr;
		Abi* _abi = nullptr;

		std::unordered_set<llvm::Value*> _toRemove;
};

} // namespace bin2llvmir
} // namespace retdec

#endif
