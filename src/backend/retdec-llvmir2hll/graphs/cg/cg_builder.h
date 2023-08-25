/**
* @file include/retdec/llvmir2hll/graphs/cg/cg_builder.h
* @brief A creator of call graphs (CGs) from modules.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_LLVMIR2HLL_GRAPHS_CG_CG_BUILDER_H
#define RETDEC_LLVMIR2HLL_GRAPHS_CG_CG_BUILDER_H

#include "backend/retdec-llvmir2hll/graphs/cg/cg.h"
#include "backend/retdec-llvmir2hll/support/smart_ptr.h"
#include "backend/retdec-llvmir2hll/support/visitors/ordered_all_visitor.h"
#include "backend/retdec-llvmir2hll/retdec-utils/non_copyable.h"

namespace retdec {
namespace llvmir2hll {

class Module;

/**
* @brief A creator of call graphs (CGs) from modules.
*
* This class implements the "static helper" (or "library") design pattern (it
* has just static functions and no public instances can be created).
*/
class CGBuilder: private OrderedAllVisitor, private retdec::utils::NonCopyable {
public:
	static ShPtr<CG> getCG(ShPtr<Module> module);

private:
	explicit CGBuilder(ShPtr<Module> module);

	void computeCG();
	ShPtr<CG::CalledFuncs> computeCGPartForFunction(ShPtr<Function> func);

	/// @name Visitor Interface
	/// @{
	using OrderedAllVisitor::visit;
	virtual void visit(ShPtr<CallExpr> expr) override;
	/// @}

private:
	/// CG that is currently being built.
	ShPtr<CG> cg;

	/// CalledFuncs that is currently being built.
	ShPtr<CG::CalledFuncs> calledFuncs;
};

} // namespace llvmir2hll
} // namespace retdec

#endif
