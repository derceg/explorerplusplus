// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

class MenuItemKey;
class RootMenuView;

class RootMenuViewTestApi
{
public:
	static std::wstring ResolveItemHelpText(const RootMenuView *menuView, UINT id);

	// This will highlight the item, resulting in the help text for that item being assigned to the
	// MenuHelpTextHost instance.
	static void HighlightItemAtIndex(RootMenuView *menuView, HMENU parentMenu, int index);

	static void SelectItem(RootMenuView *menuView, const MenuItemKey &key, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	static void SelectItem(RootMenuView *menuView, UINT id, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	static void SelectItemAtIndex(RootMenuView *menuView, int index, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	static void MiddleClickItem(RootMenuView *menuView, const MenuItemKey &key, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	static void MiddleClickItem(RootMenuView *menuView, UINT id, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	static void MiddleClickItemAtIndex(RootMenuView *menuView, int index, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	static void RightClickItem(RootMenuView *menuView, const MenuItemKey &key,
		const POINT &ptScreen);
	static void RightClickItem(RootMenuView *menuView, UINT id, const POINT &ptScreen);
	static void RightClickItemAtIndex(RootMenuView *menuView, int index, const POINT &ptScreen);

	static void NotifyMenuOpened(RootMenuView *menuView);
	static void NotifyMenuClosed(RootMenuView *menuView);
};
