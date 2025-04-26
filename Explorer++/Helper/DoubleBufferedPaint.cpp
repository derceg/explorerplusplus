// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DoubleBufferedPaint.h"
#include "WindowHelper.h"

DoubleBufferedPaint::DoubleBufferedPaint(HWND hwnd)
{
	m_hdc = wil::BeginPaint(hwnd, &m_paintDetails);
	CHECK(m_hdc);

	auto res = GetClientRect(hwnd, &m_clientRect);
	CHECK(res);

	m_memDC.reset(CreateCompatibleDC(m_hdc.get()));
	CHECK(m_memDC);

	m_compatibleBitmap.reset(
		CreateCompatibleBitmap(m_hdc.get(), m_clientRect.right, m_clientRect.bottom));
	CHECK(m_compatibleBitmap);

	m_selectCompatibleBitmap = wil::SelectObject(m_memDC.get(), m_compatibleBitmap.get());
}

DoubleBufferedPaint::~DoubleBufferedPaint()
{
	auto res = BitBlt(m_hdc.get(), 0, 0, m_clientRect.right, m_clientRect.bottom, m_memDC.get(), 0,
		0, SRCCOPY);
	DCHECK(res);
}

HDC DoubleBufferedPaint::GetMemoryDC() const
{
	return m_memDC.get();
}

RECT DoubleBufferedPaint::GetPaintRect() const
{
	return m_paintDetails.rcPaint;
}
