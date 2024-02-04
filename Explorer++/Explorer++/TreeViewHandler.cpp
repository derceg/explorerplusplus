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

	m_shellTreeView = ShellTreeView::Create(m_treeViewHolder->GetHWND(), this, this,
		&m_FileActionHandler, &m_cachedIcons);
	m_treeViewHolder->SetContentChild(m_shellTreeView->GetHWND());
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
