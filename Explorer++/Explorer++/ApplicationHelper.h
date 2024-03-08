// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>
#include <string>

class CoreInterface;

namespace Applications
{

class Application;

namespace ApplicationHelper
{

struct ApplicationInfo
{
	std::wstring application;
	std::wstring parameters;
};

ApplicationInfo ParseCommandString(const std::wstring &command);

// When provided with a file name, returns the name with the extension stripped off. If the file
// name only contains an extension (e.g. ".exe"), the original name will be returned.
std::wstring RemoveExtensionFromFileName(const std::wstring &name);

}

}
