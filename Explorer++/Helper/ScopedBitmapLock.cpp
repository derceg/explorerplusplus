// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ScopedBitmapLock.h"

ScopedBitmapLock::ScopedBitmapLock(Gdiplus::Bitmap *bitmap, const Gdiplus::Rect *rect,
	LockMode lockMode, Gdiplus::PixelFormat format) :
	m_bitmap(bitmap)
{
	Gdiplus::ImageLockMode imageLockMode =
		(lockMode == LockMode::Read) ? Gdiplus::ImageLockModeRead : Gdiplus::ImageLockModeWrite;
	auto status = bitmap->LockBits(rect, imageLockMode, format, &m_bitmapData);

	if (status == Gdiplus::Ok)
	{
		m_bitmapLocked = true;
	}
}

ScopedBitmapLock::~ScopedBitmapLock()
{
	if (m_bitmapLocked)
	{
		m_bitmap->UnlockBits(&m_bitmapData);
	}
}

const Gdiplus::BitmapData *ScopedBitmapLock::GetBitmapData() const
{
	if (m_bitmapLocked)
	{
		return &m_bitmapData;
	}

	return nullptr;
}
