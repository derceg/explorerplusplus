// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "HolderWindow.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "SetFileAttributesDialog.h"
#include "ShellTreeView/ShellTreeView.h"

void Explorerplusplus::CreateFolderControls()
{
	UINT holderStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (m_config->showFolders.get())
	{
		holderStyle |= WS_VISIBLE;
	}

	m_treeViewHolder = HolderWindow::Create(m_hContainer,
		m_app->GetResourceLoader()->LoadString(IDS_FOLDERS_WINDOW_TEXT), holderStyle,
		m_app->GetResourceLoader()->LoadString(IDS_HIDE_FOLDERS_PANE), m_app->GetConfig(),
		m_app->GetResourceLoader(), m_app->GetDarkModeManager(), m_app->GetDarkModeColorProvider());
	m_treeViewHolder->SetCloseButtonClickedCallback(
		[this]() { m_config->showFolders = !m_config->showFolders.get(); });
	m_treeViewHolder->SetResizedCallback(
		std::bind_front(&Explorerplusplus::OnTreeViewHolderResized, this));

	m_shellTreeView =
		ShellTreeView::Create(m_treeViewHolder->GetHWND(), m_app, this, &m_fileActionHandler);
	m_treeViewHolder->SetContentChild(m_shellTreeView->GetHWND());
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

			SetFileAttributesDialog setFileAttributesDialog(m_app->GetResourceLoader(),
				m_hContainer, sfaiList);
			setFileAttributesDialog.ShowModalDialog();
		}
	}
}

void Explorerplusplus::OnTreeViewHolderResized(int newWidth)
{
	m_treeViewWidth = newWidth;

	UpdateLayout();
}
