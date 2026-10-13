// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RootMenuViewTestApi.h"
#include "MenuViewTestApi.h"
#include "RootMenuView.h"

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
