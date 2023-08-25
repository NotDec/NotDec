/**
 * @file src/common/tool_info.cpp
 * @brief Common tool information representation.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <algorithm>
#include <sstream>

#include "backend/retdec-llvmir2hll/common/tool_info.h"
#include "backend/retdec-llvmir2hll/retdec-utils/string.h"

namespace {

const std::string CC_UNKNOWN = "unknown";
const std::string CC_BORLAND = "borland";
const std::string CC_GCC = "gcc";
const std::string CC_INTEL = "intel";
const std::string CC_OPEN_WATCOM = "open_watcom";
const std::string CC_VISUAL_STUDIO = "visual_studio";

const std::string T_LINKER = "linker";
const std::string T_COMPILER = "compiler";
const std::string T_INSTALLER = "installer";
const std::string T_PACKER = "packer";
const std::string T_UNKNOWN = "unknown";

} // anonymous namespace

namespace retdec {
namespace common {

//
//=============================================================================
// ToolInfo
//=============================================================================
//

bool ToolInfo::isUnknown() const      { return !isKnown(); }
bool ToolInfo::isKnown() const        { return isBorland() || isGcc() || isIntel() || isOpenWatcom() || isMsvc(); }
bool ToolInfo::isBorland() const      { return isTool(CC_BORLAND); }
bool ToolInfo::isGcc() const          { return isTool(CC_GCC); }
bool ToolInfo::isIntel() const        { return isTool(CC_INTEL); }
bool ToolInfo::isOpenWatcom() const   { return isTool(CC_OPEN_WATCOM); }

bool ToolInfo::isCompiler() const     { return _type == T_COMPILER; }
bool ToolInfo::isLinker() const       { return _type == T_LINKER; }
bool ToolInfo::isInstaller() const    { return _type == T_INSTALLER; }
bool ToolInfo::isPacker() const       { return _type == T_PACKER; }
bool ToolInfo::isUnknownType() const  { return _type == T_UNKNOWN; }
bool ToolInfo::isKnownType() const    { return !isUnknownType(); }

/**
 * @param version Any additional string which must be contained in tool name.
 *                If empty string is provided, any MSVC version is matched.
 */
bool ToolInfo::isMsvc(const std::string& version) const
{
	return (isTool(CC_VISUAL_STUDIO) ||
		(isTool("MSVC")) || isTool("Microsoft Linker")) && isToolVersion(version);
}

/**
 * Checks if this tool instance matches with the provided tool name.
 * Matching is successful if instance's name contains (case insensitive) the provided name.
 * @param n Name to match with.
 * @return @c True if matching successful, @c false otherwise.
 */
bool ToolInfo::isTool(const std::string& n) const
{
	return retdec::utils::containsCaseInsensitive(_name, n)
			|| retdec::utils::containsCaseInsensitive(_version, n);
}

/**
 * Checks if this tool version matches with the provided tool version.
 * Matching is successful if instance's version contains (case insensitive) the provided version.
 * @param v Version to match with.
 * @return @c True if matching successful, @c false otherwise.
 */
bool ToolInfo::isToolVersion(const std::string& v) const
{
	return retdec::utils::containsCaseInsensitive(_version, v);
}

void ToolInfo::setMajorVersion(unsigned int v)            { _majorVersion = v; }
void ToolInfo::setMinorVersion(unsigned int v)            { _minorVersion = v; }
void ToolInfo::setPatchVersion(unsigned int v)            { _patchVersion = v; }
void ToolInfo::setAdditionalInfo(const std::string& i)    { _additionalInfo = i; }
void ToolInfo::setPercentage(double p)                    { _percentage = p; }
void ToolInfo::setIdenticalSignificantNibbles(unsigned i) { _identicalSignificantNibbles = i; }
void ToolInfo::setTotalSignificantNibbles(unsigned i)     { _totalSignificantNibbles = i; }
void ToolInfo::setIsFromHeuristics(bool h)                { _heuristics = h; }
void ToolInfo::setName(const std::string& n)              { _name = n; }
void ToolInfo::setType(const std::string &t)              { _type = t; }

void ToolInfo::setIsUnknown()      { setName(CC_UNKNOWN); }
void ToolInfo::setIsBorland()      { setName(CC_BORLAND); }
void ToolInfo::setIsGcc()          { setName(CC_GCC); }
void ToolInfo::setIsIntel()        { setName(CC_INTEL); }
void ToolInfo::setIsOpenWatcom()   { setName(CC_OPEN_WATCOM); }
void ToolInfo::setIsVisualStudio() { setName(CC_VISUAL_STUDIO); }

std::string ToolInfo::getType() const                     { return _type; }
std::string ToolInfo::getName() const                     { return _name; }
unsigned int ToolInfo::getMajorVersion() const            { return _majorVersion; }
unsigned int ToolInfo::getMinorVersion() const            { return _minorVersion; }
unsigned int ToolInfo::getPatchVersion() const            { return _patchVersion; }
std::string ToolInfo::getAdditionalInfo() const           { return _additionalInfo; }

/**
 * @return Probability that the tool was actually used. This does not have
 * to be set. It is significant only if the value is not 0.0.
 */
double ToolInfo::getPercentage() const
{
	return _percentage;
}

/**
 * @return Total number of significant nibbles in signature recognizing
 * this tool.
 */
unsigned ToolInfo::getTotalSignificantNibbles() const
{
	return _totalSignificantNibbles;
}

/**
 * @return Number of significant nibbles that were actually found.
 */
unsigned ToolInfo::getIdenticalSignificantNibbles() const
{
	return _identicalSignificantNibbles;
}

/**
 * @return Were heuristics used to detect usage of this tool?
 */
bool ToolInfo::isFromHeuristics() const
{
	return _heuristics;
}

/**
 * Set provided string into @c version and try to parse it into its components:
 * @c majorVersion @c minorVersionc and @c patchVersion.
 * Parsing is successful only if string has the following format:
 * "majorVersion.minorVersionc.patchVersion"
 * @param n Version string.
 */
void ToolInfo::setVersion(const std::string& n)
{
	_version = n;
	_majorVersion = _minorVersion = _patchVersion = 0;

	if (_version.empty())
	{
		return;
	}

	std::string tmpvar = _version;
	std::string tmpMaj;
	std::string temMin;
	std::string temPat;
	do
	{
		auto firstDot = tmpvar.find(".");
		if (firstDot == tmpvar.npos)
		{
			tmpMaj = tmpvar;
			break;
		}

		tmpMaj = tmpvar.substr(0, firstDot);

		if ( firstDot == (tmpvar.length()-1) )
		{
			break;
		}
		tmpvar = tmpvar.substr(tmpvar.find(".")+1);

		auto secondDot = tmpvar.find(".");
		if (secondDot == tmpvar.npos)
		{
			temMin = tmpvar;
			break;
		}
		temMin = tmpvar.substr(0, secondDot);

		if ( secondDot == (tmpvar.length()-1) )
		{
			break;
		}
		tmpvar = tmpvar.substr(tmpvar.find(".")+1);

		auto thirdDot = tmpvar.find(".");
		if (thirdDot == tmpvar.npos)
		{
			temPat = tmpvar;
			break;
		}
		temPat = tmpvar.substr(0,tmpvar.find("."));
	}
	while(false);

	// Sometimes version can not be parsed - e.g. 'delphi bobsoft mini?', 'borland delphi 6.0 - 7.0'.
	// atoi() can handle this, stoi() throw an exception.
	//
	if (!tmpMaj.empty()) _majorVersion = atoi(tmpMaj.c_str());
	if (!temMin.empty()) _minorVersion = atoi(temMin.c_str());
	if (!temPat.empty()) _patchVersion = atoi(temPat.c_str());
}

/**
 * @return Returns @c version string if it was set by @c setVersion().
 *         Otherwise constructs and returns string in the following format:
 *         "majorVersion.minorVersionc.patchVersion"
 */
std::string ToolInfo::getVersion() const
{
	std::stringstream ret;

	if (!_version.empty())
	{
		ret << _version;
	}
	else
	{
		if (getMajorVersion()) ret << getMajorVersion();
		if (getMinorVersion()) ret << "." << getMinorVersion();
		if (getPatchVersion()) ret << "." << getPatchVersion();
	}

	return ret.str();
}

/**
 * @return This instance is equal with the provided one if their names
 * and versions are equal.
 */
bool ToolInfo::operator==(const ToolInfo& val) const
{
	return getName() == val.getName() && getVersion() == val.getVersion();
}

//
//=============================================================================
// ToolInfoContainer
//=============================================================================
//

/**
 * Finds out if container contains a tool with the given name.
 * See @c ToolInfo::isTool() for more details on name comparison.
 */
bool ToolInfoContainer::isTool(const std::string& name) const
{
	return std::any_of(begin(), end(),
			[&name](const auto& t){return t.isTool(name);});
}

/**
 * Gets tool with the given name from container.
 * See @c ToolInfo::isTool() for more details on name comparison.
 * @return Pointer to the tool, or @c nullptr if tool not found.
 */
const ToolInfo* ToolInfoContainer::getToolByName(const std::string& name)
{
	for (auto& t : *this)
	{
		if (t.isTool(name))
			return &t;
	}
	return nullptr;
}

/**
 * Gets tool with the maximal percentage in the container.
 * In case of severel tools with the same percentage, gets the first one.
 * @return Pointer to the tool, or @c nullptr if tool not found.
 */
const ToolInfo* ToolInfoContainer::getToolWithMaxPercentage()
{
	const ToolInfo* ret = nullptr;
	double max = 0.0;

	for (auto& t : *this)
	{
		if (t.getPercentage() > max)
		{
			max = t.getPercentage();
			ret = &t;
		}
	}
	return ret;
}

/**
 * Gets the mosts significant (i.e. first) tool in the container.
 * @return Pointer to the tool, or @c nullptr if container empty.
 */
const ToolInfo* ToolInfoContainer::getToolMostSignificant()
{
	return (empty()) ? (nullptr) : (&front());
}

bool ToolInfoContainer::isGcc() const           { return isTool("gcc"); }
bool ToolInfoContainer::isFasm() const          { return isTool("fasm"); }
bool ToolInfoContainer::isLlvm() const          { return isTool("llvm"); }
bool ToolInfoContainer::isPic32() const         { return isTool("xc32"); }
bool ToolInfoContainer::isMingw() const         { return isTool("mingw"); }
bool ToolInfoContainer::isDelphi() const        { return isTool("delphi"); }
bool ToolInfoContainer::isWatcom() const        { return isTool("watcom"); }
bool ToolInfoContainer::isIntel() const         { return isTool("intel"); }
bool ToolInfoContainer::isPspGcc() const        { return isTool("psp"); }
bool ToolInfoContainer::isBorland() const       { return isTool("borland"); }
bool ToolInfoContainer::isThumbCompiler() const { return isTool("thumb"); }

bool ToolInfoContainer::isMsvc(const std::string& version) const
{
	return std::any_of(
			begin(),
			end(),
			[&](const auto& t){return t.isMsvc(version);});
}

} // namespace common
} // namespace retdec
