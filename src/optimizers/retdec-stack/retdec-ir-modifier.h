/**
 * @file include/retdec/bin2llvmir/utils/ir_modifier.h
 * @brief Modify both LLVM IR and config.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#ifndef RETDEC_BIN2LLVMIR_UTILS_IR_MODIFIER_H
#define RETDEC_BIN2LLVMIR_UTILS_IR_MODIFIER_H

#include <unordered_set>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

// #include "retdec/bin2llvmir/providers/abi/abi.h"
// #include "retdec/bin2llvmir/providers/config.h"
// #include "retdec/bin2llvmir/providers/fileimage.h"

namespace retdec {
namespace bin2llvmir {

class IrModifier
{
	// Methods not using member data -> do not need instance of this class.
	// Can be used simply like this: \c IrModifier::method().
	//
	public:
		static llvm::AllocaInst* createAlloca(
				llvm::Function* fnc,
				llvm::Type* ty,
				const std::string& name = std::string());

		static llvm::Value* convertValueToType(
				llvm::Value* val,
				llvm::Type* type,
				llvm::Instruction* before);

		static llvm::Value* convertValueToTypeAfter(
				llvm::Value* val,
				llvm::Type* type,
				llvm::Instruction* after);

		static llvm::Constant* convertConstantToType(
				llvm::Constant* val,
				llvm::Type* type);

		static llvm::CallInst* modifyCallInst(
				llvm::CallInst* call,
				llvm::Type* ret,
				llvm::ArrayRef<llvm::Value*> args);

		static void eraseUnusedInstructionRecursive(llvm::Value* insn);
		static void eraseUnusedInstructionsRecursive(
				std::unordered_set<llvm::Value*>& insns);

	public:
		IrModifier(llvm::Module* m);

	protected:
		llvm::Module* _module = nullptr;
};

} // namespace bin2llvmir
} // namespace retdec

#endif
