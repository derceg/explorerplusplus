// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "OpenItemsContextMenuDelegate.h"
#include "BrowserList.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/Pidl.h"
#include "../Helper/ShellContextMenuBuilder.h"
#include "../Helper/ShellHelper.h"

OpenItemsContextMenuDelegate::OpenItemsContextMenuDelegate(BrowserList *browserList,
	const ResourceLoader *resourceLoader) :
	m_browserList(browserList),
	m_resourceLoader(resourceLoader)
{
}

OpenItemsContextMenuDelegate::OpenItemsContextMenuDelegate(BrowserWindow *browser,
	const ResourceLoader *resourceLoader) :
	m_browser(browser),
	m_resourceLoader(resourceLoader)
{
}

void OpenItemsContextMenuDelegate::UpdateMenuEntries(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, ShellContextMenuBuilder *builder)
{
	if (items.size() != 1)
	{
		return;
	}

	auto pidlComplete = directory + items[0];

	if (!DoesItemHaveAttributes(pidlComplete.Raw(), SFGAO_FOLDER))
	{
		return;
	}

	builder->AddStringItem(OPEN_IN_NEW_TAB_MENU_ITEM_ID,
		m_resourceLoader->LoadString(IDS_GENERAL_OPEN_IN_NEW_TAB), 1, true);
}

bool OpenItemsContextMenuDelegate::MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, const std::wstring &verb)
{
	if (verb == L"open")
	{
		auto *browser = GetTargetBrowser();

		for (const auto &item : items)
		{
			auto pidlComplete = directory + item;
			browser->OpenItem(pidlComplete.Raw());
		}

		return true;
	}

	return false;
}

void OpenItemsContextMenuDelegate::HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, UINT menuItemId)
{
	switch (menuItemId)
	{
	case OPEN_IN_NEW_TAB_MENU_ITEM_ID:
	{
		// This menu item should only be added when a single folder is selected.
		CHECK_EQ(items.size(), 1u);

		auto pidlComplete = directory + items[0];

		auto *browser = GetTargetBrowser();
		browser->OpenItem(pidlComplete.Raw(), OpenFolderDisposition::NewTabDefault);
	}
	break;

	default:
		DCHECK(false);
		break;
	}
}

std::wstring OpenItemsContextMenuDelegate::GetHelpTextForCustomItem(UINT menuItemId)
{
	switch (menuItemId)
	{
	case OPEN_IN_NEW_TAB_MENU_ITEM_ID:
		return m_resourceLoader->LoadString(IDS_GENERAL_OPEN_IN_NEW_TAB_HELP_TEXT);

	default:
		DCHECK(false);
		return L"";
	}
}

BrowserWindow *OpenItemsContextMenuDelegate::GetTargetBrowser() const
{
	if (m_browser)
	{
		return m_browser;
	}

	// If a browser wasn't provided, a BrowserList instance should have been provided instead.
	CHECK(m_browserList);

	auto *lastActiveBrowser = m_browserList->GetLastActive();
	CHECK(lastActiveBrowser);

	return lastActiveBrowser;
}
