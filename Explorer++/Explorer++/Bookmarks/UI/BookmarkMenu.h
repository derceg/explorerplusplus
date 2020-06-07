// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/UI/BookmarkContextMenu.h"
#include "Bookmarks/UI/BookmarkMenuBuilder.h"
#include "Bookmarks/UI/BookmarkMenuController.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <functional>

class BookmarkTree;
class IconFetcher;
__interface IExplorerplusplus;

// Although it's not necessary, this class is effectively designed to be held
// for the lifetime of its parent class. Doing so is more efficient, as the
// parent window will only be subclassed once (on construction). It's then safe
// to call ShowMenu() as many times as needed.
class BookmarkMenu
{
public:
	BookmarkMenu(BookmarkTree *bookmarkTree, HMODULE resourceModule, IExplorerplusplus *expp,
		Navigation *navigation, IconFetcher *iconFetcher, HWND parentWindow);

	BOOL ShowMenu(BookmarkItem *bookmarkItem, const POINT &pt,
		BookmarkMenuBuilder::IncludePredicate includePredicate = nullptr);

private:
	static const int MIN_ID = 1;
	static const int MAX_ID = 1000;

	static const UINT_PTR SUBCLASS_ID = 0;

	static LRESULT CALLBACK ParentWindowSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam,
		LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnMenuRightButtonUp(HMENU menu, int index, const POINT &pt);
	void OnMenuItemSelected(int menuItemId, BookmarkMenuBuilder::ItemIdMap &menuItemIdMappings);

	HWND m_parentWindow;
	BookmarkMenuBuilder m_menuBuilder;
	BookmarkContextMenu m_bookmarkContextMenu;
	BookmarkMenuController m_controller;

	bool m_showingMenu;
	BookmarkMenuBuilder::ItemPositionMap *m_menuItemPositionMappings;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
};