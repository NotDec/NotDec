/**
* @file src/llvmir2hll/ir/continue_stmt.cpp
* @brief Implementation of ContinueStmt.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#include "retdec/llvmir2hll/ir/continue_stmt.h"
#include "retdec/llvmir2hll/support/debug.h"
#include "retdec/llvmir2hll/support/visitor.h"

namespace retdec {
namespace llvmir2hll {

/**
* @brief Constructs a new continue statement.
*/
ContinueStmt::ContinueStmt(Address a): Statement(a) {}

ShPtr<Value> ContinueStmt::clone() {
	ShPtr<ContinueStmt> continueStmt(ContinueStmt::create(getAddress()));
	continueStmt->setMetadata(getMetadata());
	return continueStmt;
}

bool ContinueStmt::isEqualTo(ShPtr<Value> otherValue) const {
	return isa<ContinueStmt>(otherValue);
}

void ContinueStmt::replace(ShPtr<Expression> oldExpr, ShPtr<Expression> newExpr) {
	// There is nothing to do.
}

ShPtr<Expression> ContinueStmt::asExpression() const {
	// Cannot be converted into an expression.
	return {};
}

/**
* @brief Creates a new continue statement.
* @param[in] a Address.
*/
ShPtr<ContinueStmt> ContinueStmt::create(Address a) {
	return ShPtr<ContinueStmt>(new ContinueStmt(a));
}

void ContinueStmt::accept(Visitor *v) {
	v->visit(ucast<ContinueStmt>(shared_from_this()));
}

} // namespace llvmir2hll
} // namespace retdec
