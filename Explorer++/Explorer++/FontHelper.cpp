// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FontHelper.h"
#include "../Helper/DpiCompatibility.h"
#include <boost/pfr.hpp>

bool operator==(const LOGFONT &first, const LOGFONT &second)
{
	return boost::pfr::structure_to_tuple(first) == boost::pfr::structure_to_tuple(second);
}

wil::unique_hfont CreateFontFromNameAndSize(const std::wstring &name, int size, HWND hwnd)
{
	LOGFONT logFont = {};
	logFont.lfHeight = -DpiCompatibility::GetInstance().PointsToPixels(hwnd, size);
	logFont.lfWeight = FW_NORMAL;
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	logFont.lfQuality = DEFAULT_QUALITY;
	logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	StringCchCopy(logFont.lfFaceName, std::size(logFont.lfFaceName), name.c_str());
	return wil::unique_hfont(CreateFontIndirect(&logFont));
}
