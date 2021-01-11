// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Handles shell context menus (such as file
 * context menus, and the new menu).
 */

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellTreeView/ShellTreeView.h"
#include "Tab.h"
#include "TabContainer.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"

#define MENU_OPEN_IN_NEW_TAB (MAX_SHELL_MENU_ID + 1)

void Explorerplusplus::AddMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, HMENU hMenu)
{
	assert(dwData != NULL);

	auto *pfcmi = reinterpret_cast<FileContextMenuInfo *>(dwData);

	bool addNewTabMenuItem = false;

	if (pfcmi->uFrom == FROM_LISTVIEW)
	{
		if (pidlItems.size() == 1)
		{
			SFGAOF fileAttributes = SFGAO_FOLDER;

			unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems.front()));
			GetItemAttributes(pidlComplete.get(), &fileAttributes);

			if (fileAttributes & SFGAO_FOLDER)
			{
				addNewTabMenuItem = true;
			}
		}
	}
	else if (pfcmi->uFrom == FROM_TREEVIEW)
	{
		/* The treeview only contains folders,
		so the new tab menu item will always
		be shown. */
		addNewTabMenuItem = true;
	}

	if (addNewTabMenuItem)
	{
		std::wstring openInNewTabText =
			ResourceHelper::LoadString(m_hLanguageModule, IDS_GENERAL_OPEN_IN_NEW_TAB);

		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING | MIIM_ID;
		mii.wID = MENU_OPEN_IN_NEW_TAB;
		mii.dwTypeData = openInNewTabText.data();
		InsertMenuItem(hMenu, 1, TRUE, &mii);
	}
}

BOOL Explorerplusplus::HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd)
{
	auto *pfcmi = reinterpret_cast<FileContextMenuInfo *>(dwData);

	if (StrCmpI(szCmd, _T("open")) == 0)
	{
		if (pidlItems.empty())
		{
			OpenItem(pidlParent);
		}
		else
		{
			for (const auto &pidl : pidlItems)
			{
				unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidl));
				OpenItem(pidlComplete.get());
			}
		}

		m_bTreeViewOpenInNewTab = true;

		return TRUE;
	}
	else if (StrCmpI(szCmd, _T("rename")) == 0)
	{
		if (pfcmi->uFrom == FROM_LISTVIEW)
		{
			OnFileRename();
		}
		else if (pfcmi->uFrom == FROM_TREEVIEW)
		{
			m_shellTreeView->StartRenamingSelectedItem();
		}

		return TRUE;
	}
	else if (StrCmpI(szCmd, _T("copy")) == 0)
	{
		if (pfcmi->uFrom == FROM_LISTVIEW)
		{
			Tab &selectedTab = m_tabContainer->GetSelectedTab();
			selectedTab.GetShellBrowser()->CopySelectedItemToClipboard(true);
		}
		else if (pfcmi->uFrom == FROM_TREEVIEW)
		{
			m_shellTreeView->CopySelectedItemToClipboard(true);
		}

		return TRUE;
	}
	else if (StrCmpI(szCmd, _T("cut")) == 0)
	{
		if (pfcmi->uFrom == FROM_LISTVIEW)
		{
			Tab &selectedTab = m_tabContainer->GetSelectedTab();
			selectedTab.GetShellBrowser()->CopySelectedItemToClipboard(false);
		}
		else if (pfcmi->uFrom == FROM_TREEVIEW)
		{
			m_shellTreeView->CopySelectedItemToClipboard(false);
		}

		return TRUE;
	}

	return FALSE;
}

void Explorerplusplus::HandleCustomMenuItem(
	PCIDLIST_ABSOLUTE pidlParent, const std::vector<PITEMID_CHILD> &pidlItems, int iCmd)
{
	switch (iCmd)
	{
	case MENU_OPEN_IN_NEW_TAB:
	{
		// This menu item should only be added when a single folder is selected.
		assert(pidlItems.size() == 1);

		unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0]));
		m_tabContainer->CreateNewTab(
			pidlComplete.get(), TabSettings(_selected = m_config->openTabsInForeground));

		m_bTreeViewOpenInNewTab = true;
	}
	break;
	}
}