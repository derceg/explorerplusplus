// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>

// This will be used to detect whether or not the display window font (Config::displayWindowFont)
// selected by the user is different from the current font.
bool operator==(const LOGFONT &first, const LOGFONT &second);

wil::unique_hfont CreateFontFromNameAndSize(const std::wstring &name, int size, HWND hwnd);
