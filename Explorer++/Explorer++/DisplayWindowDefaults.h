// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>

namespace DisplayWindowDefaults
{

inline constexpr COLORREF CENTRE_COLOR = RGB(255, 255, 255);
inline constexpr COLORREF SURROUND_COLOR = RGB(0, 94, 138);
inline constexpr COLORREF TEXT_COLOR = RGB(0, 0, 0);
inline constexpr LOGFONT FONT = { -13, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
	OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_MODERN, L"Segoe UI" };

}
