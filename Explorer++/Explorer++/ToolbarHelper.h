// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <string>
#include <tuple>

class IconResourceLoader;

namespace ToolbarHelper
{

static constexpr int CLOSE_TOOLBAR_X_OFFSET = 4;

std::tuple<HWND, wil::unique_himagelist> CreateCloseButtonToolbar(HWND parent, int closeButtonId,
	const std::wstring &tooltip, IconResourceLoader *iconResourceLoader);

}
