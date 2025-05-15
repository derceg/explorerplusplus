// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellItemContextMenuDelegate.h"

class BrowserList;
class ResourceLoader;

// When a shell context menu is displayed for a single item, this class will add a menu entry that
// allows the location of the item to be opened.
class OpenItemLocationContextMenuDelegate : public ShellItemContextMenuDelegate
{
public:
	OpenItemLocationContextMenuDelegate(BrowserList *browserList,
		const ResourceLoader *resourceLoader);

	void UpdateMenuEntries(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		ShellContextMenuBuilder *builder) override;
	bool MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		UINT menuItemId) override;
	std::wstring GetHelpTextForCustomItem(UINT menuItemId) override;

private:
	static constexpr UINT OPEN_ITEM_LOCATION_MENU_ITEM_ID = 1;

	BrowserList *const m_browserList;
	const ResourceLoader *const m_resourceLoader;
};
