// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkTreeView.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include <boost/range/adaptor/filtered.hpp>
#include <stack>

CBookmarkTreeView::CBookmarkTreeView(HWND hTreeView, HINSTANCE hInstance, IExplorerplusplus *expp,
	BookmarkTree *bookmarkTree, const std::unordered_set<std::wstring> &setExpansion,
	std::optional<std::wstring> guidSelected) :
	m_hTreeView(hTreeView),
	m_instance(hInstance),
	m_bookmarkTree(bookmarkTree),
	m_bNewFolderCreated(false)
{
	m_windowSubclasses.push_back(WindowSubclassWrapper(hTreeView, BookmarkTreeViewProcStub,
		SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));
	m_windowSubclasses.push_back(WindowSubclassWrapper(GetParent(hTreeView), BookmarkTreeViewParentProcStub,
		PARENT_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	SetWindowTheme(hTreeView, L"Explorer", NULL);

	UINT dpi = m_dpiCompat.GetDpiForWindow(hTreeView);
	int iconWidth = m_dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = m_dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	std::tie(m_imageList, m_imageListMappings) = ResourceHelper::CreateIconImageList(expp->GetIconResourceLoader(),
		iconWidth, iconHeight, { Icon::Folder});
	TreeView_SetImageList(hTreeView, m_imageList.get(), TVSIL_NORMAL);

	SetupTreeView(setExpansion, guidSelected);

	m_connections.push_back(m_bookmarkTree->bookmarkItemAddedSignal.AddObserver(
		std::bind(&CBookmarkTreeView::OnBookmarkItemAdded, this, std::placeholders::_1, std::placeholders::_2)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemUpdatedSignal.AddObserver(
		std::bind(&CBookmarkTreeView::OnBookmarkItemUpdated, this, std::placeholders::_1, std::placeholders::_2)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemRemovedSignal.AddObserver(
		std::bind(&CBookmarkTreeView::OnBookmarkItemRemoved, this, std::placeholders::_1)));
}

LRESULT CALLBACK CBookmarkTreeView::BookmarkTreeViewProcStub(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CBookmarkTreeView *pbtv = reinterpret_cast<CBookmarkTreeView *>(dwRefData);

	return pbtv->TreeViewProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CBookmarkTreeView::TreeViewProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) == 0)
		{
			switch (LOWORD(wParam))
			{
			case IDM_BOOKMARK_TREEVIEW_RLICK_RENAME:
				OnTreeViewRename();
				break;

			case IDM_BOOKMARK_TREEVIEW_RLICK_DELETE:
				OnDelete();
				break;

			case IDM_BOOKMARK_TREEVIEW_RLICK_NEW_FOLDER:
				CreateNewFolder();
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, Msg, wParam, lParam);
}

LRESULT CALLBACK CBookmarkTreeView::BookmarkTreeViewParentProcStub(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CBookmarkTreeView *pbtv = reinterpret_cast<CBookmarkTreeView *>(dwRefData);

	return pbtv->TreeViewParentProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CBookmarkTreeView::TreeViewParentProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR *>(lParam)->code)
		{
		case TVN_KEYDOWN:
			OnKeyDown(reinterpret_cast<NMTVKEYDOWN *>(lParam));
			break;

		case TVN_BEGINLABELEDIT:
			return OnBeginLabelEdit(reinterpret_cast<NMTVDISPINFO *>(lParam));
			break;

		case TVN_ENDLABELEDIT:
			return OnEndLabelEdit(reinterpret_cast<NMTVDISPINFO *>(lParam));
			break;

		case TVN_SELCHANGED:
			OnSelChanged(reinterpret_cast<NMTREEVIEW *>(lParam));
			break;

		case NM_RCLICK:
			OnRClick(reinterpret_cast<NMHDR *>(lParam));
			break;
		}
		break;
	}

	return DefSubclassProc(hwnd, Msg, wParam, lParam);
}

LRESULT CALLBACK CBookmarkTreeView::TreeViewEditProcStub(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CBookmarkTreeView *pbtv = reinterpret_cast<CBookmarkTreeView *>(dwRefData);

	return pbtv->TreeViewEditProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CBookmarkTreeView::TreeViewEditProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_GETDLGCODE:
		switch (wParam)
		{
		case VK_ESCAPE:
		case VK_RETURN:
			return DLGC_WANTALLKEYS;
			break;
		}
		break;
	}

	return DefSubclassProc(hwnd, Msg, wParam, lParam);
}

void CBookmarkTreeView::SetupTreeView(const std::unordered_set<std::wstring> &setExpansion,
	std::optional<std::wstring> guidSelected)
{
	TreeView_DeleteAllItems(m_hTreeView);

	InsertFoldersIntoTreeViewRecursive(nullptr, m_bookmarkTree->GetRoot());

	for(const auto &guidExpanded : setExpansion)
	{
		auto itrExpanded = m_mapItem.find(guidExpanded);

		if (itrExpanded != m_mapItem.end())
		{
			const auto bookmarkFolder = GetBookmarkFolderFromTreeView(itrExpanded->second);

			if (bookmarkFolder->HasChildFolder())
			{
				TreeView_Expand(m_hTreeView, itrExpanded->second, TVE_EXPAND);
			}
		}
	}

	if (guidSelected)
	{
		auto itrSelected = m_mapItem.find(*guidSelected);

		if (itrSelected != m_mapItem.end())
		{
			TreeView_SelectItem(m_hTreeView, itrSelected->second);
		}
	}
}

void CBookmarkTreeView::InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent, BookmarkItem *bookmarkItem)
{
	int position = 0;

	for (auto &child : bookmarkItem->GetChildren() | boost::adaptors::filtered(BookmarkHelper::IsFolder))
	{
		HTREEITEM hCurrentItem = InsertFolderIntoTreeView(hParent, child.get(), position);
		position++;

		InsertFoldersIntoTreeViewRecursive(hCurrentItem, child.get());
	}
}

HTREEITEM CBookmarkTreeView::InsertFolderIntoTreeView(HTREEITEM hParent, BookmarkItem *bookmarkFolder, int position)
{
	TCHAR szText[256];
	StringCchCopy(szText, SIZEOF_ARRAY(szText), bookmarkFolder->GetName().c_str());

	int nChildren = 0;

	if (bookmarkFolder->HasChildFolder())
	{
		nChildren = 1;
	}

	TVITEMEX tviex;
	tviex.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_CHILDREN | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tviex.pszText = szText;
	tviex.iImage = m_imageListMappings.at(Icon::Folder);
	tviex.iSelectedImage = m_imageListMappings.at(Icon::Folder);
	tviex.cChildren = nChildren;
	tviex.lParam = reinterpret_cast<LPARAM>(bookmarkFolder);

	HTREEITEM hInsertAfter;

	if (position == 0)
	{
		hInsertAfter = TVI_FIRST;
	}
	else
	{
		/* Find the item one *before* Position;
		the new item will then be inserted one
		place *after* this. */
		HTREEITEM hChild = TreeView_GetChild(m_hTreeView, hParent);

		for (int i = 0; i < (position - 1); i++)
		{
			HTREEITEM hNextSibling = TreeView_GetNextSibling(m_hTreeView, hChild);

			/* Only bookmark folders are inserted into the
			treeview, so it's possible that the specified position
			will be after the last child in the treeview. */
			if (hNextSibling == NULL)
			{
				break;
			}

			hChild = hNextSibling;
		}

		hInsertAfter = hChild;
	}

	TVINSERTSTRUCT tvis;
	tvis.hParent = hParent;
	tvis.hInsertAfter = hInsertAfter;
	tvis.itemex = tviex;
	HTREEITEM hItem = TreeView_InsertItem(m_hTreeView, &tvis);

	m_mapItem.insert(std::make_pair(bookmarkFolder->GetGUID(), hItem));

	return hItem;
}

void CBookmarkTreeView::OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index)
{
	if (!bookmarkItem.IsFolder())
	{
		return;
	}

	/* Due to the fact that *all* bookmark folders will be inserted into the
	 * treeview (regardless of whether or not they are actually shown), any new
	 * folders will always need to be inserted. */
	auto itr = m_mapItem.find(bookmarkItem.GetParent()->GetGUID());
	assert(itr != m_mapItem.end());
	HTREEITEM hItem = InsertFolderIntoTreeView(itr->second, &bookmarkItem, static_cast<int>(index));

	UINT uParentState = TreeView_GetItemState(m_hTreeView, itr->second, TVIS_EXPANDED);

	if ((uParentState & TVIS_EXPANDED) != TVIS_EXPANDED)
	{
		TVITEM tvi;
		tvi.mask = TVIF_CHILDREN;
		tvi.hItem = itr->second;
		tvi.cChildren = 1;
		TreeView_SetItem(m_hTreeView, &tvi);
	}

	if (m_bNewFolderCreated && bookmarkItem.GetGUID() == m_NewFolderGUID)
	{
		SetFocus(m_hTreeView);
		TreeView_SelectItem(m_hTreeView, hItem);
		TreeView_EditLabel(m_hTreeView, hItem);

		m_bNewFolderCreated = false;
	}
}

void CBookmarkTreeView::OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType)
{
	if (!bookmarkItem.IsFolder() || propertyType != BookmarkItem::PropertyType::Name)
	{
		return;
	}

	auto itr = m_mapItem.find(bookmarkItem.GetGUID());
	assert(itr != m_mapItem.end());

	TCHAR name[256];
	StringCchCopy(name, SIZEOF_ARRAY(name), bookmarkItem.GetName().c_str());

	TVITEM tvi;
	tvi.mask = TVIF_TEXT;
	tvi.hItem = itr->second;
	tvi.pszText = name;
	TreeView_SetItem(m_hTreeView, &tvi);
}

void CBookmarkTreeView::OnBookmarkItemRemoved(const std::wstring &guid)
{
	auto itr = m_mapItem.find(guid);
	assert(itr != m_mapItem.end());

	/* TODO: Should collapse parent if it no longer
	has any children. Should also change selection if
	required (i.e. if the deleted bookmark was selected). */
	TreeView_DeleteItem(m_hTreeView, itr->second);

	m_mapItem.erase(itr);
}

void CBookmarkTreeView::OnKeyDown(const NMTVKEYDOWN *pnmtvkd)
{
	switch (pnmtvkd->wVKey)
	{
	case VK_F2:
		OnTreeViewRename();
		break;
	}
}

void CBookmarkTreeView::OnTreeViewRename()
{
	HTREEITEM hSelectedItem = TreeView_GetSelection(m_hTreeView);
	TreeView_EditLabel(m_hTreeView, hSelectedItem);
}

BOOL CBookmarkTreeView::OnBeginLabelEdit(const NMTVDISPINFO *dispInfo)
{
	auto bookmarkFolder = GetBookmarkFolderFromTreeView(dispInfo->item.hItem);

	if (m_bookmarkTree->IsPermanentNode(bookmarkFolder))
	{
		return TRUE;
	}

	HWND hEdit = TreeView_GetEditControl(m_hTreeView);
	SetWindowSubclass(hEdit, TreeViewEditProcStub, 0, reinterpret_cast<DWORD_PTR>(this));

	return FALSE;
}

BOOL CBookmarkTreeView::OnEndLabelEdit(const NMTVDISPINFO *dispInfo)
{
	HWND hEdit = TreeView_GetEditControl(m_hTreeView);
	RemoveWindowSubclass(hEdit, TreeViewEditProcStub, 0);

	if (dispInfo->item.pszText != NULL &&
		lstrlen(dispInfo->item.pszText) > 0)
	{
		auto bookmarkFolder = GetBookmarkFolderFromTreeView(dispInfo->item.hItem);
		bookmarkFolder->SetName(dispInfo->item.pszText);

		return TRUE;
	}

	return FALSE;
}

void CBookmarkTreeView::OnSelChanged(const NMTREEVIEW *treeView)
{
	auto bookmarkFolder = GetBookmarkFolderFromTreeView(treeView->itemNew.hItem);
	selectionChangedSignal.m_signal(bookmarkFolder);
}

void CBookmarkTreeView::OnDelete()
{
	HTREEITEM hSelectedItem = TreeView_GetSelection(m_hTreeView);
	auto bookmarkFolder = GetBookmarkFolderFromTreeView(hSelectedItem);

	m_bookmarkTree->RemoveBookmarkItem(bookmarkFolder);
}

void CBookmarkTreeView::OnRClick(const NMHDR *pnmhdr)
{
	if (pnmhdr->hwndFrom != m_hTreeView)
	{
		return;
	}

	DWORD dwCursorPos = GetMessagePos();

	POINT ptCursor;
	ptCursor.x = GET_X_LPARAM(dwCursorPos);
	ptCursor.y = GET_Y_LPARAM(dwCursorPos);

	TVHITTESTINFO tvhti;
	tvhti.pt = ptCursor;
	ScreenToClient(m_hTreeView, &tvhti.pt);
	HTREEITEM hItem = TreeView_HitTest(m_hTreeView, &tvhti);

	if (hItem == nullptr)
	{
		return;
	}

	TreeView_SelectItem(m_hTreeView, hItem);

	auto bookmarkFolder = GetBookmarkFolderFromTreeView(hItem);

	wil::unique_hmenu menu(LoadMenu(m_instance, MAKEINTRESOURCE(IDR_BOOKMARK_TREEVIEW_RCLICK_MENU)));

	lEnableMenuItem(menu.get(), IDM_BOOKMARK_TREEVIEW_RLICK_RENAME, !m_bookmarkTree->IsPermanentNode(bookmarkFolder));
	lEnableMenuItem(menu.get(), IDM_BOOKMARK_TREEVIEW_RLICK_DELETE, !m_bookmarkTree->IsPermanentNode(bookmarkFolder));

	TrackPopupMenu(GetSubMenu(menu.get(), 0), TPM_LEFTALIGN, ptCursor.x, ptCursor.y, 0, m_hTreeView, NULL);
}

void CBookmarkTreeView::CreateNewFolder()
{
	std::wstring newBookmarkFolderName = ResourceHelper::LoadString(m_instance, IDS_BOOKMARKS_NEWBOOKMARKFOLDER);
	auto newBookmarkFolder = std::make_unique<BookmarkItem>(std::nullopt, newBookmarkFolderName, std::nullopt);

	m_bNewFolderCreated = true;
	m_NewFolderGUID = newBookmarkFolder->GetGUID();

	HTREEITEM hSelectedItem = TreeView_GetSelection(m_hTreeView);

	assert(hSelectedItem != NULL);

	auto bookmarkFolder = GetBookmarkFolderFromTreeView(hSelectedItem);
	m_bookmarkTree->AddBookmarkItem(bookmarkFolder, std::move(newBookmarkFolder), bookmarkFolder->GetChildren().size());
}

void CBookmarkTreeView::SelectFolder(const std::wstring &guid)
{
	auto itr = m_mapItem.find(guid);

	assert(itr != m_mapItem.end());

	TreeView_SelectItem(m_hTreeView, itr->second);
}

BookmarkItem *CBookmarkTreeView::GetBookmarkFolderFromTreeView(HTREEITEM hItem)
{
	TVITEM tvi;
	tvi.mask = TVIF_HANDLE | TVIF_PARAM;
	tvi.hItem = hItem;
	TreeView_GetItem(m_hTreeView, &tvi);

	return reinterpret_cast<BookmarkItem *>(tvi.lParam);
}