// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SystemFontHelper.h"
#include "../Helper/DpiCompatibility.h"

LOGFONT GetSystemFontForDpi(SystemFont systemFont, UINT dpi);
LOGFONT GetDefaultSystemFontForDpi(UINT dpi);

// Returns a system font, scaled to the DPI associated with the provided window.
LOGFONT GetSystemFontScaledToWindow(SystemFont systemFont, HWND hwnd)
{
	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(hwnd);
	return GetSystemFontForDpi(systemFont, dpi);
}

LOGFONT GetSystemFontForDefaultDpi(SystemFont systemFont)
{
	return GetSystemFontForDpi(systemFont, USER_DEFAULT_SCREEN_DPI);
}

LOGFONT GetSystemFontForDpi(SystemFont systemFont, UINT dpi)
{
	NONCLIENTMETRICS nonClientMetrics = {};
	nonClientMetrics.cbSize = sizeof(nonClientMetrics);
	[[maybe_unused]] auto res = DpiCompatibility::GetInstance().SystemParametersInfoForDpi(
		SPI_GETNONCLIENTMETRICS, sizeof(nonClientMetrics), &nonClientMetrics, 0, dpi);
	assert(res);

	switch (systemFont)
	{
	case SystemFont::Caption:
		return nonClientMetrics.lfCaptionFont;

	case SystemFont::SmallCaption:
		return nonClientMetrics.lfSmCaptionFont;

	case SystemFont::Menu:
		return nonClientMetrics.lfMenuFont;

	case SystemFont::Status:
		return nonClientMetrics.lfStatusFont;

	case SystemFont::Message:
		return nonClientMetrics.lfMessageFont;

	default:
		throw std::runtime_error("Invalid SystemFont value");
	}
}

LOGFONT GetDefaultSystemFontScaledToWindow(HWND hwnd)
{
	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(hwnd);
	return GetDefaultSystemFontForDpi(dpi);
}

LOGFONT GetDefaultSystemFontForDefaultDpi()
{
	return GetDefaultSystemFontForDpi(USER_DEFAULT_SCREEN_DPI);
}

LOGFONT GetDefaultSystemFontForDpi(UINT dpi)
{
	return GetSystemFontForDpi(SystemFont::Message, dpi);
}
