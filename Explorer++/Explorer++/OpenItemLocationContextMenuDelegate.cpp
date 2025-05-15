// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "OpenItemLocationContextMenuDelegate.h"
#include "BrowserList.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/ShellContextMenuBuilder.h"
#include "../Helper/ShellHelper.h"

OpenItemLocationContextMenuDelegate::OpenItemLocationContextMenuDelegate(BrowserList *browserList,
	const ResourceLoader *resourceLoader) :
	m_browserList(browserList),
	m_resourceLoader(resourceLoader)
{
}

void OpenItemLocationContextMenuDelegate::UpdateMenuEntries(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, ShellContextMenuBuilder *builder)
{
	if (items.size() != 1)
	{
		return;
	}

	PidlAbsolute pidlComplete = CombinePidls(directory, items[0].Raw());

	std::wstring openLocationText;

	if (DoesItemHaveAttributes(pidlComplete.Raw(), SFGAO_FOLDER))
	{
		openLocationText = m_resourceLoader->LoadString(IDS_SEARCH_OPEN_FOLDER_LOCATION);
	}
	else
	{
		openLocationText = m_resourceLoader->LoadString(IDS_SEARCH_OPEN_FILE_LOCATION);
	}

	builder->AddStringItem(OPEN_ITEM_LOCATION_MENU_ITEM_ID, openLocationText, 1, true);
}

bool OpenItemLocationContextMenuDelegate::MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, const std::wstring &verb)
{
	UNREFERENCED_PARAMETER(directory);
	UNREFERENCED_PARAMETER(items);
	UNREFERENCED_PARAMETER(verb);

	return false;
}

void OpenItemLocationContextMenuDelegate::HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, UINT menuItemId)
{
	UNREFERENCED_PARAMETER(items);

	switch (menuItemId)
	{
	case OPEN_ITEM_LOCATION_MENU_ITEM_ID:
	{
		// TODO: The target item should be selected.
		auto *browser = m_browserList->GetLastActive();
		CHECK(browser);
		browser->OpenItem(directory);
	}
	break;

	default:
		DCHECK(false);
		break;
	}
}

std::wstring OpenItemLocationContextMenuDelegate::GetHelpTextForCustomItem(UINT menuItemId)
{
	switch (menuItemId)
	{
	case OPEN_ITEM_LOCATION_MENU_ITEM_ID:
		return m_resourceLoader->LoadString(IDS_SEARCH_OPEN_ITEM_LOCATION_HELP_TEXT);

	default:
		DCHECK(false);
		return L"";
	}
}
