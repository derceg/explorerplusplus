// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// Disables redraw on construction re-enables it on destruction.
class ScopedRedrawDisabler
{
public:
	ScopedRedrawDisabler(HWND hwnd);
	~ScopedRedrawDisabler();

private:
	const HWND m_hwnd;
};
