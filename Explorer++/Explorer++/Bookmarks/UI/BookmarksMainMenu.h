// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <memory>
#include <vector>

class BookmarkIconManager;
class BookmarkTree;
class BrowserWindow;
class IconFetcher;
class PlatformContext;
class ResourceLoader;

class BookmarksMainMenu : public MenuBase
{
public:
	BookmarksMainMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkTree *bookmarkTree, BrowserWindow *browser, IconFetcher *iconFetcher,
		PlatformContext *platformContext, const ResourceLoader *resourceLoader, UINT startId,
		UINT endId);
	~BookmarksMainMenu();

private:
	void RebuildMenu();
	void BuildMenu();

	BookmarkTree *const m_bookmarkTree;
	BrowserWindow *const m_browser;
	PlatformContext *const m_platformContext;
	const ResourceLoader *const m_resourceLoader;
	std::unique_ptr<BookmarkIconManager> m_iconManager;
	std::vector<std::unique_ptr<MenuBase>> m_childMenus;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
