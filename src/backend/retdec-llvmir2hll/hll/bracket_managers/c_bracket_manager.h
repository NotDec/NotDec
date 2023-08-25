/**
* @file include/retdec/llvmir2hll/hll/bracket_managers/c_bracket_manager.h
* @brief A brackets manager of redundant brackets for the C language.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_LLVMIR2HLL_HLL_BRACKET_MANAGERS_C_BRACKET_MANAGER_H
#define RETDEC_LLVMIR2HLL_HLL_BRACKET_MANAGERS_C_BRACKET_MANAGER_H

#include "backend/retdec-llvmir2hll/hll/bracket_manager.h"

namespace retdec {
namespace llvmir2hll {

/**
* @brief A class that contains precedence table and overrided methods which
*        decide of elimination redundant brackets for the C language.
*/
class CBracketManager: public BracketManager {
public:
	CBracketManager(ShPtr<Module> module);

	virtual std::string getId() const override;

private:
	virtual ItemOfPrecTable checkPrecTable(Operators currentOperator,
		Operators prevOperator) override;
	virtual bool isOperatorSupported(Operators currentOperator) override;

private:
	/// Precedence table of operators.
	static ItemOfPrecTable precedenceTable[PREC_TABLE_SIZE][PREC_TABLE_SIZE];
};

} // namespace llvmir2hll
} // namespace retdec

#endif
