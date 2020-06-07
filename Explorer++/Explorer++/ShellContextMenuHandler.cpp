// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Handles shell context menus (such as file
 * context menus, and the new menu).
 */

#include "stdafx.h"
#include "Explorer++.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellTreeView/ShellTreeView.h"
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
		/* If ppidl is NULL, open the item specified by pidlParent
		in the current listview. If ppidl is not NULL, open each
		of the items specified in ppidl. */
		if (pidlItems.empty())
		{
			OpenItem(pidlParent, FALSE, FALSE);
		}
		else
		{
			for (const auto &pidl : pidlItems)
			{
				unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidl));
				OpenItem(pidlComplete.get(), FALSE, FALSE);
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
			OnListViewCopy(TRUE);
		}
		else if (pfcmi->uFrom == FROM_TREEVIEW)
		{
			OnTreeViewCopy(TRUE);
		}

		return TRUE;
	}
	else if (StrCmpI(szCmd, _T("cut")) == 0)
	{
		if (pfcmi->uFrom == FROM_LISTVIEW)
		{
			OnListViewCopy(FALSE);
		}
		else if (pfcmi->uFrom == FROM_TREEVIEW)
		{
			OnTreeViewCopy(FALSE);
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
		unique_pidl_absolute pidlComplete;
		TCHAR szParsingPath[MAX_PATH];
		BOOL bOpenInNewTab;

		if (!pidlItems.empty())
		{
			pidlComplete.reset(ILCombine(pidlParent, pidlItems[0]));

			bOpenInNewTab = FALSE;
		}
		else
		{
			pidlComplete.reset(ILCloneFull(pidlParent));

			bOpenInNewTab = TRUE;
		}

		GetDisplayName(
			pidlComplete.get(), szParsingPath, SIZEOF_ARRAY(szParsingPath), SHGDN_FORPARSING);
		m_tabContainer->CreateNewTab(szParsingPath, TabSettings(_selected = true));

		m_bTreeViewOpenInNewTab = true;
	}
	break;
	}
}