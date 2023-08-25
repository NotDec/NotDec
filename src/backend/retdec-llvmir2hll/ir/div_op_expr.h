/**
* @file include/retdec/llvmir2hll/ir/div_op_expr.h
* @brief A division operator.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_LLVMIR2HLL_IR_DIV_OP_EXPR_H
#define RETDEC_LLVMIR2HLL_IR_DIV_OP_EXPR_H

#include "backend/retdec-llvmir2hll/ir/binary_op_expr.h"
#include "backend/retdec-llvmir2hll/support/smart_ptr.h"

namespace retdec {
namespace llvmir2hll {

class Expression;
class Visitor;

/**
* @brief A division operator.
*
* This operator has the same meaning as the '/' operator in C.
*
* Use create() to create instances. Instances of this class have reference
* object semantics. This class is not meant to be subclassed.
*/
class DivOpExpr final: public BinaryOpExpr {
public:
	/// Variants of the operator.
	enum class Variant {
		UDiv, /// Unsigned division.
		SDiv, /// Signed division.
		FDiv  /// Floating-point division.
	};

public:
	static ShPtr<DivOpExpr> create(ShPtr<Expression> op1,
		ShPtr<Expression> op2, Variant variant = Variant::UDiv);

	virtual bool isEqualTo(ShPtr<Value> otherValue) const override;
	virtual ShPtr<Value> clone() override;

	Variant getVariant() const;

	/// @name Visitor Interface
	/// @{
	virtual void accept(Visitor *v) override;
	/// @}

private:
	/// Variant of the operation.
	Variant variant;

private:
	// Since instances are created by calling the static function create(), the
	// constructor can be private.
	DivOpExpr(ShPtr<Expression> op1, ShPtr<Expression> op2,
		Variant variant = Variant::UDiv);
};

} // namespace llvmir2hll
} // namespace retdec

#endif
