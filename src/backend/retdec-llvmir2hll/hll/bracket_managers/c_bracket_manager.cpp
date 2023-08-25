/**
* @file src/llvmir2hll/hll/bracket_managers/c_bracket_manager.cpp
* @brief A subclass from BracketManager that contains precedence table and
*        methods which decide of elimination redundant brackets for the C language.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#include "retdec/llvmir2hll/hll/bracket_managers/c_bracket_manager.h"

namespace retdec {
namespace llvmir2hll {

/**
* @brief Constructs a new C brackets manager.
*
* @param[in] module The module to be analyzed.
*/
CBracketManager::CBracketManager(ShPtr<Module> module): BracketManager(module) {
	// Starts running of brackets elimination analyse.
	init();
}

std::string CBracketManager::getId() const {
	return "CBracketManager";
}

/**
* @brief Overrided from base class @c BracketManager. This function enter to
*        precedence table and return result. Precedence tables is indexed with
*        @c previous operator and @a current operator.
*
* @param[in] currentOperator @a current operator.
* @param[in] prevOperator @a previous operator.
*
* @return item of @c precedenceTable.
*/
BracketManager::ItemOfPrecTable CBracketManager::checkPrecTable(
		Operators currentOperator, Operators prevOperator) {
	// Get indexed item in precedence table.
	auto prevOpIdx = static_cast<int>(prevOperator);
	auto currOpIdx = static_cast<int>(currentOperator);
	return precedenceTable[prevOpIdx][currOpIdx];
}

/**
* @brief Function that check if operator is supported in chosen language.
*
* @param[in] currentOperator Operator to check.
*
* @return @c true if operator is supported, @c false otherwise.
*/
bool CBracketManager::isOperatorSupported(Operators currentOperator) {
	auto currOpIdx = static_cast<int>(currentOperator);
	ItemOfPrecTable item = precedenceTable[currOpIdx][currOpIdx];
	// S - signalize skip - Not supported operator.
	return item.association != S;
}

CBracketManager::ItemOfPrecTable CBracketManager::precedenceTable[PREC_TABLE_SIZE][PREC_TABLE_SIZE] = {
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // AddOpExpr
	{{N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}}, // AddressOpExpr
	{{N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // AndOpExpr
	{{N,F}, {N,T}, {N,F}, {L,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {L,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {L,F}, {N,F}, {N,F}}, // ArrayIndexOpExpr
	{{N,F}, {N,F}, {N,F}, {N,F}, {R,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}}, // AssignOpExpr
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // BitAndOpExpr
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // BitOrOpExpr
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {L,F}, {L,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // BitShlOpExpr
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {L,F}, {L,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // BitShrOpExpr
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // BitXorOpExpr
	{{N,F}, {N,F}, {N,F}, {L,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {L,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {L,F}, {N,F}, {N,F}}, // CallExpr
	{{N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}}, // CastExpr
	{{N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {L,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}}, // CommaOpExpr
	{{N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}}, // DerefOpExpr
	{{N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {L,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {L,F}, {L,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}}, // DivOpExpr
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {L,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // EqOpExpr
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {L,F}, {L,F}, {L,F}, {L,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // GtOpExpr
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {L,F}, {L,F}, {L,F}, {L,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // GtEqOpExpr
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {L,F}, {L,F}, {L,F}, {L,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // LtOpExpr
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {L,F}, {L,F}, {L,F}, {L,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // LtEqOpExpr
	{{N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {L,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {L,F}, {L,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}}, // ModOpExpr
	{{N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {L,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}}, // MulOpExpr
	{{N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}}, // NegOpExpr
	{{N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {L,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}}, // NeqOpExpr
	{{N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}}, // NotOpExpr
	{{N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}}, // OrOpExpr
	{{N,T}, {N,T}, {N,T}, {L,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {L,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {L,F}, {N,T}, {N,T}}, // StructIndexOpExpr
	{{L,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,T}, {N,T}, {N,T}, {N,T}, {N,T}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,T}, {N,F}, {L,F}, {N,T}}, // SubOpExpr
	{{N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,T}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {N,F}, {R,F}}, // TernaryOpExpr
};

} // namespace llvmir2hll
} // namespace retdec
