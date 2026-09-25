// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "MenuBase.h"
#include "MenuDelegate.h"

class AppServices;
class BrowserWindow;
struct Config;
class ResourceLoader;

class ToolbarContextMenu : public MenuBase, private MenuDelegate
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

	ToolbarContextMenu(MenuView *menuView, Source source, BrowserWindow *browser,
		AppServices *appServices);

private:
	void BuildMenu(Source source, const ResourceLoader *resourceLoader);

	// MenuDelegate
	bool IsItemEnabled(UINT id) const override;
	bool IsItemChecked(UINT id) const override;
	void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override;

	void OnNewBookmarkItem(BookmarkItem::Type type);
	void OnPasteBookmark();
	void OnNewApplication();

	BrowserWindow *const m_browser;
	AppServices *const m_appServices;
	const Config *const m_config;
};
