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
#include "App.h"
#include "BrowserPane.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "CoreInterface.h"
#include "ItemNameEditControl.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
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
#include "../Helper/MenuHelper.h"
#include "../Helper/ScopedRedrawDisabler.h"
#include "../Helper/ShellContextMenu.h"
#include "../Helper/ShellHelper.h"
#include <wil/common.h>
#include <propkey.h>

ShellTreeView *ShellTreeView::Create(HWND hParent, App *app, BrowserWindow *browserWindow,
	CoreInterface *coreInterface, FileActionHandler *fileActionHandler, CachedIcons *cachedIcons)
{
	return new ShellTreeView(hParent, app, browserWindow, coreInterface, fileActionHandler,
		cachedIcons);
}

ShellTreeView::ShellTreeView(HWND hParent, App *app, BrowserWindow *browserWindow,
	CoreInterface *coreInterface, FileActionHandler *fileActionHandler, CachedIcons *cachedIcons) :
	ShellDropTargetWindow(CreateTreeView(hParent)),
	m_hTreeView(GetHWND()),
	m_app(app),
	m_browserWindow(browserWindow),
	m_coreInterface(coreInterface),
	m_config(app->GetConfig()),
	m_fileActionHandler(fileActionHandler),
	m_fontSetter(GetHWND(), app->GetConfig()),
	m_iconThreadPool(1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED),
		CoUninitialize),
	m_iconResultIDCounter(0),
	m_subfoldersThreadPool(1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED),
		CoUninitialize),
	m_subfoldersResultIDCounter(0),
	m_cachedIcons(cachedIcons),
	m_dropExpandItem(nullptr),
	m_shellChangeWatcher(GetHWND(),
		std::bind_front(&ShellTreeView::ProcessShellChangeNotifications, this))
{
	TreeView_SetExtendedStyle(m_hTreeView, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hTreeView,
		std::bind_front(&ShellTreeView::TreeViewProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(hParent,
		std::bind_front(&ShellTreeView::ParentWndProc, this)));

	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(m_iFolderIcon));

	m_bDragCancelled = FALSE;
	m_bDragAllowed = FALSE;
	m_bShowHidden = TRUE;

	AddRootItems();

	m_getDragImageMessage = RegisterWindowMessage(DI_GETDRAGIMAGE);

	AddClipboardFormatListener(m_hTreeView);

	StartDirectoryMonitoringForDrives();

	m_connections.push_back(m_browserWindow->AddBrowserInitializedObserver(
		[this]()
		{
			m_browserInitialized = true;

			// Updating the treeview selection is relatively expensive, so it's not done at all
			// during startup. Therefore, the selection will be set a single time, once the
			// application initialization is complete and all tabs have been restored.
			UpdateSelection();
		}));

	m_connections.push_back(m_config->synchronizeTreeview.addObserver(
		std::bind(&ShellTreeView::UpdateSelection, this)));

	m_connections.push_back(
		m_config->showFolders.addObserver(std::bind(&ShellTreeView::UpdateSelection, this)));

	auto *tabContainer = m_browserWindow->GetActivePane()->GetTabContainer();

	m_connections.push_back(tabContainer->tabNavigationCommittedSignal.AddObserver(
		[this](const Tab &tab, const NavigationRequest *request)
		{
			UNREFERENCED_PARAMETER(request);

			if (m_browserWindow->GetActivePane()->GetTabContainer()->IsTabSelected(tab))
			{
				UpdateSelection();
			}
		}));

	m_connections.push_back(tabContainer->tabNavigationFailedSignal.AddObserver(
		[this](const Tab &tab, const NavigationRequest *request)
		{
			UNREFERENCED_PARAMETER(request);

			if (m_browserWindow->GetActivePane()->GetTabContainer()->IsTabSelected(tab))
			{
				// When manually selecting an item in the treeview, a navigation will be initiated.
				// It's possible that navigation may fail, in which case, the selection will be
				// reset here.
				UpdateSelection();
			}
		}));

	m_connections.push_back(tabContainer->tabNavigationCancelledSignal.AddObserver(
		[this](const Tab &tab, const NavigationRequest *request)
		{
			UNREFERENCED_PARAMETER(request);

			if (m_browserWindow->GetActivePane()->GetTabContainer()->IsTabSelected(tab))
			{
				UpdateSelection();
			}
		}));

	m_connections.push_back(tabContainer->tabSelectedSignal.AddObserver(
		std::bind(&ShellTreeView::UpdateSelection, this)));

	m_connections.push_back(m_cutCopiedItemManager.cutItemChangedSignal.AddObserver(
		std::bind_front(&ShellTreeView::OnCutItemChanged, this)));
	m_connections.push_back(m_config->showQuickAccessInTreeView.addObserver(
		std::bind_front(&ShellTreeView::OnShowQuickAccessUpdated, this)));
}

HWND ShellTreeView::CreateTreeView(HWND parent)
{
	return ::CreateTreeView(parent,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_SHOWSELALWAYS | TVS_HASBUTTONS | TVS_EDITLABELS
			| TVS_HASLINES | TVS_TRACKSELECT);
}

ShellTreeView::~ShellTreeView()
{
	if (m_cutCopiedItemManager.GetCutCopiedClipboardDataObject()
		&& OleIsCurrentClipboard(m_cutCopiedItemManager.GetCutCopiedClipboardDataObject()) == S_OK)
	{
		OleFlushClipboard();
	}

	m_iconThreadPool.clear_queue();
}

LRESULT ShellTreeView::TreeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_getDragImageMessage != 0 && msg == m_getDragImageMessage)
	{
		return FALSE;
	}

	switch (msg)
	{
	case WM_SETFOCUS:
		m_coreInterface->FocusChanged();
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
	// WM_CONTEXTMENU will be sent to the treeview window procedure when pressing Shift + F10 or
	// VK_APPS. However, when right-clicking, the WM_CONTEXTMENU message will be sent to the parent.
	// Since WM_CONTEXTMENU messages are sent to the parent if they're not handled, it's easiest to
	// simply handle WM_CONTEXTMENU here, which will cover all three ways in which it can be
	// triggered.
	case WM_CONTEXTMENU:
		if (reinterpret_cast<HWND>(wParam) == m_hTreeView)
		{
			OnShowContextMenu({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
			return 0;
		}
		break;

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

			case TVN_BEGINLABELEDIT:
				return OnBeginLabelEdit(reinterpret_cast<NMTVDISPINFO *>(lParam));

			case TVN_ENDLABELEDIT:
				return OnEndLabelEdit(reinterpret_cast<NMTVDISPINFO *>(lParam));

			case TVN_SELCHANGED:
				OnSelectionChanged(reinterpret_cast<NMTREEVIEW *>(lParam));
				break;
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
		ParseShellFolderNameAndCheckExistence(HOME_FOLDER_PATH, wil::out_param(quickAccessPidl));

	if (FAILED(hr))
	{
		hr = ParseShellFolderNameAndCheckExistence(QUICK_ACCESS_PATH,
			wil::out_param(quickAccessPidl));

		if (FAILED(hr))
		{
			return;
		}
	}

	m_quickAccessRootItem = AddRootItem(quickAccessPidl.get(), TVI_FIRST);
}

// Typically, something like SHParseDisplayName() will fail if the item doesn't exist. However,
// that's seemingly not true for shell:{CLSID} paths. When passed one of those paths,
// SHParseDisplayName() will succeed, regardless of whether or not the item is valid.
// Therefore, this function will perform a basic check to determine whether the item is valid before
// returning.
HRESULT ShellTreeView::ParseShellFolderNameAndCheckExistence(const std::wstring &shellFolderPath,
	PIDLIST_ABSOLUTE *pidl)
{
	wil::com_ptr_nothrow<IShellItem> shellItem;
	RETURN_IF_FAILED(
		SHCreateItemFromParsingName(shellFolderPath.c_str(), nullptr, IID_PPV_ARGS(&shellItem)));

	SFGAOF attributes = SFGAO_FOLDER;
	RETURN_IF_FAILED(shellItem->GetAttributes(attributes, &attributes));

	if (WI_IsFlagClear(attributes, SFGAO_FOLDER))
	{
		return E_FAIL;
	}

	return SHGetIDListFromObject(shellItem.get(), pidl);
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
			ptvItem->iImage = *cachedIconIndex;
			ptvItem->iSelectedImage = *cachedIconIndex;
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

	return m_cachedIcons->MaybeGetIconIndex(filePath);
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

	auto iconInfo = ExtractShellIconParts(shfi.iIcon);

	IconResult result;
	result.nodeId = nodeId;
	result.treeItem = treeItem;
	result.iconIndex = iconInfo.iconIndex;
	result.overlayIndex = iconInfo.overlayIndex;
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
		m_cachedIcons->AddOrUpdateIcon(filePath, result->iconIndex);
	}

	TVITEM tvItem;
	tvItem.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvItem.hItem = result->treeItem;
	tvItem.iImage = result->iconIndex;
	tvItem.iSelectedImage = result->iconIndex;
	tvItem.stateMask = TVIS_OVERLAYMASK;
	tvItem.state = INDEXTOOVERLAYMASK(result->overlayIndex);
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

void ShellTreeView::OnSelectionChanged(const NMTREEVIEW *eventInfo)
{
	using namespace std::chrono_literals;

	if (!m_browserInitialized)
	{
		// This class will select an item initially (to ensure that there's always a selected item).
		// That will take place before the application has finished initializing. That initial
		// selection doesn't need to be handled in any way - either the selection will be updated
		// when a navigation occurs (if the synchronize treeview option is enabled), or the
		// selection will remain on the initial item (if the synchronize treeview option is
		// disabled), until the user manually selects another item.
		return;
	}

	m_selectionChangedTimer.cancel();
	m_selectionChangedEventInfo.reset();

	if (eventInfo->action == TVC_BYKEYBOARD && m_config->treeViewDelayEnabled)
	{
		m_selectionChangedEventInfo = *eventInfo;

		// This makes it possible to navigate in the treeview using the keyboard, without triggering
		// a stream of navigations (in the case where a key is being held down and the selection is
		// continuously changing).
#pragma warning(push)
#pragma warning(                                                                                   \
	disable : 4244) // 'argument': conversion from '_Rep' to 'size_t', possible loss of data
		m_selectionChangedTimer = m_app->GetRuntime()->GetTimerQueue()->make_one_shot_timer(500ms,
			m_app->GetRuntime()->GetUiThreadExecutor(),
			std::bind_front(&ShellTreeView::OnSelectionChangedTimer, this));
#pragma warning(pop)
	}
	else
	{
		HandleSelectionChanged(eventInfo);
	}
}

void ShellTreeView::OnSelectionChangedTimer()
{
	CHECK(m_selectionChangedEventInfo);
	HandleSelectionChanged(&*m_selectionChangedEventInfo);
	m_selectionChangedEventInfo.reset();
}

void ShellTreeView::HandleSelectionChanged(const NMTREEVIEW *eventInfo)
{
	auto *shellBrowser = GetSelectedShellBrowser();
	auto pidlCurrentDirectory = shellBrowser->GetDirectoryIdl();

	auto pidlDirectory = GetNodePidl(eventInfo->itemNew.hItem);

	if (ArePidlsEquivalent(pidlDirectory.get(), pidlCurrentDirectory.get()))
	{
		return;
	}

	auto navigateParams = NavigateParams::Normal(pidlDirectory.get());
	shellBrowser->GetNavigationController()->Navigate(navigateParams);

	// The folder will only be expanded if the user explicitly selected it.
	if (m_config->treeViewAutoExpandSelected
		&& (eventInfo->action == TVC_BYMOUSE || eventInfo->action == TVC_BYKEYBOARD))
	{
		TreeView_Expand(m_hTreeView, eventInfo->itemNew.hItem, TVE_EXPAND);
	}
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

	case VK_INSERT:
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			CopySelectedItemToClipboard(true);
		}
		if (!IsKeyDown(VK_CONTROL) && IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
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
	HRESULT hr = SHBindToObject(nullptr, pidlDirectory.get(), nullptr, IID_PPV_ARGS(&shellFolder2));

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

	ScopedRedrawDisabler redrawDisabler(m_hTreeView);

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

	ShellTreeNode *parentNode = GetNodeFromTreeViewItem(hParent);
	StartDirectoryMonitoringForNode(parentNode);

	return hr;
}

HTREEITEM ShellTreeView::AddItem(HTREEITEM parent, PCIDLIST_ABSOLUTE pidl, HTREEITEM insertAfter)
{
	wil::com_ptr_nothrow<IShellItem2> shellItem;
	HRESULT hr = SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&shellItem));

	if (FAILED(hr))
	{
		// It's not expected for the SHCreateItemFromIDList() call to fail, so it would be useful to
		// know if it does.
		assert(false);
		return nullptr;
	}

	ShellTreeNodeType nodeType = parent ? ShellTreeNodeType::Child : ShellTreeNodeType::Root;
	auto node = std::make_unique<ShellTreeNode>(nodeType, pidl, shellItem.get());

	wil::unique_cotaskmem_string displayName;
	hr = node->GetShellItem()->GetDisplayName(DISPLAY_NAME_TYPE, &displayName);

	if (FAILED(hr))
	{
		assert(false);
		return nullptr;
	}

	auto *rawNode = node.get();

	if (parent)
	{
		auto *parentNode = GetNodeFromTreeViewItem(parent);
		parentNode->AddChild(std::move(node));
	}
	else
	{
		m_nodes.push_back(std::move(node));
	}

	TVITEMEX tvItem = {};
	tvItem.mask =
		TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN | TVIF_STATE;
	tvItem.pszText = displayName.get();
	tvItem.iImage = I_IMAGECALLBACK;
	tvItem.iSelectedImage = I_IMAGECALLBACK;
	tvItem.lParam = reinterpret_cast<LPARAM>(rawNode);
	tvItem.cChildren = I_CHILDRENCALLBACK;
	tvItem.stateMask = TVIS_CUT;
	tvItem.state = TestItemAttributes(rawNode, SFGAO_HIDDEN) ? TVIS_CUT : 0;

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
	m_browserWindow->OpenItem(pidl.get(), OpenFolderDisposition::ForegroundTab);
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
	auto pidl = GetNodePidl(item);

	m_fileActionHandler->DeleteFiles(m_hTreeView, { pidl.get() }, permanent, false);
}

bool ShellTreeView::OnBeginLabelEdit(const NMTVDISPINFO *dispInfo)
{
	const auto *node = GetNodeFromTreeViewItem(dispInfo->item.hItem);

	SFGAOF attributes = SFGAO_CANRENAME;
	HRESULT hr = node->GetShellItem()->GetAttributes(attributes, &attributes);

	if (FAILED(hr) || (SUCCEEDED(hr) && WI_IsFlagClear(attributes, SFGAO_CANRENAME)))
	{
		return true;
	}

	wil::unique_cotaskmem_string editingName;
	hr = node->GetShellItem()->GetDisplayName(SIGDN_PARENTRELATIVEEDITING, &editingName);

	if (FAILED(hr))
	{
		return false;
	}

	HWND editControl = TreeView_GetEditControl(m_hTreeView);
	SetWindowText(editControl, editingName.get());

	ItemNameEditControl::CreateNew(editControl, nullptr, false);

	return false;
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

	// This needs to be copied here, as the call to SHBindToParent() below will return the pidl of
	// the child item. That pidl points to a location within the full pidl, so it's important that
	// the full pidl isn't freed.
	auto currentPidl = node->GetFullPidl();

	wil::com_ptr_nothrow<IShellFolder> parent;
	PCITEMID_CHILD child;
	HRESULT hr = SHBindToParent(currentPidl.get(), IID_PPV_ARGS(&parent), &child);

	if (FAILED(hr))
	{
		return false;
	}

	unique_pidl_child newChild;
	hr = parent->SetNameOf(m_hTreeView, child, dispInfo->item.pszText, SHGDN_INFOLDER,
		wil::out_param(newChild));

	if (FAILED(hr) || hr == S_FALSE || !newChild)
	{
		return false;
	}

	unique_pidl_absolute pidlParent;
	hr = SHGetIDListFromObject(parent.get(), wil::out_param(pidlParent));

	if (FAILED(hr))
	{
		return false;
	}

	unique_pidl_absolute pidlNew(ILCombine(pidlParent.get(), newChild.get()));
	OnItemUpdated(node->GetFullPidl().get(), pidlNew.get());

	// There's no need to keep the updated text, as it will have been replaced in the call to
	// OnItemRenamed().
	return false;
}

void ShellTreeView::OnShowContextMenu(const POINT &ptScreen)
{
	HTREEITEM targetItem;
	POINT finalPoint;
	bool highlightTargetItem = false;

	if (ptScreen.x == -1 && ptScreen.y == -1)
	{
		HTREEITEM selection = TreeView_GetSelection(m_hTreeView);

		RECT itemRect;
		TreeView_GetItemRect(m_hTreeView, selection, &itemRect, TRUE);

		finalPoint = { itemRect.left, itemRect.top + (itemRect.bottom - itemRect.top) / 2 };
		ClientToScreen(m_hTreeView, &finalPoint);

		targetItem = selection;
	}
	else
	{
		POINT ptClient = ptScreen;
		ScreenToClient(m_hTreeView, &ptClient);

		TVHITTESTINFO hitTestInfo = {};
		hitTestInfo.pt = ptClient;
		auto item = TreeView_HitTest(m_hTreeView, &hitTestInfo);

		if (!item)
		{
			return;
		}

		finalPoint = ptScreen;
		targetItem = item;
		highlightTargetItem = true;
	}

	if (highlightTargetItem)
	{
		TreeView_SetItemState(m_hTreeView, targetItem, TVIS_DROPHILITED, TVIS_DROPHILITED);
	}

	auto pidl = GetNodePidl(targetItem);

	unique_pidl_child child(ILCloneChild(ILFindLastID(pidl.get())));

	ILRemoveLastID(pidl.get());

	ShellContextMenu::Flags flags = ShellContextMenu::Flags::Rename;

	if (IsKeyDown(VK_SHIFT))
	{
		WI_SetFlag(flags, ShellContextMenu::Flags::ExtendedVerbs);
	}

	ShellContextMenu shellContextMenu(pidl.get(), { child.get() }, this,
		m_coreInterface->GetStatusBar());
	shellContextMenu.ShowMenu(m_hTreeView, &finalPoint, nullptr, flags);

	if (highlightTargetItem)
	{
		TreeView_SetItemState(m_hTreeView, targetItem, 0, TVIS_DROPHILITED);
	}
}

void ShellTreeView::UpdateMenuEntries(HMENU menu, PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, IContextMenu *contextMenu)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(contextMenu);

	std::wstring openInNewTabText = ResourceHelper::LoadString(
		m_coreInterface->GetResourceInstance(), IDS_GENERAL_OPEN_IN_NEW_TAB);
	MenuHelper::AddStringItem(menu, OPEN_IN_NEW_TAB_MENU_ITEM_ID, openInNewTabText, 1, true);
}

std::wstring ShellTreeView::GetHelpTextForItem(UINT menuItemId)
{
	switch (menuItemId)
	{
	case OPEN_IN_NEW_TAB_MENU_ITEM_ID:
		return ResourceHelper::LoadString(m_coreInterface->GetResourceInstance(),
			IDS_GENERAL_OPEN_IN_NEW_TAB_HELP_TEXT);

	default:
		DCHECK(false);
		return L"";
	}
}

bool ShellTreeView::HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, const std::wstring &verb)
{
	assert(pidlItems.size() == 1);

	if (verb == L"rename")
	{
		unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0].Raw()));
		StartRenamingItem(pidlComplete.get());

		return true;
	}
	else if (verb == L"copy")
	{
		unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0].Raw()));
		CopyItemToClipboard(pidlComplete.get(), true);

		return true;
	}
	else if (verb == L"cut")
	{
		unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0].Raw()));
		CopyItemToClipboard(pidlComplete.get(), false);

		return true;
	}

	return false;
}

void ShellTreeView::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, UINT menuItemId)
{
	assert(pidlItems.size() == 1);

	switch (menuItemId)
	{
	case OPEN_IN_NEW_TAB_MENU_ITEM_ID:
	{
		unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0].Raw()));
		auto disposition = m_config->openTabsInForeground ? OpenFolderDisposition::ForegroundTab
														  : OpenFolderDisposition::BackgroundTab;
		m_browserWindow->OpenItem(pidlComplete.get(), disposition);
	}
	break;
	}
}

void ShellTreeView::UpdateSelection()
{
	if (!m_browserInitialized || !m_config->synchronizeTreeview.get()
		|| !m_config->showFolders.get())
	{
		return;
	}

	auto *selectedShellBrowser = GetSelectedShellBrowser();

	// When locating a folder in the treeview, each of the parent folders has to be enumerated. UNC
	// paths are contained within the Network folder and that folder can take a significant amount
	// of time to enumerate (e.g. 30 seconds).
	// Therefore, locating a UNC path can take a non-trivial amount of time, as the Network folder
	// will have to be enumerated first. As that work is all done on the main thread, the
	// application will hang while the enumeration completes, something that's especially noticeable
	// on startup.
	// Note that mapped drives don't have that specific issue, as they're contained within the This
	// PC folder. However, there is still the general problem that each parent folder has to be
	// enumerated and all the work is done on the main thread.
	if (PathIsUNC(selectedShellBrowser->GetDirectory().c_str()))
	{
		return;
	}

	HTREEITEM item = LocateItem(selectedShellBrowser->GetDirectoryIdl().get());

	if (!item)
	{
		return;
	}

	TreeView_SelectItem(m_hTreeView, item);
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

	std::vector<PidlAbsolute> items = { pidl.get() };
	wil::com_ptr_nothrow<IDataObject> clipboardDataObject;
	HRESULT hr;

	if (copy)
	{
		hr = CopyFiles(items, &clipboardDataObject);

		if (SUCCEEDED(hr))
		{
			m_cutCopiedItemManager.SetCopiedItem(clipboardDataObject.get());
		}
	}
	else
	{
		hr = CutFiles(items, &clipboardDataObject);

		if (SUCCEEDED(hr))
		{
			m_cutCopiedItemManager.SetCutItem(treeItem, clipboardDataObject.get());
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

	if (CanShellPasteDataObject(selectedItemPidl.get(), clipboardObject.get(), PasteType::Normal))
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

void ShellTreeView::OnClipboardUpdate()
{
	if (m_cutCopiedItemManager.GetCutCopiedClipboardDataObject()
		&& OleIsCurrentClipboard(m_cutCopiedItemManager.GetCutCopiedClipboardDataObject())
			== S_FALSE)
	{
		m_cutCopiedItemManager.ClearCutCopiedItem();
	}
}

void ShellTreeView::OnCutItemChanged(HTREEITEM previousCutItem, HTREEITEM newCutItem)
{
	if (previousCutItem)
	{
		UpdateItemState(previousCutItem, TVIS_CUT, ShouldGhostItem(previousCutItem) ? TVIS_CUT : 0);
	}

	if (newCutItem)
	{
		UpdateItemState(newCutItem, TVIS_CUT, ShouldGhostItem(newCutItem) ? TVIS_CUT : 0);
	}
}

bool ShellTreeView::ShouldGhostItem(HTREEITEM item)
{
	auto *node = GetNodeFromTreeViewItem(item);

	if (TestItemAttributes(node, SFGAO_HIDDEN))
	{
		return true;
	}

	auto cutItem = m_cutCopiedItemManager.GetCutItem();

	if (cutItem && cutItem == item)
	{
		return true;
	}

	return false;
}

bool ShellTreeView::TestItemAttributes(ShellTreeNode *node, SFGAOF attributes)
{
	SFGAOF commonAttributes = attributes;
	HRESULT hr = node->GetShellItem()->GetAttributes(commonAttributes, &commonAttributes);

	if (FAILED(hr))
	{
		return false;
	}

	return (commonAttributes & attributes) == attributes;
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

ShellBrowserImpl *ShellTreeView::GetSelectedShellBrowser() const
{
	return m_browserWindow->GetActivePane()
		->GetTabContainer()
		->GetSelectedTab()
		.GetShellBrowserImpl();
}

void ShellTreeView::CutCopiedItemManager::SetCopiedItem(IDataObject *clipboardDataObject)
{
	SetDataInternal(nullptr, clipboardDataObject);
}

void ShellTreeView::CutCopiedItemManager::SetCutItem(HTREEITEM cutItem,
	IDataObject *clipboardDataObject)
{
	SetDataInternal(cutItem, clipboardDataObject);
}

void ShellTreeView::CutCopiedItemManager::ClearCutCopiedItem()
{
	SetDataInternal(nullptr, nullptr);
}

void ShellTreeView::CutCopiedItemManager::SetDataInternal(HTREEITEM cutItem,
	IDataObject *clipboardDataObject)
{
	if (cutItem != m_cutItem)
	{
		HTREEITEM previousCutItem = m_cutItem;
		m_cutItem = cutItem;
		cutItemChangedSignal.m_signal(previousCutItem, cutItem);
	}

	m_clipboardDataObject = clipboardDataObject;
}

HTREEITEM ShellTreeView::CutCopiedItemManager::GetCutItem() const
{
	return m_cutItem;
}

IDataObject *ShellTreeView::CutCopiedItemManager::GetCutCopiedClipboardDataObject() const
{
	return m_clipboardDataObject.get();
}
