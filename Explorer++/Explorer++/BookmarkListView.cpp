// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkListView.h"
#include "MainResource.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"

CBookmarkListView::CBookmarkListView(HWND hListView, HMODULE resourceModule,
	BookmarkTree *bookmarkTree, IExplorerplusplus *expp) :
	m_hListView(hListView),
	m_resourceModule(resourceModule),
	m_bookmarkTree(bookmarkTree)
{
	SetWindowTheme(hListView, L"Explorer", NULL);
	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT);

	UINT dpi = m_dpiCompat.GetDpiForWindow(hListView);
	int iconWidth = m_dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = m_dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	std::tie(m_imageList, m_imageListMappings) = ResourceHelper::CreateIconImageList(expp->GetIconResourceLoader(),
		iconWidth, iconHeight, {Icon::Folder, Icon::Bookmarks});
	ListView_SetImageList(hListView, m_imageList.get(), LVSIL_SMALL);

	m_windowSubclasses.push_back(WindowSubclassWrapper(GetParent(m_hListView), ParentWndProcStub,
		PARENT_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));
}

LRESULT CALLBACK CBookmarkListView::ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CBookmarkListView *listView = reinterpret_cast<CBookmarkListView *>(dwRefData);
	return listView->ParentWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CBookmarkListView::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hListView)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_RCLICK:
				OnRClick(reinterpret_cast<NMITEMACTIVATE *>(lParam));
				return TRUE;
				break;

			case LVN_BEGINLABELEDIT:
				return OnBeginLabelEdit(reinterpret_cast<NMLVDISPINFO *>(lParam));
				break;

			case LVN_ENDLABELEDIT:
				return OnEndLabelEdit(reinterpret_cast<NMLVDISPINFO *>(lParam));
				break;

			case LVN_KEYDOWN:
				OnKeyDown(reinterpret_cast<NMLVKEYDOWN *>(lParam));
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void CBookmarkListView::NavigateToBookmarkFolder(BookmarkItem *bookmarkItem)
{
	assert(bookmarkItem->IsFolder());

	m_currentBookmarkFolder = bookmarkItem;

	ListView_DeleteAllItems(m_hListView);

	int position = 0;

	for (auto &childItem : bookmarkItem->GetChildren())
	{
		InsertBookmarkItemIntoListView(childItem.get(), position);

		position++;
	}
}

int CBookmarkListView::InsertBookmarkItemIntoListView(BookmarkItem *bookmarkItem, int position)
{
	assert(position >= 0 && position <= ListView_GetItemCount(m_hListView));

	TCHAR szName[256];
	StringCchCopy(szName, SIZEOF_ARRAY(szName), bookmarkItem->GetName().c_str());

	int iImage;

	if (bookmarkItem->GetType() == BookmarkItem::Type::Folder)
	{
		iImage = m_imageListMappings.at(Icon::Folder);
	}
	else
	{
		iImage = m_imageListMappings.at(Icon::Bookmarks);
	}

	LVITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iItem = position;
	lvi.iSubItem = 0;
	lvi.iImage = iImage;
	lvi.pszText = szName;
	lvi.lParam = reinterpret_cast<LPARAM>(bookmarkItem);
	int iItem = ListView_InsertItem(m_hListView, &lvi);

	return iItem;
}

BookmarkItem *CBookmarkListView::GetBookmarkItemFromListView(int iItem)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = iItem;
	lvi.iSubItem = 0;
	ListView_GetItem(m_hListView, &lvi);

	return reinterpret_cast<BookmarkItem *>(lvi.lParam);
}

BookmarkItem *CBookmarkListView::GetBookmarkItemFromListViewlParam(LPARAM lParam)
{
	return reinterpret_cast<BookmarkItem *>(lParam);
}

void CBookmarkListView::OnRClick(const NMITEMACTIVATE *itemActivate)
{
	HMENU hMenu = LoadMenu(m_resourceModule, MAKEINTRESOURCE(IDR_MANAGEBOOKMARKS_BOOKMARK_RCLICK_MENU));
	SetMenuDefaultItem(GetSubMenu(hMenu, 0), IDM_MB_BOOKMARK_OPEN, FALSE);

	POINT pt = itemActivate->ptAction;
	ClientToScreen(m_hListView, &pt);

	TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_LEFTALIGN, pt.x, pt.y, 0, m_hListView, NULL);
	DestroyMenu(hMenu);
}

BOOL CBookmarkListView::OnBeginLabelEdit(const NMLVDISPINFO *dispInfo)
{
	auto bookmarkItem = GetBookmarkItemFromListView(dispInfo->item.iItem);

	if (m_bookmarkTree->IsPermanentNode(bookmarkItem))
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CBookmarkListView::OnEndLabelEdit(const NMLVDISPINFO *dispInfo)
{
	if (dispInfo->item.pszText == nullptr && lstrlen(dispInfo->item.pszText) == 0)
	{
		return FALSE;
	}

	auto bookmarkItem = GetBookmarkItemFromListView(dispInfo->item.iItem);

	if (m_bookmarkTree->IsPermanentNode(bookmarkItem))
	{
		return FALSE;
	}

	bookmarkItem->SetName(dispInfo->item.pszText);

	return TRUE;
}

void CBookmarkListView::OnKeyDown(const NMLVKEYDOWN *keyDown)
{
	switch (keyDown->wVKey)
	{
	case VK_F2:
		OnRename();
		break;

	case 'A':
		if (IsKeyDown(VK_CONTROL) &&
			!IsKeyDown(VK_SHIFT) &&
			!IsKeyDown(VK_MENU))
		{
			NListView::ListView_SelectAllItems(m_hListView, TRUE);
		}
		break;

	/* TODO: */
	case VK_RETURN:
		break;

	case VK_DELETE:
		break;
	}
}

void CBookmarkListView::OnRename()
{
	int item = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);

	if (item != -1)
	{
		ListView_EditLabel(m_hListView, item);
	}
}