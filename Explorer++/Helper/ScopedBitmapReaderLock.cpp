// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ScopedBitmapReaderLock.h"

ScopedBitmapReaderLock::ScopedBitmapReaderLock(Gdiplus::Bitmap *bitmap, const Gdiplus::Rect *rect,
	Gdiplus::PixelFormat format) :
	m_bitmap(bitmap)
{
	auto status = bitmap->LockBits(rect, Gdiplus::ImageLockModeRead, format, &m_bitmapData);

	if (status == Gdiplus::Ok)
	{
		m_bitmapLocked = true;
	}
}

ScopedBitmapReaderLock::~ScopedBitmapReaderLock()
{
	if (m_bitmapLocked)
	{
		m_bitmap->UnlockBits(&m_bitmapData);
	}
}

const Gdiplus::BitmapData *ScopedBitmapReaderLock::GetBitmapData() const
{
	if (m_bitmapLocked)
	{
		return &m_bitmapData;
	}

	return nullptr;
}
