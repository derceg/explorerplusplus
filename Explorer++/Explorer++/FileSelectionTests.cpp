// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "ClipboardOperations.h"
#include "DirectoryOperationsHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellTreeView/ShellTreeView.h"
#include "TabContainerImpl.h"
#include "../Helper/ClipboardHelper.h"

BOOL Explorerplusplus::AnyItemsSelected() const
{
	HWND hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
	{
		const Tab &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();

		if (ListView_GetSelectedCount(selectedTab.GetShellBrowserImpl()->GetListView()) > 0)
		{
			return TRUE;
		}
	}
	else if (hFocus == m_shellTreeView->GetHWND())
	{
		if (TreeView_GetSelection(m_shellTreeView->GetHWND()) != nullptr)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL Explorerplusplus::CanPaste(PasteType pasteType) const
{
	auto directory = MaybeGetFocusedDirectory();

	if (!directory.HasValue())
	{
		return false;
	}

	return CanPasteInDirectory(m_app->GetClipboardStore(), directory.Raw(), pasteType);
}

// Tests whether a hard link or symlink can be pasted.
bool Explorerplusplus::CanPasteLink() const
{
	const auto *activeShellBrowser = GetActiveShellBrowserImpl();
	return ClipboardOperations::CanPasteLinkInDirectory(m_app->GetClipboardStore(),
		activeShellBrowser->GetDirectoryIdl().get());
}

PidlAbsolute Explorerplusplus::MaybeGetFocusedDirectory() const
{
	HWND focus = GetFocus();

	if (!focus)
	{
		return nullptr;
	}

	unique_pidl_absolute directory;

	const auto *activeShellBrowser = GetActiveShellBrowserImpl();

	if (focus == activeShellBrowser->GetListView())
	{
		directory = activeShellBrowser->GetDirectoryIdl();
	}
	else if (focus == m_shellTreeView->GetHWND())
	{
		directory = m_shellTreeView->GetSelectedNodePidl();
	}

	return directory.get();
}
