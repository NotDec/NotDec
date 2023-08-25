/**
* @file src/utils/memory.cpp
* @brief Memory utilities.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#include <cstddef>

#include "backend/retdec-llvmir2hll/retdec-utils/memory.h"
#include "backend/retdec-llvmir2hll/retdec-utils/os.h"

#ifdef OS_WINDOWS
	#include <windows.h>
#elif defined(OS_MACOS) || defined(OS_BSD)
	#include <sys/types.h>
	#include <sys/sysctl.h>
#else
	#include <sys/sysinfo.h>
#endif

#ifdef OS_POSIX
	#include <sys/resource.h>
#endif

namespace retdec {
namespace utils {

namespace {

#if defined(OS_POSIX) && !defined(OS_MACOS)

/**
* @brief Implementation of @c limitSystemMemory() on POSIX-compliant systems.
*/
bool limitSystemMemoryOnPOSIX(std::size_t limit) {
	struct rlimit rl = {
		.rlim_cur = static_cast<rlim_t>(limit),        // Soft limit.
		.rlim_max = RLIM_INFINITY // Hard limit (ceiling for rlim_cur).
	};
	auto rc = setrlimit(RLIMIT_AS, &rl);
	return rc == 0;
}

#endif

#ifdef OS_WINDOWS

/**
* @brief Implementation of @c getTotalSystemMemory() on Windows.
*/
std::size_t getTotalSystemMemoryOnWindows() {
	MEMORYSTATUSEX memoryStatus;
	memoryStatus.dwLength = sizeof(memoryStatus);
	bool succeeded = GlobalMemoryStatusEx(&memoryStatus);
	return succeeded ? memoryStatus.ullTotalPhys : 0;
}

/**
* @brief Assigns the current process into a new job and returns a handle to
*     that job.
*
* When the process cannot be assigned to a job, it returns 0.
*/
HANDLE assignProcessToNewJob() {
	auto jobHandle = CreateJobObject(nullptr, nullptr);
	if (!jobHandle) {
		return 0;
	}

	if (!AssignProcessToJobObject(jobHandle, GetCurrentProcess())) {
		return 0;
	}

	return jobHandle;
}

/**
* @brief Implementation of @c limitSystemMemory() on Windows.
*/
bool limitSystemMemoryOnWindows(std::size_t limit) {
	// On Windows 7, AssignProcessToJobObject() fails when the current process
	// has already been attached to a job. Therefore, we have to remember the
	// job and use it in all subsequent calls to limitSystemMemoryOnWindows().
	// This is not thread safe, but limitSystemMemory() is already marked as
	// not reentrant, so we are OK.
	static auto jobHandle = assignProcessToNewJob();

	JOBOBJECT_EXTENDED_LIMIT_INFORMATION extendedInfo{};
	extendedInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY;
	extendedInfo.ProcessMemoryLimit = limit;

	auto succeeded = SetInformationJobObject(
		jobHandle,
		JobObjectExtendedLimitInformation,
		&extendedInfo,
		sizeof(extendedInfo)
	);
	return succeeded;
}

#elif defined(OS_MACOS)

/**
* @brief Implementation of @c getTotalSystemMemory() on MacOS.
*/
std::size_t getTotalSystemMemoryOnMacOS() {
	int what[] = { CTL_HW, HW_MEMSIZE };
	std::size_t value = 0;
	std::size_t length = sizeof(value);
	auto rc = sysctl(what, 2, &value, &length, nullptr, 0);
	return rc != -1 ? value : 0;
}

/**
* @brief Implementation of @c limitSystemMemory() on MacOS.
*/
bool limitSystemMemoryOnMacOS(std::size_t limit) {
	// Warning: We can't limit memory on macOS. Before macOS 12,
	// limitSystemMemoryOnPOSIX() does not actually do anything on macOS (it
	// just succeeds). Since macOS 12, it returns an error and RetDec cannot
	// start. For more details, see
	// - https://github.com/avast/retdec/issues/379
	// - https://github.com/avast/retdec/issues/1045
	//
	// To be honest, Apple can control memmory limit via a so-called ledger()
	// system call which is private. An old version which was opened to
	// OpenSource (from 10.9-10.10?) used setrlimit() but at some point
	// setrlimit() was broken and not ledger(). Probably in macOS 12
	// setrlimit() was completely broken.
	//
	// Since we do not have any other way of limiting the memory on macOS, just
	// return true.
	return true;
}

#elif defined(OS_BSD)

/**
* @brief Implementation of @c getTotalSystemMemory() on *BSD.
*
* AKA FreeBSD, DragonFly, NetBSD, OpenBSD, TrueOS, PCBSD
*/
std::size_t getTotalSystemMemoryOnBSD() {
	int what[] = { CTL_HW, HW_PHYSMEM };
	std::size_t value = 0;
	std::size_t length = sizeof(value);
	auto rc = sysctl(what, 2, &value, &length, nullptr, 0);
	return rc != -1 ? value : 0;
}

/**
* @brief Implementation of @c limitSystemMemory() on *BSD.
*
* AKA FreeBSD, DragonFly, NetBSD, OpenBSD, TrueOS, PCBSD
*/
bool limitSystemMemoryOnBSD(std::size_t limit) {
	return limitSystemMemoryOnPOSIX(limit);
}

#else

/*
* @brief Implementation of @c getTotalSystemMemory() on Linux.
*/
std::size_t getTotalSystemMemoryOnLinux() {
	struct sysinfo system_info;
	auto rc = sysinfo(&system_info);
	return rc == 0 ? system_info.totalram : 0;
}

/**
* @brief Implementation of @c limitSystemMemory() on Linux.
*/
bool limitSystemMemoryOnLinux(std::size_t limit) {
	return limitSystemMemoryOnPOSIX(limit);
}

#endif

} // anonymous namespace

/**
* @brief Returns the total size of system RAM (in bytes).
*
* When the size cannot be obtained, it returns @c 0.
*/
std::size_t getTotalSystemMemory() {
#ifdef OS_WINDOWS
	return getTotalSystemMemoryOnWindows();
#elif defined(OS_MACOS)
	return getTotalSystemMemoryOnMacOS();
#elif defined(OS_BSD)
	return getTotalSystemMemoryOnBSD();
#else
	return getTotalSystemMemoryOnLinux();
#endif
}

/**
* @brief Limits system memory to the given size (in bytes).
*
* @return @c true if the limiting succeeded, @c false otherwise.
*
* When @a limit is @c 0, it immediately returns @c false.
*
* This function is not reentrant, i.e. it is not safe to call it simultaneously
* from multiple threads.
*/
bool limitSystemMemory(std::size_t limit) {
	if (limit == 0) {
		return false;
	}

#ifdef OS_WINDOWS
	return limitSystemMemoryOnWindows(limit);
#elif defined(OS_MACOS)
	return limitSystemMemoryOnMacOS(limit);
#elif defined(OS_BSD)
	return limitSystemMemoryOnBSD(limit);
#else
	return limitSystemMemoryOnLinux(limit);
#endif
}

/**
* @brief Limits system memory to half of the total memory.
*/
bool limitSystemMemoryToHalfOfTotalSystemMemory() {
	auto totalSize = getTotalSystemMemory();
	if (totalSize == 0) {
		return false;
	}

	return limitSystemMemory(totalSize / 2);
}

} // namespace utils
} // namespace retdec
