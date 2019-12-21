// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkContextMenu.h"
#include "BookmarkItem.h"
#include "BookmarkMenuBuilder.h"
#include "BookmarkTree.h"
#include "CoreInterface.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <wil/resource.h>
#include <functional>

// Although it's not necessary, this class is effectively designed to be held
// for the lifetime of its parent class. Doing so is more efficient, as the
// parent window will only be subclassed once (on construction). It's then safe
// to call ShowMenu() as many times as needed.
class BookmarkMenu
{
public:

	using MenuCallback = std::function<void(const BookmarkItem *bookmarkItem)>;

	BookmarkMenu(BookmarkTree *bookmarkTree, HMODULE resourceModule, IExplorerplusplus *expp, HWND parentWindow);

	BOOL ShowMenu(BookmarkItem *bookmarkItem, const POINT &pt, MenuCallback callback = nullptr);

private:

	static const int MIN_ID = 1;
	static const int MAX_ID = 1000;

	static const UINT_PTR SUBCLASS_ID = 0;

	static LRESULT CALLBACK ParentWindowSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnMenuRightButtonUp(HMENU menu, int index, const POINT &pt);
	void OnMenuItemSelected(int menuItemId, BookmarkMenuBuilder::ItemMap &menuItemMappings,
		MenuCallback callback);

	HWND m_parentWindow;
	HINSTANCE m_instance;
	BookmarkMenuBuilder m_menuBuilder;
	BookmarkContextMenu m_bookmarkContextMenu;

	bool m_showingMenu;
	BookmarkMenuBuilder::ItemMap *m_menuItemMappings;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
};