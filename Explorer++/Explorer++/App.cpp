// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "App.h"
#include "Explorer++.h"
#include "BrowserWindow.h"
#include "ColorRuleModel.h"
#include "ColorRuleModelFactory.h"
#include "ColumnStorage.h"
#include "ComStaThreadPoolExecutor.h"
#include "DefaultAccelerators.h"
#include "ExitCode.h"
#include "IconResourceLoader.h"
#include "LanguageHelper.h"
#include "MainRebarStorage.h"
#include "MainResource.h"
#include "RegistryAppStorage.h"
#include "RegistryAppStorageFactory.h"
#include "ResourceHelper.h"
#include "ResourceManager.h"
#include "TabStorage.h"
#include "UIThreadExecutor.h"
#include "Win32ResourceLoader.h"
#include "WindowStorage.h"
#include "XmlAppStorage.h"
#include "XmlAppStorageFactory.h"
#include "../Helper/Helper.h"
#include <fmt/format.h>
#include <fmt/xchar.h>

using namespace std::chrono_literals;

App::App(const CommandLine::Settings *commandLineSettings) :
	m_commandLineSettings(commandLineSettings),
	m_runtime(std::make_unique<UIThreadExecutor>(), std::make_unique<ComStaThreadPoolExecutor>(1)),
	m_featureList(commandLineSettings->featuresToEnable),
	m_acceleratorManager(InitializeAcceleratorManager()),
	m_cachedIcons(MAX_CACHED_ICONS),
	m_colorRuleModel(ColorRuleModelFactory::Create()),
	m_resourceInstance(GetModuleHandle(nullptr)),
	m_processManager(&m_browserList),
	m_tabRestorer(&m_globalTabEventDispatcher, &m_browserList),
	m_darkModeManager(&m_config),
	m_themeManager(&m_darkModeManager),
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

	if (m_commandLineSettings->shellChangeNotificationType)
	{
		m_config.shellChangeNotificationType = *m_commandLineSettings->shellChangeNotificationType;
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

	const auto saveFrequency = 30s;
	m_saveSettingsTimer = m_runtime.GetTimerQueue()->make_timer(saveFrequency, saveFrequency,
		m_runtime.GetUiThreadExecutor(), std::bind_front(&App::SaveSettings, this));

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

	m_iconResourceLoader =
		std::make_unique<IconResourceLoader>(m_config.iconSet, &m_darkModeManager);
	SetUpLanguageResourceInstance();

	RestoreSession(windows);
}

void App::LoadSettings(std::vector<WindowStorageData> &windows)
{
	// Settings will be loaded from the config file by default, if that file is present and can be
	// read.
	std::unique_ptr<AppStorage> appStorage = XmlAppStorageFactory::MaybeCreate(
		Storage::GetConfigFilePath(), Storage::OperationType::Load);

	if (appStorage)
	{
		m_savePreferencesToXmlFile = true;
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

	ValidateColumns(m_config.globalFolderSettings.folderColumns);
}

void App::SaveSettings()
{
	// If the application has started exiting, it's not possible to save the settings, so that's not
	// something that should be attempted. That's because one or more of the windows may have
	// already been closed.
	CHECK(!m_exitStarted);

	std::unique_ptr<AppStorage> appStorage;

	if (m_savePreferencesToXmlFile)
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
			MessageBox(nullptr, versionMismatchMessage.c_str(), App::APP_NAME, MB_ICONWARNING);
		}

		languageInfo = { LanguageHelper::DEFAULT_LANGUAGE, GetModuleHandle(nullptr) };
	}

	m_config.language = languageInfo.language;
	m_resourceInstance = languageInfo.resourceInstance;

	if (LanguageHelper::IsLanguageRTL(m_config.language))
	{
		SetProcessDefaultLayout(LAYOUT_RTL);
	}

	ResourceManager::Initialize(std::make_unique<Win32ResourceLoader>(m_resourceInstance));
}

void App::RestoreSession(const std::vector<WindowStorageData> &windows)
{
	// At the moment, only a single window is supported.
	for (const auto &window : windows)
	{
		Explorerplusplus::Create(this, &window);

		// If this feature isn't enabled, only a single window is supported.
		if (!m_featureList.IsEnabled(Feature::MultipleWindowsPerSession))
		{
			break;
		}
	}

	if (m_browserList.IsEmpty())
	{
		// No windows were loaded from the previous session, so create the default window.
		Explorerplusplus::Create(this);
	}
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

const CommandLine::Settings *App::GetCommandLineSettings() const
{
	return m_commandLineSettings;
}

bool App::GetSavePreferencesToXmlFile() const
{
	return m_savePreferencesToXmlFile;
}

void App::SetSavePreferencesToXmlFile(bool savePreferencesToXmlFile)
{
	m_savePreferencesToXmlFile = savePreferencesToXmlFile;
}

FeatureList *App::GetFeatureList()
{
	return &m_featureList;
}

AcceleratorManager *App::GetAcceleratorManager()
{
	return &m_acceleratorManager;
}

Config *App::GetConfig()
{
	return &m_config;
}

CachedIcons *App::GetCachedIcons()
{
	return &m_cachedIcons;
}

BrowserList *App::GetBrowserList()
{
	return &m_browserList;
}

ModelessDialogList *App::GetModelessDialogList()
{
	return &m_modelessDialogList;
}

BookmarkTree *App::GetBookmarkTree()
{
	return &m_bookmarkTree;
}

ColorRuleModel *App::GetColorRuleModel() const
{
	return m_colorRuleModel.get();
}

Applications::ApplicationModel *App::GetApplicationModel()
{
	return &m_applicationModel;
}

IconResourceLoader *App::GetIconResourceLoader() const
{
	return m_iconResourceLoader.get();
}

HINSTANCE App::GetResourceInstance() const
{
	return m_resourceInstance;
}

GlobalTabEventDispatcher *App::GetGlobalTabEventDispatcher()
{
	return &m_globalTabEventDispatcher;
}

TabRestorer *App::GetTabRestorer()
{
	return &m_tabRestorer;
}

DarkModeManager *App::GetDarkModeManager()
{
	return &m_darkModeManager;
}

ThemeManager *App::GetThemeManager()
{
	return &m_themeManager;
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

	std::wstring message = fmt::format(fmt::runtime(Resources::LoadString(IDS_CLOSE_ALL_WINDOWS)),
		fmt::arg(L"num_windows", numWindows));
	int response =
		MessageBox(browser->GetHWND(), message.c_str(), APP_NAME, MB_ICONINFORMATION | MB_YESNO);

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

void App::SessionEnding()
{
	if (m_exitStarted)
	{
		// The application has already started exiting, so there's no need to try and save the
		// settings, since it will have already been done.
		return;
	}

	SaveSettings();
}
