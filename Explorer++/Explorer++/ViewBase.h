// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>
#include <vector>

class WindowSubclass;

class ViewBase
{
public:
	ViewBase(HWND hwnd);
	virtual ~ViewBase();

	HWND GetHWND() const;

protected:
	void AddSubclass(std::unique_ptr<WindowSubclass> subclass);

private:
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnNcDestroy();

	HWND m_hwnd = nullptr;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
};
