// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabRestorer.h"
#include "../Helper/Macros.h"
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <wil/resource.h>
#include <unordered_map>

class CoreInterface;

class TabRestorerMenu
{
public:
	TabRestorerMenu(HINSTANCE resourceInstance, CoreInterface *coreInterface,
		TabRestorer *tabRestorer, UINT menuStartId, UINT menuEndId);
	~TabRestorerMenu();

	void OnMenuItemClicked(UINT menuItemId);

private:
	DISALLOW_COPY_AND_ASSIGN(TabRestorerMenu);

	// Maps between menu item IDs and closed tab IDs.
	using IdToClosedTabMap = std::unordered_map<UINT, int>;

	static const int MAX_MENU_ITEMS = 10;

	void OnMainMenuPreShow(HMENU mainMenu);
	wil::unique_hmenu BuildRecentlyClosedTabsMenu(std::vector<wil::unique_hbitmap> &menuImages,
		IdToClosedTabMap &menuItemMappings);
	std::optional<std::wstring> MaybeGetMenuItemHelperText(HMENU menu, UINT id);

	HINSTANCE m_resourceInstance;
	CoreInterface *m_coreInterface;

	std::vector<boost::signals2::scoped_connection> m_connections;

	TabRestorer *m_tabRestorer;
	UINT m_menuStartId;
	UINT m_menuEndId;

	wil::unique_hmenu m_recentTabsMenu;
	std::vector<wil::unique_hbitmap> m_menuImages;
	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	wil::unique_hbitmap m_defaultFolderIconBitmap;

	IdToClosedTabMap m_menuItemMappings;
};
