// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkMenu.h"
#include "Bookmarks/BookmarkIconManager.h"

BookmarkMenu::BookmarkMenu(BookmarkTree *bookmarkTree, HMODULE resourceModule,
	IExplorerplusplus *expp, Navigation *navigation, IconFetcher *iconFetcher, HWND parentWindow) :
	m_parentWindow(parentWindow),
	m_menuBuilder(expp, iconFetcher, resourceModule),
	m_bookmarkContextMenu(bookmarkTree, resourceModule, expp),
	m_controller(navigation),
	m_showingMenu(false),
	m_menuItemPositionMappings(nullptr)
{
	m_windowSubclasses.emplace_back(
		parentWindow, ParentWindowSubclassStub, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));
}

LRESULT CALLBACK BookmarkMenu::ParentWindowSubclassStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *bookmarkMenu = reinterpret_cast<BookmarkMenu *>(dwRefData);
	return bookmarkMenu->ParentWindowSubclass(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK BookmarkMenu::ParentWindowSubclass(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

	auto itr = m_menuItemPositionMappings->find({ menu, index });

	if (itr == m_menuItemPositionMappings->end())
	{
		return;
	}

	m_bookmarkContextMenu.ShowMenu(
		m_parentWindow, itr->second->GetParent(), { itr->second }, pt, true);
}

BOOL BookmarkMenu::ShowMenu(BookmarkItem *bookmarkItem, const POINT &pt,
	BookmarkMenuBuilder::IncludePredicate includePredicate)
{
	wil::unique_hmenu menu(CreatePopupMenu());

	if (!menu)
	{
		return FALSE;
	}

	std::vector<wil::unique_hbitmap> menuImages;
	BookmarkMenuBuilder::ItemIdMap menuItemIdMappings;
	BookmarkMenuBuilder::ItemPositionMap menuItemPositionMappings;
	BOOL res = m_menuBuilder.BuildMenu(m_parentWindow, menu.get(), bookmarkItem, { MIN_ID, MAX_ID },
		0, menuItemIdMappings, menuImages, &menuItemPositionMappings, includePredicate);

	if (!res)
	{
		return FALSE;
	}

	m_showingMenu = true;
	m_menuItemPositionMappings = &menuItemPositionMappings;

	int cmd = TrackPopupMenu(
		menu.get(), TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_parentWindow, nullptr);

	m_showingMenu = false;
	m_menuItemPositionMappings = nullptr;

	if (cmd != 0)
	{
		OnMenuItemSelected(cmd, menuItemIdMappings);
	}

	return TRUE;
}

void BookmarkMenu::OnMenuItemSelected(
	int menuItemId, BookmarkMenuBuilder::ItemIdMap &menuItemIdMappings)
{
	auto itr = menuItemIdMappings.find(menuItemId);

	if (itr == menuItemIdMappings.end())
	{
		return;
	}

	m_controller.OnBookmarkMenuItemSelected(itr->second);
}