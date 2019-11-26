// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkTreeView.h"
#include "MainResource.h"
#include "../Helper/Macros.h"
#include <stack>

CBookmarkTreeView::CBookmarkTreeView(HWND hTreeView, HINSTANCE hInstance,
	IExplorerplusplus *expp, CBookmarkFolder *pAllBookmarks, const GUID &guidSelected,
	const NBookmarkHelper::setExpansion_t &setExpansion) :
	m_hTreeView(hTreeView),
	m_instance(hInstance),
	m_pAllBookmarks(pAllBookmarks),
	m_uIDCounter(0),
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

	SetupTreeView(guidSelected, setExpansion);
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
				/* TODO: Handle menu item. */
				break;

			case IDM_BOOKMARK_TREEVIEW_RLICK_NEW_FOLDER:
				CreateNewFolder();
				break;
			}
		}
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR *>(lParam)->code)
		{
		case TVN_DELETEITEM:
			OnTvnDeleteItem(reinterpret_cast<NMTREEVIEW *>(lParam));
			break;
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
			OnTvnKeyDown(reinterpret_cast<NMTVKEYDOWN *>(lParam));
			break;

		case TVN_BEGINLABELEDIT:
			OnTvnBeginLabelEdit();
			break;

		case TVN_ENDLABELEDIT:
			return OnTvnEndLabelEdit(reinterpret_cast<NMTVDISPINFO *>(lParam));
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

void CBookmarkTreeView::SetupTreeView(const GUID &guidSelected, const NBookmarkHelper::setExpansion_t &setExpansion)
{
	TreeView_DeleteAllItems(m_hTreeView);

	HTREEITEM hRoot = InsertFolderIntoTreeView(NULL, *m_pAllBookmarks, 0);
	InsertFoldersIntoTreeViewRecursive(hRoot, *m_pAllBookmarks);

	for(const auto &guidExpanded : setExpansion)
	{
		auto itrExpanded = m_mapItem.find(guidExpanded);

		if (itrExpanded != m_mapItem.end())
		{
			CBookmarkFolder &BookmarkFolder = GetBookmarkFolderFromTreeView(itrExpanded->second);

			if (BookmarkFolder.HasChildFolder())
			{
				TreeView_Expand(m_hTreeView, itrExpanded->second, TVE_EXPAND);
			}
		}
	}

	auto itrSelected = m_mapItem.find(guidSelected);

	if (itrSelected != m_mapItem.end())
	{
		TreeView_SelectItem(m_hTreeView, itrSelected->second);
	}
}

void CBookmarkTreeView::InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent, const CBookmarkFolder &BookmarkFolder)
{
	std::size_t Position = 0;

	for (auto itr = BookmarkFolder.begin(); itr != BookmarkFolder.end(); ++itr)
	{
		if (itr->type() == typeid(CBookmarkFolder))
		{
			const CBookmarkFolder &BookmarkFolderChild = boost::get<CBookmarkFolder>(*itr);

			HTREEITEM hCurrentItem = InsertFolderIntoTreeView(hParent,
				BookmarkFolderChild, Position);
			Position++;

			if (BookmarkFolderChild.HasChildFolder())
			{
				InsertFoldersIntoTreeViewRecursive(hCurrentItem,
					BookmarkFolderChild);
			}
		}
	}
}

HTREEITEM CBookmarkTreeView::InsertFolderIntoTreeView(HTREEITEM hParent, const CBookmarkFolder &BookmarkFolder, std::size_t Position)
{
	TCHAR szText[256];
	StringCchCopy(szText, SIZEOF_ARRAY(szText), BookmarkFolder.GetName().c_str());

	int nChildren = 0;

	if (BookmarkFolder.HasChildFolder())
	{
		nChildren = 1;
	}

	TVITEMEX tviex;
	tviex.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_CHILDREN | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tviex.pszText = szText;
	tviex.iImage = m_imageListMappings.at(Icon::Folder);
	tviex.iSelectedImage = m_imageListMappings.at(Icon::Folder);
	tviex.cChildren = nChildren;
	tviex.lParam = m_uIDCounter;

	HTREEITEM hInsertAfter;

	if (Position == 0)
	{
		hInsertAfter = TVI_FIRST;
	}
	else
	{
		/* Find the item one *before* Position;
		the new item will then be inserted one
		place *after* this. */
		HTREEITEM hChild = TreeView_GetChild(m_hTreeView, hParent);

		for (std::size_t i = 0; i < (Position - 1); i++)
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

	m_mapID.insert(std::make_pair(m_uIDCounter, BookmarkFolder.GetGUID()));
	++m_uIDCounter;

	m_mapItem.insert(std::make_pair(BookmarkFolder.GetGUID(), hItem));

	return hItem;
}

HTREEITEM CBookmarkTreeView::BookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmarkFolder &BookmarkFolder, std::size_t Position)
{
	/* Due to the fact that *all* bookmark folders will be inserted
	into the treeview (regardless of whether or not they are actually
	shown), any new folders will always need to be inserted. */
	auto itr = m_mapItem.find(ParentBookmarkFolder.GetGUID());
	assert(itr != m_mapItem.end());
	HTREEITEM hItem = InsertFolderIntoTreeView(itr->second, BookmarkFolder, Position);

	UINT uParentState = TreeView_GetItemState(m_hTreeView, itr->second, TVIS_EXPANDED);

	if ((uParentState & TVIS_EXPANDED) != TVIS_EXPANDED)
	{
		TVITEM tvi;
		tvi.mask = TVIF_CHILDREN;
		tvi.hItem = itr->second;
		tvi.cChildren = 1;
		TreeView_SetItem(m_hTreeView, &tvi);
	}

	if (m_bNewFolderCreated &&
		IsEqualGUID(BookmarkFolder.GetGUID(), m_NewFolderGUID))
	{
		/* If a new folder has been created, it will be selected,
		as it is assumed that the user intends to place any
		new bookmark within that folder. */
		SetFocus(m_hTreeView);
		TreeView_SelectItem(m_hTreeView, hItem);
		TreeView_EditLabel(m_hTreeView, hItem);

		m_bNewFolderCreated = false;
	}

	return hItem;
}

void CBookmarkTreeView::BookmarkFolderModified(const GUID &guid)
{
	auto itr = m_mapItem.find(guid);
	assert(itr != m_mapItem.end());

	CBookmarkFolder &BookmarkFolder = GetBookmarkFolderFromTreeView(itr->second);

	TCHAR szText[256];
	StringCchCopy(szText, SIZEOF_ARRAY(szText), BookmarkFolder.GetName().c_str());

	/* The only property of the bookmark folder shown
	within the treeview is its name, so that is all
	that needs to be updated here. */
	TVITEM tvi;
	tvi.mask = TVIF_TEXT;
	tvi.hItem = itr->second;
	tvi.pszText = szText;
	TreeView_SetItem(m_hTreeView, &tvi);
}

void CBookmarkTreeView::BookmarkFolderRemoved(const GUID &guid)
{
	auto itr = m_mapItem.find(guid);
	assert(itr != m_mapItem.end());

	/* TODO: Should collapse parent if it no longer
	has any children. Should also change selection if
	required (i.e. if the deleted bookmark was selected). */
	TreeView_DeleteItem(m_hTreeView, itr->second);
}

void CBookmarkTreeView::OnTvnKeyDown(NMTVKEYDOWN *pnmtvkd)
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

void CBookmarkTreeView::OnTvnBeginLabelEdit()
{
	HWND hEdit = TreeView_GetEditControl(m_hTreeView);
	SetWindowSubclass(hEdit, TreeViewEditProcStub, 0, reinterpret_cast<DWORD_PTR>(this));
}

BOOL CBookmarkTreeView::OnTvnEndLabelEdit(NMTVDISPINFO *pnmtvdi)
{
	HWND hEdit = TreeView_GetEditControl(m_hTreeView);
	RemoveWindowSubclass(hEdit, TreeViewEditProcStub, 0);

	if (pnmtvdi->item.pszText != NULL &&
		lstrlen(pnmtvdi->item.pszText) > 0)
	{
		CBookmarkFolder &BookmarkFolder = GetBookmarkFolderFromTreeView(pnmtvdi->item.hItem);
		BookmarkFolder.SetName(pnmtvdi->item.pszText);

		return TRUE;
	}

	return FALSE;
}

void CBookmarkTreeView::OnRClick(NMHDR *pnmhdr)
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

	if (hItem != NULL)
	{
		TreeView_SelectItem(m_hTreeView, hItem);

		HMENU hMenu = LoadMenu(m_instance, MAKEINTRESOURCE(IDR_BOOKMARK_TREEVIEW_RCLICK_MENU));
		TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_LEFTALIGN, ptCursor.x, ptCursor.y, 0, m_hTreeView, NULL);
		DestroyMenu(hMenu);
	}
}

void CBookmarkTreeView::OnTvnDeleteItem(NMTREEVIEW *pnmtv)
{
	auto itrID = m_mapID.find(static_cast<UINT>(pnmtv->itemOld.lParam));

	if (itrID == m_mapID.end())
	{
		assert(false);
	}

	auto itrItem = m_mapItem.find(itrID->second);

	if (itrItem == m_mapItem.end())
	{
		assert(false);
	}

	m_mapItem.erase(itrItem);
	m_mapID.erase(itrID);
}

void CBookmarkTreeView::CreateNewFolder()
{
	TCHAR szTemp[64];
	LoadString(m_instance, IDS_BOOKMARKS_NEWBOOKMARKFOLDER, szTemp, SIZEOF_ARRAY(szTemp));
	CBookmarkFolder NewBookmarkFolder = CBookmarkFolder::Create(szTemp);

	m_bNewFolderCreated = true;
	m_NewFolderGUID = NewBookmarkFolder.GetGUID();

	HTREEITEM hSelectedItem = TreeView_GetSelection(m_hTreeView);

	assert(hSelectedItem != NULL);

	CBookmarkFolder &ParentBookmarkFolder = GetBookmarkFolderFromTreeView(hSelectedItem);
	ParentBookmarkFolder.InsertBookmarkFolder(NewBookmarkFolder);
}

void CBookmarkTreeView::SelectFolder(const GUID &guid)
{
	auto itr = m_mapItem.find(guid);

	assert(itr != m_mapItem.end());

	TreeView_SelectItem(m_hTreeView, itr->second);
}

CBookmarkFolder &CBookmarkTreeView::GetBookmarkFolderFromTreeView(HTREEITEM hItem)
{
	TVITEM tvi;
	tvi.mask = TVIF_HANDLE | TVIF_PARAM;
	tvi.hItem = hItem;
	TreeView_GetItem(m_hTreeView, &tvi);

	std::stack<UINT> stackIDs;
	HTREEITEM hParent;
	HTREEITEM hCurrentItem = hItem;

	while ((hParent = TreeView_GetParent(m_hTreeView, hCurrentItem)) != NULL)
	{
		tvi.mask = TVIF_HANDLE | TVIF_PARAM;
		tvi.hItem = hCurrentItem;
		TreeView_GetItem(m_hTreeView, &tvi);

		stackIDs.push(static_cast<UINT>(tvi.lParam));

		hCurrentItem = hParent;
	}

	CBookmarkFolder *pBookmarkFolder = m_pAllBookmarks;

	while (!stackIDs.empty())
	{
		UINT uID = stackIDs.top();
		auto itr = m_mapID.find(uID);

		VariantBookmark &variantBookmark = NBookmarkHelper::GetBookmarkItem(*pBookmarkFolder, itr->second);
		pBookmarkFolder = boost::get<CBookmarkFolder>(&variantBookmark);

		stackIDs.pop();
	}

	return *pBookmarkFolder;
}