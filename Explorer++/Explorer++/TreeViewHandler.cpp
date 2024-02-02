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
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <glog/logging.h>

LRESULT CALLBACK TreeViewHolderProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK TreeViewSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

void Explorerplusplus::CreateFolderControls()
{
	UINT holderStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (m_config->showFolders.get())
	{
		holderStyle |= WS_VISIBLE;
	}

	m_treeViewHolder = HolderWindow::Create(m_hContainer,
		ResourceHelper::LoadString(m_resourceInstance, IDS_FOLDERS_WINDOW_TEXT), holderStyle,
		ResourceHelper::LoadString(m_resourceInstance, IDS_HIDE_FOLDERS_PANE), this);
	m_treeViewHolder->SetCloseButtonClickedCallback(
		std::bind(&Explorerplusplus::ToggleFolders, this));
	m_treeViewHolder->SetResizedCallback(
		std::bind_front(&Explorerplusplus::OnTreeViewHolderResized, this));

	SetWindowSubclass(m_treeViewHolder->GetHWND(), TreeViewHolderProcStub, 0, (DWORD_PTR) this);

	m_shellTreeView = ShellTreeView::Create(m_treeViewHolder->GetHWND(), this, this,
		&m_FileActionHandler, &m_cachedIcons);
	m_treeViewHolder->SetContentChild(m_shellTreeView->GetHWND());

	/* Now, subclass the treeview again. This is needed for messages
	such as WM_MOUSEWHEEL, which need to be intercepted before they
	reach the window procedure provided by ShellTreeView. */
	SetWindowSubclass(m_shellTreeView->GetHWND(), TreeViewSubclassStub, 1, (DWORD_PTR) this);

	m_treeViewInitialized = true;
}

LRESULT CALLBACK TreeViewSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pContainer = (Explorerplusplus *) dwRefData;

	return pContainer->TreeViewSubclass(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewSubclass(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SETFOCUS:
		FocusChanged();
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void Explorerplusplus::OnTreeViewCopyItemPath() const
{
	auto hItem = TreeView_GetSelection(m_shellTreeView->GetHWND());

	if (hItem != nullptr)
	{
		auto pidl = m_shellTreeView->GetNodePidl(hItem);

		std::wstring fullFileName;
		GetDisplayName(pidl.get(), SHGDN_FORPARSING, fullFileName);

		BulkClipboardWriter clipboardWriter;
		clipboardWriter.WriteText(fullFileName);
	}
}

void Explorerplusplus::OnTreeViewCopyUniversalPaths() const
{
	HTREEITEM hItem;
	UNIVERSAL_NAME_INFO uni;
	DWORD dwBufferSize;
	DWORD dwRet;

	hItem = TreeView_GetSelection(m_shellTreeView->GetHWND());

	if (hItem != nullptr)
	{
		auto pidl = m_shellTreeView->GetNodePidl(hItem);

		std::wstring fullFileName;
		GetDisplayName(pidl.get(), SHGDN_FORPARSING, fullFileName);

		dwBufferSize = sizeof(uni);
		dwRet = WNetGetUniversalName(fullFileName.c_str(), UNIVERSAL_NAME_INFO_LEVEL,
			(void **) &uni, &dwBufferSize);

		BulkClipboardWriter clipboardWriter;

		if (dwRet == NO_ERROR)
		{
			clipboardWriter.WriteText(uni.lpUniversalName);
		}
		else
		{
			clipboardWriter.WriteText(fullFileName);
		}
	}
}

void Explorerplusplus::OnTreeViewSelectionChangedTimer()
{
	// It's important that the timer be killed here, before the navigation has started. Otherwise,
	// what can happen is that if access to the folder is denied, a dialog will be shown and the
	// message loop will run. That will then cause the timer to fire again, which will start another
	// navigation, ad infinitum.
	KillTimer(m_treeViewHolder->GetHWND(), TREEVIEW_SELECTION_CHANGED_TIMER_ID);

	CHECK(m_treeViewSelectionChangedEventInfo);
	HandleTreeViewSelectionChanged(&*m_treeViewSelectionChangedEventInfo);
	m_treeViewSelectionChangedEventInfo.reset();
}

void Explorerplusplus::OnTreeViewSelectionChanged(const NMTREEVIEW *eventInfo)
{
	if (!m_treeViewInitialized)
	{
		// The ShellTreeView will select an item initially (to ensure that there's always a selected
		// item). That will take place before the treeview has finished initializing. That initial
		// selection doesn't need to be handled in any way - either the selection will be updated
		// when a navigation occurs (if the synchronize treeview option is enabled), or the
		// selection will remain on the initial item (if the synchronize treeview option is
		// disabled), until the user manually selects another item.
		return;
	}

	KillTimer(m_treeViewHolder->GetHWND(), TREEVIEW_SELECTION_CHANGED_TIMER_ID);
	m_treeViewSelectionChangedEventInfo.reset();

	if (eventInfo->action == TVC_BYKEYBOARD && m_config->treeViewDelayEnabled)
	{
		m_treeViewSelectionChangedEventInfo = *eventInfo;

		// This makes it possible to navigate in the treeview using the keyboard, without triggering
		// a stream of navigations (in the case where a key is being held down and the selection is
		// continuously changing).
		SetTimer(m_treeViewHolder->GetHWND(), TREEVIEW_SELECTION_CHANGED_TIMER_ID,
			TREEVIEW_SELECTION_CHANGED_TIMEOUT, nullptr);
	}
	else
	{
		HandleTreeViewSelectionChanged(eventInfo);
	}
}

void Explorerplusplus::HandleTreeViewSelectionChanged(const NMTREEVIEW *eventInfo)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	auto pidlCurrentDirectory = selectedTab.GetShellBrowser()->GetDirectoryIdl();

	auto pidlDirectory = m_shellTreeView->GetNodePidl(eventInfo->itemNew.hItem);

	if (ArePidlsEquivalent(pidlDirectory.get(), pidlCurrentDirectory.get()))
	{
		return;
	}

	auto navigateParams = NavigateParams::Normal(pidlDirectory.get());
	HRESULT hr = selectedTab.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);

	if (SUCCEEDED(hr))
	{
		// The folder will only be expanded if the user explicitly selected it.
		if (m_config->treeViewAutoExpandSelected
			&& (eventInfo->action == TVC_BYMOUSE || eventInfo->action == TVC_BYKEYBOARD))
		{
			TreeView_Expand(m_shellTreeView->GetHWND(), eventInfo->itemNew.hItem, TVE_EXPAND);
		}
	}
}

LRESULT CALLBACK TreeViewHolderProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pContainer = (Explorerplusplus *) dwRefData;

	return pContainer->TreeViewHolderProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewHolderProc(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_NOTIFY:
		return TreeViewHolderWindowNotifyHandler(hwnd, msg, wParam, lParam);

	case WM_TIMER:
		if (wParam == TREEVIEW_SELECTION_CHANGED_TIMER_ID)
		{
			OnTreeViewSelectionChangedTimer();
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::TreeViewHolderWindowNotifyHandler(HWND hwnd, UINT msg,
	WPARAM wParam, LPARAM lParam)
{
	switch (((LPNMHDR) lParam)->code)
	{
	case TVN_SELCHANGED:
		OnTreeViewSelectionChanged(reinterpret_cast<NMTREEVIEW *>(lParam));
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
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

	auto pidlItem = m_shellTreeView->GetNodePidl(hItem);

	std::wstring fullFileName;
	HRESULT hr = GetDisplayName(pidlItem.get(), SHGDN_FORPARSING, fullFileName);

	if (hr == S_OK)
	{
		StringCchCopy(sfai.szFullFileName, SIZEOF_ARRAY(sfai.szFullFileName), fullFileName.c_str());

		HANDLE hFindFile = FindFirstFile(sfai.szFullFileName, &sfai.wfd);

		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			FindClose(hFindFile);

			sfaiList.push_back(sfai);

			SetFileAttributesDialog setFileAttributesDialog(m_resourceInstance, m_hContainer,
				sfaiList);
			setFileAttributesDialog.ShowModalDialog();
		}
	}
}

void Explorerplusplus::OnTreeViewHolderResized(int newWidth)
{
	m_config->treeViewWidth = newWidth;

	UpdateLayout();
}
