// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

class ResourceLoader;

// These values are used when loading and saving data and shouldn't be changed.
enum class Theme
{
	Light = 0,
	Dark = 1,
	System = 2
};

std::wstring GetThemeText(Theme theme, const ResourceLoader *resourceLoader);
