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
	if (pidlItems.empty())
	{
		UpdateBackgroundContextMenu(menu, contextMenu);
	}
	else
	{
		UpdateItemContextMenu(menu, pidlParent, pidlItems);
	}
}

void Explorerplusplus::UpdateBackgroundContextMenu(HMENU menu, IContextMenu *contextMenu)
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

		if (!res || WI_IsFlagSet(mii.fType, MFT_SEPARATOR)
			|| mii.wID < FileContextMenuManager::MIN_SHELL_MENU_ID
			|| mii.wID > FileContextMenuManager::MAX_SHELL_MENU_ID)
		{
			continue;
		}

		// Note that verbs are only present for the majority of system items in the background
		// context menu starting in Windows 8. In Windows 7, verbs are only set for a couple of
		// items. Therefore, the code below isn't going to work on anything earlier then Windows 8.
		TCHAR verb[64] = _T("");
		HRESULT hr =
			contextMenu->GetCommandString(mii.wID - FileContextMenuManager::MIN_SHELL_MENU_ID,
				GCS_VERB, nullptr, reinterpret_cast<LPSTR>(verb), SIZEOF_ARRAY(verb));

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
			MenuHelper::AddSubMenuItem(menu, 0, text, std::move(viewsMenu), i, TRUE);
		}
		else if (StrCmpI(verb, L"arrange") == 0)
		{
			DeleteMenu(menu, i, MF_BYPOSITION);

			SortMenuBuilder sortMenuBuilder(m_resourceInstance);
			auto sortMenus =
				sortMenuBuilder.BuildMenus(GetActivePane()->GetTabContainer()->GetSelectedTab());

			std::wstring text =
				ResourceHelper::LoadString(m_resourceInstance, IDS_BACKGROUND_CONTEXT_MENU_SORT_BY);
			MenuHelper::AddSubMenuItem(menu, 0, text, std::move(sortMenus.sortByMenu), i, TRUE);
		}
		else if (StrCmpI(verb, L"groupby") == 0)
		{
			DeleteMenu(menu, i, MF_BYPOSITION);

			SortMenuBuilder sortMenuBuilder(m_resourceInstance);
			auto sortMenus =
				sortMenuBuilder.BuildMenus(GetActivePane()->GetTabContainer()->GetSelectedTab());

			std::wstring text = ResourceHelper::LoadString(m_resourceInstance,
				IDS_BACKGROUND_CONTEXT_MENU_GROUP_BY);
			MenuHelper::AddSubMenuItem(menu, 0, text, std::move(sortMenus.groupByMenu), i, TRUE);
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
	else if (verb == L"viewcustomwizard")
	{
		// This item is only shown on the background context menu.
		assert(pidlItems.empty());

		// This verb (which corresponds to the "Customize this folder..." menu item shown in the
		// background context menu) is normally handled by the view in Explorer. Because of that,
		// selecting the item will have no effect, which is why it's manually handled here.
		// Note that the call below won't always result in the customize tab being selected. That's
		// because the properties dialog will select the tab based on its display name, which can
		// change as the display language is changed in Windows.
		// In Explorer, the title of the dialog is dynamically retrieved. Although it might be
		// possible to do that here as well, that strategy would break if the properties dialog
		// resource ID ever changed.
		// Another alternative might be to load the "customize" string from the string table. But
		// the language used by the application has nothing to do with the language used by Windows
		// itself. Also, the text would have to be exactly the same as that used by Windows for a
		// given language, which probably wouldn't be clear to translators. Minor variations within
		// a language (e.g. customize vs customise) could cause the tab to not be selected.
		// Therefore, this will only work when the actual title of the properties dialog is
		// "customize" (ignoring case). That's not ideal, but not too much of an issue, since the
		// properties dialog will always be opened, just not always on the customize tab.
		ExecuteFileAction(m_hActiveListView, pidlParent, L"properties", L"customize", L"");

		return true;
	}
	else if (verb == L"refresh")
	{
		OnRefresh();

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
	else if (verb == L"paste")
	{
		if (pidlItems.empty())
		{
			// The paste item on the background context menu is non-functional, so needs to be
			// handled here.
			OnListViewPaste();
			return true;
		}
	}
	else if (verb == L"pastelink")
	{
		// This item should only be shown in the background context menu.
		assert(pidlItems.empty());

		GetActiveShellBrowser()->PasteShortcut();

		return true;
	}

	return false;
}

void Explorerplusplus::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, int cmd)
{
	switch (cmd)
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
		SendMessage(m_hContainer, WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
		break;
	}
}
