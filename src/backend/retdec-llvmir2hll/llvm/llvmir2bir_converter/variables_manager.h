/**
* @file include/retdec/llvmir2hll/llvm/llvmir2bir_converter/variables_manager.h
* @brief Managing of local variables created during conversion of LLVM
*        functions to BIR.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_LLVMIR2HLL_LLVM_LLVMIR2BIR_CONVERTER_VARIABLES_MANAGER_H
#define RETDEC_LLVMIR2HLL_LLVM_LLVMIR2BIR_CONVERTER_VARIABLES_MANAGER_H

#include <unordered_map>

#include "backend/retdec-llvmir2hll/support/smart_ptr.h"
#include "backend/retdec-llvmir2hll/support/types.h"
#include "backend/retdec-llvmir2hll/var_name_gen/var_name_gen.h"
#include "backend/retdec-llvmir2hll/retdec-utils/non_copyable.h"

namespace llvm {

class Value;

} // namespace llvm

namespace retdec {
namespace llvmir2hll {

class Module;
class Type;
class Variable;

/**
* @brief Managing of local variables created during conversion of LLVM
*        functions to BIR.
*/
class VariablesManager final: private retdec::utils::NonCopyable {
public:
	VariablesManager(ShPtr<Module> resModule);

	void reset();

	void addGlobalValVarPair(llvm::Value *val, ShPtr<Variable> var);
	ShPtr<Variable> getVarByValue(llvm::Value *val);
	VarSet getLocalVars() const;

private:
	void assignNameToValue(llvm::Value *val) const;
	ShPtr<Variable> getGlobalVar(llvm::Value *val);
	ShPtr<Variable> getOrCreateLocalVar(llvm::Value *val);

	/// Mapping of a LLVM value to an existing variable.
	std::unordered_map<llvm::Value*, ShPtr<Variable>> localVarsMap;

	/// Mapping of LLVM functions and globals to existing variables.
	std::unordered_map<llvm::Value*, ShPtr<Variable>> globalVarsMap;

	/// Variable names generator.
	UPtr<VarNameGen> varNameGen;

	/// The resulting module in BIR.
	ShPtr<Module> resModule;
};

} // namespace llvmir2hll
} // namespace retdec

#endif
