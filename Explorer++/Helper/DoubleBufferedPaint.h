// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>

// This class is designed to be used within WM_PAINT and should be instantiated where the call to
// BeginPaint() is usually made. It allows clients to easily draw onto a memory DC, as opposed to
// drawing directly to the screen. The contents of the memory DC will be copied to the screen on
// destruction.
class DoubleBufferedPaint
{
public:
	DoubleBufferedPaint(HWND hwnd);
	~DoubleBufferedPaint();

	HDC GetMemoryDC() const;
	RECT GetPaintRect() const;

private:
	wil::unique_hdc_paint m_hdc;
	PAINTSTRUCT m_paintDetails;
	RECT m_clientRect;
	wil::unique_hdc m_memDC;
	wil::unique_hbitmap m_compatibleBitmap;
	wil::unique_select_object m_selectCompatibleBitmap;
};
