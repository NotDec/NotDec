/**
* @file include/retdec/llvmir2hll/hll/bracket_manager.h
* @brief A base class of all BracketManagers.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_LLVMIR2HLL_HLL_BRACKET_MANAGER_H
#define RETDEC_LLVMIR2HLL_HLL_BRACKET_MANAGER_H

#include <map>
#include <stack>

#include "backend/retdec-llvmir2hll/support/smart_ptr.h"
#include "backend/retdec-llvmir2hll/support/types.h"
#include "backend/retdec-llvmir2hll/support/visitors/ordered_all_visitor.h"

namespace retdec {
namespace llvmir2hll {

class BinaryOpExpr;
class CastExpr;
class Expression;
class TernaryOpExpr;
class UnaryOpExpr;

/**
* @brief A base class of all brackets managers.
*
* Every brackets manager should subclass this class and override protected
* @c checkPrecTable(...) and @c isOperatorSupported(...) functions.
* In subclass is needed to add a @a Precedence table, which is generated by script
* @a hll_prec_table_gen.py which is in @a /decompiler/scripts/. In @a C/Py/.. HLL Writer
* need to override @c emitTargetCode(...) and call the constructor of subclass
* BracketManager. In constructor of subclass need to place function @c init(...).
*
* Instances of this class have reference object semantics.
*/
class BracketManager: private OrderedAllVisitor {
protected:
	/// Enumeration for operators.
	enum class Operators {
		ADD,
		ADDRESS,
		AND,
		ARRAY,
		ASSIGN,
		BITAND,
		BITOR,
		BITSHL,
		BITSHR,
		BITXOR,
		CALL,
		CAST,
		COMMA,
		DEREF,
		DIV,
		EQ,
		GT,
		GTEQ,
		LT,
		LTEQ,
		MOD,
		MUL,
		NEG,
		NEQ,
		NOT,
		OR,
		STRUCT,
		SUB,
		TERNARY
	};

	/// Enumeration for direction tree traversal.
	enum class Direction {
		CENTER,
		LEFT,
		RIGHT
	};

	/// String constants for L = left-associative, R = right-associative, N = doesn't
	/// mattter, S = skip operator, operator is not supported.
	// It is not a scoped enum (no "class" after "enum") because the code is
	// more readable without the enum's name.
	enum FirstParInItemOfPrecTable {
		L,
		R,
		N,
		S
	};

	/// Structure for one item in precedence table. Structure contains associativity
	/// and status if brackets are needed which is set by priority of operator.
	struct ItemOfPrecTable {
		FirstParInItemOfPrecTable association;
		bool bracketsNeeded;
	};

	/// Structure for one item in prevOperatorsStack. Structure contains previous
	/// operator and direction of tree traversal.
	struct PrevOperators {
		Operators prevOperator;
		Direction treeDirection;
	};

	/// Bool constants.
	static const bool T = true;
	static const bool F = false;
	/// Constant for size of precedence table.
	// Has to correspond to the number of elements in the Operators enum (!).
	static const int PREC_TABLE_SIZE = 29;

private:
	/// Stack of structures. Every structure contains previous operator and direction
	/// of tree traversal.
	std::stack<PrevOperators> prevOperatorsStack;
	/// This map contains Expression adresses and status to write, or doesn't write
	/// brackets.
	std::map<ShPtr<Expression>, bool> bracketsAreNeededMap;
	/// The module to be analyzed.
	ShPtr<Module> module;

public:
	BracketManager(ShPtr<Module> module);

	void init();

	/**
	* @brief Returns the ID of the BracketManager.
	*/
	virtual std::string getId() const = 0;

	bool areBracketsNeeded(ShPtr<Expression> expr);

	/// @name OrderedAllVisitor Interface
	/// @{
	using OrderedAllVisitor::visit;
	virtual void visit(ShPtr<AddressOpExpr> expr) override;
	virtual void visit(ShPtr<AssignOpExpr> expr) override;
	virtual void visit(ShPtr<ArrayIndexOpExpr> expr) override;
	virtual void visit(ShPtr<StructIndexOpExpr> expr) override;
	virtual void visit(ShPtr<DerefOpExpr> expr) override;
	virtual void visit(ShPtr<NotOpExpr> expr) override;
	virtual void visit(ShPtr<NegOpExpr> expr) override;
	virtual void visit(ShPtr<EqOpExpr> expr) override;
	virtual void visit(ShPtr<NeqOpExpr> expr) override;
	virtual void visit(ShPtr<LtEqOpExpr> expr) override;
	virtual void visit(ShPtr<GtEqOpExpr> expr) override;
	virtual void visit(ShPtr<LtOpExpr> expr) override;
	virtual void visit(ShPtr<GtOpExpr> expr) override;
	virtual void visit(ShPtr<AddOpExpr> expr) override;
	virtual void visit(ShPtr<SubOpExpr> expr) override;
	virtual void visit(ShPtr<MulOpExpr> expr) override;
	virtual void visit(ShPtr<ModOpExpr> expr) override;
	virtual void visit(ShPtr<DivOpExpr> expr) override;
	virtual void visit(ShPtr<AndOpExpr> expr) override;
	virtual void visit(ShPtr<OrOpExpr> expr) override;
	virtual void visit(ShPtr<BitAndOpExpr> expr) override;
	virtual void visit(ShPtr<BitOrOpExpr> expr) override;
	virtual void visit(ShPtr<BitXorOpExpr> expr) override;
	virtual void visit(ShPtr<BitShlOpExpr> expr) override;
	virtual void visit(ShPtr<BitShrOpExpr> expr) override;
	virtual void visit(ShPtr<TernaryOpExpr> expr) override;
	virtual void visit(ShPtr<CallExpr> expr) override;
	virtual void visit(ShPtr<CommaOpExpr> expr) override;
	// Casts
	virtual void visit(ShPtr<BitCastExpr> expr) override;
	virtual void visit(ShPtr<ExtCastExpr> expr) override;
	virtual void visit(ShPtr<TruncCastExpr> expr) override;
	virtual void visit(ShPtr<FPToIntCastExpr> expr) override;
	virtual void visit(ShPtr<IntToFPCastExpr> expr) override;
	virtual void visit(ShPtr<IntToPtrCastExpr> expr) override;
	virtual void visit(ShPtr<PtrToIntCastExpr> expr) override;
	// Constants
	virtual void visit(ShPtr<ConstBool> constant) override;
	virtual void visit(ShPtr<ConstFloat> constant) override;
	virtual void visit(ShPtr<ConstInt> constant) override;
	virtual void visit(ShPtr<ConstNullPointer> constant) override;
	virtual void visit(ShPtr<ConstString> constant) override;
	virtual void visit(ShPtr<ConstArray> constant) override;
	virtual void visit(ShPtr<ConstStruct> constant) override;
	virtual void visit(ShPtr<ConstSymbol> constant) override;
	// Extra
	virtual void visit(ShPtr<Variable> var) override;
	/// @}

protected:
	virtual ItemOfPrecTable checkPrecTable(Operators currentOperator,
		Operators prevOperator) = 0;
	virtual bool isOperatorSupported(Operators currentOperator) = 0;

	PrevOperators getPrevOperator();

private:
	void addOperatorOnStackIfSupported(Operators currentOperator,
		Direction direction);
	void areBracketsNeededForExpr(ShPtr<Expression> expr,
		Operators currentOperator);
	bool areBracketsNeededPrecTable(Operators currentOperator);
	void removeOperatorFromStackIfSupported(Operators currentOperator);

	void treeTraversalForBinaryOpWithStackOperations(
		ShPtr<BinaryOpExpr> expr, Operators currentOperator);
	void treeTraversalForCallWithStackOperations(
		ShPtr<CallExpr> expr, Operators currentOperator);
	void treeTraversalForCastWithStackOperations(
		ShPtr<CastExpr> expr, Operators currentOperator);
	void treeTraversalForTernaryOpWithStackOperations(
		ShPtr<TernaryOpExpr> expr, Operators currentOperator);
	void treeTraversalForUnaryOpWithStackOperations(
		ShPtr<UnaryOpExpr> expr, Operators currentOperator);
};

} // namespace llvmir2hll
} // namespace retdec

#endif
