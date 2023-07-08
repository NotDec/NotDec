/**
 * @file include/retdec/bin2llvmir/providers/abi/abi.h
 * @brief ABI information.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#ifndef RETDEC_BIN2LLVMIR_PROVIDERS_ABI_ABI_H
#define RETDEC_BIN2LLVMIR_PROVIDERS_ABI_ABI_H

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include <llvm/IR/Module.h>

// #include "retdec/bin2llvmir/providers/asm_instruction.h"
// #include "retdec/bin2llvmir/providers/config.h"
// #include "retdec/bin2llvmir/providers/calling_convention/calling_convention.h"

//#include "retdec/capstone2llvmir/x86/x86_defs.h"

namespace retdec {
namespace bin2llvmir {

class Abi
{
	// Constants.
	//
	public:
		static const uint32_t REG_INVALID;
		static const unsigned DEFAULT_ADDR_SPACE;

	// Ctors, dtors.
	//
	public:
		Abi(llvm::Module* m);
		virtual ~Abi() = default;

	// Registers.
	//
	public:
		bool isRegister(const llvm::Value* val) const;
		// bool isRegister(const llvm::Value* val, uint32_t r) const;
		bool isStackPointerRegister(const llvm::Value* val) const;
        void setStackPointer(const llvm::GlobalVariable* val) const;

	// Stacks.
	//
	public:
		bool isStackVariable(const llvm::Value* val) const;

	// Private data - misc.
	//
	protected:
		llvm::Module* _module = nullptr;
        llvm::GlobalVariable* sp;
		// Config* _config = nullptr;

};

} // namespace bin2llvmir
} // namespace retdec

#endif
