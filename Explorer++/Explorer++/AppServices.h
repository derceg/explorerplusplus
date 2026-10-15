// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <tuple>

class AcceleratorManager;
class AppController;
class AsyncIconFetcher;
class BookmarkTree;
class BrowserList;
class BrowserWindowFactory;
class CachedIcons;
class ClipboardWatcher;
class ColorRuleModel;
struct Config;
class DarkModeColorProvider;
class DarkModeManager;
class DirectoryWatcherFactory;
class DriveModel;
class FeatureList;
class FrequentLocationsModel;
class HistoryModel;
class ModelessDialogList;
class NavigationEvents;
class PlatformContext;
class ResourceLoader;
class Runtime;
class ShellBrowserEvents;
class TabEvents;
class TabList;
class TabRestorer;
class ThemeManager;

namespace Applications
{

class ApplicationModel;

}

namespace CommandLine
{

struct Settings;

}

namespace Plugins
{

class PluginCommandManager;
class PluginMenuItemsRegistry;

}

// Provides access to various application services. A "service" here refers to an object instance
// that will be available for the lifetime of the application.
//
// This is useful in situations where a high-level class might indirectly require access to a number
// of different services. Passing an instance of this class can help stop constructors for
// higher-level classes from becoming bloated, at the cost of having less explicit dependencies (it
// may no longer be as clear exactly which dependencies a high-level component needs).
//
// Using this class, as opposed to App, is also advantageous, as App performs a lot of work that
// should only be done when running the application. So, a class that has a dependency on App can't
// be tested. This class more cleanly separates the code that deals with service retrieval from the
// code that deals with setting up and running the application.
//
// Since this class doesn't own anything, it also helps to facilitate testing, since a client class
// can create a service instance in whatever manner they want, then assign it here.
class AppServices
{
public:
	AcceleratorManager *GetAcceleratorManager();
	const AcceleratorManager *GetAcceleratorManager() const;
	void SetAcceleratorManager(AcceleratorManager *acceleratorManager);

	AppController *GetAppController();
	const AppController *GetAppController() const;
	void SetAppController(AppController *appController);

	Applications::ApplicationModel *GetApplicationModel();
	const Applications::ApplicationModel *GetApplicationModel() const;
	void SetApplicationModel(Applications::ApplicationModel *applicationModel);

	AsyncIconFetcher *GetAsyncIconFetcher();
	const AsyncIconFetcher *GetAsyncIconFetcher() const;
	void SetAsyncIconFetcher(AsyncIconFetcher *asyncIconFetcher);

	BookmarkTree *GetBookmarkTree();
	const BookmarkTree *GetBookmarkTree() const;
	void SetBookmarkTree(BookmarkTree *bookmarkTree);

	BrowserList *GetBrowserList();
	const BrowserList *GetBrowserList() const;
	void SetBrowserList(BrowserList *browserList);

	BrowserWindowFactory *GetBrowserWindowFactory();
	const BrowserWindowFactory *GetBrowserWindowFactory() const;
	void SetBrowserWindowFactory(BrowserWindowFactory *browserWindowFactory);

	CachedIcons *GetCachedIcons();
	const CachedIcons *GetCachedIcons() const;
	void SetCachedIcons(CachedIcons *cachedIcons);

	ClipboardWatcher *GetClipboardWatcher();
	const ClipboardWatcher *GetClipboardWatcher() const;
	void SetClipboardWatcher(ClipboardWatcher *clipboardWatcher);

	ColorRuleModel *GetColorRuleModel();
	const ColorRuleModel *GetColorRuleModel() const;
	void SetColorRuleModel(ColorRuleModel *colorRuleModel);

	const CommandLine::Settings *GetCommandLineSettings() const;
	void SetCommandLineSettings(const CommandLine::Settings *commandLineSettings);

	Config *GetConfig();
	const Config *GetConfig() const;
	void SetConfig(Config *config);

	DarkModeColorProvider *GetDarkModeColorProvider();
	const DarkModeColorProvider *GetDarkModeColorProvider() const;
	void SetDarkModeColorProvider(DarkModeColorProvider *darkModeColorProvider);

	DarkModeManager *GetDarkModeManager();
	const DarkModeManager *GetDarkModeManager() const;
	void SetDarkModeManager(DarkModeManager *darkModeManager);

	DirectoryWatcherFactory *GetDirectoryWatcherFactory();
	const DirectoryWatcherFactory *GetDirectoryWatcherFactory() const;
	void SetDirectoryWatcherFactory(DirectoryWatcherFactory *directoryWatcherFactory);

	DriveModel *GetDriveModel();
	const DriveModel *GetDriveModel() const;
	void SetDriveModel(DriveModel *driveModel);

	FeatureList *GetFeatureList();
	const FeatureList *GetFeatureList() const;
	void SetFeatureList(FeatureList *featureList);

	FrequentLocationsModel *GetFrequentLocationsModel();
	const FrequentLocationsModel *GetFrequentLocationsModel() const;
	void SetFrequentLocationsModel(FrequentLocationsModel *frequentLocationsModel);

	HistoryModel *GetHistoryModel();
	const HistoryModel *GetHistoryModel() const;
	void SetHistoryModel(HistoryModel *historyModel);

	ModelessDialogList *GetModelessDialogList();
	const ModelessDialogList *GetModelessDialogList() const;
	void SetModelessDialogList(ModelessDialogList *modelessDialogList);

	NavigationEvents *GetNavigationEvents();
	const NavigationEvents *GetNavigationEvents() const;
	void SetNavigationEvents(NavigationEvents *navigationEvents);

	PlatformContext *GetPlatformContext();
	const PlatformContext *GetPlatformContext() const;
	void SetPlatformContext(PlatformContext *platformContext);

	Plugins::PluginCommandManager *GetPluginCommandManager();
	const Plugins::PluginCommandManager *GetPluginCommandManager() const;
	void SetPluginCommandManager(Plugins::PluginCommandManager *pluginCommandManager);

	Plugins::PluginMenuItemsRegistry *GetPluginMenuItemsRegistry();
	const Plugins::PluginMenuItemsRegistry *GetPluginMenuItemsRegistry() const;
	void SetPluginMenuItemsRegistry(Plugins::PluginMenuItemsRegistry *pluginMenuItemsRegistry);

	ResourceLoader *GetResourceLoader();
	const ResourceLoader *GetResourceLoader() const;
	void SetResourceLoader(ResourceLoader *resourceLoader);

	Runtime *GetRuntime();
	const Runtime *GetRuntime() const;
	void SetRuntime(Runtime *runtime);

	ShellBrowserEvents *GetShellBrowserEvents();
	const ShellBrowserEvents *GetShellBrowserEvents() const;
	void SetShellBrowserEvents(ShellBrowserEvents *shellBrowserEvents);

	TabEvents *GetTabEvents();
	const TabEvents *GetTabEvents() const;
	void SetTabEvents(TabEvents *tabEvents);

	TabList *GetTabList();
	const TabList *GetTabList() const;
	void SetTabList(TabList *tabList);

	TabRestorer *GetTabRestorer();
	const TabRestorer *GetTabRestorer() const;
	void SetTabRestorer(TabRestorer *tabRestorer);

	ThemeManager *GetThemeManager();
	const ThemeManager *GetThemeManager() const;
	void SetThemeManager(ThemeManager *themeManager);

	void CheckFullyInitialized() const;

private:
	template <class T>
	T *Get();

	template <class T>
	const T *Get() const;

	template <class T>
	void Set(T *service);

	std::tuple<AcceleratorManager *, AppController *, Applications::ApplicationModel *,
		AsyncIconFetcher *, BookmarkTree *, BrowserList *, BrowserWindowFactory *, CachedIcons *,
		ClipboardWatcher *, ColorRuleModel *, const CommandLine::Settings *, Config *,
		DarkModeColorProvider *, DarkModeManager *, DirectoryWatcherFactory *, DriveModel *,
		FeatureList *, FrequentLocationsModel *, HistoryModel *, ModelessDialogList *,
		NavigationEvents *, PlatformContext *, Plugins::PluginCommandManager *,
		Plugins::PluginMenuItemsRegistry *, ResourceLoader *, Runtime *, ShellBrowserEvents *,
		TabEvents *, TabList *, TabRestorer *, ThemeManager *>
		m_services;
};
