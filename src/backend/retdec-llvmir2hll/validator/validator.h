/**
* @file include/retdec/llvmir2hll/validator/validator.h
* @brief A base class for all validators.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_LLVMIR2HLL_VALIDATOR_VALIDATOR_H
#define RETDEC_LLVMIR2HLL_VALIDATOR_VALIDATOR_H

#include <string>

#include "backend/retdec-llvmir2hll/support/smart_ptr.h"
#include "backend/retdec-llvmir2hll/support/visitors/ordered_all_visitor.h"
#include "backend/retdec-llvmir2hll/retdec-utils/io/log.h"

using namespace retdec::utils::io;

namespace retdec {
namespace llvmir2hll {

/**
* @brief A base class for all validators.
*
* Every concrete validator has to:
*  - define a static <tt>ShPtr<Validator> create()</tt> function
*  - define @c getId(), which returns the ID of the validator
*  - when there is a validation error, validationError() has to be run
*  - in its description, mention what validations are performed
*  - if necessary, redefine @c runValidation()
*
* A concrete validator can utilize protected members of this base class.
*
* Instances of this class have reference object semantics.
*/
class Validator: protected OrderedAllVisitor {
public:
	virtual std::string getId() const = 0;

	bool validate(ShPtr<Module> module, bool printMessageOnError = false);

protected:
	Validator();

	void traverseAllGlobalVariables();
	void traverseAllFunctions();

	/**
	* @brief Function to be called when there is a validation error.
	*/
	void validationError(const std::string &warningMessage) {
		moduleIsCorrect = false;
		if (printMessageOnError) {
			Log::error() << Log::Warning << warningMessage << std::endl;
		}
	}

protected:
	/// The validated module.
	ShPtr<Module> module;

	/// The currently traversed function.
	ShPtr<Function> func;

private:
	virtual void runValidation();

private:
	/// Should we print a warning message when encountering an error?
	bool printMessageOnError;

	/// @c true if there has not been an error, @c false otherwise.
	bool moduleIsCorrect;
};

} // namespace llvmir2hll
} // namespace retdec

#endif
