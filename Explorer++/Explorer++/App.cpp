// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "App.h"
#include "Explorer++.h"
#include "ColorRuleModel.h"
#include "ColorRuleModelFactory.h"
#include "ColumnStorage.h"
#include "DefaultAccelerators.h"
#include "ExitCode.h"
#include "Explorer++_internal.h"
#include "MainRebarStorage.h"
#include "RegistryAppStorage.h"
#include "RegistryAppStorageFactory.h"
#include "TabStorage.h"
#include "WindowStorage.h"
#include "XmlAppStorage.h"
#include "XmlAppStorageFactory.h"
#include "../Helper/Helper.h"
#include <ranges>

App::App(const CommandLine::Settings *commandLineSettings) :
	m_commandLineSettings(commandLineSettings),
	m_featureList(commandLineSettings->featuresToEnable),
	m_acceleratorManager(InitializeAcceleratorManager()),
	m_cachedIcons(MAX_CACHED_ICONS),
	m_colorRuleModel(ColorRuleModelFactory::Create()),
	m_processManager(&m_browserList),
	m_uniqueGdiplusShutdown(CheckedGdiplusStartup()),
	m_richEditLib(LoadSystemLibrary(
		L"Msftedit.dll")), // This is needed for version 5 of the Rich Edit control.
	m_oleCleanup(wil::OleInitialize_failfast())
{
	CHECK(m_richEditLib);

	Initialize();
}

App::~App() = default;

void App::Initialize()
{
	INITCOMMONCONTROLSEX commonControls = {};
	commonControls.dwSize = sizeof(commonControls);
	commonControls.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_LISTVIEW_CLASSES
		| ICC_USEREX_CLASSES | ICC_STANDARD_CLASSES | ICC_LINK_CLASS;
	BOOL res = InitCommonControlsEx(&commonControls);
	CHECK(res);

	m_browserList.browserRemovedSignal.AddObserver(std::bind_front(&App::OnBrowserRemoved, this));

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

	RestoreSession(windows);
}

void App::OnBrowserRemoved()
{
	if (m_browserList.IsEmpty())
	{
		// The last top-level browser window has been closed, so exit the application.
		PostQuitMessage(EXIT_CODE_NORMAL);
	}
}

void App::LoadSettings(std::vector<WindowStorageData> &windows)
{
	BOOL loadSettingsFromXML = TestConfigFile();
	std::unique_ptr<AppStorage> appStorage;

	if (loadSettingsFromXML)
	{
		appStorage = XmlAppStorageFactory::MaybeCreate();

		m_savePreferencesToXmlFile = true;
	}
	else
	{
		appStorage = RegistryAppStorageFactory::MaybeCreate();
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

void App::RestoreSession(const std::vector<WindowStorageData> &windows)
{
	// At the moment, only a single window is supported.
	for (const auto &window : windows | std::views::take(1))
	{
		Explorerplusplus::Create(this, &window);
	}

	if (m_browserList.IsEmpty())
	{
		// No windows were loaded from the previous session, so create the default window.
		Explorerplusplus::Create(this);
	}
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
