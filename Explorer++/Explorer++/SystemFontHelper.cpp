// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SystemFontHelper.h"
#include "../Helper/DpiCompatibility.h"

// Returns a system font, scaled to the DPI associated with the provided window.
LOGFONT GetSystemFont(SystemFont systemFont, HWND hwnd)
{
	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(hwnd);

	NONCLIENTMETRICS nonClientMetrics = {};
	nonClientMetrics.cbSize = sizeof(nonClientMetrics);
	[[maybe_unused]] auto res = dpiCompat.SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS,
		sizeof(nonClientMetrics), &nonClientMetrics, 0, dpi);
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

LOGFONT GetDefaultSystemFont(HWND hwnd)
{
	return GetSystemFont(SystemFont::Message, hwnd);
}
