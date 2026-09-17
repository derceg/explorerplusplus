// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <wil/com.h>
#include <memory>
#include <optional>
#include <string>

class MenuDelegate;
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
	void RightClickItem(UINT id, const POINT &ptScreen);

	void NotifyMenuWillShowForTesting(UINT dpi);
	void NotifyMenuClosedForTesting();

private:
	void NotifyMenuWillShowForDpi(UINT dpi);

	LRESULT OwnerWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnInitMenuPopup(HWND hwnd, HMENU menu);
	void OnUninitMenuPopup(HMENU menu);
	void OnMenuClosed();
	void OnMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown);
	void OnMenuRightButtonUp(HMENU menu, int index);
	LRESULT OnMenuDrag(HMENU menu, int index);
	LRESULT OnMenuGetObject(MENUGETOBJECTINFO *objectInfo);

	std::optional<std::wstring> OnHelpTextRequested(HMENU menu, int id);

	MenuDelegate *MaybeGetDelegateForActionableItem(UINT id);

	MenuView *const m_rootView;
	MenuHelpTextHost *const m_menuHelpTextHost;
	std::unique_ptr<WindowSubclass> m_subclass;
	boost::signals2::scoped_connection m_helpTextConnection;
	wil::com_ptr_nothrow<IDropTarget> m_dropTarget;
};
