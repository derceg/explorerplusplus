// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PopupMenuView.h"
#include "../Helper/PidlHelper.h"
#include <wil/resource.h>

class IconFetcher;
class Navigator;

// Displays a set of shell items in a menu, with the name and icon of each item being displayed.
// An item can be both clicked (to open it in the current tab) and middle-clicked (to open it in a
// new tab), with the ctrl and shift keys used to control exactly how an item is opened.
class ShellItemsMenu : public MenuController
{
public:
	ShellItemsMenu(const std::vector<PidlAbsolute> &pidls, Navigator *navigator,
		IconFetcher *iconFetcher);

	void Show(HWND hwnd, const POINT &point);

private:
	std::shared_ptr<PopupMenuView> BuildMenu(const std::vector<PidlAbsolute> &pidls);

	void AddMenuItemForPidl(std::shared_ptr<PopupMenuView> menuView, PCIDLIST_ABSOLUTE pidl);
	wil::unique_hbitmap GetShellItemIcon(PCIDLIST_ABSOLUTE pidl);
	static void OnIconRetrieved(std::weak_ptr<PopupMenuView> weakMenuView, int menuItemId,
		IImageList *systemImageList, int iconIndex);

	// MenuController
	void OnMenuItemSelected(int menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown) override;
	void OnMenuItemMiddleClicked(int menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown) override;

	void OpenSelectedItem(int menuItemId, bool isMiddleButtonDown, bool isCtrlKeyDown,
		bool isShiftKeyDown);

	// The PopupMenuView instance is uniquely owned by this class. The only reason this uses a
	// shared_ptr is to allow weak references to the instance to be created.
	std::shared_ptr<PopupMenuView> m_menuView;

	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	Navigator *m_navigator = nullptr;
	IconFetcher *m_iconFetcher = nullptr;
	int m_idCounter = 1;
	std::unordered_map<UINT, PidlAbsolute> m_idPidlMap;
};
