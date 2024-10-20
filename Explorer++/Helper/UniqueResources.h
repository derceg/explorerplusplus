// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <gdiplus.h>

using unique_gdiplus_shutdown = wil::unique_any<ULONG_PTR, decltype(&Gdiplus::GdiplusShutdown),
	Gdiplus::GdiplusShutdown, wil::details::pointer_access_none>;

[[nodiscard]] unique_gdiplus_shutdown CheckedGdiplusStartup();
