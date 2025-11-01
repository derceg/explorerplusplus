// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorManager.h"
#include "ApplicationModel.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserList.h"
#include "CommandLine.h"
#include "Config.h"
#include "DarkModeColorProvider.h"
#include "DarkModeManager.h"
#include "DirectoryWatcherFactory.h"
#include "DriveModel.h"
#include "DriveWatcherImpl.h"
#include "EventWindow.h"
#include "FeatureList.h"
#include "FrequentLocationsModel.h"
#include "FrequentLocationsTracker.h"
#include "HistoryModel.h"
#include "HistoryTracker.h"
#include "ModelessDialogList.h"
#include "PlatformContextImpl.h"
#include "ProcessManager.h"
#include "Runtime.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "ShellWatcherManager.h"
#include "TabEvents.h"
#include "TabList.h"
#include "TabRestorer.h"
#include "ThemeManager.h"
#include "../Helper/ClipboardWatcher.h"
#include "../Helper/UniqueResources.h"
#include <boost/core/noncopyable.hpp>
#include <wil/resource.h>
#include <memory>
#include <vector>

class AsyncIconFetcher;
class CachedIcons;
class ColorRuleModel;
class ResourceLoader;
struct WindowStorageData;

class App : private boost::noncopyable
{
public:
	static constexpr wchar_t APP_NAME[] = L"Explorer++";

	App(const CommandLine::Settings *commandLineSettings);
	~App();

	int Run();

	const CommandLine::Settings *GetCommandLineSettings() const;
	bool GetSavePreferencesToXmlFile() const;
	void SetSavePreferencesToXmlFile(bool savePreferencesToXmlFile);
	PlatformContext *GetPlatformContext();
	Runtime *GetRuntime();
	ClipboardWatcher *GetClipboardWatcher();
	FeatureList *GetFeatureList();
	AcceleratorManager *GetAcceleratorManager();
	Config *GetConfig();
	DirectoryWatcherFactory *GetDirectoryWatcherFactory();
	CachedIcons *GetCachedIcons();
	std::shared_ptr<AsyncIconFetcher> GetIconFetcher();
	BrowserList *GetBrowserList();
	ModelessDialogList *GetModelessDialogList();
	BookmarkTree *GetBookmarkTree();
	ColorRuleModel *GetColorRuleModel() const;
	Applications::ApplicationModel *GetApplicationModel();
	HINSTANCE GetResourceInstance() const;
	ResourceLoader *GetResourceLoader() const;
	TabEvents *GetTabEvents();
	ShellBrowserEvents *GetShellBrowserEvents();
	NavigationEvents *GetNavigationEvents();
	TabList *GetTabList();
	TabRestorer *GetTabRestorer();
	DarkModeManager *GetDarkModeManager();
	DarkModeColorProvider *GetDarkModeColorProvider();
	ThemeManager *GetThemeManager();
	HistoryModel *GetHistoryModel();
	FrequentLocationsModel *GetFrequentLocationsModel();
	DriveModel *GetDriveModel();

	void TryExit();
	void SessionEnding();

private:
	// Represents the maximum number of icons that can be cached. This cache is shared between
	// various components in the application.
	static constexpr int MAX_CACHED_ICONS = 1000;

	static constexpr int MIN_COM_STA_THREADPOOL_SIZE = 5;

	void OnBrowserRemoved();
	void SetUpSession();
	void LoadSettings(std::vector<WindowStorageData> &windows);
	void SaveSettings();
	void SetUpLanguageResourceInstance();
	void RestoreSession(const std::vector<WindowStorageData> &windows);
	void RestorePreviousWindows(const std::vector<WindowStorageData> &windows);
	void CreateStartupFolders();
	bool IsModelessDialogMessage(MSG *msg);
	bool MaybeTranslateAccelerator(MSG *msg);

	void OnWillRemoveBrowser();
	bool ConfirmExit();
	void Exit();
	void OnExitStarted();

	const CommandLine::Settings *const m_commandLineSettings;
	bool m_savePreferencesToXmlFile = false;
	PlatformContextImpl m_platformContext;
	Runtime m_runtime;
	EventWindow m_eventWindow;
	ClipboardWatcher m_clipboardWatcher;
	FeatureList m_featureList;
	AcceleratorManager m_acceleratorManager;
	Config m_config;
	ShellWatcherManager m_shellWatcherManager;
	DirectoryWatcherFactory m_directoryWatcherFactory;
	DarkModeManager m_darkModeManager;
	DarkModeColorProvider m_darkModeColorProvider;
	ThemeManager m_themeManager;
	std::shared_ptr<CachedIcons> m_cachedIcons;
	std::shared_ptr<AsyncIconFetcher> m_iconFetcher;
	BrowserList m_browserList;
	ModelessDialogList m_modelessDialogList;
	BookmarkTree m_bookmarkTree;
	std::unique_ptr<ColorRuleModel> m_colorRuleModel;
	Applications::ApplicationModel m_applicationModel;
	HINSTANCE m_resourceInstance;
	std::unique_ptr<ResourceLoader> m_resourceLoader;
	ProcessManager m_processManager;
	TabEvents m_tabEvents;
	ShellBrowserEvents m_shellBrowserEvents;
	NavigationEvents m_navigationEvents;
	TabList m_tabList;
	TabRestorer m_tabRestorer;

	HistoryModel m_historyModel;
	HistoryTracker m_historyTracker;

	FrequentLocationsModel m_frequentLocationsModel;
	FrequentLocationsTracker m_frequentLocationsTracker;

	DriveWatcherImpl m_driveWatcher;
	DriveModel m_driveModel;

	concurrencpp::timer m_saveSettingsTimer;

	unique_gdiplus_shutdown m_uniqueGdiplusShutdown;
	wil::unique_hmodule m_richEditLib;
	wil::unique_oleuninitialize_call m_oleCleanup;

	bool m_exitStarted = false;
};
