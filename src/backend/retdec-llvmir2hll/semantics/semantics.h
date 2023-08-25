/**
* @file include/retdec/llvmir2hll/semantics/semantics.h
* @brief A base class for all descriptions of function semantics.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_LLVMIR2HLL_SEMANTICS_SEMANTICS_H
#define RETDEC_LLVMIR2HLL_SEMANTICS_SEMANTICS_H

#include <optional>
#include <string>

#include "backend/retdec-llvmir2hll/support/types.h"
#include "backend/retdec-llvmir2hll/retdec-utils/non_copyable.h"

namespace retdec {
namespace llvmir2hll {

/**
* @brief A base class for all descriptions of function semantics.
*
* This class should be used as a base class for all semantics.
*
* To implement a new semantics, you should:
*  - create a new class that subclasses this class or its subclass (you can
*    copy and edit an existing semantics)
*  - override the needed virtual member functions
*  - create a static create() function and register the semantics at
*    SemanticsFactory (see the implementation of existing subclasses)
*
* Instances of this class have reference object semantics.
*/
class Semantics: private retdec::utils::NonCopyable {
public:
	virtual ~Semantics() = default;

	/**
	* @brief Returns the ID of the semantics.
	*/
	virtual std::string getId() const = 0;

	/**
	* @brief Returns the name of the main function.
	*/
	virtual std::optional<std::string> getMainFuncName() const = 0;

	/**
	* @brief Returns the C header file for the given function.
	*
	* @return @c "file" if the header file is known, @c std::nullopt otherwise.
	*
	* @param[in] funcName Name of the function.
	*
	* For example, semantics describing the standard C library may return
	* @c "stdio.h" for @c "printf".
	*/
	virtual std::optional<std::string> getCHeaderFileForFunc(
		const std::string &funcName) const = 0;

	/**
	* @brief Gets the information whether the given function never returns.

	* @return @c true if the given function never returns, @c false
	*         if the function may return, and @c std::nullopt otherwise.
	*
	* @param[in] funcName Name of the function.
	*
	* For example, semantics describing the standard C library may return @c
	* true for @c "exit".
	*/
	virtual std::optional<bool> funcNeverReturns(const std::string &funcName) const = 0;

	/**
	* @brief Returns an appropriate name of the variable that stores the result
	*        of calls to the given function.
	*
	* @return @c "name" if the name is known, @c std::nullopt otherwise.
	*
	* @param[in] funcName Name of the function.
	*
	* For example, semantics describing the standard C library may return
	* @c "c" for @c "getchar".
	*/
	virtual std::optional<std::string> getNameOfVarStoringResult(
		const std::string &funcName) const = 0;

	/**
	* @brief Returns an appropriate name of the given function's parameter.
	*
	* @return @c "name" if the name is known, @c std::nullopt otherwise.
	*
	* @param[in] funcName Name of the function.
	* @param[in] paramPos Position of the parameter.
	*
	* For example, semantics describing the standard C library may return
	* @c "file_path" for the first parameter of @c "fopen".
	*/
	virtual std::optional<std::string> getNameOfParam(const std::string &funcName,
		unsigned paramPos) const = 0;

	/**
	* @brief Returns a mapping of integers into their symbolic names for the
	*        given parameter of the given function.
	*
	* @return @c map if the mapping is known, @c std::nullopt otherwise.
	*
	* @param[in] funcName Name of the function.
	* @param[in] paramPos Position of the parameter.
	*
	* For example, a semantics may return
	* @code
	* 1 -> LOCK_SH
	* 2 -> LOCK_EX
	* 4 -> LOCK_NB
	* 8 -> LOCK_UN
	* @endcode
	* for @c "flock" and @c 2.
	*/
	virtual std::optional<IntStringMap> getSymbolicNamesForParam(
		const std::string &funcName, unsigned paramPos) const = 0;

protected:
	Semantics() = default;
};

} // namespace llvmir2hll
} // namespace retdec

#endif
