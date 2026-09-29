// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "MenuBase.h"

class AppServices;
class BrowserWindow;
struct Config;
class ResourceLoader;

class ToolbarOptionsMenu : public MenuBase
{
public:
	enum class Source
	{
		AddressBar,
		MainToolbar,
		BookmarksToolbar,
		DrivesToolbar,
		ApplicationToolbar,
		MainMenu
	};

	ToolbarOptionsMenu(MenuView *menuView, Source source, BrowserWindow *browser,
		AppServices *appServices);

private:
	void BuildMenu(Source source, const ResourceLoader *resourceLoader);
	bool CanPasteBookmark() const;

	void OnNewBookmarkItem(BookmarkItem::Type type);
	void OnPasteBookmark();
	void OnNewApplication();

	BrowserWindow *const m_browser;
	AppServices *const m_appServices;
	Config *const m_config;
};
