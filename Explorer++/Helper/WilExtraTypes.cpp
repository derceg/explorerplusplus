// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WilExtraTypes.h"

void ReleaseFormatEtc(FORMATETC *formatEtc)
{
	if (formatEtc->ptd)
	{
		CoTaskMemFree(formatEtc->ptd);
	}
}

void InitializeStockIconInfo(SHSTOCKICONINFO *info)
{
	*info = {};
	info->cbSize = sizeof(*info);
}

void DestroyStockIconInfo(SHSTOCKICONINFO *info)
{
	if (info->hIcon)
	{
		auto res = DestroyIcon(info->hIcon);
		DCHECK(res);
	}
}
