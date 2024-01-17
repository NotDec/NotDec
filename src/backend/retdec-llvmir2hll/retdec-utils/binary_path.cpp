/**
 * @file src/utils/binary_path.cpp
 * @brief Absolute path of currently running binary getters.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <cassert>
// #include <whereami/whereami.h>

#include "backend/retdec-llvmir2hll/retdec-utils/binary_path.h"

namespace {

// std::string getStringThisBinaryPath(
// 		std::size_t &directorySize)
// {
// 	int pathSize = 0;
// 	int dirPathSize = 0;
// 	std::string result;

// 	// Get length of path.
// 	pathSize = wai_getExecutablePath(NULL, 0, &dirPathSize);

// 	// Get path again.
// 	if (pathSize > 0)
// 	{
// 		result.resize(pathSize);
// 		wai_getExecutablePath(&result[0], pathSize, &dirPathSize);
// 	}

// 	directorySize = dirPathSize;
// 	return result;
// }

} // anonymous namespace

namespace retdec {
namespace utils {

//
//=============================================================================
//  Binary path
//=============================================================================
//

/**
 * Get absolute path of currently running binary.
 *
 * @return absolute path
 */
fs::path getThisBinaryPath()
{
	assert(!"not implemented");
}

/**
 * Get absolute path to directory of currently running binary.
 *
 * @return absolute path
 */
fs::path getThisBinaryDirectoryPath()
{
	assert(!"not implemented");
}

} // namespace utils
} // namespace retdec
