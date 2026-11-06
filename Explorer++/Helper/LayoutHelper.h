// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class HorizontalLayout
{
public:
	HorizontalLayout(const RECT &bounds);

	RECT TakeLeft(int width);
	RECT TakeRight(int width);
	RECT Remaining() const;

private:
	RECT m_remaining;
};

RECT CenterVertically(const RECT &rect, int height);
