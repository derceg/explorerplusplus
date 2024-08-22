// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "VersionHelper.h"
#include "Version.h"
#include "VersionConstants.h"

namespace VersionHelper
{

const Version &GetVersion()
{
	static Version version{ MAJOR_VERSION, MINOR_VERSION, MICRO_VERSION, BUILD_VERSION };
	return version;
}

std::wstring GetBuildDate()
{
	return BUILD_DATE_STRING;
}

Channel GetChannel()
{
#if defined(ENVIRONMENT_RELEASE_STABLE)
	return Channel::Stable;
#elif defined(ENVIRONMENT_RELEASE_BETA)
	return Channel::Beta;
#else
	return Channel::Dev;
#endif
}

Platform GetPlatform()
{
#if defined(BUILD_WIN32)
	return Platform::x86;
#elif defined(BUILD_WIN64)
	return Platform::x64;
#elif defined(BUILD_ARM64)
	return Platform::Arm64;
#else
	#error "Unknown target platform"
#endif
}

}
