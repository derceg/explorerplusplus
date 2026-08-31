// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "App.h"
#include "AppInfo.h"
#include "BrowserWindow.h"
#include "ColorRuleModel.h"
#include "ColorRuleModelFactory.h"
#include "ColumnStorage.h"
#include "ComStaThreadPoolExecutor.h"
#include "DefaultAccelerators.h"
#include "DriveEnumeratorImpl.h"
#include "ExitCode.h"
#include "FileSystemWatcher.h"
#include "JumpListHelper.h"
#include "LanguageHelper.h"
#include "MainRebarStorage.h"
#include "MainResource.h"
#include "MenuRanges.h"
#include "RegistryAppStorage.h"
#include "RegistryAppStorageFactory.h"
#include "ResourceHelper.h"
#include "SessionRestorer.h"
#include "ShellWatcher.h"
#include "TabStorage.h"
#include "UIThreadExecutor.h"
#include "Win32ResourceLoader.h"
#include "WindowStorage.h"
#include "XmlAppStorage.h"
#include "XmlAppStorageFactory.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/Helper.h"
#include "../Helper/ProcessHelper.h"
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <filesystem>

using namespace std::chrono_literals;

App::App(const CommandLine::Settings *commandLineSettings) :
	m_commandLineSettings(commandLineSettings),
	m_runtime(std::make_unique<UIThreadExecutor>(),
		std::make_unique<ComStaThreadPoolExecutor>(std::max(
			static_cast<int>(std::thread::hardware_concurrency()), MIN_COM_STA_THREADPOOL_SIZE))),
	m_featureList(commandLineSettings->featuresToEnable),
	m_acceleratorManager(InitializeAcceleratorManager()),
	m_directoryWatcherFactory(&m_config, &m_shellWatcherManager, m_runtime.GetUiThreadExecutor()),
	m_darkModeManager(&m_eventWindow, &m_config),
	m_themeManager(&m_darkModeManager, &m_darkModeColorProvider),
	m_cachedIcons(MAX_CACHED_ICONS),
	m_iconFetcher(&m_runtime, &m_cachedIcons),
	m_colorRuleModel(ColorRuleModelFactory::Create()),
	m_processManager(&m_browserList),
	m_tabList(&m_tabEvents),
	m_tabRestorer(&m_tabEvents, &m_browserList),
	m_historyTracker(&m_historyModel, &m_navigationEvents),
	m_frequentLocationsModel(m_platformContext.GetSystemClock()),
	m_frequentLocationsTracker(&m_frequentLocationsModel, &m_navigationEvents),
	m_driveWatcher(&m_eventWindow),
	m_driveModel(std::make_unique<DriveEnumeratorImpl>(), &m_driveWatcher),
	m_pluginMenuManager(&m_browserList, MENU_PLUGIN_START_ID, MENU_PLUGIN_END_ID),
	m_pluginCommandManager(&m_acceleratorManager, ACCELERATOR_PLUGIN_START_ID,
		ACCELERATOR_PLUGIN_END_ID),
	m_pluginManager(&m_appServices),
	m_uniqueGdiplusShutdown(CheckedGdiplusStartup()),
	m_richEditLib(LoadSystemLibrary(
		L"Msftedit.dll")), // This is needed for version 5 of the Rich Edit control.
	m_oleCleanup(wil::OleInitialize_failfast())
{
	CHECK(m_richEditLib);

	INITCOMMONCONTROLSEX commonControls = {};
	commonControls.dwSize = sizeof(commonControls);
	commonControls.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_LISTVIEW_CLASSES
		| ICC_USEREX_CLASSES | ICC_STANDARD_CLASSES | ICC_LINK_CLASS;
	BOOL res = InitCommonControlsEx(&commonControls);
	CHECK(res);

	m_browserList.willRemoveBrowserSignal.AddObserver(std::bind(&App::OnWillRemoveBrowser, this));
	m_browserList.browserRemovedSignal.AddObserver(std::bind(&App::OnBrowserRemoved, this));

	if (m_commandLineSettings->changeNotifyMode)
	{
		m_config.changeNotifyMode = *m_commandLineSettings->changeNotifyMode;
	}
	else if (IsWindowsPE())
	{
		m_config.changeNotifyMode = ChangeNotifyMode::Filesystem;
	}
}

App::~App() = default;

void App::OnBrowserRemoved()
{
	if (m_browserList.IsEmpty())
	{
		// The last top-level browser window has been closed, so exit the application.
		PostQuitMessage(EXIT_CODE_NORMAL);
	}
}

int App::Run()
{
	SetUpSession();

	// Internally, concurrencpp converts the duration to size_t, which triggers a warning in the
	// 32-bit build. The conversion is fine, since the duration here is well below the point at
	// which truncation would occur and it's not reasonable for the duration to ever be large enough
	// for truncation to occur.
#pragma warning(push)
#pragma warning(                                                                                   \
	disable : 4244) // 'argument': conversion from '_Rep' to 'size_t', possible loss of data
	const auto saveFrequency = 30s;
	m_saveSettingsTimer = m_runtime.GetTimerQueue()->make_timer(saveFrequency, saveFrequency,
		m_runtime.GetUiThreadExecutor(), std::bind_front(&App::SaveSettings, this));
#pragma warning(pop)

	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		if (!IsModelessDialogMessage(&msg) && !MaybeTranslateAccelerator(&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return static_cast<int>(msg.wParam);
}

void App::SetUpSession()
{
	std::vector<WindowStorageData> windows;
	LoadSettings(windows);

	// This function may attempt to notify an existing process if the allowMultipleInstances config
	// value is disabled. Therefore, this call needs to be made after the settings have been loaded.
	// If the allowMultipleInstances setting is removed, this call can be made earlier.
	if (!m_processManager.InitializeCurrentProcess(m_commandLineSettings, &m_config))
	{
		PostQuitMessage(EXIT_CODE_NORMAL_EXISTING_PROCESS);
		return;
	}

	SetUpLanguageResourceInstance();
	SetUpAppServices();
	SetupJumpListTasks(m_resourceLoader.get());
	InitializePlugins();

	SessionRestorer sessionRestorer(m_commandLineSettings, &m_config, &m_featureList,
		m_browserWindowFactory.get(), &m_shellNameParser);
	sessionRestorer.Restore(windows);
}

void App::LoadSettings(std::vector<WindowStorageData> &windows)
{
	// Settings will be loaded from the config file by default, if that file is present and can be
	// read.
	std::unique_ptr<AppStorage> appStorage = XmlAppStorageFactory::MaybeCreate(
		Storage::GetConfigFilePath(), Storage::OperationType::Load);

	if (appStorage)
	{
		m_saveLocation = SaveLocation::ConfigFile;
	}
	else
	{
		appStorage = RegistryAppStorageFactory::MaybeCreate(Storage::REGISTRY_APPLICATION_KEY_PATH,
			Storage::OperationType::Load);
	}

	if (!appStorage)
	{
		return;
	}

	appStorage->LoadConfig(m_config);
	windows = appStorage->LoadWindows();
	appStorage->LoadBookmarks(&m_bookmarkTree);
	appStorage->LoadColorRules(m_colorRuleModel.get());
	appStorage->LoadApplications(&m_applicationModel);
	appStorage->LoadDialogStates();
	appStorage->LoadDefaultColumns(m_config.globalFolderSettings.folderColumns);
	appStorage->LoadFrequentLocations(&m_frequentLocationsModel);

	ValidateColumns(m_config.globalFolderSettings.folderColumns);
}

void App::SaveSettings()
{
	// If the application has started exiting, it's not possible to save the settings, so that's not
	// something that should be attempted. That's because one or more of the windows may have
	// already been closed.
	CHECK(!m_exitStarted);

	std::unique_ptr<AppStorage> appStorage;

	if (m_saveLocation == SaveLocation::ConfigFile)
	{
		appStorage = XmlAppStorageFactory::MaybeCreate(Storage::GetConfigFilePath(),
			Storage::OperationType::Save);
	}
	else
	{
		appStorage = RegistryAppStorageFactory::MaybeCreate(Storage::REGISTRY_APPLICATION_KEY_PATH,
			Storage::OperationType::Save);
	}

	if (!appStorage)
	{
		return;
	}

	std::vector<WindowStorageData> windows;

	for (const auto *browser : m_browserList.GetList())
	{
		windows.push_back(browser->GetStorageData());
	}

	DCHECK_GE(windows.size(), 1u);

	appStorage->SaveConfig(m_config);
	appStorage->SaveWindows(windows);
	appStorage->SaveBookmarks(&m_bookmarkTree);
	appStorage->SaveColorRules(m_colorRuleModel.get());
	appStorage->SaveApplications(&m_applicationModel);
	appStorage->SaveDialogStates();
	appStorage->SaveDefaultColumns(m_config.globalFolderSettings.folderColumns);
	appStorage->SaveFrequentLocations(&m_frequentLocationsModel);

	appStorage->Commit();
}

void App::SetUpLanguageResourceInstance()
{
	auto languageResult = LanguageHelper::MaybeLoadTranslationDll(m_commandLineSettings, &m_config);
	LanguageHelper::LanguageInfo languageInfo;

	if (std::holds_alternative<LanguageHelper::LanguageInfo>(languageResult))
	{
		languageInfo = std::get<LanguageHelper::LanguageInfo>(languageResult);
	}
	else
	{
		auto errorCode = std::get<LanguageHelper::LoadError>(languageResult);

		if (errorCode == LanguageHelper::LoadError::VersionMismatch)
		{
			std::wstring versionMismatchMessage = ResourceHelper::LoadString(
				GetModuleHandle(nullptr), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH);
			MessageBox(nullptr, versionMismatchMessage.c_str(), AppInfo::NAME, MB_ICONWARNING);
		}

		languageInfo = { LanguageHelper::DEFAULT_LANGUAGE, GetModuleHandle(nullptr) };
	}

	m_config.language = languageInfo.language;
	auto resourceInstance = languageInfo.resourceInstance;

	if (LanguageHelper::IsLanguageRTL(m_config.language))
	{
		SetProcessDefaultLayout(LAYOUT_RTL);
	}

	m_resourceLoader = std::make_unique<Win32ResourceLoader>(resourceInstance, m_config.iconSet,
		&m_darkModeManager, &m_themeManager);
	m_browserWindowFactory =
		std::make_unique<BrowserWindowFactoryImpl>(&m_appServices, resourceInstance);
}

void App::SetUpAppServices()
{
	m_appServices.SetAcceleratorManager(&m_acceleratorManager);
	m_appServices.SetAppController(this);
	m_appServices.SetApplicationModel(&m_applicationModel);
	m_appServices.SetAsyncIconFetcher(&m_iconFetcher);
	m_appServices.SetBookmarkTree(&m_bookmarkTree);
	m_appServices.SetBrowserList(&m_browserList);
	m_appServices.SetBrowserWindowFactory(m_browserWindowFactory.get());
	m_appServices.SetCachedIcons(&m_cachedIcons);
	m_appServices.SetClipboardWatcher(&m_clipboardWatcher);
	m_appServices.SetColorRuleModel(m_colorRuleModel.get());
	m_appServices.SetCommandLineSettings(m_commandLineSettings);
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
	m_appServices.SetResourceLoader(m_resourceLoader.get());
	m_appServices.SetRuntime(&m_runtime);
	m_appServices.SetShellBrowserEvents(&m_shellBrowserEvents);
	m_appServices.SetTabEvents(&m_tabEvents);
	m_appServices.SetTabList(&m_tabList);
	m_appServices.SetTabRestorer(&m_tabRestorer);
	m_appServices.SetThemeManager(&m_themeManager);
	m_appServices.CheckFullyInitialized();
}

void App::InitializePlugins()
{
	if (!m_featureList.IsEnabled(Feature::Plugins))
	{
		return;
	}

	std::filesystem::path pluginsPath(GetCurrentProcessPath());
	pluginsPath.remove_filename();
	pluginsPath.append(PLUGIN_FOLDER_NAME);

	m_pluginManager.LoadAllPlugins(pluginsPath);
}

bool App::IsModelessDialogMessage(MSG *msg)
{
	for (auto modelessDialog : m_modelessDialogList.GetList())
	{
		if (IsChild(modelessDialog, msg->hwnd))
		{
			return IsDialogMessage(modelessDialog, msg);
		}
	}

	return false;
}

bool App::MaybeTranslateAccelerator(MSG *msg)
{
	for (auto *browser : m_browserList.GetList())
	{
		if (IsChild(browser->GetHWND(), msg->hwnd))
		{
			return TranslateAccelerator(browser->GetHWND(),
				m_acceleratorManager.GetAcceleratorTable(), msg);
		}
	}

	return false;
}

SaveLocation App::GetSaveLocation() const
{
	return m_saveLocation;
}

void App::SetSaveLocation(SaveLocation saveLocation)
{
	m_saveLocation = saveLocation;
}

void App::OnWillRemoveBrowser()
{
	if (m_browserList.GetSize() == 1 && !m_exitStarted)
	{
		// The last browser window is about to be closed, which indicates that the application is
		// going to exit. Note that the exit may have already started (e.g. if there were multiple
		// windows open and the user selected the "Exit" menu item). In that case, this branch won't
		// be taken.
		OnExitStarted();
	}
}

void App::TryExit()
{
	if (!ConfirmExit())
	{
		return;
	}

	Exit();
}

bool App::ConfirmExit()
{
	if (!m_config.confirmCloseTabs)
	{
		return true;
	}

	auto numWindows = m_browserList.GetSize();

	if (numWindows == 1)
	{
		return true;
	}

	auto *browser = m_browserList.GetLastActive();
	CHECK(browser);

	std::wstring message =
		fmt::format(fmt::runtime(m_resourceLoader->LoadString(IDS_CLOSE_ALL_WINDOWS)),
			fmt::arg(L"num_windows", numWindows));
	int response = MessageBox(browser->GetHWND(), message.c_str(), AppInfo::NAME,
		MB_ICONINFORMATION | MB_YESNO);

	if (response == IDNO)
	{
		return false;
	}

	return true;
}

void App::Exit()
{
	if (m_exitStarted)
	{
		DCHECK(false);
		return;
	}

	OnExitStarted();

	std::vector<BrowserWindow *> browsers;

	// Closing a browser window will alter the list of browsers, which is why the list is copied
	// here.
	for (auto *browser : m_browserList.GetList())
	{
		browsers.push_back(browser);
	}

	for (auto *browser : browsers)
	{
		browser->Close();
	}
}

void App::OnExitStarted()
{
	CHECK(!m_exitStarted);

	// The application is going to exit, so the settings need to be saved before the shutdown
	// begins.
	m_saveSettingsTimer.cancel();
	SaveSettings();

	m_exitStarted = true;
}

void App::NotifySessionEnding()
{
	if (m_exitStarted)
	{
		// The application has already started exiting, so there's no need to try and save the
		// settings, since it will have already been done.
		return;
	}

	SaveSettings();
}
