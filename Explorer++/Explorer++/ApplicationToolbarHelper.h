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

namespace ApplicationToolbarHelper
{

struct ApplicationInfo
{
	std::wstring application;
	std::wstring parameters;
};

ApplicationInfo ParseCommandString(const std::wstring &command);
void OpenApplication(CoreInterface *coreInterface, HWND errorDialogParent,
	const Application *application, std::wstring extraParameters = {});

}

}
