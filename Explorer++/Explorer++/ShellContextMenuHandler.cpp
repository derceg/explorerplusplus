// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "DirectoryOperationsHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellTreeView/ShellTreeView.h"
#include "SortMenuBuilder.h"
#include "Tab.h"
#include "TabContainer.h"
#include "ViewModeHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"

void Explorerplusplus::UpdateMenuEntries(HMENU menu, PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, IContextMenu *contextMenu)
{
	if (pidlItems.empty())
	{
		UpdateBackgroundContextMenu(menu, pidlParent, contextMenu);
	}
	else
	{
		UpdateItemContextMenu(menu, pidlParent, pidlItems);
	}
}

void Explorerplusplus::UpdateBackgroundContextMenu(HMENU menu, PCIDLIST_ABSOLUTE folderPidl,
	IContextMenu *contextMenu)
{
	RemoveNonFunctionalItemsFromBackgroundContextMenu(menu, contextMenu);

	UINT position = 0;

	auto viewsMenu = BuildViewsMenu();
	std::wstring text =
		ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_BACKGROUND_CONTEXT_MENU_VIEW);
	MenuHelper::AddSubMenuItem(menu, 0, text, std::move(viewsMenu), position++, true);

	SortMenuBuilder sortMenuBuilder(m_app->GetResourceInstance());
	auto sortMenus =
		sortMenuBuilder.BuildMenus(GetActivePane()->GetTabContainer()->GetSelectedTab());
	text = ResourceHelper::LoadString(m_app->GetResourceInstance(),
		IDS_BACKGROUND_CONTEXT_MENU_SORT_BY);
	MenuHelper::AddSubMenuItem(menu, 0, text, std::move(sortMenus.sortByMenu), position++, true);

	text = ResourceHelper::LoadString(m_app->GetResourceInstance(),
		IDS_BACKGROUND_CONTEXT_MENU_GROUP_BY);
	MenuHelper::AddSubMenuItem(menu, 0, text, std::move(sortMenus.groupByMenu), position++, true);

	text = ResourceHelper::LoadString(m_app->GetResourceInstance(),
		IDS_BACKGROUND_CONTEXT_MENU_REFRESH);
	MenuHelper::AddStringItem(menu, IDM_BACKGROUND_CONTEXT_MENU_REFRESH, text, position++, true);

	MenuHelper::AddSeparator(menu, position++, true);

	if (CanCustomizeDirectory(folderPidl))
	{
		text = ResourceHelper::LoadString(m_app->GetResourceInstance(),
			IDS_BACKGROUND_CONTEXT_MENU_CUSTOMIZE);
		MenuHelper::AddStringItem(menu, IDM_BACKGROUND_CONTEXT_MENU_CUSTOMIZE, text, position++,
			true);
		MenuHelper::AddSeparator(menu, position++, true);
	}

	text =
		ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_BACKGROUND_CONTEXT_MENU_PASTE);
	MenuHelper::AddStringItem(menu, IDM_BACKGROUND_CONTEXT_MENU_PASTE, text, position++, true);

	if (!CanPasteInDirectory(folderPidl, PasteType::Normal))
	{
		MenuHelper::EnableItem(menu, IDM_BACKGROUND_CONTEXT_MENU_PASTE, false);
	}

	text = ResourceHelper::LoadString(m_app->GetResourceInstance(),
		IDS_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT);
	MenuHelper::AddStringItem(menu, IDM_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT, text, position++,
		true);

	if (!CanPasteInDirectory(folderPidl, PasteType::Shortcut))
	{
		MenuHelper::EnableItem(menu, IDM_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT, false);
	}

	MenuHelper::AddSeparator(menu, position++, true);
}

void Explorerplusplus::RemoveNonFunctionalItemsFromBackgroundContextMenu(HMENU menu,
	IContextMenu *contextMenu)
{
	int numItems = GetMenuItemCount(menu);

	if (numItems == -1)
	{
		DCHECK(false);
		return;
	}

	for (int i = numItems - 1; i >= 0; i--)
	{
		MENUITEMINFO menuItemInfo = {};
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_ID | MIIM_FTYPE;
		BOOL res = GetMenuItemInfo(menu, i, TRUE, &menuItemInfo);

		if (!res || WI_IsFlagSet(menuItemInfo.fType, MFT_SEPARATOR)
			|| menuItemInfo.wID < ShellContextMenu::MIN_SHELL_MENU_ID
			|| menuItemInfo.wID > ShellContextMenu::MAX_SHELL_MENU_ID)
		{
			continue;
		}

		TCHAR verb[64] = _T("");
		HRESULT hr = contextMenu->GetCommandString(menuItemInfo.wID
				- ShellContextMenu::MIN_SHELL_MENU_ID,
			GCS_VERB, nullptr, reinterpret_cast<LPSTR>(verb), static_cast<UINT>(std::size(verb)));

		if (FAILED(hr))
		{
			continue;
		}

		if (StrCmpI(verb, L"savesearch") == 0)
		{
			// This menu item appears on the background context menu for a search results folder.
			// When it's clicked, the shell will request view information, using IFolderView2, along
			// with at least one undocumented interface. Because attempting to implement an
			// undocumented interface is potentially difficult and carries risk, along with the fact
			// that the view information in Explorer++ doesn't correspond precisely with the view
			// information in Explorer anyway, this item is removed here.
			DeleteMenu(menu, i, MF_BYPOSITION);
		}
	}
}

void Explorerplusplus::UpdateItemContextMenu(HMENU menu, PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems)
{
	bool addNewTabMenuItem = false;

	if (pidlItems.size() == 1)
	{
		SFGAOF fileAttributes = SFGAO_FOLDER;

		unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0].Raw()));
		GetItemAttributes(pidlComplete.get(), &fileAttributes);

		if (fileAttributes & SFGAO_FOLDER)
		{
			addNewTabMenuItem = true;
		}
	}

	if (addNewTabMenuItem)
	{
		std::wstring openInNewTabText =
			ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_GENERAL_OPEN_IN_NEW_TAB);

		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING | MIIM_ID;
		mii.wID = OPEN_IN_NEW_TAB_MENU_ITEM_ID;
		mii.dwTypeData = openInNewTabText.data();
		InsertMenuItem(menu, 1, TRUE, &mii);
	}
}

bool Explorerplusplus::HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, const std::wstring &verb)
{
	if (verb == L"open")
	{
		for (const auto &pidl : pidlItems)
		{
			unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidl.Raw()));
			OpenItem(pidlComplete.get());
		}

		return true;
	}
	else if (verb == L"rename")
	{
		OnFileRename();

		return true;
	}
	else if (verb == L"copy")
	{
		Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
		selectedTab.GetShellBrowser()->CopySelectedItemsToClipboard(true);

		return true;
	}
	else if (verb == L"cut")
	{
		Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
		selectedTab.GetShellBrowser()->CopySelectedItemsToClipboard(false);

		return true;
	}

	return false;
}

void Explorerplusplus::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, UINT menuItemId)
{
	switch (menuItemId)
	{
	case OPEN_IN_NEW_TAB_MENU_ITEM_ID:
	{
		// This menu item should only be added when a single folder is selected.
		DCHECK_EQ(pidlItems.size(), 1u);

		unique_pidl_absolute pidlComplete(ILCombine(pidlParent, pidlItems[0].Raw()));
		auto navigateParams = NavigateParams::Normal(pidlComplete.get());
		GetActivePane()->GetTabContainer()->CreateNewTab(navigateParams,
			TabSettings(_selected = m_config->openTabsInForeground));
	}
	break;

	// Custom items in the background context menu will be handled by the WM_COMMAND handler.
	default:
		SendMessage(m_hContainer, WM_COMMAND, MAKEWPARAM(menuItemId, 0), 0);
		break;
	}
}

std::wstring Explorerplusplus::GetHelpTextForItem(UINT menuItemId)
{
	// By default, the help text will be looked up via the menu item ID.
	UINT menuHelpTextId = menuItemId;

	switch (menuItemId)
	{
	case OPEN_IN_NEW_TAB_MENU_ITEM_ID:
		menuHelpTextId = IDS_GENERAL_OPEN_IN_NEW_TAB_HELP_TEXT;
		break;

	case IDM_BACKGROUND_CONTEXT_MENU_REFRESH:
		menuHelpTextId = IDM_VIEW_REFRESH;
		break;

	case IDM_BACKGROUND_CONTEXT_MENU_PASTE:
		menuHelpTextId = IDM_EDIT_PASTE;
		break;

	case IDM_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT:
		menuHelpTextId = IDM_EDIT_PASTESHORTCUT;
		break;
	}

	auto helpText = ResourceHelper::MaybeLoadString(m_app->GetResourceInstance(), menuHelpTextId);

	if (helpText)
	{
		return *helpText;
	}

	return L"";
}
