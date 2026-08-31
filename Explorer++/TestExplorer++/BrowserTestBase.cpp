// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "DriveEnumeratorFake.h"
#include "MenuRanges.h"
#include "PidlTestHelper.h"
#include "RuntimeTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include <wil/common.h>

BrowserTestBase::BrowserTestBase() :
	m_runtime(BuildRuntimeForTest()),
	m_directoryWatcherFactory(&m_fileSystem),
	m_cachedIcons(10),
	m_resourceLoader(GetModuleHandle(nullptr), IconSet::Color, nullptr, nullptr),
	m_tabList(&m_tabEvents),
	m_historyTracker(&m_historyModel, &m_navigationEvents),
	m_frequentLocationsModel(m_platformContext.GetSystemClock()),
	m_frequentLocationsTracker(&m_frequentLocationsModel, &m_navigationEvents),
	m_browserWindowFactory(
		[this](const WindowStorageData &storageData) { return AddBrowser(storageData); }),
	m_tabRestorer(&m_tabEvents, &m_browserList),
	m_driveModel(std::make_unique<DriveEnumeratorFake>(), &m_driveWatcher),
	m_pluginMenuManager(&m_browserList, MENU_PLUGIN_START_ID, MENU_PLUGIN_END_ID),
	m_pluginCommandManager(&m_acceleratorManager, ACCELERATOR_PLUGIN_START_ID,
		ACCELERATOR_PLUGIN_END_ID)
{
	SetUpAppServices();
}

void BrowserTestBase::SetUpAppServices()
{
	m_appServices.SetAcceleratorManager(&m_acceleratorManager);
	m_appServices.SetAppController(&m_appController);
	m_appServices.SetApplicationModel(&m_applicationModel);
	m_appServices.SetAsyncIconFetcher(&m_iconFetcher);
	m_appServices.SetBookmarkTree(&m_bookmarkTree);
	m_appServices.SetBrowserList(&m_browserList);
	m_appServices.SetBrowserWindowFactory(&m_browserWindowFactory);
	m_appServices.SetCachedIcons(&m_cachedIcons);
	m_appServices.SetClipboardWatcher(&m_clipboardWatcher);
	m_appServices.SetColorRuleModel(&m_colorRuleModel);
	m_appServices.SetCommandLineSettings(&m_commandLineSettings);
	m_appServices.SetConfig(&m_config);
	m_appServices.SetDarkModeColorProvider(&m_darkModeColorProvider);
	m_appServices.SetDarkModeManager(&m_darkModeManager);
	m_appServices.SetDirectoryWatcherFactory(&m_directoryWatcherFactory);
	m_appServices.SetDriveModel(&m_driveModel);
	m_appServices.SetFeatureList(&m_featureList);
	m_appServices.SetFrequentLocationsModel(&m_frequentLocationsModel);
	m_appServices.SetHistoryModel(&m_historyModel);
	m_appServices.SetModelessDialogList(&m_modelessDialogList);
	m_appServices.SetNavigationEvents(&m_navigationEvents);
	m_appServices.SetPlatformContext(&m_platformContext);
	m_appServices.SetPluginCommandManager(&m_pluginCommandManager);
	m_appServices.SetPluginMenuManager(&m_pluginMenuManager);
	m_appServices.SetResourceLoader(&m_resourceLoader);
	m_appServices.SetRuntime(&m_runtime);
	m_appServices.SetShellBrowserEvents(&m_shellBrowserEvents);
	m_appServices.SetTabEvents(&m_tabEvents);
	m_appServices.SetTabList(&m_tabList);
	m_appServices.SetTabRestorer(&m_tabRestorer);
	m_appServices.SetThemeManager(&m_themeManager);
	m_appServices.CheckFullyInitialized();
}

BrowserTestBase::~BrowserTestBase() = default;

BrowserWindowFake *BrowserTestBase::AddBrowser(const WindowStorageData &storageData)
{
	auto browser = std::make_unique<BrowserWindowFake>(storageData, &m_appServices);
	auto *rawBrowser = browser.get();

	m_browsers.push_back(std::move(browser));
	m_browserList.AddBrowser(rawBrowser);

	return rawBrowser;
}

void BrowserTestBase::RemoveBrowser(BrowserWindowFake *browser)
{
	browser->NotifyBrowserClosing();

	m_browserList.RemoveBrowser(browser);

	auto itr = std::ranges::find_if(m_browsers,
		[browser](const auto &currentBrowser) { return currentBrowser.get() == browser; });
	ASSERT_TRUE(itr != m_browsers.end());
	m_browsers.erase(itr);
}

void BrowserTestBase::NavigateTab(Tab *tab, const std::wstring &path, PidlAbsolute *outputPidl)
{
	auto pidl = CreateSimplePidlForTest(path);
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	tab->GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);

	wil::assign_to_opt_param(outputPidl, pidl);
}
