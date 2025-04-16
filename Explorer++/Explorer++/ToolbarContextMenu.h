// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <vector>

class App;
class BrowserWindow;
class ResourceLoader;

class ToolbarContextMenu : public MenuBase
{
public:
	enum class Source
	{
		AddressBar,
		MainToolbar,
		BookmarksToolbar,
		DrivesToolbar,
		ApplicationToolbar
	};

	ToolbarContextMenu(MenuView *menuView, Source source, App *app, BrowserWindow *browser);

private:
	void BuildMenu(Source source, const ResourceLoader *resourceLoader);

	void OnMenuItemSelected(UINT menuItemId);
	void OnNewBookmarkItem(BookmarkItem::Type type);
	void OnPasteBookmark();
	void OnNewApplication();

	App *const m_app;
	BrowserWindow *const m_browser;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
