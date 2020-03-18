// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/DpiCompatibility.h"

class HolderWindow
{
public:

	HolderWindow(HWND hHolder);

	LRESULT CALLBACK	HolderWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

private:

	void	OnHolderWindowPaint(HWND hwnd);
	void	OnHolderWindowLButtonDown(LPARAM lParam);
	void	OnHolderWindowLButtonUp();
	int		OnHolderWindowMouseMove(LPARAM lParam);


	HWND	m_hHolder;
	BOOL	m_bHolderResizing;

	DpiCompatibility m_dpiCompat;
};