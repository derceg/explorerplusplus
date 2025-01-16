// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LayoutDefaults.h"
#include "../Helper/WindowHelper.h"

namespace LayoutDefaults
{

RECT GetDefaultMainWindowBounds()
{
	RECT workArea;
	BOOL res = SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
	CHECK(res);

	// The strategy here is fairly simple - the window will be sized to a portion of the work area
	// of the primary monitor and centered.
	auto width = static_cast<int>(GetRectWidth(&workArea) * 0.60);
	auto height = static_cast<int>(GetRectHeight(&workArea) * 0.60);
	int x = (GetRectWidth(&workArea) - width) / 2;
	int y = (GetRectHeight(&workArea) - height) / 2;

	return { x, y, x + width, y + height };
}

}
