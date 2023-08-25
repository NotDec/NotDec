/**
* @file include/retdec/llvmir2hll/semantics/semantics/gcc_general_semantics.h
* @brief A general semantics for the GCC compiler.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_LLVMIR2HLL_SEMANTICS_SEMANTICS_GCC_GENERAL_SEMANTICS_H
#define RETDEC_LLVMIR2HLL_SEMANTICS_SEMANTICS_GCC_GENERAL_SEMANTICS_H

#include <optional>
#include <string>

#include "backend/retdec-llvmir2hll/semantics/semantics/default_semantics.h"
#include "backend/retdec-llvmir2hll/support/smart_ptr.h"

namespace retdec {
namespace llvmir2hll {

/**
* @brief A general semantics for the GCC compiler.
*
* This class provides a general semantics for the GCC compiler. It may return
* implementation-defined results. The data for it were obtained from a
* GNU/Linux system (Arch Linux 64b, kernel 3.8.6), running gcc 4.8.0 and glibc
* 2.17.
*
* Instances of this class have reference object semantics.
*/
class GCCGeneralSemantics: public DefaultSemantics {
public:
	static ShPtr<Semantics> create();

	/// @name Semantics Interface
	/// @{
	virtual std::string getId() const override;
	virtual std::optional<std::string> getCHeaderFileForFunc(
		const std::string &funcName) const override;
	virtual std::optional<std::string> getNameOfVarStoringResult(
		const std::string &funcName) const override;
	virtual std::optional<std::string> getNameOfParam(const std::string &funcName,
		unsigned paramPos) const override;
	virtual std::optional<IntStringMap> getSymbolicNamesForParam(
		const std::string &funcName, unsigned paramPos) const override;
	/// @}

protected:
	GCCGeneralSemantics();
};

} // namespace llvmir2hll
} // namespace retdec

#endif
