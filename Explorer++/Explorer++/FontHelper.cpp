// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FontHelper.h"
#include "../Helper/DpiCompatibility.h"

bool operator==(const LOGFONT &first, const LOGFONT &second)
{
	return first.lfHeight == second.lfHeight && first.lfWidth == second.lfWidth
		&& first.lfEscapement == second.lfEscapement && first.lfOrientation == second.lfOrientation
		&& first.lfWeight == second.lfWeight && first.lfItalic == second.lfItalic
		&& first.lfUnderline == second.lfUnderline && first.lfStrikeOut == second.lfStrikeOut
		&& first.lfCharSet == second.lfCharSet && first.lfOutPrecision == second.lfOutPrecision
		&& first.lfClipPrecision == second.lfClipPrecision && first.lfQuality == second.lfQuality
		&& first.lfPitchAndFamily == second.lfPitchAndFamily
		&& lstrcmp(first.lfFaceName, second.lfFaceName) == 0;
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
