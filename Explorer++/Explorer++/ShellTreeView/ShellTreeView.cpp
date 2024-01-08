// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Wraps a treeview control. Specifically handles
 * adding directories to it and selecting directories.
 * Each non-network drive in the system is also
 * monitored for changes.
 *
 * Notes:
 *  - All items are sorted alphabetically, except for:
 *     - Items on the desktop
 *     - Items in My Computer
 */

#include "stdafx.h"
#include "ShellTreeView.h"
#include "Config.h"
#include "CoreInterface.h"
#include "ShellBrowser/ShellNavigator.h"
#include "ShellTreeNode.h"
#include "TabContainer.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/ClipboardHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/DriveInfo.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <wil/common.h>
#include <propkey.h>

ShellTreeView *ShellTreeView::Create(HWND hParent, CoreInterface *coreInterface,
	TabContainer *tabContainer, FileActionHandler *fileActionHandler, CachedIcons *cachedIcons)
{
	return new ShellTreeView(hParent, coreInterface, tabContainer, fileActionHandler, cachedIcons);
}

ShellTreeView::ShellTreeView(HWND hParent, CoreInterface *coreInterface, TabContainer *tabContainer,
	FileActionHandler *fileActionHandler, CachedIcons *cachedIcons) :
	ShellDropTargetWindow(CreateTreeView(hParent)),
	m_hTreeView(GetHWND()),
	m_config(coreInterface->GetConfig()),
	m_tabContainer(tabContainer),
	m_fileActionHandler(fileActionHandler),
	m_cachedIcons(cachedIcons),
	m_iconThreadPool(1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED),
		CoUninitialize),
	m_iconResultIDCounter(0),
	m_subfoldersThreadPool(1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED),
		CoUninitialize),
	m_subfoldersResultIDCounter(0),
	m_cutItem(nullptr),
	m_dropExpandItem(nullptr),
	m_shellChangeWatcher(GetHWND(),
		std::bind_front(&ShellTreeView::ProcessShellChangeNotifications, this)),
	m_fontSetter(m_hTreeView, coreInterface->GetConfig())
{
	TreeView_SetExtendedStyle(m_hTreeView, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hTreeView,
		std::bind_front(&ShellTreeView::TreeViewProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(hParent,
		std::bind_front(&ShellTreeView::ParentWndProc, this)));

	m_iFolderIcon = GetDefaultFolderIconIndex();

	m_bDragCancelled = FALSE;
	m_bDragAllowed = FALSE;
	m_bShowHidden = TRUE;

	AddRootItems();

	m_getDragImageMessage = RegisterWindowMessage(DI_GETDRAGIMAGE);

	AddClipboardFormatListener(m_hTreeView);

	StartDirectoryMonitoringForDrives();

	m_connections.push_back(m_config->showQuickAccessInTreeView.addObserver(
		std::bind_front(&ShellTreeView::OnShowQuickAccessUpdated, this)));
	m_connections.push_back(coreInterface->AddApplicationShuttingDownObserver(
		std::bind_front(&ShellTreeView::OnApplicationShuttingDown, this)));
}

HWND ShellTreeView::CreateTreeView(HWND parent)
{
	return ::CreateTreeView(parent,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_SHOWSELALWAYS | TVS_HASBUTTONS | TVS_EDITLABELS
			| TVS_HASLINES | TVS_TRACKSELECT);
}

ShellTreeView::~ShellTreeView()
{
	m_iconThreadPool.clear_queue();
}

void ShellTreeView::OnApplicationShuttingDown()
{
	if (m_clipboardDataObject && OleIsCurrentClipboard(m_clipboardDataObject.get()) == S_OK)
	{
		OleFlushClipboard();
	}
}

LRESULT ShellTreeView::TreeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_getDragImageMessage != 0 && msg == m_getDragImageMessage)
	{
		return FALSE;
	}

	switch (msg)
	{
	case WM_TIMER:
		if (wParam == DROP_EXPAND_TIMER_ID)
		{
			OnDropExpandTimer();
		}
		break;

	case WM_RBUTTONDOWN:
		if ((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON) && !(wParam & MK_MBUTTON))
		{
			TVHITTESTINFO tvhti;

			tvhti.pt.x = LOWORD(lParam);
			tvhti.pt.y = HIWORD(lParam);

			/* Test to see if the mouse click was
			on an item or not. */
			TreeView_HitTest(m_hTreeView, &tvhti);

			if (!(tvhti.flags & LVHT_NOWHERE))
			{
				m_bDragAllowed = TRUE;
			}
		}
		break;

	case WM_RBUTTONUP:
		m_bDragCancelled = FALSE;
		m_bDragAllowed = FALSE;
		break;

	case WM_MBUTTONDOWN:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		OnMiddleButtonDown(&pt);
	}
	break;

	case WM_MBUTTONUP:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		OnMiddleButtonUp(&pt, static_cast<UINT>(wParam));
	}
	break;

	case WM_MOUSEMOVE:
	{
		if (!IsWithinDrag() && !m_bDragCancelled && m_bDragAllowed)
		{
			if ((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON) && !(wParam & MK_MBUTTON))
			{
				TVHITTESTINFO tvhti;
				TVITEM tvItem;
				POINT pt;
				DWORD dwPos;
				HRESULT hr;
				BOOL bRet;

				dwPos = GetMessagePos();
				pt.x = GET_X_LPARAM(dwPos);
				pt.y = GET_Y_LPARAM(dwPos);
				MapWindowPoints(HWND_DESKTOP, m_hTreeView, &pt, 1);

				tvhti.pt = pt;

				/* Test to see if the mouse click was
				on an item or not. */
				TreeView_HitTest(m_hTreeView, &tvhti);

				if (!(tvhti.flags & LVHT_NOWHERE))
				{
					tvItem.mask = TVIF_PARAM | TVIF_HANDLE;
					tvItem.hItem = tvhti.hItem;
					bRet = TreeView_GetItem(m_hTreeView, &tvItem);

					if (bRet)
					{
						hr = OnBeginDrag(reinterpret_cast<ShellTreeNode *>(tvItem.lParam));

						if (hr == DRAGDROP_S_CANCEL)
						{
							m_bDragCancelled = TRUE;
						}
					}
				}
			}
		}
	}
	break;

	case WM_CLIPBOARDUPDATE:
		OnClipboardUpdate();
		return 0;

	case WM_APP_ICON_RESULT_READY:
		ProcessIconResult(static_cast<int>(wParam));
		break;

	case WM_APP_SUBFOLDERS_RESULT_READY:
		ProcessSubfoldersResult(static_cast<int>(wParam));
		break;

	case WM_DESTROY:
		RemoveClipboardFormatListener(m_hTreeView);
		break;

	case WM_NCDESTROY:
		delete this;
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT ShellTreeView::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hTreeView)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case TVN_BEGINDRAG:
			{
				auto *pnmTreeView = reinterpret_cast<NMTREEVIEW *>(lParam);
				OnBeginDrag(reinterpret_cast<ShellTreeNode *>(pnmTreeView->itemNew.lParam));
			}
			break;

			case TVN_GETDISPINFO:
				OnGetDisplayInfo(reinterpret_cast<NMTVDISPINFO *>(lParam));
				break;

			case TVN_ITEMEXPANDING:
				OnItemExpanding(reinterpret_cast<NMTREEVIEW *>(lParam));
				break;

			case TVN_KEYDOWN:
				return OnKeyDown(reinterpret_cast<NMTVKEYDOWN *>(lParam));

			case TVN_ENDLABELEDIT:
				return OnEndLabelEdit(reinterpret_cast<NMTVDISPINFO *>(lParam));
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void ShellTreeView::AddRootItems()
{
	if (m_config->showQuickAccessInTreeView.get())
	{
		AddQuickAccessRootItem();
	}

	AddShellNamespaceRootItem();

	// This will ensure that the treeview always has a selected item.
	TreeView_SelectItem(m_hTreeView, TreeView_GetRoot(m_hTreeView));
}

void ShellTreeView::AddQuickAccessRootItem()
{
	// The quick access root item should only be added to the treeview if it's not already there.
	assert(!m_quickAccessRootItem);

	unique_pidl_absolute quickAccessPidl;
	HRESULT hr =
		SHParseDisplayName(QUICK_ACCESS_PATH, nullptr, wil::out_param(quickAccessPidl), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		m_quickAccessRootItem = AddRootItem(quickAccessPidl.get(), TVI_FIRST);
	}
}

void ShellTreeView::AddShellNamespaceRootItem()
{
	unique_pidl_absolute rootPidl;
	HRESULT hr = GetRootPidl(wil::out_param(rootPidl));

	if (SUCCEEDED(hr))
	{
		AddRootItem(rootPidl.get());
	}
}

HTREEITEM ShellTreeView::AddRootItem(PCIDLIST_ABSOLUTE pidl, HTREEITEM insertAfter)
{
	auto rootItem = AddItem(nullptr, pidl, insertAfter);
	assert(rootItem);
	SendMessage(m_hTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(rootItem));

	return rootItem;
}

void ShellTreeView::OnShowQuickAccessUpdated(bool newValue)
{
	if (newValue)
	{
		AddQuickAccessRootItem();
	}
	else
	{
		if (m_quickAccessRootItem)
		{
			RemoveItem(m_quickAccessRootItem);
			m_quickAccessRootItem = nullptr;
		}
	}
}

void ShellTreeView::OnGetDisplayInfo(NMTVDISPINFO *pnmtvdi)
{
	TVITEM *ptvItem = &pnmtvdi->item;

	if (WI_IsFlagSet(ptvItem->mask, TVIF_IMAGE))
	{
		const ShellTreeNode *node = reinterpret_cast<ShellTreeNode *>(ptvItem->lParam);
		auto cachedIconIndex = GetCachedIconIndex(node);

		if (cachedIconIndex)
		{
			ptvItem->iImage = (*cachedIconIndex & 0x0FFF);
			ptvItem->iSelectedImage = (*cachedIconIndex & 0x0FFF);
		}
		else
		{
			ptvItem->iImage = m_iFolderIcon;
			ptvItem->iSelectedImage = m_iFolderIcon;
		}

		QueueIconTask(ptvItem->hItem);
	}

	if (WI_IsFlagSet(ptvItem->mask, TVIF_CHILDREN))
	{
		ptvItem->cChildren = 1;

		QueueSubfoldersTask(ptvItem->hItem);
	}

	ptvItem->mask |= TVIF_DI_SETITEM;
}

std::optional<int> ShellTreeView::GetCachedIconIndex(const ShellTreeNode *node)
{
	std::wstring filePath;
	HRESULT hr = GetDisplayName(node->GetFullPidl().get(), SHGDN_FORPARSING, filePath);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	auto cachedItr = m_cachedIcons->findByPath(filePath);

	if (cachedItr == m_cachedIcons->end())
	{
		return std::nullopt;
	}

	return cachedItr->iconIndex;
}

void ShellTreeView::QueueIconTask(HTREEITEM treeItem)
{
	ShellTreeNode *node = GetNodeFromTreeViewItem(treeItem);

	BasicItemInfo basicItemInfo;
	basicItemInfo.pidl = node->GetFullPidl();

	int iconResultID = m_iconResultIDCounter++;

	auto result = m_iconThreadPool.push(
		[this, iconResultID, nodeId = node->GetId(), treeItem, basicItemInfo](int id)
		{
			UNREFERENCED_PARAMETER(id);

			return FindIconAsync(m_hTreeView, iconResultID, nodeId, treeItem,
				basicItemInfo.pidl.get());
		});

	m_iconResults.insert({ iconResultID, std::move(result) });
}

std::optional<ShellTreeView::IconResult> ShellTreeView::FindIconAsync(HWND treeView,
	int iconResultId, int nodeId, HTREEITEM treeItem, PCIDLIST_ABSOLUTE pidl)
{
	SHFILEINFO shfi;
	DWORD_PTR res = SHGetFileInfo(reinterpret_cast<LPCTSTR>(pidl), 0, &shfi, sizeof(SHFILEINFO),
		SHGFI_PIDL | SHGFI_ICON | SHGFI_OVERLAYINDEX);

	if (res == 0)
	{
		return std::nullopt;
	}

	DestroyIcon(shfi.hIcon);

	PostMessage(treeView, WM_APP_ICON_RESULT_READY, iconResultId, 0);

	IconResult result;
	result.nodeId = nodeId;
	result.treeItem = treeItem;
	result.iconIndex = shfi.iIcon;
	return result;
}

void ShellTreeView::ProcessIconResult(int iconResultId)
{
	auto iconResultsItr = m_iconResults.find(iconResultId);

	if (iconResultsItr == m_iconResults.end())
	{
		return;
	}

	auto cleanup =
		wil::scope_exit([this, iconResultsItr]() { m_iconResults.erase(iconResultsItr); });

	auto result = iconResultsItr->second.get();

	if (!result)
	{
		return;
	}

	auto *node = GetNodeById(result->nodeId);

	// The item may have been removed (e.g. if the associated folder was deleted, or the parent was
	// collapsed).
	if (!node)
	{
		return;
	}

	std::wstring filePath;
	HRESULT hr = GetDisplayName(node->GetFullPidl().get(), SHGDN_FORPARSING, filePath);

	if (SUCCEEDED(hr))
	{
		m_cachedIcons->addOrUpdateFileIcon(filePath, result->iconIndex);
	}

	TVITEM tvItem;
	tvItem.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvItem.hItem = result->treeItem;
	tvItem.iImage = result->iconIndex;
	tvItem.iSelectedImage = result->iconIndex;
	tvItem.stateMask = TVIS_OVERLAYMASK;
	tvItem.state = INDEXTOOVERLAYMASK(result->iconIndex >> 24);
	TreeView_SetItem(m_hTreeView, &tvItem);
}

void ShellTreeView::QueueSubfoldersTask(HTREEITEM item)
{
	BasicItemInfo basicItemInfo;
	basicItemInfo.pidl = GetNodePidl(item);

	int subfoldersResultID = m_subfoldersResultIDCounter++;

	auto result = m_subfoldersThreadPool.push(
		[this, subfoldersResultID, item, basicItemInfo](int id)
		{
			UNREFERENCED_PARAMETER(id);

			return CheckSubfoldersAsync(m_hTreeView, subfoldersResultID, item,
				basicItemInfo.pidl.get());
		});

	m_subfoldersResults.insert({ subfoldersResultID, std::move(result) });
}

std::optional<ShellTreeView::SubfoldersResult> ShellTreeView::CheckSubfoldersAsync(HWND treeView,
	int subfoldersResultId, HTREEITEM item, PCIDLIST_ABSOLUTE pidl)
{
	wil::com_ptr_nothrow<IShellFolder> pShellFolder;
	PCITEMID_CHILD pidlRelative;
	HRESULT hr = SHBindToParent(pidl, IID_PPV_ARGS(&pShellFolder), &pidlRelative);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	ULONG attributes = SFGAO_HASSUBFOLDER;
	hr = pShellFolder->GetAttributesOf(1, &pidlRelative, &attributes);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	PostMessage(treeView, WM_APP_SUBFOLDERS_RESULT_READY, subfoldersResultId, 0);

	SubfoldersResult result;
	result.item = item;
	result.hasSubfolder = WI_IsFlagSet(attributes, SFGAO_HASSUBFOLDER);

	return result;
}

void ShellTreeView::ProcessSubfoldersResult(int subfoldersResultId)
{
	auto itr = m_subfoldersResults.find(subfoldersResultId);

	if (itr == m_subfoldersResults.end())
	{
		return;
	}

	auto cleanup = wil::scope_exit([this, itr]() { m_subfoldersResults.erase(itr); });

	auto result = itr->second.get();

	if (!result)
	{
		return;
	}

	if (result->hasSubfolder)
	{
		// By default it's assumed that an item has subfolders, so if it does
		// actually have subfolders, there's nothing else that needs to be done.
		return;
	}

	TVITEM tvItem;
	tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
	tvItem.hItem = result->item;
	tvItem.cChildren = 0;
	TreeView_SetItem(m_hTreeView, &tvItem);
}

void ShellTreeView::OnItemExpanding(const NMTREEVIEW *nmtv)
{
	HTREEITEM parentItem = nmtv->itemNew.hItem;

	if (nmtv->action == TVE_EXPAND)
	{
		ExpandDirectory(parentItem);
	}
	else
	{
		auto hSelection = TreeView_GetSelection(m_hTreeView);

		if (hSelection != nullptr)
		{
			HTREEITEM hItem = hSelection;

			do
			{
				hItem = TreeView_GetParent(m_hTreeView, hItem);
			} while (hItem != parentItem && hItem != nullptr);

			// If the currently selected item is below the item being
			// collapsed, the selection should be adjusted to the parent item.
			if (hItem == parentItem)
			{
				TreeView_SelectItem(m_hTreeView, parentItem);
			}
		}

		ShellTreeNode *parentNode = GetNodeFromTreeViewItem(parentItem);
		StopDirectoryMonitoringForNodeAndChildren(parentNode);
		parentNode->RemoveAllChildren();

		SendMessage(m_hTreeView, TVM_EXPAND, TVE_COLLAPSE | TVE_COLLAPSERESET,
			reinterpret_cast<LPARAM>(parentItem));
	}
}

LRESULT ShellTreeView::OnKeyDown(const NMTVKEYDOWN *keyDown)
{
	switch (keyDown->wVKey)
	{
	case 'C':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			CopySelectedItemToClipboard(true);
		}
		break;

	case 'X':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			CopySelectedItemToClipboard(false);
		}
		break;

	case 'V':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			Paste();
		}
		break;

	case VK_DELETE:
		if (IsKeyDown(VK_SHIFT))
		{
			DeleteSelectedItem(true);
		}
		else
		{
			DeleteSelectedItem(false);
		}
		break;
	}

	// If the ctrl key is down, this key sequence is likely a modifier. Stop any other pressed key
	// from been used in an incremental search.
	if (IsKeyDown(VK_CONTROL))
	{
		return 1;
	}

	return 0;
}

/* Sorts items in the following order:
 - Drives
 - Virtual Items
 - Real Items

Each set is ordered alphabetically. */
int CALLBACK ShellTreeView::CompareItemsStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	ShellTreeView *shellTreeView = nullptr;

	shellTreeView = (ShellTreeView *) lParamSort;

	return shellTreeView->CompareItems(lParam1, lParam2);
}

int CALLBACK ShellTreeView::CompareItems(LPARAM lParam1, LPARAM lParam2)
{
	TCHAR szTemp[MAX_PATH];

	const ShellTreeNode *node1 = reinterpret_cast<ShellTreeNode *>(lParam1);
	const ShellTreeNode *node2 = reinterpret_cast<ShellTreeNode *>(lParam2);

	auto pidl1 = node1->GetFullPidl();
	auto pidl2 = node2->GetFullPidl();

	std::wstring displayName1;
	GetDisplayName(pidl1.get(), SHGDN_FORPARSING, displayName1);

	std::wstring displayName2;
	GetDisplayName(pidl2.get(), SHGDN_FORPARSING, displayName2);

	if (PathIsRoot(displayName1.c_str()) && !PathIsRoot(displayName2.c_str()))
	{
		return -1;
	}
	else if (!PathIsRoot(displayName1.c_str()) && PathIsRoot(displayName2.c_str()))
	{
		return 1;
	}
	else if (PathIsRoot(displayName1.c_str()) && PathIsRoot(displayName2.c_str()))
	{
		return lstrcmpi(displayName1.c_str(), displayName2.c_str());
	}
	else
	{
		if (!SHGetPathFromIDList(pidl1.get(), szTemp) && SHGetPathFromIDList(pidl2.get(), szTemp))
		{
			return -1;
		}
		else if (SHGetPathFromIDList(pidl1.get(), szTemp)
			&& !SHGetPathFromIDList(pidl2.get(), szTemp))
		{
			return 1;
		}
		else
		{
			GetDisplayName(pidl1.get(), SHGDN_INFOLDER, displayName1);
			GetDisplayName(pidl2.get(), SHGDN_INFOLDER, displayName2);

			if (m_config->globalFolderSettings.useNaturalSortOrder)
			{
				return StrCmpLogicalW(displayName1.c_str(), displayName2.c_str());
			}
			else
			{
				return StrCmpIW(displayName1.c_str(), displayName2.c_str());
			}
		}
	}
}

HRESULT ShellTreeView::ExpandDirectory(HTREEITEM hParent)
{
	auto pidlDirectory = GetNodePidl(hParent);

	wil::com_ptr_nothrow<IShellFolder2> shellFolder2;
	HRESULT hr = BindToIdl(pidlDirectory.get(), IID_PPV_ARGS(&shellFolder2));

	if (FAILED(hr))
	{
		return hr;
	}

	SHCONTF enumFlags = SHCONTF_FOLDERS;

	if (m_bShowHidden)
	{
		enumFlags |= SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN;
	}

	wil::com_ptr_nothrow<IEnumIDList> pEnumIDList;
	hr = shellFolder2->EnumObjects(nullptr, enumFlags, &pEnumIDList);

	if (FAILED(hr) || !pEnumIDList)
	{
		return hr;
	}

	SendMessage(m_hTreeView, WM_SETREDRAW, FALSE, 0);

	std::vector<unique_pidl_absolute> items;

	unique_pidl_child pidlItem;
	ULONG uFetched = 1;

	while (pEnumIDList->Next(1, wil::out_param(pidlItem), &uFetched) == S_OK && (uFetched == 1))
	{
		if (m_config->checkPinnedToNamespaceTreeProperty)
		{
			BOOL showItem = GetBooleanVariant(shellFolder2.get(), pidlItem.get(),
				&PKEY_IsPinnedToNameSpaceTree, TRUE);

			if (!showItem)
			{
				continue;
			}
		}

		if (m_config->globalFolderSettings.hideSystemFiles)
		{
			PCITEMID_CHILD child = pidlItem.get();
			SFGAOF attributes = SFGAO_SYSTEM;
			hr = shellFolder2->GetAttributesOf(1, &child, &attributes);

			if (FAILED(hr) || (WI_IsFlagSet(attributes, SFGAO_SYSTEM)))
			{
				continue;
			}
		}

		items.emplace_back(ILCombine(pidlDirectory.get(), pidlItem.get()));
	}

	for (const auto &item : items)
	{
		AddItem(hParent, item.get());
	}

	SortChildren(hParent);

	SendMessage(m_hTreeView, WM_SETREDRAW, TRUE, 0);

	ShellTreeNode *parentNode = GetNodeFromTreeViewItem(hParent);
	StartDirectoryMonitoringForNode(parentNode);

	return hr;
}

HTREEITEM ShellTreeView::AddItem(HTREEITEM parent, PCIDLIST_ABSOLUTE pidl, HTREEITEM insertAfter)
{
	std::wstring name;
	HRESULT hr = GetDisplayName(pidl, SHGDN_NORMAL, name);

	if (FAILED(hr))
	{
		return nullptr;
	}

	ShellTreeNode *node;

	if (parent)
	{
		auto childNode =
			std::make_unique<ShellTreeNode>(unique_pidl_child(ILCloneChild(ILFindLastID(pidl))));

		auto *parentNode = GetNodeFromTreeViewItem(parent);
		node = parentNode->AddChild(std::move(childNode));
	}
	else
	{
		auto rootNode = std::make_unique<ShellTreeNode>(unique_pidl_absolute(ILCloneFull(pidl)));
		node = rootNode.get();

		m_nodes.push_back(std::move(rootNode));
	}

	TVITEMEX tvItem = {};
	tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
	tvItem.pszText = name.data();
	tvItem.iImage = I_IMAGECALLBACK;
	tvItem.iSelectedImage = I_IMAGECALLBACK;
	tvItem.lParam = reinterpret_cast<LPARAM>(node);
	tvItem.cChildren = I_CHILDRENCALLBACK;

	TVINSERTSTRUCT tvInsertData = {};
	tvInsertData.hInsertAfter = insertAfter;
	tvInsertData.hParent = parent;
	tvInsertData.itemex = tvItem;

	[[maybe_unused]] auto item = TreeView_InsertItem(m_hTreeView, &tvInsertData);
	assert(item);

	return item;
}

void ShellTreeView::SortChildren(HTREEITEM parent)
{
	TVSORTCB tvSort;
	tvSort.hParent = parent;
	tvSort.lpfnCompare = ShellTreeView::CompareItemsStub;
	tvSort.lParam = reinterpret_cast<LPARAM>(this);
	TreeView_SortChildrenCB(m_hTreeView, &tvSort, 0);
}

unique_pidl_absolute ShellTreeView::GetSelectedNodePidl() const
{
	auto selectedItem = TreeView_GetSelection(m_hTreeView);
	return GetNodePidl(selectedItem);
}

unique_pidl_absolute ShellTreeView::GetNodePidl(HTREEITEM hTreeItem) const
{
	const ShellTreeNode *node = GetNodeFromTreeViewItem(hTreeItem);
	return node->GetFullPidl();
}

ShellTreeNode *ShellTreeView::GetNodeFromTreeViewItem(HTREEITEM item) const
{
	TVITEMEX tvItemEx;
	tvItemEx.mask = TVIF_HANDLE | TVIF_PARAM;
	tvItemEx.hItem = item;
	[[maybe_unused]] bool res = TreeView_GetItem(m_hTreeView, &tvItemEx);
	assert(res);

	return reinterpret_cast<ShellTreeNode *>(tvItemEx.lParam);
}

ShellTreeNode *ShellTreeView::GetNodeById(int id) const
{
	for (auto &rootNode : m_nodes)
	{
		auto *foundNode = GetNodeByIdRecursive(rootNode.get(), id);

		if (foundNode)
		{
			return foundNode;
		}
	}

	return nullptr;
}

ShellTreeNode *ShellTreeView::GetNodeByIdRecursive(ShellTreeNode *node, int id) const
{
	if (node->GetId() == id)
	{
		return node;
	}

	for (auto &childNode : node->GetChildren())
	{
		auto *foundNode = GetNodeByIdRecursive(childNode.get(), id);

		if (foundNode)
		{
			return foundNode;
		}
	}

	return nullptr;
}

HTREEITEM ShellTreeView::LocateItem(PCIDLIST_ABSOLUTE pidlDirectory)
{
	return LocateItemInternal(pidlDirectory, FALSE);
}

HTREEITEM ShellTreeView::LocateExistingItem(PCIDLIST_ABSOLUTE pidlDirectory)
{
	return LocateItemInternal(pidlDirectory, TRUE);
}

HTREEITEM ShellTreeView::LocateItemInternal(PCIDLIST_ABSOLUTE pidlDirectory,
	BOOL bOnlyLocateExistingItem)
{
	HTREEITEM hRoot;
	HTREEITEM hItem;
	TVITEMEX item;
	BOOL bFound = FALSE;

	/* Get the root of the tree (root of namespace). */
	hRoot = TreeView_GetRoot(m_hTreeView);
	hItem = hRoot;

	item.mask = TVIF_PARAM | TVIF_HANDLE;
	item.hItem = hItem;
	TreeView_GetItem(m_hTreeView, &item);

	/* Keep searching until the specified item
	is found or it is found the item does not
	exist in the treeview.
	Look through each item, once an ancestor is
	found, look through it's children, expanding
	the parent node if necessary. */
	while (!bFound && hItem != nullptr)
	{
		auto *node = reinterpret_cast<ShellTreeNode *>(item.lParam);
		auto currentPidl = node->GetFullPidl();

		if (ArePidlsEquivalent(currentPidl.get(), pidlDirectory))
		{
			bFound = TRUE;

			break;
		}

		if (ILIsParent(currentPidl.get(), pidlDirectory, FALSE))
		{
			if ((TreeView_GetChild(m_hTreeView, hItem)) == nullptr)
			{
				if (bOnlyLocateExistingItem)
				{
					return nullptr;
				}
				else
				{
					SendMessage(m_hTreeView, TVM_EXPAND, TVE_EXPAND, (LPARAM) hItem);
				}
			}

			hItem = TreeView_GetChild(m_hTreeView, hItem);
		}
		else
		{
			hItem = TreeView_GetNextSibling(m_hTreeView, hItem);
		}

		item.mask = TVIF_PARAM | TVIF_HANDLE;
		item.hItem = hItem;
		TreeView_GetItem(m_hTreeView, &item);
	}

	return hItem;
}

void ShellTreeView::OnMiddleButtonDown(const POINT *pt)
{
	TVHITTESTINFO hitTestInfo;
	hitTestInfo.pt = *pt;

	TreeView_HitTest(m_hTreeView, &hitTestInfo);

	if (hitTestInfo.flags != LVHT_NOWHERE)
	{
		m_middleButtonItem = hitTestInfo.hItem;
	}
	else
	{
		m_middleButtonItem = nullptr;
	}
}

void ShellTreeView::OnMiddleButtonUp(const POINT *pt, UINT keysDown)
{
	TVHITTESTINFO hitTestInfo;
	hitTestInfo.pt = *pt;

	TreeView_HitTest(m_hTreeView, &hitTestInfo);

	if (hitTestInfo.flags == LVHT_NOWHERE)
	{
		return;
	}

	// Only open an item if it was the one on which the middle mouse button was initially clicked
	// on.
	if (hitTestInfo.hItem != m_middleButtonItem)
	{
		return;
	}

	bool switchToNewTab = m_config->openTabsInForeground;

	if (WI_IsFlagSet(keysDown, MK_SHIFT))
	{
		switchToNewTab = !switchToNewTab;
	}

	auto pidl = GetNodePidl(hitTestInfo.hItem);
	auto navigateParams = NavigateParams::Normal(pidl.get());
	m_tabContainer->CreateNewTab(navigateParams, TabSettings(_selected = switchToNewTab));
}

void ShellTreeView::SetShowHidden(BOOL bShowHidden)
{
	m_bShowHidden = bShowHidden;
}

void ShellTreeView::RefreshAllIcons()
{
	auto hRoot = TreeView_GetRoot(m_hTreeView);

	TVITEMEX tvItemEx;
	tvItemEx.mask = TVIF_HANDLE | TVIF_PARAM;
	tvItemEx.hItem = hRoot;
	TreeView_GetItem(m_hTreeView, &tvItemEx);

	const ShellTreeNode *node = reinterpret_cast<ShellTreeNode *>(tvItemEx.lParam);

	SHFILEINFO shfi;
	SHGetFileInfo(reinterpret_cast<LPCTSTR>(node->GetFullPidl().get()), 0, &shfi, sizeof(shfi),
		SHGFI_PIDL | SHGFI_SYSICONINDEX);

	tvItemEx.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvItemEx.hItem = hRoot;
	tvItemEx.iImage = shfi.iIcon;
	tvItemEx.iSelectedImage = shfi.iIcon;
	TreeView_SetItem(m_hTreeView, &tvItemEx);

	RefreshAllIconsInternal(TreeView_GetChild(m_hTreeView, hRoot));
}

void ShellTreeView::RefreshAllIconsInternal(HTREEITEM hFirstSibling)
{
	HTREEITEM hNextSibling;
	HTREEITEM hChild;
	TVITEM tvItem;
	SHFILEINFO shfi;

	hNextSibling = TreeView_GetNextSibling(m_hTreeView, hFirstSibling);

	tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
	tvItem.hItem = hFirstSibling;
	TreeView_GetItem(m_hTreeView, &tvItem);

	const ShellTreeNode *node = reinterpret_cast<ShellTreeNode *>(tvItem.lParam);
	SHGetFileInfo(reinterpret_cast<LPCTSTR>(node->GetFullPidl().get()), 0, &shfi, sizeof(shfi),
		SHGFI_PIDL | SHGFI_SYSICONINDEX);

	tvItem.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvItem.hItem = hFirstSibling;
	tvItem.iImage = shfi.iIcon;
	tvItem.iSelectedImage = shfi.iIcon;
	TreeView_SetItem(m_hTreeView, &tvItem);

	hChild = TreeView_GetChild(m_hTreeView, hFirstSibling);

	if (hChild != nullptr)
		RefreshAllIconsInternal(hChild);

	while (hNextSibling != nullptr)
	{
		tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
		tvItem.hItem = hNextSibling;
		TreeView_GetItem(m_hTreeView, &tvItem);

		const ShellTreeNode *nextNode = reinterpret_cast<ShellTreeNode *>(tvItem.lParam);
		SHGetFileInfo(reinterpret_cast<LPCTSTR>(nextNode->GetFullPidl().get()), 0, &shfi,
			sizeof(shfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);

		tvItem.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvItem.hItem = hNextSibling;
		tvItem.iImage = shfi.iIcon;
		tvItem.iSelectedImage = shfi.iIcon;
		TreeView_SetItem(m_hTreeView, &tvItem);

		hChild = TreeView_GetChild(m_hTreeView, hNextSibling);

		if (hChild != nullptr)
			RefreshAllIconsInternal(hChild);

		hNextSibling = TreeView_GetNextSibling(m_hTreeView, hNextSibling);
	}
}

HRESULT ShellTreeView::OnBeginDrag(const ShellTreeNode *node)
{
	wil::com_ptr_nothrow<IDataObject> dataObject;
	auto pidl = node->GetFullPidl();
	std::vector<PCIDLIST_ABSOLUTE> items = { pidl.get() };
	RETURN_IF_FAILED(CreateDataObjectForShellTransfer(items, &dataObject));

	m_performingDrag = true;
	m_draggedItemPidl = pidl.get();

	DWORD effect;
	HRESULT hr = SHDoDragDrop(m_hTreeView, dataObject.get(), nullptr,
		DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK, &effect);

	m_draggedItemPidl = nullptr;
	m_performingDrag = false;

	return hr;
}

void ShellTreeView::StartRenamingSelectedItem()
{
	auto selectedItem = TreeView_GetSelection(m_hTreeView);
	TreeView_EditLabel(m_hTreeView, selectedItem);
}

void ShellTreeView::StartRenamingItem(PCIDLIST_ABSOLUTE pidl)
{
	auto item = LocateExistingItem(pidl);

	if (!item)
	{
		return;
	}

	TreeView_EditLabel(m_hTreeView, item);
}

void ShellTreeView::ShowPropertiesOfSelectedItem() const
{
	auto pidlDirectory = GetSelectedNodePidl();
	ShowMultipleFileProperties(pidlDirectory.get(), {}, m_hTreeView);
}

void ShellTreeView::DeleteSelectedItem(bool permanent)
{
	HTREEITEM item = TreeView_GetSelection(m_hTreeView);
	HTREEITEM parentItem = TreeView_GetParent(m_hTreeView, item);

	// Select the parent item to release the lock and allow deletion.
	TreeView_Select(m_hTreeView, parentItem, TVGN_CARET);

	auto pidl = GetNodePidl(item);

	DWORD mask = 0;

	if (permanent)
	{
		mask = CMIC_MASK_SHIFT_DOWN;
	}

	ExecuteActionFromContextMenu(pidl.get(), {}, m_hTreeView, _T("delete"), mask, nullptr);
}

bool ShellTreeView::OnEndLabelEdit(const NMTVDISPINFO *dispInfo)
{
	// If label editing was canceled or no text was entered, simply notify the control to revert to
	// the previous text.
	if (!dispInfo->item.pszText || lstrlen(dispInfo->item.pszText) == 0)
	{
		return false;
	}

	const auto *node = GetNodeFromTreeViewItem(dispInfo->item.hItem);

	std::wstring oldFileName;
	HRESULT hr = GetDisplayName(node->GetFullPidl().get(), SHGDN_FORPARSING, oldFileName);

	if (FAILED(hr))
	{
		return false;
	}

	TCHAR newFileName[MAX_PATH];
	StringCchCopy(newFileName, SIZEOF_ARRAY(newFileName), oldFileName.c_str());
	PathRemoveFileSpec(newFileName);
	bool res = PathAppend(newFileName, dispInfo->item.pszText);

	if (!res)
	{
		return false;
	}

	FileActionHandler::RenamedItem_t renamedItem;
	renamedItem.strOldFilename = oldFileName;
	renamedItem.strNewFilename = newFileName;

	TrimStringRight(renamedItem.strNewFilename, _T(" "));

	std::list<FileActionHandler::RenamedItem_t> renamedItemList;
	renamedItemList.push_back(renamedItem);
	m_fileActionHandler->RenameFiles(renamedItemList);

	return true;
}

void ShellTreeView::CopySelectedItemToClipboard(bool copy)
{
	HTREEITEM item = TreeView_GetSelection(m_hTreeView);
	CopyItemToClipboard(item, copy);
}

void ShellTreeView::CopyItemToClipboard(PCIDLIST_ABSOLUTE pidl, bool copy)
{
	auto item = LocateExistingItem(pidl);

	if (!item)
	{
		return;
	}

	CopyItemToClipboard(item, copy);
}

void ShellTreeView::CopyItemToClipboard(HTREEITEM treeItem, bool copy)
{
	auto *node = GetNodeFromTreeViewItem(treeItem);
	auto pidl = node->GetFullPidl();

	std::vector<PCIDLIST_ABSOLUTE> items = { pidl.get() };
	wil::com_ptr_nothrow<IDataObject> clipboardDataObject;
	HRESULT hr;

	if (copy)
	{
		hr = CopyFiles(items, &clipboardDataObject);

		if (SUCCEEDED(hr))
		{
			UpdateCurrentClipboardObject(clipboardDataObject);
		}
	}
	else
	{
		hr = CutFiles(items, &clipboardDataObject);

		if (SUCCEEDED(hr))
		{
			UpdateCurrentClipboardObject(clipboardDataObject);

			m_cutItem = treeItem;
			UpdateItemState(treeItem, TVIS_CUT, TVIS_CUT);
		}
	}
}

void ShellTreeView::Paste()
{
	wil::com_ptr_nothrow<IDataObject> clipboardObject;
	HRESULT hr = OleGetClipboard(&clipboardObject);

	if (FAILED(hr))
	{
		return;
	}

	auto *selectedNode = GetNodeFromTreeViewItem(TreeView_GetSelection(m_hTreeView));
	auto selectedItemPidl = selectedNode->GetFullPidl();

	if (CanShellPasteDataObject(selectedItemPidl.get(), clipboardObject.get(),
			DROPEFFECT_COPY | DROPEFFECT_MOVE))
	{
		ExecuteActionFromContextMenu(selectedItemPidl.get(), {}, m_hTreeView, L"paste", 0, nullptr);
	}
	else
	{
		std::wstring destinationPath;
		hr = GetDisplayName(selectedItemPidl.get(), SHGDN_FORPARSING, destinationPath);

		if (FAILED(hr))
		{
			return;
		}

		DropHandler *dropHandler = DropHandler::CreateNew();
		dropHandler->CopyClipboardData(clipboardObject.get(), m_hTreeView, destinationPath.c_str(),
			nullptr);
		dropHandler->Release();
	}
}

void ShellTreeView::PasteShortcut()
{
	auto *selectedNode = GetNodeFromTreeViewItem(TreeView_GetSelection(m_hTreeView));
	ExecuteActionFromContextMenu(selectedNode->GetFullPidl().get(), {}, m_hTreeView, L"pastelink",
		0, nullptr);
}

void ShellTreeView::UpdateCurrentClipboardObject(
	wil::com_ptr_nothrow<IDataObject> clipboardDataObject)
{
	// When copying an item, the WM_CLIPBOARDUPDATE message will be processed after the copy
	// operation has been fully completed. Therefore, any previously cut item will need to have its
	// state restored first. Relying on the WM_CLIPBOARDUPDATE handler wouldn't work, as by the time
	// it runs, m_cutItem would refer to the newly cut item.
	if (m_cutItem)
	{
		UpdateItemState(m_cutItem, TVIS_CUT, 0);
	}

	m_clipboardDataObject = clipboardDataObject;
}

void ShellTreeView::OnClipboardUpdate()
{
	if (m_clipboardDataObject && OleIsCurrentClipboard(m_clipboardDataObject.get()) == S_FALSE)
	{
		if (m_cutItem)
		{
			UpdateItemState(m_cutItem, TVIS_CUT, 0);

			m_cutItem = nullptr;
		}

		m_clipboardDataObject.reset();
	}
}

void ShellTreeView::UpdateItemState(HTREEITEM item, UINT stateMask, UINT state)
{
	TVITEM tvItem;
	tvItem.mask = TVIF_HANDLE | TVIF_STATE;
	tvItem.hItem = item;
	tvItem.stateMask = stateMask;
	tvItem.state = state;
	[[maybe_unused]] bool res = TreeView_SetItem(m_hTreeView, &tvItem);
	assert(res);
}
