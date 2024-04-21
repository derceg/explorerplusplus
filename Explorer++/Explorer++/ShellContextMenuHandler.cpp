// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
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
	UNREFERENCED_PARAMETER(contextMenu);

	if (pidlItems.empty())
	{
		UpdateBackgroundContextMenu(menu, pidlParent);
	}
	else
	{
		UpdateItemContextMenu(menu, pidlParent, pidlItems);
	}
}

void Explorerplusplus::UpdateBackgroundContextMenu(HMENU menu, PCIDLIST_ABSOLUTE folderPidl)
{
	UINT position = 0;

	auto viewsMenu = BuildViewsMenu();
	std::wstring text =
		ResourceHelper::LoadString(m_resourceInstance, IDS_BACKGROUND_CONTEXT_MENU_VIEW);
	MenuHelper::AddSubMenuItem(menu, 0, text, std::move(viewsMenu), position++, true);

	SortMenuBuilder sortMenuBuilder(m_resourceInstance);
	auto sortMenus =
		sortMenuBuilder.BuildMenus(GetActivePane()->GetTabContainer()->GetSelectedTab());
	text = ResourceHelper::LoadString(m_resourceInstance, IDS_BACKGROUND_CONTEXT_MENU_SORT_BY);
	MenuHelper::AddSubMenuItem(menu, 0, text, std::move(sortMenus.sortByMenu), position++, true);

	text = ResourceHelper::LoadString(m_resourceInstance, IDS_BACKGROUND_CONTEXT_MENU_GROUP_BY);
	MenuHelper::AddSubMenuItem(menu, 0, text, std::move(sortMenus.groupByMenu), position++, true);

	text = ResourceHelper::LoadString(m_resourceInstance, IDS_BACKGROUND_CONTEXT_MENU_REFRESH);
	MenuHelper::AddStringItem(menu, IDM_BACKGROUND_CONTEXT_MENU_REFRESH, text, position++, true);

	MenuHelper::AddSeparator(menu, position++, true);

	SFGAOF attributes = SFGAO_FILESYSTEM;
	HRESULT hr = GetItemAttributes(folderPidl, &attributes);

	if (SUCCEEDED(hr) && WI_IsFlagSet(attributes, SFGAO_FILESYSTEM))
	{
		text =
			ResourceHelper::LoadString(m_resourceInstance, IDS_BACKGROUND_CONTEXT_MENU_CUSTOMIZE);
		MenuHelper::AddStringItem(menu, IDM_BACKGROUND_CONTEXT_MENU_CUSTOMIZE, text, position++,
			true);
		MenuHelper::AddSeparator(menu, position++, true);
	}

	text = ResourceHelper::LoadString(m_resourceInstance, IDS_BACKGROUND_CONTEXT_MENU_PASTE);
	MenuHelper::AddStringItem(menu, IDM_BACKGROUND_CONTEXT_MENU_PASTE, text, position++, true);

	if (!CanPaste())
	{
		MenuHelper::EnableItem(menu, IDM_BACKGROUND_CONTEXT_MENU_PASTE, false);
	}

	text =
		ResourceHelper::LoadString(m_resourceInstance, IDS_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT);
	MenuHelper::AddStringItem(menu, IDM_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT, text, position++,
		true);

	if (!CanPasteShortcut())
	{
		MenuHelper::EnableItem(menu, IDM_BACKGROUND_CONTEXT_MENU_PASTE_SHORTCUT, false);
	}

	MenuHelper::AddSeparator(menu, position++, true);
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
			ResourceHelper::LoadString(m_resourceInstance, IDS_GENERAL_OPEN_IN_NEW_TAB);

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
		assert(pidlItems.size() == 1);

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

	auto helpText = ResourceHelper::MaybeLoadString(m_resourceInstance, menuHelpTextId);

	if (helpText)
	{
		return *helpText;
	}

	return L"";
}
