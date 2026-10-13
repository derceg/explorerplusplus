// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <oleidl.h>

enum class MenuDropPosition;
class MenuItemKey;
class MenuView;

class MenuViewTestApi
{
public:
	static HMENU GetNativeMenu(const MenuView *menuView);

	static UINT GetItemId(const MenuView *menuView, int index);
	static std::optional<UINT> MaybeGetIdForKey(const MenuView *menuView, const MenuItemKey &key);
	static std::wstring GetItemFullText(const MenuView *menuView, UINT id);
	static std::wstring GetItemHelpText(const MenuView *menuView, UINT id);
	static HBITMAP GetItemBitmap(const MenuView *menuView, UINT id);
	static bool IsItemEnabled(const MenuView *menuView, const MenuItemKey &key);
	static bool IsItemEnabled(const MenuView *menuView, UINT id);
	static bool IsItemChecked(const MenuView *menuView, const MenuItemKey &key);
	static bool IsItemChecked(const MenuView *menuView, UINT id);
	static const MenuView *GetSubMenuView(const MenuView *menuView, const MenuItemKey &key);
	static const MenuView *GetSubMenuView(const MenuView *menuView, UINT id);
	static wil::com_ptr_nothrow<IDropTarget> MaybeGetDropTarget(MenuView *menuView, UINT id,
		MenuDropPosition position);

	static void OnPopupWillShowForDpi(MenuView *menuView, UINT dpi);
	static void OnPopupClosed(MenuView *menuView);
};
