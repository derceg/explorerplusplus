// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "HolderWindow.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "SetFileAttributesDialog.h"
#include "ShellTreeView/ShellTreeView.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/Macros.h"

void Explorerplusplus::CreateFolderControls()
{
	UINT holderStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (m_config->showFolders.get())
	{
		holderStyle |= WS_VISIBLE;
	}

	m_treeViewHolder = HolderWindow::Create(m_hContainer,
		ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_FOLDERS_WINDOW_TEXT),
		holderStyle,
		ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_HIDE_FOLDERS_PANE),
		m_app->GetConfig(), m_app->GetIconResourceLoader());
	m_treeViewHolder->SetCloseButtonClickedCallback(
		std::bind(&Explorerplusplus::ToggleFolders, this));
	m_treeViewHolder->SetResizedCallback(
		std::bind_front(&Explorerplusplus::OnTreeViewHolderResized, this));

	m_shellTreeView = ShellTreeView::Create(m_treeViewHolder->GetHWND(), m_app, this, this,
		&m_FileActionHandler, m_app->GetCachedIcons());
	m_treeViewHolder->SetContentChild(m_shellTreeView->GetHWND());
}

void Explorerplusplus::OnTreeViewCopyItemPath() const
{
	auto pidl = m_shellTreeView->GetSelectedNodePidl();

	std::wstring fullFileName;
	GetDisplayName(pidl.get(), SHGDN_FORPARSING, fullFileName);

	BulkClipboardWriter clipboardWriter;
	clipboardWriter.WriteText(fullFileName);
}

void Explorerplusplus::OnTreeViewCopyUniversalPaths() const
{
	auto pidl = m_shellTreeView->GetSelectedNodePidl();

	std::wstring fullFileName;
	GetDisplayName(pidl.get(), SHGDN_FORPARSING, fullFileName);

	UNIVERSAL_NAME_INFO uni;
	DWORD dwBufferSize = sizeof(uni);
	DWORD dwRet = WNetGetUniversalName(fullFileName.c_str(), UNIVERSAL_NAME_INFO_LEVEL,
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

void Explorerplusplus::OnTreeViewSetFileAttributes() const
{
	std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo> sfaiList;
	NSetFileAttributesDialogExternal::SetFileAttributesInfo sfai;

	auto pidlItem = m_shellTreeView->GetSelectedNodePidl();

	std::wstring fullFileName;
	HRESULT hr = GetDisplayName(pidlItem.get(), SHGDN_FORPARSING, fullFileName);

	if (hr == S_OK)
	{
		StringCchCopy(sfai.szFullFileName, std::size(sfai.szFullFileName), fullFileName.c_str());

		HANDLE hFindFile = FindFirstFile(sfai.szFullFileName, &sfai.wfd);

		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			FindClose(hFindFile);

			sfaiList.push_back(sfai);

			SetFileAttributesDialog setFileAttributesDialog(m_app->GetResourceInstance(),
				m_hContainer, sfaiList);
			setFileAttributesDialog.ShowModalDialog();
		}
	}
}

void Explorerplusplus::OnTreeViewHolderResized(int newWidth)
{
	m_config->treeViewWidth = newWidth;

	UpdateLayout();
}
