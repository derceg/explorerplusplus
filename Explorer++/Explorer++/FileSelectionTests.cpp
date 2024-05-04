// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "ClipboardOperations.h"
#include "DirectoryOperationsHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellTreeView/ShellTreeView.h"
#include "TabContainer.h"
#include "../Helper/ClipboardHelper.h"

BOOL Explorerplusplus::AnyItemsSelected() const
{
	HWND hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
	{
		const Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();

		if (ListView_GetSelectedCount(selectedTab.GetShellBrowser()->GetListView()) > 0)
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

bool Explorerplusplus::CanCreate() const
{
	const Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	auto pidlDirectory = selectedTab.GetShellBrowser()->GetDirectoryIdl();
	return CanCreateInDirectory(pidlDirectory.get());
}

BOOL Explorerplusplus::CanCut() const
{
	return TestItemAttributes(SFGAO_CANMOVE);
}

BOOL Explorerplusplus::CanCopy() const
{
	return TestItemAttributes(SFGAO_CANCOPY);
}

BOOL Explorerplusplus::CanRename() const
{
	return TestItemAttributes(SFGAO_CANRENAME);
}

BOOL Explorerplusplus::CanDelete() const
{
	return TestItemAttributes(SFGAO_CANDELETE);
}

BOOL Explorerplusplus::CanShowFileProperties() const
{
	return TestItemAttributes(SFGAO_HASPROPSHEET);
}

/* Returns TRUE if all the specified attributes are set on the selected items. */
BOOL Explorerplusplus::TestItemAttributes(SFGAOF attributes) const
{
	SFGAOF commonAttributes = attributes;
	HRESULT hr = GetSelectionAttributes(&commonAttributes);

	if (SUCCEEDED(hr))
	{
		return (commonAttributes & attributes) == attributes;
	}

	return FALSE;
}

HRESULT Explorerplusplus::GetSelectionAttributes(SFGAOF *pItemAttributes) const
{
	HWND hFocus;
	HRESULT hr = E_FAIL;

	hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
	{
		const Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
		hr = selectedTab.GetShellBrowser()->GetListViewSelectionAttributes(pItemAttributes);
	}
	else if (hFocus == m_shellTreeView->GetHWND())
	{
		hr = GetTreeViewSelectionAttributes(pItemAttributes);
	}

	return hr;
}

HRESULT Explorerplusplus::GetTreeViewSelectionAttributes(SFGAOF *pItemAttributes) const
{
	auto pidl = m_shellTreeView->GetSelectedNodePidl();
	return GetItemAttributes(pidl.get(), pItemAttributes);
}

BOOL Explorerplusplus::CanPaste(PasteType pasteType) const
{
	auto directory = MaybeGetFocusedDirectory();

	if (!directory.HasValue())
	{
		return false;
	}

	return CanPasteInDirectory(directory.Raw(), pasteType);
}

bool Explorerplusplus::CanPasteHardLink() const
{
	const auto *activeShellBrowser = GetActiveShellBrowserImpl();
	return CanPasteHardLinkInDirectory(activeShellBrowser->GetDirectoryIdl().get());
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
