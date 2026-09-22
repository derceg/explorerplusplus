// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IconSize.h"
#include "DpiCompatibility.h"

int GetIconPixelSizeAtDefaultDpi(IconSize size)
{
	int pixelSize = DpiCompatibility::GetInstance().GetSystemMetricsForDpi(
		size == IconSize::Small ? SM_CXSMICON : SM_CXICON, USER_DEFAULT_SCREEN_DPI);
	DCHECK(pixelSize != 0);
	return pixelSize;
}
