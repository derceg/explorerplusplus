// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "DarkModeHelper.h"
#include "HolderWindow.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "ResourceHelper.h"
#include "SetFileAttributesDialog.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTreeView/ShellTreeView.h"
#include "TabContainer.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"

#define TREEVIEW_FOLDER_OPEN_DELAY 500
#define FOLDERS_TOOLBAR_CLOSE 6000

LRESULT CALLBACK TreeViewHolderProcStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK TreeViewSubclassStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

/* Used to keep track of which item was selected in
the treeview control. */
HTREEITEM g_newSelectionItem;

void Explorerplusplus::CreateFolderControls()
{
	TCHAR szTemp[32];
	UINT uStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (m_config->showFolders)
	{
		uStyle |= WS_VISIBLE;
	}

	LoadString(m_hLanguageModule, IDS_FOLDERS_WINDOW_TEXT, szTemp, SIZEOF_ARRAY(szTemp));
	m_hHolder = CreateHolderWindow(m_hContainer, szTemp, uStyle);
	SetWindowSubclass(m_hHolder, TreeViewHolderProcStub, 0, (DWORD_PTR) this);

	m_shellTreeView = new ShellTreeView(
		m_hHolder, this, m_pDirMon, m_tabContainer, &m_FileActionHandler, &m_cachedIcons);

	/* Now, subclass the treeview again. This is needed for messages
	such as WM_MOUSEWHEEL, which need to be intercepted before they
	reach the window procedure provided by ShellTreeView. */
	SetWindowSubclass(m_shellTreeView->GetHWND(), TreeViewSubclassStub, 1, (DWORD_PTR) this);

	m_foldersToolbarParent =
		CreateWindow(WC_STATIC, EMPTY_STRING, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0,
			m_hHolder, nullptr, GetModuleHandle(nullptr), nullptr);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_foldersToolbarParent,
		std::bind(&Explorerplusplus::FoldersToolbarParentProc, this, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
		0));

	m_hFoldersToolbar = CreateTabToolbar(m_foldersToolbarParent, FOLDERS_TOOLBAR_CLOSE,
		ResourceHelper::LoadString(m_hLanguageModule, IDS_HIDEFOLDERSPANE));

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hHolder);

	int scaledCloseToolbarWidth = MulDiv(CLOSE_TOOLBAR_WIDTH, dpi, USER_DEFAULT_SCREEN_DPI);
	int scaledCloseToolbarHeight = MulDiv(CLOSE_TOOLBAR_HEIGHT, dpi, USER_DEFAULT_SCREEN_DPI);
	SetWindowPos(m_foldersToolbarParent, nullptr, 0, 0, scaledCloseToolbarWidth,
		scaledCloseToolbarHeight, SWP_NOZORDER);
	SetWindowPos(m_hFoldersToolbar, nullptr, 0, 0, scaledCloseToolbarWidth,
		scaledCloseToolbarHeight, SWP_NOZORDER);

	m_InitializationFinished.addObserver([this](bool newValue) {
		if (newValue)
		{
			// Updating the treeview selection is relatively expensive, so it's
			// not done at all during startup. Therefore, the selection will be
			// set a single time, once the application initialization is
			// complete and all tabs have been restored.
			UpdateTreeViewSelection();
		}
	});

	m_tabContainer->tabCreatedSignal.AddObserver([this](int tabId, BOOL switchToNewTab) {
		UNREFERENCED_PARAMETER(tabId);
		UNREFERENCED_PARAMETER(switchToNewTab);

		UpdateTreeViewSelection();
	});

	m_tabContainer->tabNavigationCompletedSignal.AddObserver([this](const Tab &tab) {
		UNREFERENCED_PARAMETER(tab);

		UpdateTreeViewSelection();
	});

	m_tabContainer->tabSelectedSignal.AddObserver([this](const Tab &tab) {
		UNREFERENCED_PARAMETER(tab);

		UpdateTreeViewSelection();
	});

	m_tabContainer->tabRemovedSignal.AddObserver([this](int tabId) {
		UNREFERENCED_PARAMETER(tabId);

		UpdateTreeViewSelection();
	});
}

LRESULT CALLBACK Explorerplusplus::FoldersToolbarParentProc(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case FOLDERS_TOOLBAR_CLOSE:
			ToggleFolders();
			return 0;
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK TreeViewSubclassStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pContainer = (Explorerplusplus *) dwRefData;

	return pContainer->TreeViewSubclass(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewSubclass(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SETFOCUS:
		m_mainToolbar->UpdateToolbarButtonStates();
		m_hLastActiveWindow = m_shellTreeView->GetHWND();
		break;

	case WM_MOUSEWHEEL:
		if (OnMouseWheel(MousewheelSource::TreeView, wParam, lParam))
		{
			return 0;
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void Explorerplusplus::OnTreeViewRightClick(WPARAM wParam, LPARAM lParam)
{
	POINT *ppt = nullptr;
	HTREEITEM hItem;
	HTREEITEM hPrevItem;
	IShellFolder *pShellParentFolder = nullptr;
	PCITEMID_CHILD pidlRelative = nullptr;
	HRESULT hr;

	hItem = (HTREEITEM) wParam;
	ppt = (POINT *) lParam;

	m_bTreeViewRightClick = true;

	hPrevItem = TreeView_GetSelection(m_shellTreeView->GetHWND());
	TreeView_SelectItem(m_shellTreeView->GetHWND(), hItem);
	auto pidl = m_shellTreeView->GetItemPidl(hItem);

	hr = SHBindToParent(pidl.get(), IID_PPV_ARGS(&pShellParentFolder), &pidlRelative);

	if (SUCCEEDED(hr))
	{
		HTREEITEM hParent;

		hParent = TreeView_GetParent(m_shellTreeView->GetHWND(), hItem);

		/* If we right-click on the "Desktop" item in the treeview, there is no parent.
		   In such case, use "Desktop" as parent item as well, to allow the context menu
		   to be shown. */
		if (hParent == nullptr)
		{
			hParent = hItem;
		}

		if (hParent != nullptr)
		{
			auto pidlParent = m_shellTreeView->GetItemPidl(hParent);

			if (pidlParent)
			{
				m_bTreeViewOpenInNewTab = false;

				std::vector<PCITEMID_CHILD> pidlItems;
				pidlItems.push_back(pidlRelative);

				FileContextMenuManager fcmm(m_hContainer, pidlParent.get(), pidlItems);

				FileContextMenuInfo fcmi;
				fcmi.uFrom = FROM_TREEVIEW;

				StatusBar statusBar(m_hStatusBar);

				fcmm.ShowMenu(this, MIN_SHELL_MENU_ID, MAX_SHELL_MENU_ID, ppt, &statusBar,
					reinterpret_cast<DWORD_PTR>(&fcmi), TRUE, IsKeyDown(VK_SHIFT));
			}
		}

		pShellParentFolder->Release();
	}

	/* Don't switch back to the previous folder if
	the folder that was right-clicked was opened in
	a new tab (i.e. can just keep the selection the
	same). */
	if (!m_bTreeViewOpenInNewTab)
	{
		TreeView_SelectItem(m_shellTreeView->GetHWND(), hPrevItem);
	}

	m_bTreeViewRightClick = false;
}

void Explorerplusplus::OnTreeViewCopyItemPath() const
{
	auto hItem = TreeView_GetSelection(m_shellTreeView->GetHWND());

	if (hItem != nullptr)
	{
		auto pidl = m_shellTreeView->GetItemPidl(hItem);

		TCHAR szFullFileName[MAX_PATH];
		GetDisplayName(pidl.get(), szFullFileName, SIZEOF_ARRAY(szFullFileName), SHGDN_FORPARSING);

		BulkClipboardWriter clipboardWriter;
		clipboardWriter.WriteText(szFullFileName);
	}
}

void Explorerplusplus::OnTreeViewCopyUniversalPaths() const
{
	HTREEITEM hItem;
	TCHAR szFullFileName[MAX_PATH];
	UNIVERSAL_NAME_INFO uni;
	DWORD dwBufferSize;
	DWORD dwRet;

	hItem = TreeView_GetSelection(m_shellTreeView->GetHWND());

	if (hItem != nullptr)
	{
		auto pidl = m_shellTreeView->GetItemPidl(hItem);

		GetDisplayName(pidl.get(), szFullFileName, SIZEOF_ARRAY(szFullFileName), SHGDN_FORPARSING);

		dwBufferSize = sizeof(uni);
		dwRet = WNetGetUniversalName(
			szFullFileName, UNIVERSAL_NAME_INFO_LEVEL, (void **) &uni, &dwBufferSize);

		BulkClipboardWriter clipboardWriter;

		if (dwRet == NO_ERROR)
		{
			clipboardWriter.WriteText(uni.lpUniversalName);
		}
		else
		{
			clipboardWriter.WriteText(szFullFileName);
		}
	}
}

void Explorerplusplus::OnTreeViewHolderWindowTimer()
{
	auto pidlDirectory = m_shellTreeView->GetItemPidl(g_newSelectionItem);
	auto pidlCurrentDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

	if (!m_bSelectingTreeViewDirectory && !m_bTreeViewRightClick
		&& !CompareIdls(pidlDirectory.get(), pidlCurrentDirectory.get()))
	{
		Tab &selectedTab = m_tabContainer->GetSelectedTab();
		selectedTab.GetShellBrowser()->GetNavigationController()->BrowseFolder(pidlDirectory.get());

		if (m_config->treeViewAutoExpandSelected)
		{
			TreeView_Expand(m_shellTreeView->GetHWND(), g_newSelectionItem, TVE_EXPAND);
		}
	}

	KillTimer(m_hHolder, 0);
}

void Explorerplusplus::OnTreeViewSelChanged(LPARAM lParam)
{
	NMTREEVIEW *pnmtv = nullptr;
	TVITEM *tvItem = nullptr;

	/* Check whether the selection was changed because a new directory
	was browsed to, or if the treeview control is involved in a
	drag and drop operation. */
	if (!m_bSelectingTreeViewDirectory && !m_bTreeViewRightClick
		&& !m_shellTreeView->QueryDragging())
	{
		pnmtv = (LPNMTREEVIEW) lParam;

		tvItem = &pnmtv->itemNew;

		g_newSelectionItem = tvItem->hItem;

		if (m_config->treeViewDelayEnabled)
		{
			/* Schedule a folder change. This adds enough
			of a delay for the treeview selection to be changed
			without the current folder been changed immediately. */
			SetTimer(m_hHolder, 0, TREEVIEW_FOLDER_OPEN_DELAY, nullptr);
		}
		else
		{
			/* The treeview delay is disabled. For simplicity, just
			set a timer of length 0. */
			SetTimer(m_hHolder, 0, 0, nullptr);
		}
	}
	else
	{
		m_bSelectingTreeViewDirectory = false;
	}
}

LRESULT CALLBACK TreeViewHolderProcStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pContainer = (Explorerplusplus *) dwRefData;

	return pContainer->TreeViewHolderProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewHolderProc(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CTLCOLORSTATIC:
		if (auto result = OnHolderCtlColorStatic(
				reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam)))
		{
			return *result;
		}
		break;

	case WM_NOTIFY:
		return TreeViewHolderWindowNotifyHandler(hwnd, msg, wParam, lParam);

	case WM_TIMER:
		OnTreeViewHolderWindowTimer();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewHolderWindowNotifyHandler(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (((LPNMHDR) lParam)->code)
	{
	case TVN_SELCHANGED:
		OnTreeViewSelChanged(lParam);
		break;

	case NM_RCLICK:
	{
		NMHDR *nmhdr = nullptr;
		POINT ptCursor;
		DWORD dwPos;
		TVHITTESTINFO tvht;

		nmhdr = (NMHDR *) lParam;

		if (nmhdr->hwndFrom == m_shellTreeView->GetHWND())
		{
			dwPos = GetMessagePos();
			ptCursor.x = GET_X_LPARAM(dwPos);
			ptCursor.y = GET_Y_LPARAM(dwPos);

			tvht.pt = ptCursor;

			ScreenToClient(m_shellTreeView->GetHWND(), &tvht.pt);

			TreeView_HitTest(m_shellTreeView->GetHWND(), &tvht);

			if ((tvht.flags & TVHT_NOWHERE) == 0)
			{
				OnTreeViewRightClick((WPARAM) tvht.hItem, (LPARAM) &ptCursor);
			}
		}
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

std::optional<LRESULT> Explorerplusplus::OnHolderCtlColorStatic(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hdc);

	if (hwnd != m_foldersToolbarParent)
	{
		return std::nullopt;
	}

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return std::nullopt;
	}

	return reinterpret_cast<LRESULT>(darkModeHelper.GetBackgroundBrush());
}

void Explorerplusplus::OnTreeViewSetFileAttributes() const
{
	auto hItem = TreeView_GetSelection(m_shellTreeView->GetHWND());

	if (hItem == nullptr)
	{
		return;
	}

	std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo> sfaiList;
	NSetFileAttributesDialogExternal::SetFileAttributesInfo sfai;

	auto pidlItem = m_shellTreeView->GetItemPidl(hItem);
	HRESULT hr = GetDisplayName(
		pidlItem.get(), sfai.szFullFileName, SIZEOF_ARRAY(sfai.szFullFileName), SHGDN_FORPARSING);

	if (hr == S_OK)
	{
		HANDLE hFindFile = FindFirstFile(sfai.szFullFileName, &sfai.wfd);

		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			FindClose(hFindFile);

			sfaiList.push_back(sfai);

			SetFileAttributesDialog setFileAttributesDialog(
				m_hLanguageModule, m_hContainer, sfaiList);
			setFileAttributesDialog.ShowModalDialog();
		}
	}
}

void Explorerplusplus::UpdateTreeViewSelection()
{
	HTREEITEM hItem;
	TCHAR szDirectory[MAX_PATH];
	TCHAR szRoot[MAX_PATH];
	UINT uDriveType;
	BOOL bNetworkPath = FALSE;

	if (!m_InitializationFinished.get() || !m_config->synchronizeTreeview || !m_config->showFolders)
	{
		return;
	}

	auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

	GetDisplayName(pidlDirectory.get(), szDirectory, SIZEOF_ARRAY(szDirectory), SHGDN_FORPARSING);

	if (PathIsUNC(szDirectory))
	{
		bNetworkPath = TRUE;
	}
	else
	{
		StringCchCopy(szRoot, SIZEOF_ARRAY(szRoot), szDirectory);
		PathStripToRoot(szRoot);
		uDriveType = GetDriveType(szRoot);

		bNetworkPath = (uDriveType == DRIVE_REMOTE);
	}

	/* To improve performance, do not automatically sync the
	treeview with network or UNC paths. */
	if (!bNetworkPath)
	{
		hItem = m_shellTreeView->LocateItem(pidlDirectory.get());

		if (hItem != nullptr)
		{
			/* TVN_SELCHANGED is NOT sent when the new selected
			item is the same as the old selected item. It is only
			sent when the two are different.
			Therefore, the only case to handle is when the treeview
			selection is changed by browsing using the listview. */
			if (TreeView_GetSelection(m_shellTreeView->GetHWND()) != hItem)
			{
				m_bSelectingTreeViewDirectory = true;
			}

			SendMessage(
				m_shellTreeView->GetHWND(), TVM_SELECTITEM, (WPARAM) TVGN_CARET, (LPARAM) hItem);
		}
	}
}