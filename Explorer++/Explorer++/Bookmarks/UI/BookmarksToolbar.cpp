// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/AddBookmarkDialog.h"
#include "CoreInterface.h"
#include "DarkModeHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/iDropSource.h"
#include <wil/com.h>
#include <wil/common.h>
#include <algorithm>

BookmarksToolbar::BookmarksToolbar(HWND hToolbar, HINSTANCE instance, IExplorerplusplus *pexpp,
	Navigation *navigation, IconFetcher *iconFetcher, BookmarkTree *bookmarkTree, UINT uIDStart,
	UINT uIDEnd) :
	BookmarkDropTargetWindow(hToolbar, bookmarkTree),
	m_hToolbar(hToolbar),
	m_instance(instance),
	m_pexpp(pexpp),
	m_navigation(navigation),
	m_bookmarkTree(bookmarkTree),
	m_uIDStart(uIDStart),
	m_uIDEnd(uIDEnd),
	m_bookmarkContextMenu(bookmarkTree, instance, pexpp),
	m_bookmarkMenu(bookmarkTree, instance, pexpp, navigation, iconFetcher, hToolbar),
	m_uIDCounter(0)
{
	InitializeToolbar(iconFetcher);
}

void BookmarksToolbar::InitializeToolbar(IconFetcher *iconFetcher)
{
	SendMessage(m_hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	SetUpToolbarImageList(iconFetcher);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(
		m_hToolbar, BookmarksToolbarProcStub, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	/* Also subclass the parent window, so that WM_COMMAND/WM_NOTIFY messages
	can be caught. */
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(GetParent(m_hToolbar),
		BookmarksToolbarParentProcStub, PARENT_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	InsertBookmarkItems();

	m_connections.push_back(m_bookmarkTree->bookmarkItemAddedSignal.AddObserver(
		std::bind(&BookmarksToolbar::OnBookmarkItemAdded, this, std::placeholders::_1,
			std::placeholders::_2)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemUpdatedSignal.AddObserver(
		std::bind(&BookmarksToolbar::OnBookmarkItemUpdated, this, std::placeholders::_1,
			std::placeholders::_2)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemMovedSignal.AddObserver(std::bind(
		&BookmarksToolbar::OnBookmarkItemMoved, this, std::placeholders::_1, std::placeholders::_2,
		std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemPreRemovalSignal.AddObserver(
		std::bind(&BookmarksToolbar::OnBookmarkItemPreRemoval, this, std::placeholders::_1)));
	m_connections.push_back(m_pexpp->AddToolbarContextMenuObserver(
		std::bind(&BookmarksToolbar::OnToolbarContextMenuPreShow, this, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3)));

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (darkModeHelper.IsDarkModeEnabled())
	{
		darkModeHelper.SetDarkModeForToolbarTooltips(m_hToolbar);
	}
}

void BookmarksToolbar::SetUpToolbarImageList(IconFetcher *iconFetcher)
{
	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hToolbar);
	int iconWidth = m_dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = m_dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	SendMessage(m_hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));

	m_bookmarkIconManager = std::make_unique<BookmarkIconManager>(m_pexpp, iconFetcher,
		std::bind(&BookmarksToolbar::OnBookmarkIconAvailable, this, std::placeholders::_1,
			std::placeholders::_2),
		iconWidth, iconHeight);

	SendMessage(m_hToolbar, TB_SETIMAGELIST, 0,
		reinterpret_cast<LPARAM>(m_bookmarkIconManager->GetImageList()));
}

LRESULT CALLBACK BookmarksToolbar::BookmarksToolbarProcStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pbt = reinterpret_cast<BookmarksToolbar *>(dwRefData);

	return pbt->BookmarksToolbarProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK BookmarksToolbar::BookmarksToolbarProc(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		OnLButtonDown(pt);
	}
	break;

	case WM_MOUSEMOVE:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		OnMouseMove(static_cast<int>(wParam), pt);
	}
	break;

	case WM_LBUTTONUP:
		OnLButtonUp();
		break;

	case WM_MBUTTONUP:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		OnMButtonUp(pt);
	}
	break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void BookmarksToolbar::OnLButtonDown(const POINT &pt)
{
	// The cursor point saved below will be used to determine whether or not to
	// enter a drag loop when the mouse moves. Note that although there's a
	// TBN_BEGINDRAG message, it appears that it's sent as soon as a mouse
	// button goes down (even if the mouse hasn't moved while the button is
	// down). Therefore, there's not much point using it.
	// Additionally, that event is also sent when the right button goes down,
	// though a drag should only begin when the left button goes down.
	m_leftButtonDownPoint = pt;
}

void BookmarksToolbar::OnMouseMove(int keys, const POINT &pt)
{
	if (!m_leftButtonDownPoint || !WI_IsFlagSet(keys, MK_LBUTTON))
	{
		m_leftButtonDownPoint.reset();
		return;
	}

	if (IsWithinDrag())
	{
		return;
	}

	RECT rect = { m_leftButtonDownPoint->x, m_leftButtonDownPoint->y, m_leftButtonDownPoint->x,
		m_leftButtonDownPoint->y };
	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hToolbar);
	InflateRect(&rect, m_dpiCompat.GetSystemMetricsForDpi(SM_CXDRAG, dpi),
		m_dpiCompat.GetSystemMetricsForDpi(SM_CYDRAG, dpi));

	if (!PtInRect(&rect, pt))
	{
		StartDrag(DragType::LeftClick, *m_leftButtonDownPoint);

		m_leftButtonDownPoint.reset();
	}
}

void BookmarksToolbar::StartDrag(DragType dragType, const POINT &pt)
{
	int index =
		static_cast<int>(SendMessage(m_hToolbar, TB_HITTEST, 0, reinterpret_cast<LPARAM>(&pt)));

	if (index < 0)
	{
		return;
	}

	wil::com_ptr<IDragSourceHelper> dragSourceHelper;
	HRESULT hr = CoCreateInstance(
		CLSID_DragDropHelper, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dragSourceHelper));

	if (FAILED(hr))
	{
		return;
	}

	wil::com_ptr<IDropSource> dropSource;
	hr = CreateDropSource(&dropSource, dragType);

	if (FAILED(hr))
	{
		return;
	}

	BookmarkItem *bookmarkItem = GetBookmarkItemFromToolbarIndex(index);
	auto &ownedPtr = bookmarkItem->GetParent()->GetChildOwnedPtr(bookmarkItem);
	auto dataObject = BookmarkDataExchange::CreateDataObject({ ownedPtr });

	DWORD effect;
	DoDragDrop(dataObject.get(), dropSource.get(), DROPEFFECT_MOVE, &effect);
}

void BookmarksToolbar::OnLButtonUp()
{
	m_leftButtonDownPoint.reset();
}

void BookmarksToolbar::OnMButtonUp(const POINT &pt)
{
	int index =
		static_cast<int>(SendMessage(m_hToolbar, TB_HITTEST, 0, reinterpret_cast<LPARAM>(&pt)));

	if (index < 0)
	{
		return;
	}

	auto bookmarkItem = GetBookmarkItemFromToolbarIndex(index);

	if (bookmarkItem)
	{
		BookmarkHelper::OpenBookmarkItemInNewTab(bookmarkItem, m_pexpp);
	}
}

LRESULT CALLBACK BookmarksToolbar::BookmarksToolbarParentProcStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pbt = reinterpret_cast<BookmarksToolbar *>(dwRefData);

	return pbt->BookmarksToolbarParentProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK BookmarksToolbar::BookmarksToolbarParentProc(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (OnCommand(wParam, lParam))
		{
			return 0;
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hToolbar)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_RCLICK:
				if (OnRightClick(reinterpret_cast<NMMOUSE *>(lParam)))
				{
					return TRUE;
				}
				break;

			case TBN_GETINFOTIP:
				if (OnGetInfoTip(reinterpret_cast<NMTBGETINFOTIP *>(lParam)))
				{
					return 0;
				}
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

BOOL BookmarksToolbar::OnRightClick(const NMMOUSE *nmm)
{
	if (nmm->dwItemSpec == -1)
	{
		return FALSE;
	}

	int index = static_cast<int>(SendMessage(m_hToolbar, TB_COMMANDTOINDEX, nmm->dwItemSpec, 0));

	if (index == -1)
	{
		return FALSE;
	}

	auto bookmarkItem = GetBookmarkItemFromToolbarIndex(index);

	if (!bookmarkItem)
	{
		return FALSE;
	}

	POINT pt = nmm->pt;
	BOOL res = ClientToScreen(m_hToolbar, &pt);

	if (!res)
	{
		return FALSE;
	}

	m_bookmarkContextMenu.ShowMenu(
		m_hToolbar, m_bookmarkTree->GetBookmarksToolbarFolder(), { bookmarkItem }, pt);

	return TRUE;
}

bool BookmarksToolbar::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		return false;
	}

	if ((LOWORD(wParam) >= m_uIDStart && LOWORD(wParam) <= m_uIDEnd))
	{
		return OnButtonClick(LOWORD(wParam));
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDM_BT_NEWBOOKMARK:
		case IDM_BT_NEWFOLDER:
		case IDM_BT_PASTE:
			OnToolbarContextMenuItemClicked(LOWORD(wParam));
			return true;
		}
	}

	return false;
}

bool BookmarksToolbar::OnButtonClick(int command)
{
	int index = static_cast<int>(SendMessage(m_hToolbar, TB_COMMANDTOINDEX, command, 0));

	if (index == -1)
	{
		return false;
	}

	auto bookmarkItem = GetBookmarkItemFromToolbarIndex(index);

	if (!bookmarkItem)
	{
		return false;
	}

	if (bookmarkItem->IsFolder())
	{
		ShowBookmarkFolderMenu(bookmarkItem, command, index);
	}
	else
	{
		m_navigation->BrowseFolderInCurrentTab(bookmarkItem->GetLocation().c_str());
	}

	return true;
}

void BookmarksToolbar::ShowBookmarkFolderMenu(BookmarkItem *bookmarkItem, int command, int index)
{
	RECT rc;
	BOOL res = static_cast<BOOL>(
		SendMessage(m_hToolbar, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rc)));

	if (!res)
	{
		return;
	}

	SetLastError(ERROR_SUCCESS);
	auto mapRes = MapWindowPoints(m_hToolbar, nullptr, reinterpret_cast<LPPOINT>(&rc), 2);

	if (mapRes == 0 && GetLastError() != ERROR_SUCCESS)
	{
		return;
	}

	SendMessage(m_hToolbar, TB_PRESSBUTTON, command, MAKEWORD(TRUE, 0));

	POINT pt;
	pt.x = rc.left;
	pt.y = rc.bottom;
	m_bookmarkMenu.ShowMenu(bookmarkItem, pt);

	SendMessage(m_hToolbar, TB_PRESSBUTTON, command, MAKEWORD(FALSE, 0));
}

void BookmarksToolbar::ShowOverflowMenu(const POINT &ptScreen)
{
	m_bookmarkMenu.ShowMenu(m_bookmarkTree->GetBookmarksToolbarFolder(), ptScreen,
		[this](const BookmarkItem *bookmarkItem) {
			auto index = GetBookmarkItemIndex(bookmarkItem);

			if (!index)
			{
				assert(false);
				return false;
			}

			RECT toolbarRect;
			GetClientRect(m_hToolbar, &toolbarRect);

			RECT buttonRect;
			SendMessage(m_hToolbar, TB_GETITEMRECT, *index, reinterpret_cast<LPARAM>(&buttonRect));

			if (buttonRect.right > toolbarRect.right)
			{
				return true;
			}

			return false;
		});
}

void BookmarksToolbar::OnToolbarContextMenuItemClicked(int menuItemId)
{
	assert(m_contextMenuLocation);

	POINT ptClient = *m_contextMenuLocation;
	ScreenToClient(m_hToolbar, &ptClient);
	int targetIndex = FindNextButtonIndex(ptClient);

	switch (menuItemId)
	{
	case IDM_BT_NEWBOOKMARK:
		OnNewBookmarkItem(BookmarkItem::Type::Bookmark, targetIndex);
		break;

	case IDM_BT_NEWFOLDER:
		OnNewBookmarkItem(BookmarkItem::Type::Folder, targetIndex);
		break;

	case IDM_BT_PASTE:
		OnPaste(targetIndex);
		break;
	}

	m_contextMenuLocation.reset();
}

void BookmarksToolbar::OnNewBookmarkItem(BookmarkItem::Type type, size_t targetIndex)
{
	BookmarkHelper::AddBookmarkItem(m_bookmarkTree, type,
		m_bookmarkTree->GetBookmarksToolbarFolder(), targetIndex, m_hToolbar, m_pexpp);
}

void BookmarksToolbar::OnPaste(size_t targetIndex)
{
	BookmarkHelper::PasteBookmarkItems(
		m_bookmarkTree, m_bookmarkTree->GetBookmarksToolbarFolder(), targetIndex);
}

// Returns the index of the button that comes after the specified point. If the
// point is past the last button on the toolbar, this index will be one past the
// last button (or 0 if there are no buttons).
int BookmarksToolbar::FindNextButtonIndex(const POINT &ptClient)
{
	int numButtons = static_cast<int>(SendMessage(m_hToolbar, TB_BUTTONCOUNT, 0, 0));
	int nextIndex = 0;

	for (int i = 0; i < numButtons; i++)
	{
		RECT rc;
		SendMessage(m_hToolbar, TB_GETITEMRECT, i, reinterpret_cast<LPARAM>(&rc));

		if (ptClient.x < rc.right)
		{
			break;
		}

		nextIndex = i + 1;
	}

	return nextIndex;
}

void BookmarksToolbar::OnEditBookmarkItem(BookmarkItem *bookmarkItem)
{
	BookmarkHelper::EditBookmarkItem(bookmarkItem, m_bookmarkTree, m_instance, m_hToolbar, m_pexpp);
}

bool BookmarksToolbar::OnGetInfoTip(NMTBGETINFOTIP *infoTip)
{
	int index = static_cast<int>(SendMessage(m_hToolbar, TB_COMMANDTOINDEX, infoTip->iItem, 0));

	if (index == -1)
	{
		return false;
	}

	auto bookmarkItem = GetBookmarkItemFromToolbarIndex(index);

	if (!bookmarkItem)
	{
		return false;
	}

	if (bookmarkItem->IsBookmark())
	{
		StringCchPrintf(infoTip->pszText, infoTip->cchTextMax, _T("%s\n%s"),
			bookmarkItem->GetName().c_str(), bookmarkItem->GetLocation().c_str());

		return true;
	}

	return false;
}

void BookmarksToolbar::InsertBookmarkItems()
{
	int position = 0;

	for (const auto &bookmarkItem : m_bookmarkTree->GetBookmarksToolbarFolder()->GetChildren())
	{
		InsertBookmarkItem(bookmarkItem.get(), position);

		position++;
	}
}

void BookmarksToolbar::InsertBookmarkItem(BookmarkItem *bookmarkItem, int position)
{
	assert(position <= SendMessage(m_hToolbar, TB_BUTTONCOUNT, 0, 0));

	TCHAR szName[256];
	StringCchCopy(szName, SIZEOF_ARRAY(szName), bookmarkItem->GetName().c_str());

	int iconIndex = m_bookmarkIconManager->GetBookmarkItemIconIndex(bookmarkItem);

	TBBUTTON tbb;
	tbb.iBitmap = iconIndex;
	tbb.idCommand = m_uIDStart + m_uIDCounter;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_NOPREFIX;
	tbb.dwData = reinterpret_cast<DWORD_PTR>(bookmarkItem);
	tbb.iString = reinterpret_cast<INT_PTR>(szName);
	SendMessage(m_hToolbar, TB_INSERTBUTTON, position, reinterpret_cast<LPARAM>(&tbb));

	UpdateToolbarBandSizing(GetParent(m_hToolbar), m_hToolbar);

	++m_uIDCounter;
}

void BookmarksToolbar::OnBookmarkIconAvailable(std::wstring_view guid, int iconIndex)
{
	auto index = GetBookmarkItemIndexUsingGuid(guid);

	if (!index)
	{
		return;
	}

	TBBUTTONINFO buttonInfo;
	buttonInfo.cbSize = sizeof(buttonInfo);
	buttonInfo.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
	buttonInfo.iImage = iconIndex;
	SendMessage(m_hToolbar, TB_SETBUTTONINFO, *index, reinterpret_cast<LPARAM>(&buttonInfo));
}

void BookmarksToolbar::OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index)
{
	if (bookmarkItem.GetParent() == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		InsertBookmarkItem(&bookmarkItem, static_cast<int>(index));
	}
}

void BookmarksToolbar::OnBookmarkItemUpdated(
	BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType)
{
	if (propertyType != BookmarkItem::PropertyType::Name)
	{
		return;
	}

	auto index = GetBookmarkItemIndex(&bookmarkItem);

	if (!index)
	{
		return;
	}

	TCHAR name[128];
	StringCchCopy(name, std::size(name), bookmarkItem.GetName().c_str());

	TBBUTTONINFO tbbi;
	tbbi.cbSize = sizeof(tbbi);
	tbbi.dwMask = TBIF_BYINDEX | TBIF_TEXT;
	tbbi.pszText = name;
	SendMessage(m_hToolbar, TB_SETBUTTONINFO, *index, reinterpret_cast<LPARAM>(&tbbi));
}

void BookmarksToolbar::OnBookmarkItemMoved(BookmarkItem *bookmarkItem,
	const BookmarkItem *oldParent, size_t oldIndex, const BookmarkItem *newParent, size_t newIndex)
{
	UNREFERENCED_PARAMETER(oldIndex);

	if (oldParent == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		RemoveBookmarkItem(bookmarkItem);
	}

	if (newParent == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		InsertBookmarkItem(bookmarkItem, static_cast<int>(newIndex));
	}
}

void BookmarksToolbar::OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem)
{
	if (bookmarkItem.GetParent() == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		RemoveBookmarkItem(&bookmarkItem);
	}
}

void BookmarksToolbar::RemoveBookmarkItem(const BookmarkItem *bookmarkItem)
{
	auto index = GetBookmarkItemIndex(bookmarkItem);
	assert(index);

	TBBUTTONINFO buttonInfo;
	buttonInfo.cbSize = sizeof(buttonInfo);
	buttonInfo.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
	SendMessage(m_hToolbar, TB_GETBUTTONINFO, *index, reinterpret_cast<LPARAM>(&buttonInfo));

	SendMessage(m_hToolbar, TB_DELETEBUTTON, *index, 0);

	m_bookmarkIconManager->RemoveIcon(buttonInfo.iImage);

	UpdateToolbarBandSizing(GetParent(m_hToolbar), m_hToolbar);
}

void BookmarksToolbar::OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow, const POINT &pt)
{
	if (sourceWindow != m_hToolbar)
	{
		return;
	}

	std::wstring newBookmark =
		ResourceHelper::LoadString(m_instance, IDS_BOOKMARKS_TOOLBAR_NEW_BOOKMARK);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.dwTypeData = newBookmark.data();
	mii.wID = IDM_BT_NEWBOOKMARK;
	InsertMenuItem(menu, IDM_TOOLBARS_CUSTOMIZE, FALSE, &mii);

	std::wstring newBookmarkFolder =
		ResourceHelper::LoadString(m_instance, IDS_BOOKMARKS_TOOLBAR_NEW_FOLDER);

	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.dwTypeData = newBookmarkFolder.data();
	mii.wID = IDM_BT_NEWFOLDER;
	InsertMenuItem(menu, IDM_TOOLBARS_CUSTOMIZE, FALSE, &mii);

	std::wstring paste = ResourceHelper::LoadString(m_instance, IDS_BOOKMARKS_TOOLBAR_PASTE);

	UINT state = MFS_DISABLED;

	if (IsClipboardFormatAvailable(BookmarkClipboard::GetClipboardFormat()))
	{
		state = MFS_ENABLED;
	}

	mii.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
	mii.dwTypeData = paste.data();
	mii.wID = IDM_BT_PASTE;
	mii.fState = state;
	InsertMenuItem(menu, IDM_TOOLBARS_CUSTOMIZE, FALSE, &mii);

	// TODO: It would be better to have a callback function invoked when a menu
	// item is selected, rather than handling the selection in WM_COMMAND. That
	// way, the point here wouldn't need to be saved (it could be passed
	// directly to the callback instead).
	m_contextMenuLocation = pt;
}

BookmarkItem *BookmarksToolbar::GetBookmarkItemFromToolbarIndex(int index)
{
	TBBUTTON tbButton;
	BOOL ret = static_cast<BOOL>(
		SendMessage(m_hToolbar, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&tbButton)));

	if (!ret)
	{
		return nullptr;
	}

	return reinterpret_cast<BookmarkItem *>(tbButton.dwData);
}

std::optional<int> BookmarksToolbar::GetBookmarkItemIndex(const BookmarkItem *bookmarkItem) const
{
	return GetBookmarkItemIndexUsingGuid(bookmarkItem->GetGUID());
}

std::optional<int> BookmarksToolbar::GetBookmarkItemIndexUsingGuid(std::wstring_view guid) const
{
	int nButtons = static_cast<int>(SendMessage(m_hToolbar, TB_BUTTONCOUNT, 0, 0));

	for (int i = 0; i < nButtons; i++)
	{
		TBBUTTON tb;
		SendMessage(m_hToolbar, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tb));

		auto *currentBookmarkItem = reinterpret_cast<BookmarkItem *>(tb.dwData);

		if (currentBookmarkItem->GetGUID() == guid)
		{
			return i;
		}
	}

	return std::nullopt;
}

BookmarksToolbar::DropLocation BookmarksToolbar::GetDropLocation(const POINT &pt)
{
	POINT ptClient = pt;
	ScreenToClient(m_hToolbar, &ptClient);
	int index = static_cast<int>(
		SendMessage(m_hToolbar, TB_HITTEST, 0, reinterpret_cast<LPARAM>(&ptClient)));

	BookmarkItem *parentFolder = nullptr;
	size_t position;
	bool parentFolderSelected = false;

	if (index >= 0)
	{
		auto bookmarkItem = GetBookmarkItemFromToolbarIndex(index);

		RECT buttonRect;
		SendMessage(m_hToolbar, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&buttonRect));

		if (bookmarkItem->IsFolder())
		{
			RECT folderCentralRect = buttonRect;
			int indent =
				static_cast<int>(FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE * GetRectWidth(&buttonRect));
			InflateRect(&folderCentralRect, -indent, 0);

			if (ptClient.x < folderCentralRect.left)
			{
				parentFolder = m_bookmarkTree->GetBookmarksToolbarFolder();
				position = index;
			}
			else if (ptClient.x > folderCentralRect.right)
			{
				parentFolder = m_bookmarkTree->GetBookmarksToolbarFolder();
				position = index + 1;
			}
			else
			{
				parentFolder = bookmarkItem;
				position = bookmarkItem->GetChildren().size();
				parentFolderSelected = true;
			}
		}
		else
		{
			parentFolder = m_bookmarkTree->GetBookmarksToolbarFolder();
			position = index;

			if (ptClient.x > (buttonRect.left + GetRectWidth(&buttonRect) / 2))
			{
				position++;
			}
		}
	}
	else
	{
		parentFolder = m_bookmarkTree->GetBookmarksToolbarFolder();
		position = FindNextButtonIndex(ptClient);
	}

	return { parentFolder, position, parentFolderSelected };
}

void BookmarksToolbar::UpdateUiForDropLocation(const DropLocation &dropLocation)
{
	RemoveDropHighlight();

	if (dropLocation.parentFolder == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		DWORD flags;
		int numButtons = static_cast<int>(SendMessage(m_hToolbar, TB_BUTTONCOUNT, 0, 0));
		size_t finalPosition = dropLocation.position;

		if (finalPosition == static_cast<size_t>(numButtons))
		{
			finalPosition--;
			flags = TBIMHT_AFTER;
		}
		else
		{
			flags = 0;
		}

		TBINSERTMARK tbim;
		tbim.iButton = static_cast<int>(finalPosition);
		tbim.dwFlags = flags;
		SendMessage(m_hToolbar, TB_SETINSERTMARK, 0, reinterpret_cast<LPARAM>(&tbim));

		m_previousDropButton.reset();
	}
	else
	{
		RemoveInsertionMark();

		auto selectedButtonIndex = GetBookmarkItemIndex(dropLocation.parentFolder);
		assert(selectedButtonIndex);

		SetButtonPressedState(*selectedButtonIndex, true);
		m_previousDropButton = *selectedButtonIndex;
	}
}

void BookmarksToolbar::SetButtonPressedState(int index, bool pressed)
{
	TBBUTTON tbButton;
	BOOL res = static_cast<BOOL>(
		SendMessage(m_hToolbar, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&tbButton)));

	if (!res)
	{
		return;
	}

	SendMessage(m_hToolbar, TB_PRESSBUTTON, tbButton.idCommand, MAKEWORD(pressed, 0));
}

void BookmarksToolbar::ResetDropUiState()
{
	RemoveInsertionMark();
	RemoveDropHighlight();
}

void BookmarksToolbar::RemoveInsertionMark()
{
	TBINSERTMARK tbim;
	tbim.iButton = -1;
	SendMessage(m_hToolbar, TB_SETINSERTMARK, 0, reinterpret_cast<LPARAM>(&tbim));
}

void BookmarksToolbar::RemoveDropHighlight()
{
	if (m_previousDropButton)
	{
		SetButtonPressedState(*m_previousDropButton, false);
		m_previousDropButton.reset();
	}
}