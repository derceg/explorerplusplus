// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Switches events based on the currently selected window
 * (principally the listview and treeview).
 */

#include "stdafx.h"
#include "Explorer++.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellTreeView/ShellTreeView.h"

void Explorerplusplus::OnSetFileAttributes() const
{
	HWND hFocus;

	hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
	{
		OnListViewSetFileAttributes();
	}
	else if (hFocus == m_shellTreeView->GetHWND())
	{
		OnTreeViewSetFileAttributes();
	}
}

void Explorerplusplus::OnPaste()
{
	HWND focus = GetFocus();

	if (focus == m_hActiveListView)
	{
		OnListViewPaste();
	}
	else if (focus == m_shellTreeView->GetHWND())
	{
		m_shellTreeView->Paste();
	}
}

void Explorerplusplus::OnPasteShortcut()
{
	HWND focus = GetFocus();

	if (focus == m_hActiveListView)
	{
		GetActiveShellBrowserImpl()->PasteShortcut();
	}
	else if (focus == m_shellTreeView->GetHWND())
	{
		m_shellTreeView->PasteShortcut();
	}
}
