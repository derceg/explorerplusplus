// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <optional>

class DarkModeColorProvider;

// Draws an entire tab control (the tabs, plus background) in dark mode.
class DarkModeTabControlPainter
{
public:
	DarkModeTabControlPainter(HWND hwnd, const DarkModeColorProvider *darkModeColorProvider);

	void SetHotItem(int hotItem);
	void ClearHotItem();
	void Paint(HDC hdc, const RECT &paintRect);

private:
	void DrawTab(int index, HDC hdc);
	RECT GetTabRect(int index);

	const HWND m_hwnd;
	const DarkModeColorProvider *const m_darkModeColorProvider;
	std::optional<int> m_hotItem;
};
