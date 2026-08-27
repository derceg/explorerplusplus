// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorManager.h"
#include "AppControllerFake.h"
#include "AppServices.h"
#include "ApplicationModel.h"
#include "AsyncIconFetcherFake.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserList.h"
#include "BrowserWindowFactoryFake.h"
#include "ColorRuleModel.h"
#include "CommandLine.h"
#include "Config.h"
#include "DarkModeColorProvider.h"
#include "DarkModeManagerFake.h"
#include "DriveModel.h"
#include "DriveWatcherFake.h"
#include "FeatureList.h"
#include "FrequentLocationsModel.h"
#include "FrequentLocationsTracker.h"
#include "HistoryModel.h"
#include "HistoryTracker.h"
#include "MainRebarStorage.h"
#include "ModelessDialogList.h"
#include "NoOpClipboardWatcher.h"
#include "NoOpThemeManager.h"
#include "PlatformContextFake.h"
#include "Plugins/PluginCommandManager.h"
#include "Plugins/PluginMenuManager.h"
#include "Runtime.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "SimulatedFileSystem.h"
#include "SimulatedFileSystemWatcherFactory.h"
#include "TabEvents.h"
#include "TabList.h"
#include "TabRestorer.h"
#include "TabStorage.h"
#include "Win32ResourceLoader.h"
#include "WindowStorage.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/Pidl.h"
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

	BrowserWindowFake *AddBrowser(const WindowStorageData &storageData = {});
	void RemoveBrowser(BrowserWindowFake *browser);

	static void NavigateTab(Tab *tab, const std::wstring &path, PidlAbsolute *outputPidl = nullptr);

	CommandLine::Settings m_commandLineSettings;
	AppServices m_appServices;
	AppControllerFake m_appController;
	PlatformContextFake m_platformContext;
	Runtime m_runtime;
	SimulatedFileSystem m_fileSystem;
	SimulatedFileSystemWatcherFactory m_directoryWatcherFactory;
	NoOpClipboardWatcher m_clipboardWatcher;

	FeatureList m_featureList;
	Config m_config;
	AcceleratorManager m_acceleratorManager;
	ModelessDialogList m_modelessDialogList;
	DarkModeManagerFake m_darkModeManager;
	DarkModeColorProvider m_darkModeColorProvider;
	NoOpThemeManager m_themeManager;
	BookmarkTree m_bookmarkTree;
	CachedIcons m_cachedIcons;
	AsyncIconFetcherFake m_iconFetcher;
	Win32ResourceLoader m_resourceLoader;
	ColorRuleModel m_colorRuleModel;
	Applications::ApplicationModel m_applicationModel;

	TabEvents m_tabEvents;
	ShellBrowserEvents m_shellBrowserEvents;
	NavigationEvents m_navigationEvents;
	TabList m_tabList;
	HistoryModel m_historyModel;
	HistoryTracker m_historyTracker;
	FrequentLocationsModel m_frequentLocationsModel;
	FrequentLocationsTracker m_frequentLocationsTracker;
	BrowserList m_browserList;
	BrowserWindowFactoryFake m_browserWindowFactory;
	TabRestorer m_tabRestorer;

	DriveWatcherFake m_driveWatcher;
	DriveModel m_driveModel;

	// Plugins
	Plugins::PluginMenuManager m_pluginMenuManager;
	Plugins::PluginCommandManager m_pluginCommandManager;

private:
	void SetUpAppServices();

	std::vector<std::unique_ptr<BrowserWindowFake>> m_browsers;
};
