// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SearchResultsItemContextMenu.h"
#include "BrowserList.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/ShellHelper.h"

SearchResultsItemContextMenu::SearchResultsItemContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, const PidlAbsolute &directory,
	const std::vector<PidlChild> &items, HWND hwnd, IUnknown *site, Flags flags,
	BrowserList *browserList, const ResourceLoader *resourceLoader) :
	ShellItemContextMenu(menuView, acceleratorManager, directory, items, hwnd, site, flags,
		browserList, resourceLoader),
	m_browserList(browserList)
{
	MaybeAddOpenLocationItem(directory, items, resourceLoader);
}

void SearchResultsItemContextMenu::MaybeAddOpenLocationItem(const PidlAbsolute &directory,
	const std::vector<PidlChild> &items, const ResourceLoader *resourceLoader)
{
	if (items.size() != 1)
	{
		return;
	}

	auto pidlComplete = directory + items[0];

	std::wstring openLocationText;

	if (DoesItemHaveAttributes(pidlComplete.Raw(), SFGAO_FOLDER))
	{
		openLocationText = resourceLoader->LoadString(IDS_SEARCH_OPEN_FOLDER_LOCATION);
	}
	else
	{
		openLocationText = resourceLoader->LoadString(IDS_SEARCH_OPEN_FILE_LOCATION);
	}

	auto openItemIndex = MaybeGetShellItemIndex(L"open");

	m_rootMenuView->AddItem(
		MenuItemParams(openLocationText)
			.SetHelpText(resourceLoader->LoadString(IDS_SEARCH_OPEN_ITEM_LOCATION_HELP_TEXT))
			.SetSelectedCallback([this, directory] { OnOpenLocation(directory); }),
		openItemIndex ? *openItemIndex + 1 : 0);
}

void SearchResultsItemContextMenu::OnOpenLocation(const PidlAbsolute &directory)
{
	// TODO: The target item should be selected.
	auto *browser = m_browserList->GetLastActive();
	CHECK(browser);

	browser->OpenItem(directory.Raw());
}
