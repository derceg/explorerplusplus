// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkMenu.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/UI/BookmarkMenuDropTarget.h"
#include "../BookmarkDataExchange.h"
#include "../Helper/DropSourceImpl.h"
#include "../Helper/MenuHelper.h"

BookmarkMenu::BookmarkMenu(BookmarkTree *bookmarkTree, HMODULE resourceModule,
	CoreInterface *coreInterface, Navigation *navigation, IconFetcher *iconFetcher,
	HWND parentWindow) :
	m_bookmarkTree(bookmarkTree),
	m_parentWindow(parentWindow),
	m_menuBuilder(coreInterface, iconFetcher, resourceModule),
	m_bookmarkContextMenu(bookmarkTree, resourceModule, coreInterface),
	m_controller(navigation)
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(parentWindow,
		ParentWindowSubclassStub, reinterpret_cast<DWORD_PTR>(this)));
}

LRESULT CALLBACK BookmarkMenu::ParentWindowSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *bookmarkMenu = reinterpret_cast<BookmarkMenu *>(dwRefData);
	return bookmarkMenu->ParentWindowSubclass(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK BookmarkMenu::ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam)
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

	case WM_MENUDRAG:
		return OnMenuDrag(reinterpret_cast<HMENU>(lParam), static_cast<int>(wParam));

	case WM_MENUGETOBJECT:
		return OnMenuGetObject(reinterpret_cast<MENUGETOBJECTINFO *>(lParam));
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT BookmarkMenu::OnMenuDrag(HMENU menu, int itemPosition)
{
	// It appears that dragging at the very top of the menu (above the first item) or at the very
	// bottom of the menu (below the last item) will trigger a WM_MENUDRAG message, with a position
	// of -1. It doesn't make sense to begin a drag here in that case, though, since no actual item
	// is being dragged.
	if (!m_showingMenu || !m_menuInfo->menus.contains(menu) || itemPosition == -1)
	{
		return MND_CONTINUE;
	}

	auto itr = m_menuInfo->itemPositionMap.find({ menu, itemPosition });

	if (itr == m_menuInfo->itemPositionMap.end())
	{
		// All menu items should appear in the set of position mappings, so this branch should never
		// be taken.
		assert(false);
		return MND_CONTINUE;
	}

	// It's not possible to drag the single item shown in an empty folder (since it doesn't actually
	// represent any bookmark item).
	if (itr->second.menuItemType == BookmarkMenuBuilder::MenuItemType::EmptyItem)
	{
		return MND_CONTINUE;
	}

	BookmarkItem *bookmarkItem = itr->second.bookmarkItem;
	auto &ownedPtr = bookmarkItem->GetParent()->GetChildOwnedPtr(bookmarkItem);
	auto dataObject = BookmarkDataExchange::CreateDataObject({ ownedPtr });

	auto dropSource = winrt::make_self<DropSourceImpl>();

	DWORD effect;
	DoDragDrop(dataObject.get(), dropSource.get(), DROPEFFECT_MOVE, &effect);

	return MND_ENDMENU;
}

LRESULT BookmarkMenu::OnMenuGetObject(MENUGETOBJECTINFO *objectInfo)
{
	if (!m_showingMenu || !m_menuInfo->menus.contains(objectInfo->hmenu))
	{
		return MNGO_NOINTERFACE;
	}

	auto requestedIid = reinterpret_cast<IID *>(objectInfo->riid);

	if (!IsEqualIID(*requestedIid, IID_IDropTarget))
	{
		return MNGO_NOINTERFACE;
	}

	BookmarkItem *targetFolder = nullptr;
	size_t targetIndex;
	bool targetFound = false;

	// If neither MNGOF_TOPGAP or MNGOF_BOTTOMGAP is set, the cursor is over an item.
	if (WI_AreAllFlagsClear(objectInfo->dwFlags, MNGOF_TOPGAP | MNGOF_BOTTOMGAP))
	{
		auto itr = m_menuInfo->itemPositionMap.find({ objectInfo->hmenu, objectInfo->uPos });

		if (itr->second.bookmarkItem->IsFolder())
		{
			targetFolder = itr->second.bookmarkItem;
			targetIndex = targetFolder->GetChildren().size();
			targetFound = true;
		}
	}

	if (!targetFound || WI_IsAnyFlagSet(objectInfo->dwFlags, MNGOF_TOPGAP | MNGOF_BOTTOMGAP))
	{
		// Because all items in a specific menu have the same parent, it doesn't matter which item
		// is used to retrieve that parent. It's also guaranteed that there will be at least a
		// single entry in each menu, so simply retrieving the first item is safe.
		auto itr = m_menuInfo->itemPositionMap.find({ objectInfo->hmenu, 0 });

		if (itr->second.menuItemType == BookmarkMenuBuilder::MenuItemType::EmptyItem)
		{
			// This is the "empty" menu item, so bookmarkItem refers to the parent.
			targetFolder = itr->second.bookmarkItem;
			targetIndex = targetFolder->GetChildren().size();
		}
		else
		{
			// The documentation for the MENUGETOBJECTINFO structure appears to be worded somewhat
			// misleadingly. It states that MNGOF_TOPGAP will be set if "The mouse is on the top of
			// the item indicated by uPos.", while MNGOF_BOTTOMGAP will be set if "The mouse is on
			// the bottom of the item indicated by uPos.".
			// If there are, for example, 3 items in the menu and the source is dragged between the
			// second and third items:
			//
			// A
			// B
			// <-- Drag position
			// C
			//
			// The following values will be set:
			// dwFlags = MNGOF_BOTTOMGAP
			// uPos = 2
			//
			// That doesn't really align with the documentation, since the cursor is not at the
			// bottom of item 2. It can be considered to be at the bottom of item 1 or the top of
			// item 2, but it can't be at the bottom of item 2.
			//
			// On the other hand, if the cursor is at the top of the first item:
			//
			// <-- Drag position
			// A
			// B
			// C
			//
			// The following values will be set:
			// dwFlags = MNGOF_TOPGAP
			// uPos = 0
			//
			// Which matches the explanation given in the documentation.
			//
			// Ultimately, it appears the uPos indicates the target drop position and
			// MNGOF_TOPGAP/MNGOF_BOTTOMGAP can be effectively ignored (since the relative position
			// has already been incorporated into uPos).
			targetFolder = itr->second.bookmarkItem->GetParent();
			targetIndex = objectInfo->uPos;
		}
	}

	// This needs to be held in a member variable, otherwise it would be immediately released when
	// returning from this method.
	m_dropTarget =
		winrt::make_self<BookmarkMenuDropTarget>(targetFolder, targetIndex, m_bookmarkTree);

	objectInfo->pvObj = m_dropTarget.get();

	return MNGO_NOERROR;
}

void BookmarkMenu::OnMenuRightButtonUp(HMENU menu, int index, const POINT &pt)
{
	// The bookmark context menu can be shown on top of this menu, so the menu item that was
	// right-clicked might be from that menu.
	if (!m_showingMenu || !m_menuInfo->menus.contains(menu))
	{
		return;
	}

	auto itr = m_menuInfo->itemPositionMap.find({ menu, index });

	if (itr == m_menuInfo->itemPositionMap.end())
	{
		assert(false);
		return;
	}

	m_bookmarkContextMenu.ShowMenu(m_parentWindow, itr->second.bookmarkItem->GetParent(),
		{ itr->second.bookmarkItem }, pt, true);
}

BOOL BookmarkMenu::ShowMenu(BookmarkItem *bookmarkItem, const POINT &pt,
	BookmarkMenuBuilder::IncludePredicate includePredicate)
{
	wil::unique_hmenu menu(CreatePopupMenu());

	if (!menu)
	{
		return FALSE;
	}

	MenuHelper::SetMenuStyle(menu.get(), MNS_DRAGDROP);

	std::vector<wil::unique_hbitmap> menuImages;
	BookmarkMenuBuilder::MenuInfo menuInfo;
	BOOL res = m_menuBuilder.BuildMenu(m_parentWindow, menu.get(), bookmarkItem, { MIN_ID, MAX_ID },
		0, menuImages, menuInfo, includePredicate);

	if (!res)
	{
		return FALSE;
	}

	m_menuInfo = &menuInfo;
	m_showingMenu = true;

	int cmd = TrackPopupMenu(menu.get(), TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0,
		m_parentWindow, nullptr);

	m_showingMenu = false;
	m_menuInfo = nullptr;
	m_dropTarget = nullptr;

	if (cmd != 0)
	{
		OnMenuItemSelected(cmd, menuInfo.itemIdMap);
	}

	return TRUE;
}

void BookmarkMenu::OnMenuItemSelected(int menuItemId,
	BookmarkMenuBuilder::ItemIdMap &menuItemIdMappings)
{
	auto itr = menuItemIdMappings.find(menuItemId);

	if (itr == menuItemIdMappings.end())
	{
		return;
	}

	m_controller.OnBookmarkMenuItemSelected(itr->second);
}
