// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserList.h"
#include "Config.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "TabEvents.h"
#include "Win32ResourceLoader.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/PidlHelper.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

class BrowserWindowFake;
class Tab;

class BrowserTestBase : public testing::Test
{
protected:
	BrowserTestBase();
	~BrowserTestBase();

	BrowserWindowFake *AddBrowser();
	void RemoveBrowser(BrowserWindowFake *browser);

	static void NavigateTab(Tab *tab, const std::wstring &path, PidlAbsolute *outputPidl = nullptr);

	Config m_config;
	AcceleratorManager m_acceleratorManager;
	BookmarkTree m_bookmarkTree;
	CachedIcons m_cachedIcons;
	Win32ResourceLoader m_resourceLoader;

	TabEvents m_tabEvents;
	ShellBrowserEvents m_shellBrowserEvents;
	NavigationEvents m_navigationEvents;

	BrowserList m_browserList;

private:
	std::vector<std::unique_ptr<BrowserWindowFake>> m_browsers;
};
