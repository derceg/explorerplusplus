// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <gdiplus.h>

class ScopedBitmapLock
{
public:
	enum class LockMode
	{
		Read,
		Write
	};

	ScopedBitmapLock(Gdiplus::Bitmap *bitmap, const Gdiplus::Rect *rect, LockMode lockMode,
		Gdiplus::PixelFormat format);
	~ScopedBitmapLock();

	const Gdiplus::BitmapData *GetBitmapData() const;

private:
	Gdiplus::Bitmap *const m_bitmap;
	Gdiplus::BitmapData m_bitmapData;
	bool m_bitmapLocked = false;
};
