// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <memory>
#include <optional>
#include <string>

class MenuHelpTextHost;
class MenuView;
class WindowSubclass;

class MenuController
{
public:
	MenuController(MenuView *rootView, MenuHelpTextHost *menuHelpTextHost);
	~MenuController();

	void NotifyMenuWillShow(HWND ownerWindow);

	void SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);
	void MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);

	void NotifyMenuWillShowForTesting(UINT dpi);
	void NotifyMenuClosedForTesting();

private:
	void NotifyMenuWillShowForDpi(UINT dpi);

	LRESULT OwnerWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnInitMenuPopup(HWND hwnd, HMENU menu);
	void OnUninitMenuPopup(HMENU menu);
	void OnMenuClosed();
	void OnMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown);

	std::optional<std::wstring> OnHelpTextRequested(HMENU menu, int id);

	MenuView *const m_rootView;
	MenuHelpTextHost *const m_menuHelpTextHost;
	std::unique_ptr<WindowSubclass> m_subclass;
	boost::signals2::scoped_connection m_helpTextConnection;
};
