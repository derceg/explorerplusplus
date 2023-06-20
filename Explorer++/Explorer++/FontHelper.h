// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>

wil::unique_hfont CreateFontFromNameAndSize(const std::wstring &name, int size, HWND hwnd);
