// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellTreeView/ShellTreeView.h"
#include "SortMenuBuilder.h"
#include "Tab.h"
#include "TabContainer.h"
#include "ViewModeHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"

#define MENU_OPEN_IN_NEW_TAB (MAX_SHELL_MENU_ID + 1)

void Explorerplusplus::UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, IContextMenu *contextMenu,
	HMENU hMenu)
{
	assert(dwData != NULL);

	if (pidlItems.empty())
	{
		UpdateBackgroundContextMenu(contextMenu, hMenu);
	}
	else
	{
		UpdateItemContextMenu(pidlParent, pidlItems, dwData, hMenu);
	}
}

void Explorerplusplus::UpdateBackgroundContextMenu(IContextMenu *contextMenu, HMENU menu)
{
	int numItems = GetMenuItemCount(menu);

	if (numItems == -1)
	{
		return;
	}

	for (int i = numItems - 1; i >= 0; i--)
	{
		MENUITEMINFO mii = {};
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_ID | MIIM_FTYPE;
		BOOL res = GetMenuItemInfo(menu, i, TRUE, &mii);

		if (!res || WI_IsFlagSet(mii.fType, MFT_SEPARATOR) || mii.wID < MIN_SHELL_MENU_ID
			|| mii.wID > MAX_SHELL_MENU_ID)
		{
			continue;
		}

		// Note that verbs are only present for the majority of system items in the background
		// context menu starting in Windows 8. In Windows 7, verbs are only set for a couple of
		// items. Therefore, the code below isn't going to work on anything earlier then Windows 8.
		TCHAR verb[64] = _T("");
		HRESULT hr = contextMenu->GetCommandString(mii.wID - MIN_SHELL_MENU_ID, GCS_VERB, nullptr,
			reinterpret_cast<LPSTR>(verb), SIZEOF_ARRAY(verb));

		if (FAILED(hr))
		{
			continue;
		}

		// TODO: Library folders have an "Arrange by" menu that appears at the top of the background
		// context menu. That menu should be removed, if there's a reliable way of doing it (the
		// menu item doesn't have a verb at present).

		if (StrCmpI(verb, L"view") == 0)
		{
			DeleteMenu(menu, i, MF_BYPOSITION);

			auto viewsMenu = BuildViewsMenu();
			std::wstring text =
				ResourceHelper::LoadString(m_resourceInstance, IDS_BACKGROUND_CONTEXT_MENU_VIEW);
			MenuHelper::AddSubMenuItem(menu, text, std::move(viewsMenu), i, TRUE);
		}
		else if (StrCmpI(verb, L"arrange") == 0)
		{
			DeleteMenu(menu, i, MF_BYPOSITION);

			SortMenuBuilder sortMenuBuilder(m_resourceInstance);
			auto sortMenus =
				sortMenuBuilder.BuildMenus(GetActivePane()->GetTabContainer()->GetSelectedTab());

			std::wstring text =
				ResourceHelper::LoadString(m_resourceInstance, IDS_BACKGROUND_CONTEXT_MENU_SORT_BY);
			MenuHelper::AddSubMenuItem(menu, text, std::move(sortMenus.sortByMenu), i, TRUE);
		}
		else if (StrCmpI(verb, L"groupby") == 0)
		{
			DeleteMenu(menu, i, MF_BYPOSITION);

			SortMenuBuilder sortMenuBuilder(m_resourceInstance);
			auto sortMenus =
				sortMenuBuilder.BuildMenus(GetActivePane()->GetTabContainer()->GetSelectedTab());

			std::wstring text = ResourceHelper::LoadString(m_resourceInstance,
				IDS_BACKGROUND_CONTEXT_MENU_GROUP_BY);
			MenuHelper::AddSubMenuItem(menu, text, std::move(sortMenus.groupByMenu), i, TRUE);
		}
		else if (StrCmpI(verb, L"paste") == 0)
		{
			UINT flags = MF_BYPOSITION;

			if (CanPaste())
			{
				flags |= MF_ENABLED;
			}
			else
			{
				flags |= MF_DISABLED;
			}

			EnableMenuItem(menu, i, flags);
		}
		else if (StrCmpI(verb, L"pastelink") == 0)
		{
			UINT flags = MF_BYPOSITION;

			if (CanPasteShortcut())
			{
				flags |= MF_ENABLED;
			}
			else
			{
				flags |= MF_DISABLED;
			}

			EnableMenuItem(menu, i, flags);
		}
		else if (StrCmpI(verb, L"undo") == 0 || StrCmpI(verb, L"redo") == 0)
		{
			// Most context menu items are handled via IContextMenu::InvokeCommand(). However, some
			// items can't be handled that way. For example, the rename item needs to be handled by
			// the view, since the view is what's responsible for putting items into rename mode.
			// Therefore, the context menu handler that the shell constructs will call SendMessage()
			// to notify the view when certain items are selected. Undo + redo are two items that
			// are handled by the view, which is why they don't work when the parent menu is hosted
			// in Explorer++ and why they're removed here.
			DeleteMenu(menu, i, MF_BYPOSITION);
		}
	}
}

void Explorerplusplus::UpdateItemContextMenu(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR data, HMENU menu)
{
	auto *pfcmi = reinterpret_cast<FileContextMenuInfo *>(data);

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
			ResourceHelper::LoadString(m_resourceInstance, IDS_GENERAL_OPEN_IN_NEW_TAB);

		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING | MIIM_ID;
		mii.wID = MENU_OPEN_IN_NEW_TAB;
		mii.dwTypeData = openInNewTabText.data();
		InsertMenuItem(menu, 1, TRUE, &mii);
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

		return TRUE;
	}
	else if (StrCmpI(szCmd, _T("viewcustomwizard")) == 0)
	{
		// This item is only shown on the background context menu and that menu can only be shown
		// within the listview.
		assert(pfcmi->uFrom == FROM_LISTVIEW);
		assert(pidlItems.empty());

		// This verb (which corresponds to the "Customize this folder..." menu item shown in the
		// background context menu) doesn't appear to be a standard verb that's handled by the
		// system. Therefore, it will be handled here.
		return ExecuteFileAction(m_hActiveListView, L"properties", L"customize", nullptr,
			pidlParent);
	}
	else if (StrCmpI(szCmd, _T("refresh")) == 0)
	{
		OnRefresh();

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
			assert(pidlItems.size() == 1);
			unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0]));
			m_shellTreeView->StartRenamingItem(pidlComplete.get());
		}

		return TRUE;
	}
	else if (StrCmpI(szCmd, _T("copy")) == 0)
	{
		if (pfcmi->uFrom == FROM_LISTVIEW)
		{
			Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
			selectedTab.GetShellBrowser()->CopySelectedItemsToClipboard(true);
		}
		else if (pfcmi->uFrom == FROM_TREEVIEW)
		{
			assert(pidlItems.size() == 1);
			unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0]));
			m_shellTreeView->CopyItemToClipboard(pidlComplete.get(), true);
		}

		return TRUE;
	}
	else if (StrCmpI(szCmd, _T("cut")) == 0)
	{
		if (pfcmi->uFrom == FROM_LISTVIEW)
		{
			Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
			selectedTab.GetShellBrowser()->CopySelectedItemsToClipboard(false);
		}
		else if (pfcmi->uFrom == FROM_TREEVIEW)
		{
			assert(pidlItems.size() == 1);
			unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0]));
			m_shellTreeView->CopyItemToClipboard(pidlComplete.get(), false);
		}

		return TRUE;
	}
	else if (StrCmpI(szCmd, _T("paste")) == 0)
	{
		if (pfcmi->uFrom == FROM_LISTVIEW && pidlItems.empty())
		{
			// The paste item on the background context menu is non-functional, so needs to be
			// handled here.
			OnListViewPaste();
			return TRUE;
		}
	}
	else if (StrCmpI(szCmd, _T("pastelink")) == 0)
	{
		// This item should only be shown in the background context menu.
		assert(pfcmi->uFrom == FROM_LISTVIEW);
		assert(pidlItems.empty());

		GetActiveShellBrowser()->PasteShortcut();

		return TRUE;
	}

	return FALSE;
}

void Explorerplusplus::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, int iCmd)
{
	switch (iCmd)
	{
	case MENU_OPEN_IN_NEW_TAB:
	{
		// This menu item should only be added when a single folder is selected.
		assert(pidlItems.size() == 1);

		unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0]));
		auto navigateParams = NavigateParams::Normal(pidlComplete.get());
		GetActivePane()->GetTabContainer()->CreateNewTab(navigateParams,
			TabSettings(_selected = m_config->openTabsInForeground));
	}
	break;

	// Custom items in the background context menu will be handled by the WM_COMMAND handler.
	default:
		SendMessage(m_hContainer, WM_COMMAND, MAKEWPARAM(iCmd, 0), 0);
		break;
	}
}
