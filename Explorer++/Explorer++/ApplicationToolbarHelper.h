// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

namespace ApplicationToolbarHelper
{
struct ApplicationInfo
{
	std::wstring application;
	std::wstring parameters;
};

ApplicationInfo ParseCommandString(const std::wstring &command);
}