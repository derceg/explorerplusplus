// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "HeaderHelper.h"
#include <wil/common.h>

void HeaderHelper::AddOrRemoveFormatOptions(HWND header, int index, int formatOptions, bool add)
{
	HDITEM item;
	item.mask = HDI_FORMAT;
	Header_GetItem(header, index, &item);

	if (add)
	{
		WI_SetAllFlags(item.fmt, formatOptions);
	}
	else
	{
		WI_ClearAllFlags(item.fmt, formatOptions);
	}

	Header_SetItem(header, index, &item);
}