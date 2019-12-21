// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkMenu.h"
#include "MainResource.h"
#include "../Helper/Macros.h"

BookmarkMenu::BookmarkMenu(BookmarkTree *bookmarkTree, HMODULE resourceModule,
	IExplorerplusplus *expp, HWND parentWindow) :
	m_parentWindow(parentWindow),
	m_instance(resourceModule),
	m_menuBuilder(resourceModule),
	m_bookmarkContextMenu(bookmarkTree, resourceModule, expp),
	m_showingMenu(false),
	m_menuItemMappings(nullptr)
{
	m_windowSubclasses.push_back(WindowSubclassWrapper(parentWindow, ParentWindowSubclassStub,
		SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));
}

LRESULT CALLBACK BookmarkMenu::ParentWindowSubclassStub(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	BookmarkMenu *bookmarkMenu = reinterpret_cast<BookmarkMenu *>(dwRefData);
	return bookmarkMenu->ParentWindowSubclass(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK BookmarkMenu::ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MENURBUTTONUP:
	{
		POINT pt;
		DWORD messagePos = GetMessagePos();
		POINTSTOPOINT(pt, MAKEPOINTS(messagePos));
		OnMenuRightButtonUp(reinterpret_cast<HMENU>(lParam), static_cast<int>(wParam), pt);
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void BookmarkMenu::OnMenuRightButtonUp(HMENU menu, int index, const POINT &pt)
{
	// Note that there are no specific checks here to make sure that the item
	// being right-clicked is an item from the bookmarks menu created by this
	// class.
	// It's not enough to check that the specified menu is the menu that was
	// created, since if the item that was right-clicked was on a submenu, then
	// such a check would fail (as the submenu is really its own separate menu).
	// Performing the check reliably would mean checking all submenus.
	// However, while the bookmarks menu is being shown, the only other menu
	// that should ever be shown is the context menu for an individual bookmark
	// item. Therefore, that's all that's checked here.
	// In the worst case, if there ever was another menu being shown at the same
	// time as the bookmarks menu, and that menu included an item with an ID
	// that matched one of the item's on the bookmarks menu, then the context
	// menu for that item would be shown. Which isn't ideal, but is well-defined
	// behavior.
	if (!m_showingMenu || (m_showingMenu && m_bookmarkContextMenu.IsShowingMenu()))
	{
		return;
	}

	int menuItemId = GetMenuItemID(menu, index);

	if (menuItemId == -1)
	{
		return;
	}

	auto itr = m_menuItemMappings->find(menuItemId);

	if (itr == m_menuItemMappings->end())
	{
		return;
	}

	m_bookmarkContextMenu.ShowMenu(m_parentWindow, itr->second, pt, true);
}

BOOL BookmarkMenu::ShowMenu(BookmarkItem *bookmarkItem, const POINT &pt, MenuCallback callback)
{
	wil::unique_hmenu menu(CreatePopupMenu());

	if (!menu)
	{
		return FALSE;
	}

	BookmarkMenuBuilder::ItemMap menuItemMappings;
	BOOL res = m_menuBuilder.BuildMenu(menu.get(), bookmarkItem, { MIN_ID, MAX_ID }, 0, menuItemMappings);

	if (!res)
	{
		return FALSE;
	}

	m_showingMenu = true;
	m_menuItemMappings = &menuItemMappings;

	int cmd = TrackPopupMenu(menu.get(), TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_parentWindow, nullptr);

	m_showingMenu = false;
	m_menuItemMappings = nullptr;

	if (cmd != 0)
	{
		OnMenuItemSelected(cmd, menuItemMappings, callback);
	}

	return TRUE;
}

void BookmarkMenu::OnMenuItemSelected(int menuItemId, BookmarkMenuBuilder::ItemMap &menuItemMappings,
	MenuCallback callback)
{
	auto itr = menuItemMappings.find(menuItemId);

	if (itr == menuItemMappings.end())
	{
		return;
	}

	if (!callback)
	{
		return;
	}

	callback(itr->second);
}