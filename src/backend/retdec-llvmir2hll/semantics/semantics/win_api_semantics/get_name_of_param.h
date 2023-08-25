/**
* @file include/retdec/llvmir2hll/semantics/semantics/win_api_semantics/get_name_of_param.h
* @brief Provides function getNameOfParam() for WinAPISemantics in the
*        semantics::win_api namespace.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef RETDEC_LLVMIR2HLL_SEMANTICS_SEMANTICS_WIN_API_SEMANTICS_GET_NAME_OF_PARAM_H
#define RETDEC_LLVMIR2HLL_SEMANTICS_SEMANTICS_WIN_API_SEMANTICS_GET_NAME_OF_PARAM_H

#include <optional>
#include <string>

namespace retdec {
namespace llvmir2hll {
namespace semantics {
namespace win_api {

std::optional<std::string> getNameOfParam(const std::string &funcName,
	unsigned paramPos);

} // namespace win_api
} // namespace semantics
} // namespace llvmir2hll
} // namespace retdec

#endif
