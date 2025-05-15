// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellItemContextMenuDelegate.h"

class BrowserList;
class BrowserWindow;
class ResourceLoader;

// Handles the "Open" menu item for a shell context menu, when that menu is being shown for one or
// more items. Additionally, if the menu is being shown for a single folder, an "Open in new tab"
// menu item will be added.
class OpenItemsContextMenuDelegate : public ShellItemContextMenuDelegate
{
public:
	OpenItemsContextMenuDelegate(BrowserList *browserList, const ResourceLoader *resourceLoader);
	OpenItemsContextMenuDelegate(BrowserWindow *browser, const ResourceLoader *resourceLoader);

	void UpdateMenuEntries(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		ShellContextMenuBuilder *builder) override;
	bool MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		UINT menuItemId) override;
	std::wstring GetHelpTextForCustomItem(UINT menuItemId) override;

private:
	static constexpr UINT OPEN_IN_NEW_TAB_MENU_ITEM_ID = 1;

	BrowserWindow *GetTargetBrowser() const;

	BrowserList *m_browserList = nullptr;
	BrowserWindow *m_browser = nullptr;
	const ResourceLoader *const m_resourceLoader;
};
