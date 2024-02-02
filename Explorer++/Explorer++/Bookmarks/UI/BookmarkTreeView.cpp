// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkTreeView.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkTree.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/DropSourceImpl.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowHelper.h"
#include <boost/range/adaptor/filtered.hpp>
#include <glog/logging.h>
#include <wil/com.h>

BookmarkTreeView::BookmarkTreeView(HWND hTreeView, HINSTANCE resourceInstance,
	CoreInterface *coreInterface, BookmarkTree *bookmarkTree,
	const std::unordered_set<std::wstring> &setExpansion,
	std::optional<std::wstring> guidSelected) :
	BookmarkDropTargetWindow(hTreeView, bookmarkTree),
	m_hTreeView(hTreeView),
	m_resourceInstance(resourceInstance),
	m_bookmarkTree(bookmarkTree),
	m_bNewFolderCreated(false)
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(hTreeView,
		std::bind_front(&BookmarkTreeView::TreeViewProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(GetParent(hTreeView),
		std::bind_front(&BookmarkTreeView::TreeViewParentProc, this)));

	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(hTreeView);
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	std::tie(m_imageList, m_imageListMappings) = ResourceHelper::CreateIconImageList(
		coreInterface->GetIconResourceLoader(), iconWidth, iconHeight, { Icon::Folder });
	TreeView_SetImageList(hTreeView, m_imageList.get(), TVSIL_NORMAL);

	SetupTreeView(setExpansion, guidSelected);

	m_connections.push_back(m_bookmarkTree->bookmarkItemAddedSignal.AddObserver(
		std::bind_front(&BookmarkTreeView::OnBookmarkItemAdded, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemUpdatedSignal.AddObserver(
		std::bind_front(&BookmarkTreeView::OnBookmarkItemUpdated, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemMovedSignal.AddObserver(
		std::bind_front(&BookmarkTreeView::OnBookmarkItemMoved, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemPreRemovalSignal.AddObserver(
		std::bind_front(&BookmarkTreeView::OnBookmarkItemPreRemoval, this)));
}

LRESULT BookmarkTreeView::TreeViewProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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
				DeleteSelection();
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

LRESULT BookmarkTreeView::TreeViewParentProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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

		case TVN_ENDLABELEDIT:
			return OnEndLabelEdit(reinterpret_cast<NMTVDISPINFO *>(lParam));

		case TVN_SELCHANGED:
			OnSelChanged(reinterpret_cast<NMTREEVIEW *>(lParam));
			break;

		case TVN_BEGINDRAG:
			OnBeginDrag(reinterpret_cast<NMTREEVIEW *>(lParam));
			break;

		case NM_RCLICK:
			OnRClick(reinterpret_cast<NMHDR *>(lParam));
			break;
		}
		break;
	}

	return DefSubclassProc(hwnd, Msg, wParam, lParam);
}

LRESULT CALLBACK BookmarkTreeView::TreeViewEditProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pbtv = reinterpret_cast<BookmarkTreeView *>(dwRefData);

	return pbtv->TreeViewEditProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK BookmarkTreeView::TreeViewEditProc(HWND hwnd, UINT Msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (Msg)
	{
	case WM_GETDLGCODE:
		switch (wParam)
		{
		case VK_ESCAPE:
		case VK_RETURN:
			return DLGC_WANTALLKEYS;
		}
		break;
	}

	return DefSubclassProc(hwnd, Msg, wParam, lParam);
}

void BookmarkTreeView::SetupTreeView(const std::unordered_set<std::wstring> &setExpansion,
	std::optional<std::wstring> guidSelected)
{
	TreeView_DeleteAllItems(m_hTreeView);

	InsertFoldersIntoTreeViewRecursive(nullptr, m_bookmarkTree->GetRoot());

	for (const auto &guidExpanded : setExpansion)
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

void BookmarkTreeView::InsertFoldersIntoTreeViewRecursive(HTREEITEM hParent,
	BookmarkItem *bookmarkItem)
{
	int position = 0;

	for (auto &child :
		bookmarkItem->GetChildren() | boost::adaptors::filtered(BookmarkHelper::IsFolder))
	{
		HTREEITEM hCurrentItem = InsertFolderIntoTreeView(hParent, child.get(), position);
		position++;

		InsertFoldersIntoTreeViewRecursive(hCurrentItem, child.get());
	}
}

HTREEITEM BookmarkTreeView::InsertFolderIntoTreeView(HTREEITEM hParent,
	BookmarkItem *bookmarkFolder, int position)
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
		auto hChild = TreeView_GetChild(m_hTreeView, hParent);

		for (int i = 0; i < (position - 1); i++)
		{
			auto hNextSibling = TreeView_GetNextSibling(m_hTreeView, hChild);

			/* Only bookmark folders are inserted into the
			treeview, so it's possible that the specified position
			will be after the last child in the treeview. */
			if (hNextSibling == nullptr)
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
	auto hItem = TreeView_InsertItem(m_hTreeView, &tvis);

	m_mapItem.insert(std::make_pair(bookmarkFolder->GetGUID(), hItem));

	return hItem;
}

void BookmarkTreeView::OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index)
{
	UNREFERENCED_PARAMETER(index);

	if (!bookmarkItem.IsFolder())
	{
		return;
	}

	// Due to the fact that *all* bookmark folders will be inserted into the
	// treeview (regardless of whether or not they are actually shown), any new
	// folders will always need to be inserted.
	HTREEITEM newItem = AddNewFolderToTreeView(&bookmarkItem);

	if (m_bNewFolderCreated && bookmarkItem.GetGUID() == m_NewFolderGUID)
	{
		SetFocus(m_hTreeView);
		TreeView_SelectItem(m_hTreeView, newItem);
		TreeView_EditLabel(m_hTreeView, newItem);

		m_bNewFolderCreated = false;
	}
}

void BookmarkTreeView::OnBookmarkItemUpdated(BookmarkItem &bookmarkItem,
	BookmarkItem::PropertyType propertyType)
{
	if (!bookmarkItem.IsFolder() || propertyType != BookmarkItem::PropertyType::Name)
	{
		return;
	}

	auto itr = m_mapItem.find(bookmarkItem.GetGUID());
	CHECK(itr != m_mapItem.end());

	TCHAR name[256];
	StringCchCopy(name, SIZEOF_ARRAY(name), bookmarkItem.GetName().c_str());

	TVITEM tvi;
	tvi.mask = TVIF_TEXT;
	tvi.hItem = itr->second;
	tvi.pszText = name;
	TreeView_SetItem(m_hTreeView, &tvi);
}

void BookmarkTreeView::OnBookmarkItemMoved(BookmarkItem *bookmarkItem,
	const BookmarkItem *oldParent, size_t oldIndex, const BookmarkItem *newParent, size_t newIndex)
{
	UNREFERENCED_PARAMETER(oldParent);
	UNREFERENCED_PARAMETER(oldIndex);
	UNREFERENCED_PARAMETER(newParent);
	UNREFERENCED_PARAMETER(newIndex);

	if (!bookmarkItem->IsFolder())
	{
		return;
	}

	RemoveBookmarkItem(bookmarkItem);
	AddNewFolderToTreeView(bookmarkItem);
}

HTREEITEM BookmarkTreeView::AddNewFolderToTreeView(BookmarkItem *bookmarkFolder)
{
	auto parentItr = m_mapItem.find(bookmarkFolder->GetParent()->GetGUID());
	CHECK(parentItr != m_mapItem.end());

	size_t relativeIndex = GetFolderRelativeIndex(bookmarkFolder);

	HTREEITEM newItem = InsertFolderIntoTreeView(parentItr->second, bookmarkFolder,
		static_cast<int>(relativeIndex));

	if (newItem)
	{
		TVITEM item;
		item.mask = TVIF_CHILDREN;
		item.hItem = parentItr->second;
		item.cChildren = 1;
		TreeView_SetItem(m_hTreeView, &item);
	}

	return newItem;
}

// As a bookmark folder can contain both bookmark folders as well as bookmarks
// (as children), the index of a bookmark folder won't necessarily match the
// index of the folder in the treeview. This is because the treeview only
// contains bookmark folders.
// Therefore, this function returns the index of a folder, as it should appear
// in the treeview (i.e. it only takes into account other bookmark folders).
size_t BookmarkTreeView::GetFolderRelativeIndex(BookmarkItem *bookmarkFolder) const
{
	DCHECK(bookmarkFolder->IsFolder());

	size_t index = bookmarkFolder->GetParent()->GetChildIndex(bookmarkFolder);
	auto &children = bookmarkFolder->GetParent()->GetChildren();
	auto itr = children.begin() + index;

	auto numPreviousFolders =
		std::count_if(children.begin(), itr, [](auto &child) { return child->IsFolder(); });

	return numPreviousFolders;
}

void BookmarkTreeView::OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem)
{
	if (!bookmarkItem.IsFolder())
	{
		return;
	}

	RemoveBookmarkItem(&bookmarkItem);
}

void BookmarkTreeView::RemoveBookmarkItem(const BookmarkItem *bookmarkItem)
{
	auto itr = m_mapItem.find(bookmarkItem->GetGUID());
	CHECK(itr != m_mapItem.end());

	TreeView_DeleteItem(m_hTreeView, itr->second);

	auto parentItr = m_mapItem.find(bookmarkItem->GetParent()->GetGUID());
	CHECK(parentItr != m_mapItem.end());

	auto firstChild = TreeView_GetChild(m_hTreeView, parentItr->second);

	if (!firstChild)
	{
		TreeView_Expand(m_hTreeView, parentItr->second, TVE_COLLAPSE);

		TVITEM item;
		item.mask = TVIF_CHILDREN;
		item.hItem = parentItr->second;
		item.cChildren = 0;
		TreeView_SetItem(m_hTreeView, &item);
	}

	m_mapItem.erase(itr);
}

void BookmarkTreeView::OnKeyDown(const NMTVKEYDOWN *pnmtvkd)
{
	switch (pnmtvkd->wVKey)
	{
	case VK_F2:
		OnTreeViewRename();
		break;

	case VK_DELETE:
		DeleteSelection();
		break;
	}
}

void BookmarkTreeView::OnTreeViewRename()
{
	auto hSelectedItem = TreeView_GetSelection(m_hTreeView);
	TreeView_EditLabel(m_hTreeView, hSelectedItem);
}

BOOL BookmarkTreeView::OnBeginLabelEdit(const NMTVDISPINFO *dispInfo)
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

BOOL BookmarkTreeView::OnEndLabelEdit(const NMTVDISPINFO *dispInfo)
{
	HWND hEdit = TreeView_GetEditControl(m_hTreeView);
	RemoveWindowSubclass(hEdit, TreeViewEditProcStub, 0);

	if (dispInfo->item.pszText != nullptr && lstrlen(dispInfo->item.pszText) > 0)
	{
		auto bookmarkFolder = GetBookmarkFolderFromTreeView(dispInfo->item.hItem);
		bookmarkFolder->SetName(dispInfo->item.pszText);

		return TRUE;
	}

	return FALSE;
}

void BookmarkTreeView::OnSelChanged(const NMTREEVIEW *treeView)
{
	auto bookmarkFolder = GetBookmarkFolderFromTreeView(treeView->itemNew.hItem);
	selectionChangedSignal.m_signal(bookmarkFolder);
}

void BookmarkTreeView::OnBeginDrag(const NMTREEVIEW *treeView)
{
	auto bookmarkFolder = GetBookmarkFolderFromTreeView(treeView->itemNew.hItem);

	if (m_bookmarkTree->IsPermanentNode(bookmarkFolder))
	{
		return;
	}

	auto dropSource = winrt::make_self<DropSourceImpl>();

	auto &ownedPtr = bookmarkFolder->GetParent()->GetChildOwnedPtr(bookmarkFolder);
	auto dataObject = BookmarkDataExchange::CreateDataObject({ ownedPtr });

	wil::com_ptr_nothrow<IDragSourceHelper> dragSourceHelper;
	HRESULT hr = CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_ALL,
		IID_PPV_ARGS(&dragSourceHelper));

	if (SUCCEEDED(hr))
	{
		dragSourceHelper->InitializeFromWindow(m_hTreeView, nullptr, dataObject.get());
	}

	DWORD effect;
	DoDragDrop(dataObject.get(), dropSource.get(), DROPEFFECT_MOVE, &effect);
}

bool BookmarkTreeView::CanDelete()
{
	auto hSelectedItem = TreeView_GetSelection(m_hTreeView);
	auto bookmarkFolder = GetBookmarkFolderFromTreeView(hSelectedItem);

	return !m_bookmarkTree->IsPermanentNode(bookmarkFolder);
}

void BookmarkTreeView::DeleteSelection()
{
	auto hSelectedItem = TreeView_GetSelection(m_hTreeView);
	auto bookmarkFolder = GetBookmarkFolderFromTreeView(hSelectedItem);

	if (!m_bookmarkTree->IsPermanentNode(bookmarkFolder))
	{
		m_bookmarkTree->RemoveBookmarkItem(bookmarkFolder);
	}
}

void BookmarkTreeView::OnRClick(const NMHDR *pnmhdr)
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
	auto hItem = TreeView_HitTest(m_hTreeView, &tvhti);

	if (hItem == nullptr)
	{
		return;
	}

	TreeView_SelectItem(m_hTreeView, hItem);

	auto bookmarkFolder = GetBookmarkFolderFromTreeView(hItem);

	wil::unique_hmenu menu(
		LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_BOOKMARK_TREEVIEW_RCLICK_MENU)));

	MenuHelper::EnableItem(menu.get(), IDM_BOOKMARK_TREEVIEW_RLICK_RENAME,
		!m_bookmarkTree->IsPermanentNode(bookmarkFolder));
	MenuHelper::EnableItem(menu.get(), IDM_BOOKMARK_TREEVIEW_RLICK_DELETE,
		!m_bookmarkTree->IsPermanentNode(bookmarkFolder));

	TrackPopupMenu(GetSubMenu(menu.get(), 0), TPM_LEFTALIGN, ptCursor.x, ptCursor.y, 0, m_hTreeView,
		nullptr);
}

void BookmarkTreeView::CreateNewFolder()
{
	std::wstring newBookmarkFolderName =
		ResourceHelper::LoadString(m_resourceInstance, IDS_BOOKMARKS_NEWBOOKMARKFOLDER);
	auto newBookmarkFolder =
		std::make_unique<BookmarkItem>(std::nullopt, newBookmarkFolderName, std::nullopt);

	m_bNewFolderCreated = true;
	m_NewFolderGUID = newBookmarkFolder->GetGUID();

	auto hSelectedItem = TreeView_GetSelection(m_hTreeView);
	DCHECK_NOTNULL(hSelectedItem);

	auto bookmarkFolder = GetBookmarkFolderFromTreeView(hSelectedItem);
	m_bookmarkTree->AddBookmarkItem(bookmarkFolder, std::move(newBookmarkFolder),
		bookmarkFolder->GetChildren().size());
}

void BookmarkTreeView::SelectFolder(const std::wstring &guid)
{
	auto itr = m_mapItem.find(guid);
	CHECK(itr != m_mapItem.end());

	TreeView_SelectItem(m_hTreeView, itr->second);
}

BookmarkItem *BookmarkTreeView::GetBookmarkFolderFromTreeView(HTREEITEM hItem)
{
	TVITEM tvi;
	tvi.mask = TVIF_HANDLE | TVIF_PARAM;
	tvi.hItem = hItem;
	TreeView_GetItem(m_hTreeView, &tvi);

	return reinterpret_cast<BookmarkItem *>(tvi.lParam);
}

BookmarkTreeView::DropLocation BookmarkTreeView::GetDropLocation(const POINT &pt)
{
	POINT ptClient = pt;
	ScreenToClient(m_hTreeView, &ptClient);

	TVHITTESTINFO hitTestInfo;
	hitTestInfo.pt = ptClient;
	auto treeItem = TreeView_HitTest(m_hTreeView, &hitTestInfo);

	BookmarkItem *parentFolder = nullptr;
	size_t position;
	bool parentFolderSelected = false;

	if (treeItem)
	{
		auto bookmarkFolder = GetBookmarkFolderFromTreeView(treeItem);

		RECT itemRect;
		TreeView_GetItemRect(m_hTreeView, treeItem, &itemRect, FALSE);

		RECT folderCentralRect = itemRect;
		int indent =
			static_cast<int>(FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE * GetRectHeight(&itemRect));
		InflateRect(&folderCentralRect, 0, -indent);

		if (ptClient.y < folderCentralRect.top)
		{
			parentFolder = bookmarkFolder->GetParent();
			position = bookmarkFolder->GetParent()->GetChildIndex(bookmarkFolder);
		}
		else if (ptClient.y > folderCentralRect.bottom)
		{
			parentFolder = bookmarkFolder->GetParent();
			position = bookmarkFolder->GetParent()->GetChildIndex(bookmarkFolder) + 1;
		}
		else
		{
			parentFolder = bookmarkFolder;
			position = bookmarkFolder->GetChildren().size();
			parentFolderSelected = true;
		}
	}
	else
	{
		HTREEITEM nextItem = FindNextItem(ptClient);

		if (nextItem)
		{
			auto nextBookmarkFolder = GetBookmarkFolderFromTreeView(nextItem);
			parentFolder = nextBookmarkFolder->GetParent();
			position = nextBookmarkFolder->GetParent()->GetChildIndex(nextBookmarkFolder);
		}
		else
		{
			parentFolder = m_bookmarkTree->GetRoot();
			position = m_bookmarkTree->GetRoot()->GetChildren().size();
		}
	}

	return { parentFolder, position, parentFolderSelected };
}

HTREEITEM BookmarkTreeView::FindNextItem(const POINT &ptClient) const
{
	auto treeItem = TreeView_GetFirstVisible(m_hTreeView);

	do
	{
		RECT rc;
		TreeView_GetItemRect(m_hTreeView, treeItem, &rc, FALSE);

		if (ptClient.y < rc.top)
		{
			break;
		}

		treeItem = TreeView_GetNextVisible(m_hTreeView, treeItem);
	} while (treeItem != nullptr);

	return treeItem;
}

void BookmarkTreeView::UpdateUiForDropLocation(const DropLocation &dropLocation)
{
	RemoveDropHighlight();

	if (dropLocation.parentFolderSelected)
	{
		RemoveInsertionMark();

		HTREEITEM selectedItem = m_mapItem.at(dropLocation.parentFolder->GetGUID());

		TreeView_SetItemState(m_hTreeView, selectedItem, TVIS_DROPHILITED, TVIS_DROPHILITED);
		m_previousDropItem = selectedItem;
	}
	else
	{
		HTREEITEM treeItem;
		BOOL after;

		auto &children = dropLocation.parentFolder->GetChildren();
		auto insertItr = children.begin() + dropLocation.position;

		auto nextFolderItr =
			std::find_if(insertItr, children.end(), [](auto &child) { return child->IsFolder(); });

		// If the cursor is not over a folder, it means that it's within a
		// particular parent folder. That parent must have at least one child
		// folder, since if it didn't, that would mean that the folder wouldn't
		// be expanded and the only possible positions would be within the
		// grandparent folder, either before or after the parent (but not within
		// it).
		if (nextFolderItr == children.end())
		{
			auto reverseInsertItr = children.rbegin() + (children.size() - dropLocation.position);

			auto previousFolderItr = std::find_if(reverseInsertItr, children.rend(),
				[](auto &child) { return child->IsFolder(); });

			treeItem = m_mapItem.at((*previousFolderItr)->GetGUID());
			after = TRUE;
		}
		else
		{
			treeItem = m_mapItem.at((*nextFolderItr)->GetGUID());
			after = FALSE;
		}

		TreeView_SetInsertMark(m_hTreeView, treeItem, after);
	}
}

void BookmarkTreeView::ResetDropUiState()
{
	RemoveInsertionMark();
	RemoveDropHighlight();
}

void BookmarkTreeView::RemoveInsertionMark()
{
	TreeView_SetInsertMark(m_hTreeView, nullptr, FALSE);
}

void BookmarkTreeView::RemoveDropHighlight()
{
	if (m_previousDropItem)
	{
		TreeView_SetItemState(m_hTreeView, *m_previousDropItem, 0, TVIS_DROPHILITED);
		m_previousDropItem.reset();
	}
}
