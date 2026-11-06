// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LayoutHelper.h"
#include "WindowHelper.h"

HorizontalLayout::HorizontalLayout(const RECT &bounds) : m_remaining(bounds)
{
}

RECT HorizontalLayout::TakeLeft(int width)
{
	RECT result = m_remaining;
	result.right = result.left + width;

	m_remaining.left += width;

	return result;
}

RECT HorizontalLayout::TakeRight(int width)
{
	RECT result = m_remaining;
	result.left = result.right - width;

	m_remaining.right -= width;

	return result;
}

RECT HorizontalLayout::Remaining() const
{
	return m_remaining;
}

RECT CenterVertically(const RECT &rect, int height)
{
	RECT result = rect;
	result.top += (GetRectHeight(&result) - height) / 2;
	result.bottom = result.top + height;
	return result;
}
