/**
 * @file src/utils/version.cpp
 * @brief RetDec version implementation.
 * @copyright (c) 2021 Avast Software, licensed under the MIT license
 */

#include "backend/retdec-llvmir2hll/retdec-utils/version.h"

namespace retdec {
namespace utils {
namespace version {

std::string getCommitHash()
{
	return "6238ecada5a880cf3df6b6ad90d49b0c06fdb84b";
}

std::string getShortCommitHash(unsigned length)
{
	return getCommitHash().substr(0, length);
}

std::string getBuildDate()
{
	return "2023-05-25";
}

std::string getVersionTag()
{
	return "master";
}

std::string getVersionStringLong()
{
	return  "RetDec version :  " + getVersionTag() + "\n"
			"Commit hash    :  " + getCommitHash() + "\n"
			"Build date     :  " + getBuildDate();
}

std::string getVersionStringShort()
{
	return  "RetDec " + getVersionTag() +
			" built on " + getBuildDate();
}

} // namespace version
} // namespace utils
} // namespace retdec
