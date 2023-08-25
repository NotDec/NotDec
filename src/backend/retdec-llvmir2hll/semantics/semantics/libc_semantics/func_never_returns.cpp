/**
* @file src/llvmir2hll/semantics/semantics/libc_semantics/func_never_returns.cpp
* @brief Implementation of semantics::libc::funcNeverReturns() for
*        LibcSemantics.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#include "retdec/llvmir2hll/semantics/semantics/libc_semantics/func_never_returns.h"
#include "retdec/llvmir2hll/support/types.h"
#include "retdec/utils/container.h"

using retdec::utils::hasItem;

namespace retdec {
namespace llvmir2hll {
namespace semantics {
namespace libc {

namespace {

/**
* @brief This function is used to initialize FUNC_NEVER_RETURNS later in
*        the file.
*/
const StringSet &initFuncNeverReturns() {
	static StringSet fnr;

	// Currently, we only list the functions about which we actually know that
	// they never return. The reason is that when using funcNeverReturns(), we
	// actually only care whether it returns true. If it returns false or no
	// answer at all is irrelevant.

	// C89
	fnr.insert("exit");
	fnr.insert("abort");
	fnr.insert("longjmp");

	// C99
	fnr.insert("_Exit");

	// C11
	fnr.insert("quick_exit");
	fnr.insert("thrd_exit");

	return fnr;
}

/// Functions that do not return.
const StringSet &FUNC_NEVER_RETURNS(initFuncNeverReturns());

} // anonymous namespace

/**
* @brief Implements funcNeverReturns() for LibcSemantics.
*
* See its description for more details.
*/
std::optional<bool> funcNeverReturns(const std::string &funcName) {
	return hasItem(FUNC_NEVER_RETURNS, funcName)
			? std::optional<bool>(true) : std::nullopt;
}

} // namespace libc
} // namespace semantics
} // namespace llvmir2hll
} // namespace retdec
