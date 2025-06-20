// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserTestBase.h"
#include "Config.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "ShellBrowserFactoryFake.h"
#include "TabContainer.h"
#include "TabEvents.h"
#include "TabNavigationMock.h"
#include "Win32ResourceLoader.h"
#include "../Helper/CachedIcons.h"

class BrowserWindowFake;
class MainTabView;

// Supports tests that require a TabContainer instance.
class TabContainerTestBase : public BrowserTestBase
{
protected:
	TabContainerTestBase();

	[[nodiscard]] int AddTabAndReturnId(const std::wstring &path,
		const TabSettings &tabSettings = {});
	Tab &AddTab(const std::wstring &path, const TabSettings &tabSettings = {});

	BookmarkTree m_bookmarkTree;
	AcceleratorManager m_acceleratorManager;
	Config m_config;
	Win32ResourceLoader m_resourceLoader;
	CachedIcons m_cachedIcons;

	TabEvents m_tabEvents;
	ShellBrowserEvents m_shellBrowserEvents;
	NavigationEvents m_navigationEvents;

	BrowserWindowFake *const m_browser;
	TabNavigationMock m_tabNavigation;
	ShellBrowserFactoryFake m_shellBrowserFactory;
	MainTabView *const m_tabView;
	TabContainer *const m_tabContainer;
};
