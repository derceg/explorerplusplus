// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarksToolbar.h"
#include "AddBookmarkDialog.h"
#include "BookmarkClipboard.h"
#include "BookmarkDataExchange.h"
#include "BookmarkHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "TabContainer.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/iDropSource.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include <wil/com.h>
#include <wil/common.h>
#include <algorithm>

CBookmarksToolbar::CBookmarksToolbar(HWND hToolbar, HINSTANCE instance, IExplorerplusplus *pexpp,
	Navigation *navigation, BookmarkTree *bookmarkTree, UINT uIDStart, UINT uIDEnd) :
	m_hToolbar(hToolbar),
	m_instance(instance),
	m_pexpp(pexpp),
	m_navigation(navigation),
	m_bookmarkTree(bookmarkTree),
	m_uIDStart(uIDStart),
	m_uIDEnd(uIDEnd),
	m_bookmarkContextMenu(bookmarkTree, instance, pexpp),
	m_bookmarkMenu(bookmarkTree, instance, pexpp, hToolbar),
	m_uIDCounter(0)
{
	InitializeToolbar();
}

void CBookmarksToolbar::InitializeToolbar()
{
	SendMessage(m_hToolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);

	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hToolbar);
	int iconWidth = m_dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = m_dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	SendMessage(m_hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));

	std::tie(m_imageList, m_imageListMappings) = ResourceHelper::CreateIconImageList(
		m_pexpp->GetIconResourceLoader(), iconWidth, iconHeight, { Icon::Folder, Icon::Bookmarks});
	SendMessage(m_hToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(m_imageList.get()));

	m_dropTarget = DropTarget::Create(m_hToolbar, this);

	m_windowSubclasses.push_back(WindowSubclassWrapper(m_hToolbar, BookmarksToolbarProcStub, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	/* Also subclass the parent window, so that WM_COMMAND/WM_NOTIFY messages
	can be caught. */
	m_windowSubclasses.push_back(WindowSubclassWrapper(GetParent(m_hToolbar), BookmarksToolbarParentProcStub, PARENT_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this)));

	InsertBookmarkItems();

	m_connections.push_back(m_bookmarkTree->bookmarkItemAddedSignal.AddObserver(
		std::bind(&CBookmarksToolbar::OnBookmarkItemAdded, this, std::placeholders::_1, std::placeholders::_2)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemUpdatedSignal.AddObserver(
		std::bind(&CBookmarksToolbar::OnBookmarkItemUpdated, this, std::placeholders::_1, std::placeholders::_2)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemMovedSignal.AddObserver(
		std::bind(&CBookmarksToolbar::OnBookmarkItemMoved, this, std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemPreRemovalSignal.AddObserver(
		std::bind(&CBookmarksToolbar::OnBookmarkItemPreRemoval, this, std::placeholders::_1)));
	m_connections.push_back(m_pexpp->AddToolbarContextMenuObserver(
		std::bind(&CBookmarksToolbar::OnToolbarContextMenuPreShow, this, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3)));
}

LRESULT CALLBACK CBookmarksToolbar::BookmarksToolbarProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CBookmarksToolbar *pbt = reinterpret_cast<CBookmarksToolbar *>(dwRefData);

	return pbt->BookmarksToolbarProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CBookmarksToolbar::BookmarksToolbarProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
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

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void CBookmarksToolbar::OnLButtonDown(const POINT &pt)
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

void CBookmarksToolbar::OnMouseMove(int keys, const POINT &pt)
{
	if (!m_leftButtonDownPoint || !WI_IsFlagSet(keys, MK_LBUTTON))
	{
		m_leftButtonDownPoint.reset();
		return;
	}

	if (m_dropTarget->IsWithinDrag())
	{
		return;
	}

	RECT rect = {m_leftButtonDownPoint->x, m_leftButtonDownPoint->y, m_leftButtonDownPoint->x, m_leftButtonDownPoint->y};
	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hToolbar);
	InflateRect(&rect, m_dpiCompat.GetSystemMetricsForDpi(SM_CXDRAG, dpi), m_dpiCompat.GetSystemMetricsForDpi(SM_CYDRAG, dpi));

	if (!PtInRect(&rect, pt))
	{
		StartDrag(DragType::LeftClick, *m_leftButtonDownPoint);

		m_leftButtonDownPoint.reset();
	}
}

void CBookmarksToolbar::StartDrag(DragType dragType, const POINT &pt)
{
	int index = static_cast<int>(SendMessage(m_hToolbar, TB_HITTEST, 0,
		reinterpret_cast<LPARAM>(&pt)));

	if (index < 0)
	{
		return;
	}

	wil::com_ptr<IDragSourceHelper> dragSourceHelper;
	HRESULT hr = CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_ALL,
		IID_PPV_ARGS(&dragSourceHelper));

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
	auto dataObject = BookmarkDataExchange::CreateDataObject(ownedPtr);

	DWORD effect;
	DoDragDrop(dataObject.get(), dropSource.get(), DROPEFFECT_MOVE, &effect);
}

void CBookmarksToolbar::OnLButtonUp()
{
	m_leftButtonDownPoint.reset();
}

void CBookmarksToolbar::OnMButtonUp(const POINT &pt)
{
	int index = static_cast<int>(SendMessage(m_hToolbar, TB_HITTEST, 0,
		reinterpret_cast<LPARAM>(&pt)));

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

LRESULT CALLBACK CBookmarksToolbar::BookmarksToolbarParentProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CBookmarksToolbar *pbt = reinterpret_cast<CBookmarksToolbar *>(dwRefData);

	return pbt->BookmarksToolbarParentProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CBookmarksToolbar::BookmarksToolbarParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		if (OnCommand(wParam, lParam))
		{
			return 0;
		}
		break;

	case WM_NOTIFY:
		if(reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hToolbar)
		{
			switch(reinterpret_cast<LPNMHDR>(lParam)->code)
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

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

BOOL CBookmarksToolbar::OnRightClick(const NMMOUSE *nmm)
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

	m_bookmarkContextMenu.ShowMenu(m_hToolbar, bookmarkItem, pt);

	return TRUE;
}

bool CBookmarksToolbar::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		return false;
	}

	if ((LOWORD(wParam) >= m_uIDStart &&
		LOWORD(wParam) <= m_uIDEnd))
	{
		return OnButtonClick(LOWORD(wParam));
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDM_BT_NEWBOOKMARK:
			OnNewBookmarkItem(BookmarkItem::Type::Bookmark);
			return true;

		case IDM_BT_NEWFOLDER:
			OnNewBookmarkItem(BookmarkItem::Type::Folder);
			return true;

		case IDM_BT_PASTE:
			OnPaste();
			return true;
		}
	}

	return false;
}

bool CBookmarksToolbar::OnButtonClick(int command)
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

void CBookmarksToolbar::ShowBookmarkFolderMenu(BookmarkItem *bookmarkItem, int command, int index)
{
	RECT rc;
	BOOL res = static_cast<BOOL>(SendMessage(m_hToolbar, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rc)));

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

	auto state = SendMessage(m_hToolbar, TB_GETSTATE, command, 0);

	if (state == -1)
	{
		return;
	}

	SendMessage(m_hToolbar, TB_SETSTATE, command, MAKEWORD(state | TBSTATE_PRESSED, 0));

	POINT pt;
	pt.x = rc.left;
	pt.y = rc.bottom;
	m_bookmarkMenu.ShowMenu(bookmarkItem, pt, std::bind(&CBookmarksToolbar::OnBookmarkMenuItemClicked,
		this, std::placeholders::_1));

	SendMessage(m_hToolbar, TB_SETSTATE, command, MAKEWORD(state & ~TBSTATE_PRESSED, 0));
}

void CBookmarksToolbar::OnBookmarkMenuItemClicked(const BookmarkItem *bookmarkItem)
{
	assert(bookmarkItem->IsBookmark());

	m_navigation->BrowseFolderInCurrentTab(bookmarkItem->GetLocation().c_str());
}

void CBookmarksToolbar::OnNewBookmarkItem(BookmarkItem::Type type)
{
	BookmarkHelper::AddBookmarkItem(m_bookmarkTree, type, m_instance, m_hToolbar,
		m_pexpp->GetTabContainer(), m_pexpp);
}

void CBookmarksToolbar::OnPaste()
{
	BookmarkClipboard bookmarkClipboard;
	auto copiedBookmarkItem = bookmarkClipboard.ReadBookmark();

	if (!copiedBookmarkItem)
	{
		return;
	}

	assert(m_contextMenuLocation);

	POINT ptClient = *m_contextMenuLocation;
	ScreenToClient(m_hToolbar, &ptClient);
	int newIndex = FindNextButtonIndex(ptClient);

	m_bookmarkTree->AddBookmarkItem(m_bookmarkTree->GetBookmarksToolbarFolder(), std::move(copiedBookmarkItem), newIndex);

	m_contextMenuLocation.reset();
}

// Returns the index of the button that comes after the specified point. If the
// point is past the last button on the toolbar, this index will be one past the
// last button (or 0 if there are no buttons).
int CBookmarksToolbar::FindNextButtonIndex(const POINT &ptClient)
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

void CBookmarksToolbar::OnEditBookmarkItem(BookmarkItem *bookmarkItem)
{
	BookmarkHelper::EditBookmarkItem(bookmarkItem, m_bookmarkTree, m_instance,
		m_hToolbar, m_pexpp);
}

bool CBookmarksToolbar::OnGetInfoTip(NMTBGETINFOTIP *infoTip)
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

void CBookmarksToolbar::InsertBookmarkItems()
{
	int position = 0;

	for (const auto &bookmarkItem : m_bookmarkTree->GetBookmarksToolbarFolder()->GetChildren())
	{
		InsertBookmarkItem(bookmarkItem.get(), position);

		position++;
	}
}

void CBookmarksToolbar::InsertBookmarkItem(BookmarkItem *bookmarkItem, int position)
{
	assert(position <= SendMessage(m_hToolbar, TB_BUTTONCOUNT, 0, 0));

	TCHAR szName[256];
	StringCchCopy(szName, SIZEOF_ARRAY(szName), bookmarkItem->GetName().c_str());

	int iImage;

	if(bookmarkItem->IsFolder())
	{
		iImage = m_imageListMappings.at(Icon::Folder);
	}
	else
	{
		iImage = m_imageListMappings.at(Icon::Bookmarks);
	}

	TBBUTTON tbb;
	tbb.iBitmap		= iImage;
	tbb.idCommand	= m_uIDStart + m_uIDCounter;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE|BTNS_SHOWTEXT|BTNS_NOPREFIX;
	tbb.dwData		= reinterpret_cast<DWORD_PTR>(bookmarkItem);
	tbb.iString		= reinterpret_cast<INT_PTR>(szName);
	SendMessage(m_hToolbar, TB_INSERTBUTTON, position, reinterpret_cast<LPARAM>(&tbb));

	++m_uIDCounter;
}

void CBookmarksToolbar::OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index)
{
	if(bookmarkItem.GetParent() == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		InsertBookmarkItem(&bookmarkItem, static_cast<int>(index));
	}
}

void CBookmarksToolbar::OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType)
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

void CBookmarksToolbar::OnBookmarkItemMoved(BookmarkItem *bookmarkItem, const BookmarkItem *oldParent,
	size_t oldIndex, const BookmarkItem *newParent, size_t newIndex)
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

void CBookmarksToolbar::OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem)
{
	if (bookmarkItem.GetParent() == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		RemoveBookmarkItem(&bookmarkItem);
	}
}

void CBookmarksToolbar::RemoveBookmarkItem(const BookmarkItem *bookmarkItem)
{
	auto index = GetBookmarkItemIndex(bookmarkItem);
	assert(index);

	SendMessage(m_hToolbar, TB_DELETEBUTTON, *index, 0);

	/* TODO: */
	//UpdateToolbarBandSizing(m_hMainRebar,m_hBookmarksToolbar);
}

void CBookmarksToolbar::OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow, const POINT &pt)
{
	if (sourceWindow != m_hToolbar)
	{
		return;
	}

	std::wstring newBookmark = ResourceHelper::LoadString(m_instance, IDS_BOOKMARKS_TOOLBAR_NEW_BOOKMARK);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.dwTypeData = newBookmark.data();
	mii.wID = IDM_BT_NEWBOOKMARK;
	InsertMenuItem(menu, IDM_TOOLBARS_CUSTOMIZE, FALSE, &mii);

	std::wstring newBookmarkFolder = ResourceHelper::LoadString(m_instance, IDS_BOOKMARKS_TOOLBAR_NEW_FOLDER);

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

BookmarkItem *CBookmarksToolbar::GetBookmarkItemFromToolbarIndex(int index)
{
	TBBUTTON tbButton;
	BOOL ret = static_cast<BOOL>(SendMessage(m_hToolbar, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&tbButton)));

	if (!ret)
	{
		return nullptr;
	}

	return reinterpret_cast<BookmarkItem *>(tbButton.dwData);
}

std::optional<int> CBookmarksToolbar::GetBookmarkItemIndex(const BookmarkItem *bookmarkItem) const
{
	int nButtons = static_cast<int>(SendMessage(m_hToolbar, TB_BUTTONCOUNT, 0, 0));

	for (int i = 0; i < nButtons; i++)
	{
		TBBUTTON tb;
		SendMessage(m_hToolbar, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tb));

		BookmarkItem *currentBookmarkItem = reinterpret_cast<BookmarkItem *>(tb.dwData);

		if (currentBookmarkItem == bookmarkItem)
		{
			return i;
		}
	}

	return std::nullopt;
}

DWORD CBookmarksToolbar::DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);
	UNREFERENCED_PARAMETER(effect);

	m_cachedDropEffect = GetDropEffect(dataObject);
	return m_cachedDropEffect;
}

DWORD CBookmarksToolbar::DragOver(DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	if (m_previousDropButton)
	{
		SetButtonPressedState(*m_previousDropButton, false);
	}

	auto [parentFolder, position, selectedButtonIndex] = GetDropTarget(pt);

	if (parentFolder == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		DWORD flags;
		int numButtons = static_cast<int>(SendMessage(m_hToolbar, TB_BUTTONCOUNT, 0, 0));

		if (position == numButtons)
		{
			position--;
			flags = TBIMHT_AFTER;
		}
		else
		{
			flags = 0;
		}

		TBINSERTMARK tbim;
		tbim.iButton = static_cast<int>(position);
		tbim.dwFlags = flags;
		SendMessage(m_hToolbar, TB_SETINSERTMARK, 0, reinterpret_cast<LPARAM>(&tbim));

		m_previousDropButton.reset();
	}
	else
	{
		RemoveInsertionMark();

		SetButtonPressedState(selectedButtonIndex, true);
		m_previousDropButton = selectedButtonIndex;
	}

	return m_cachedDropEffect;
}

void CBookmarksToolbar::DragLeave()
{
	ResetToolbarState();
}

DWORD CBookmarksToolbar::Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	DWORD finalEffect = DROPEFFECT_NONE;
	auto [parentFolder, position, selectedButtonIndex] = GetDropTarget(pt);

	if (IsDropFormatAvailable(dataObject, BookmarkDataExchange::GetFormatEtc()))
	{
		auto bookmarkItem = BookmarkDataExchange::ExtractBookmarkItem(dataObject);

		if (bookmarkItem)
		{
			auto exingBookmarkItem = BookmarkHelper::GetBookmarkItemById(m_bookmarkTree, *bookmarkItem->GetOriginalGUID());

			if (exingBookmarkItem)
			{
				m_bookmarkTree->MoveBookmarkItem(exingBookmarkItem, parentFolder, position);

				finalEffect = DROPEFFECT_MOVE;
			}
		}
	}
	else if (IsDropFormatAvailable(dataObject, GetDroppedFilesFormatEtc()))
	{
		BookmarkItems bookmarkItems = CreateBookmarkItemsFromDroppedFiles(dataObject);
		size_t i = 0;

		for (auto &bookmarkItem : bookmarkItems)
		{
			m_bookmarkTree->AddBookmarkItem(parentFolder, std::move(bookmarkItem), position + i);
			i++;
		}

		if (!bookmarkItems.empty())
		{
			finalEffect = DROPEFFECT_COPY;
		}
	}

	ResetToolbarState();

	return finalEffect;
}

DWORD CBookmarksToolbar::GetDropEffect(IDataObject *dataObject)
{
	if (IsDropFormatAvailable(dataObject, BookmarkDataExchange::GetFormatEtc()))
	{
		// TODO: Should block drop if a folder is dragged over itself.
		return DROPEFFECT_MOVE;
	}
	else if (IsDropFormatAvailable(dataObject, GetDroppedFilesFormatEtc()))
	{
		auto droppedFiles = ExtractDroppedFilesList(dataObject);

		bool allFolders = std::all_of(droppedFiles.begin(), droppedFiles.end(), [] (const std::wstring &path) {
			return PathIsDirectory(path.c_str());
		});

		if (!droppedFiles.empty() && allFolders)
		{
			return DROPEFFECT_COPY;
		}
	}

	return DROPEFFECT_NONE;
}

CBookmarksToolbar::BookmarkDropTarget CBookmarksToolbar::GetDropTarget(const POINT &pt)
{
	POINT ptClient = pt;
	ScreenToClient(m_hToolbar, &ptClient);
	int index = static_cast<int>(SendMessage(m_hToolbar, TB_HITTEST, 0, reinterpret_cast<LPARAM>(&ptClient)));

	BookmarkItem *parentFolder = nullptr;
	size_t position;

	if (index >= 0)
	{
		auto bookmarkItem = GetBookmarkItemFromToolbarIndex(index);

		RECT buttonRect;
		SendMessage(m_hToolbar, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&buttonRect));

		if (bookmarkItem->IsFolder())
		{
			RECT folderCentralRect = buttonRect;
			int indent = static_cast<int>(FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE * GetRectWidth(&buttonRect));
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

	return { parentFolder, position, index };
}

void CBookmarksToolbar::SetButtonPressedState(int index, bool pressed)
{
	TBBUTTON tbButton;
	BOOL res = static_cast<BOOL>(SendMessage(m_hToolbar, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&tbButton)));

	if (!res)
	{
		return;
	}

	auto state = SendMessage(m_hToolbar, TB_GETSTATE, tbButton.idCommand, 0);

	if (state == -1)
	{
		return;
	}

	if (pressed)
	{
		WI_SetFlag(state, TBSTATE_PRESSED);
	}
	else
	{
		WI_ClearFlag(state, TBSTATE_PRESSED);
	}

	SendMessage(m_hToolbar, TB_SETSTATE, tbButton.idCommand, MAKEWORD(state, 0));
}

BookmarkItems CBookmarksToolbar::CreateBookmarkItemsFromDroppedFiles(IDataObject *dataObject)
{
	BookmarkItems bookmarkItems;
	auto droppedFiles = ExtractDroppedFilesList(dataObject);

	for (auto &droppedFile : droppedFiles)
	{
		if (!PathIsDirectory(droppedFile.c_str()))
		{
			continue;
		}

		TCHAR displayName[MAX_PATH];
		GetDisplayName(droppedFile.c_str(), displayName, SIZEOF_ARRAY(displayName), SHGDN_INFOLDER);

		auto bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt, displayName, droppedFile.c_str());
		bookmarkItems.push_back(std::move(bookmarkItem));
	}

	return bookmarkItems;
}

void CBookmarksToolbar::ResetToolbarState()
{
	RemoveInsertionMark();

	if (m_previousDropButton)
	{
		SetButtonPressedState(*m_previousDropButton, false);

		m_previousDropButton.reset();
	}
}

void CBookmarksToolbar::RemoveInsertionMark()
{
	TBINSERTMARK tbim;
	tbim.iButton = -1;
	SendMessage(m_hToolbar, TB_SETINSERTMARK, 0, reinterpret_cast<LPARAM>(&tbim));
}