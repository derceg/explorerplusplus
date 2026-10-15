// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AppServices.h"

AcceleratorManager *AppServices::GetAcceleratorManager()
{
	return Get<AcceleratorManager>();
}

const AcceleratorManager *AppServices::GetAcceleratorManager() const
{
	return Get<AcceleratorManager>();
}

void AppServices::SetAcceleratorManager(AcceleratorManager *acceleratorManager)
{
	Set(acceleratorManager);
}

AppController *AppServices::GetAppController()
{
	return Get<AppController>();
}

const AppController *AppServices::GetAppController() const
{
	return Get<AppController>();
}

void AppServices::SetAppController(AppController *appController)
{
	Set(appController);
}

Applications::ApplicationModel *AppServices::GetApplicationModel()
{
	return Get<Applications::ApplicationModel>();
}

const Applications::ApplicationModel *AppServices::GetApplicationModel() const
{
	return Get<Applications::ApplicationModel>();
}

void AppServices::SetApplicationModel(Applications::ApplicationModel *applicationModel)
{
	Set(applicationModel);
}

AsyncIconFetcher *AppServices::GetAsyncIconFetcher()
{
	return Get<AsyncIconFetcher>();
}

const AsyncIconFetcher *AppServices::GetAsyncIconFetcher() const
{
	return Get<AsyncIconFetcher>();
}

void AppServices::SetAsyncIconFetcher(AsyncIconFetcher *asyncIconFetcher)
{
	Set(asyncIconFetcher);
}

BookmarkTree *AppServices::GetBookmarkTree()
{
	return Get<BookmarkTree>();
}

const BookmarkTree *AppServices::GetBookmarkTree() const
{
	return Get<BookmarkTree>();
}

void AppServices::SetBookmarkTree(BookmarkTree *bookmarkTree)
{
	Set(bookmarkTree);
}

BrowserList *AppServices::GetBrowserList()
{
	return Get<BrowserList>();
}

const BrowserList *AppServices::GetBrowserList() const
{
	return Get<BrowserList>();
}

void AppServices::SetBrowserList(BrowserList *browserList)
{
	Set(browserList);
}

BrowserWindowFactory *AppServices::GetBrowserWindowFactory()
{
	return Get<BrowserWindowFactory>();
}

const BrowserWindowFactory *AppServices::GetBrowserWindowFactory() const
{
	return Get<BrowserWindowFactory>();
}

void AppServices::SetBrowserWindowFactory(BrowserWindowFactory *browserWindowFactory)
{
	Set(browserWindowFactory);
}

CachedIcons *AppServices::GetCachedIcons()
{
	return Get<CachedIcons>();
}

const CachedIcons *AppServices::GetCachedIcons() const
{
	return Get<CachedIcons>();
}

void AppServices::SetCachedIcons(CachedIcons *cachedIcons)
{
	Set(cachedIcons);
}

ClipboardWatcher *AppServices::GetClipboardWatcher()
{
	return Get<ClipboardWatcher>();
}

const ClipboardWatcher *AppServices::GetClipboardWatcher() const
{
	return Get<ClipboardWatcher>();
}

void AppServices::SetClipboardWatcher(ClipboardWatcher *clipboardWatcher)
{
	Set(clipboardWatcher);
}

ColorRuleModel *AppServices::GetColorRuleModel()
{
	return Get<ColorRuleModel>();
}

const ColorRuleModel *AppServices::GetColorRuleModel() const
{
	return Get<ColorRuleModel>();
}

void AppServices::SetColorRuleModel(ColorRuleModel *colorRuleModel)
{
	Set(colorRuleModel);
}

const CommandLine::Settings *AppServices::GetCommandLineSettings() const
{
	return Get<const CommandLine::Settings>();
}

void AppServices::SetCommandLineSettings(const CommandLine::Settings *commandLineSettings)
{
	Set(commandLineSettings);
}

Config *AppServices::GetConfig()
{
	return Get<Config>();
}

const Config *AppServices::GetConfig() const
{
	return Get<Config>();
}

void AppServices::SetConfig(Config *config)
{
	Set(config);
}

DarkModeColorProvider *AppServices::GetDarkModeColorProvider()
{
	return Get<DarkModeColorProvider>();
}

const DarkModeColorProvider *AppServices::GetDarkModeColorProvider() const
{
	return Get<DarkModeColorProvider>();
}

void AppServices::SetDarkModeColorProvider(DarkModeColorProvider *darkModeColorProvider)
{
	Set(darkModeColorProvider);
}

DarkModeManager *AppServices::GetDarkModeManager()
{
	return Get<DarkModeManager>();
}

const DarkModeManager *AppServices::GetDarkModeManager() const
{
	return Get<DarkModeManager>();
}

void AppServices::SetDarkModeManager(DarkModeManager *darkModeManager)
{
	Set(darkModeManager);
}

DirectoryWatcherFactory *AppServices::GetDirectoryWatcherFactory()
{
	return Get<DirectoryWatcherFactory>();
}

const DirectoryWatcherFactory *AppServices::GetDirectoryWatcherFactory() const
{
	return Get<DirectoryWatcherFactory>();
}

void AppServices::SetDirectoryWatcherFactory(DirectoryWatcherFactory *directoryWatcherFactory)
{
	Set(directoryWatcherFactory);
}

DriveModel *AppServices::GetDriveModel()
{
	return Get<DriveModel>();
}

const DriveModel *AppServices::GetDriveModel() const
{
	return Get<DriveModel>();
}

void AppServices::SetDriveModel(DriveModel *driveModel)
{
	Set(driveModel);
}

FeatureList *AppServices::GetFeatureList()
{
	return Get<FeatureList>();
}

const FeatureList *AppServices::GetFeatureList() const
{
	return Get<FeatureList>();
}

void AppServices::SetFeatureList(FeatureList *featureList)
{
	Set(featureList);
}

FrequentLocationsModel *AppServices::GetFrequentLocationsModel()
{
	return Get<FrequentLocationsModel>();
}

const FrequentLocationsModel *AppServices::GetFrequentLocationsModel() const
{
	return Get<FrequentLocationsModel>();
}

void AppServices::SetFrequentLocationsModel(FrequentLocationsModel *frequentLocationsModel)
{
	Set(frequentLocationsModel);
}

HistoryModel *AppServices::GetHistoryModel()
{
	return Get<HistoryModel>();
}

const HistoryModel *AppServices::GetHistoryModel() const
{
	return Get<HistoryModel>();
}

void AppServices::SetHistoryModel(HistoryModel *historyModel)
{
	Set(historyModel);
}

ModelessDialogList *AppServices::GetModelessDialogList()
{
	return Get<ModelessDialogList>();
}

const ModelessDialogList *AppServices::GetModelessDialogList() const
{
	return Get<ModelessDialogList>();
}

void AppServices::SetModelessDialogList(ModelessDialogList *modelessDialogList)
{
	Set(modelessDialogList);
}

NavigationEvents *AppServices::GetNavigationEvents()
{
	return Get<NavigationEvents>();
}

const NavigationEvents *AppServices::GetNavigationEvents() const
{
	return Get<NavigationEvents>();
}

void AppServices::SetNavigationEvents(NavigationEvents *navigationEvents)
{
	Set(navigationEvents);
}

PlatformContext *AppServices::GetPlatformContext()
{
	return Get<PlatformContext>();
}

const PlatformContext *AppServices::GetPlatformContext() const
{
	return Get<PlatformContext>();
}

void AppServices::SetPlatformContext(PlatformContext *platformContext)
{
	Set(platformContext);
}

Plugins::PluginCommandManager *AppServices::GetPluginCommandManager()
{
	return Get<Plugins::PluginCommandManager>();
}

const Plugins::PluginCommandManager *AppServices::GetPluginCommandManager() const
{
	return Get<Plugins::PluginCommandManager>();
}

void AppServices::SetPluginCommandManager(Plugins::PluginCommandManager *pluginCommandManager)
{
	Set(pluginCommandManager);
}

Plugins::PluginMenuItemsRegistry *AppServices::GetPluginMenuItemsRegistry()
{
	return Get<Plugins::PluginMenuItemsRegistry>();
}

const Plugins::PluginMenuItemsRegistry *AppServices::GetPluginMenuItemsRegistry() const
{
	return Get<Plugins::PluginMenuItemsRegistry>();
}

void AppServices::SetPluginMenuItemsRegistry(
	Plugins::PluginMenuItemsRegistry *pluginMenuItemsRegistry)
{
	Set(pluginMenuItemsRegistry);
}

ResourceLoader *AppServices::GetResourceLoader()
{
	return Get<ResourceLoader>();
}

const ResourceLoader *AppServices::GetResourceLoader() const
{
	return Get<ResourceLoader>();
}

void AppServices::SetResourceLoader(ResourceLoader *resourceLoader)
{
	Set(resourceLoader);
}

Runtime *AppServices::GetRuntime()
{
	return Get<Runtime>();
}

const Runtime *AppServices::GetRuntime() const
{
	return Get<Runtime>();
}

void AppServices::SetRuntime(Runtime *runtime)
{
	Set(runtime);
}

ShellBrowserEvents *AppServices::GetShellBrowserEvents()
{
	return Get<ShellBrowserEvents>();
}

const ShellBrowserEvents *AppServices::GetShellBrowserEvents() const
{
	return Get<ShellBrowserEvents>();
}

void AppServices::SetShellBrowserEvents(ShellBrowserEvents *shellBrowserEvents)
{
	Set(shellBrowserEvents);
}

TabEvents *AppServices::GetTabEvents()
{
	return Get<TabEvents>();
}

const TabEvents *AppServices::GetTabEvents() const
{
	return Get<TabEvents>();
}

void AppServices::SetTabEvents(TabEvents *tabEvents)
{
	Set(tabEvents);
}

TabList *AppServices::GetTabList()
{
	return Get<TabList>();
}

const TabList *AppServices::GetTabList() const
{
	return Get<TabList>();
}

void AppServices::SetTabList(TabList *tabList)
{
	Set(tabList);
}

TabRestorer *AppServices::GetTabRestorer()
{
	return Get<TabRestorer>();
}

const TabRestorer *AppServices::GetTabRestorer() const
{
	return Get<TabRestorer>();
}

void AppServices::SetTabRestorer(TabRestorer *tabRestorer)
{
	Set(tabRestorer);
}

ThemeManager *AppServices::GetThemeManager()
{
	return Get<ThemeManager>();
}

const ThemeManager *AppServices::GetThemeManager() const
{
	return Get<ThemeManager>();
}

void AppServices::SetThemeManager(ThemeManager *themeManager)
{
	Set(themeManager);
}

void AppServices::CheckFullyInitialized() const
{
	std::apply([](const auto *...services) { CHECK((services && ...)); }, m_services);
}

template <class T>
T *AppServices::Get()
{
	auto *service = std::get<T *>(m_services);
	CHECK(service);
	return service;
}

template <class T>
const T *AppServices::Get() const
{
	const auto *service = std::get<T *>(m_services);
	CHECK(service);
	return service;
}

template <class T>
void AppServices::Set(T *service)
{
	CHECK(service);

	auto &currentService = std::get<T *>(m_services);
	CHECK(!currentService);

	currentService = service;
}
