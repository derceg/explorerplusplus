// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <gdiplus.h>

class ScopedBitmapReaderLock
{
public:
	ScopedBitmapReaderLock(Gdiplus::Bitmap *bitmap, const Gdiplus::Rect *rect,
		Gdiplus::PixelFormat format);
	~ScopedBitmapReaderLock();

	const Gdiplus::BitmapData *GetBitmapData() const;

private:
	Gdiplus::Bitmap *const m_bitmap;
	Gdiplus::BitmapData m_bitmapData;
	bool m_bitmapLocked = false;
};
