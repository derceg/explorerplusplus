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
