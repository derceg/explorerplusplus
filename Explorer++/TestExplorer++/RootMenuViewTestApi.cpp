// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RootMenuViewTestApi.h"
#include "MenuViewTestApi.h"
#include "RootMenuView.h"

std::wstring RootMenuViewTestApi::ResolveItemHelpText(const RootMenuView *menuView, UINT id)
{
	return menuView->ResolveItemHelpText(id);
}

void RootMenuViewTestApi::HighlightItemAtIndex(RootMenuView *menuView, HMENU parentMenu, int index)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
	auto res = GetMenuItemInfo(parentMenu, index, true, &menuItemInfo);
	CHECK(res);

	WORD indexOrId = static_cast<WORD>(index);
	UINT flags = MF_HILITE;

	if (WI_IsFlagSet(menuItemInfo.fType, MFT_SEPARATOR))
	{
		WI_SetFlag(flags, MF_SEPARATOR);
	}
	else if (menuItemInfo.hSubMenu)
	{
		WI_SetFlag(flags, MF_POPUP);
	}
	else
	{
		indexOrId = static_cast<WORD>(menuItemInfo.wID);
	}

	menuItemInfo.fMask = MIIM_STATE;
	WI_SetFlag(menuItemInfo.fState, MFS_HILITE);
	res = SetMenuItemInfo(parentMenu, index, true, &menuItemInfo);
	CHECK(res);

	menuView->OnMenuItemHighlighted(parentMenu, indexOrId, flags);
}

void RootMenuViewTestApi::SelectItem(RootMenuView *menuView, const MenuItemKey &key,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	auto id = MenuViewTestApi::MaybeGetIdForKey(menuView, key);
	CHECK(id);
	menuView->SelectItem(*id, isCtrlKeyDown, isShiftKeyDown);
}

void RootMenuViewTestApi::SelectItem(RootMenuView *menuView, UINT id, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	menuView->SelectItem(id, isCtrlKeyDown, isShiftKeyDown);
}

void RootMenuViewTestApi::SelectItemAtIndex(RootMenuView *menuView, int index, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	menuView->SelectItem(MenuViewTestApi::GetItemId(menuView, index), isCtrlKeyDown,
		isShiftKeyDown);
}

void RootMenuViewTestApi::MiddleClickItem(RootMenuView *menuView, const MenuItemKey &key,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	auto id = MenuViewTestApi::MaybeGetIdForKey(menuView, key);
	CHECK(id);
	menuView->MiddleClickItem(*id, isCtrlKeyDown, isShiftKeyDown);
}

void RootMenuViewTestApi::MiddleClickItem(RootMenuView *menuView, UINT id, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	menuView->MiddleClickItem(id, isCtrlKeyDown, isShiftKeyDown);
}

void RootMenuViewTestApi::MiddleClickItemAtIndex(RootMenuView *menuView, int index,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	menuView->MiddleClickItem(MenuViewTestApi::GetItemId(menuView, index), isCtrlKeyDown,
		isShiftKeyDown);
}

void RootMenuViewTestApi::RightClickItem(RootMenuView *menuView, const MenuItemKey &key,
	const POINT &ptScreen)
{
	auto id = MenuViewTestApi::MaybeGetIdForKey(menuView, key);
	CHECK(id);
	menuView->RightClickItem(*id, ptScreen);
}

void RootMenuViewTestApi::RightClickItem(RootMenuView *menuView, UINT id, const POINT &ptScreen)
{
	menuView->RightClickItem(id, ptScreen);
}

void RootMenuViewTestApi::RightClickItemAtIndex(RootMenuView *menuView, int index,
	const POINT &ptScreen)
{
	menuView->RightClickItem(MenuViewTestApi::GetItemId(menuView, index), ptScreen);
}

void RootMenuViewTestApi::NotifyMenuOpened(RootMenuView *menuView)
{
	menuView->OnMenuOpened();
}

void RootMenuViewTestApi::NotifyMenuClosed(RootMenuView *menuView)
{
	menuView->OnMenuClosed();
}
