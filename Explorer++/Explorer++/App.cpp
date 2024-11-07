// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "App.h"
#include "Bookmarks/BookmarkTreeFactory.h"
#include "ColorRuleModel.h"
#include "ColorRuleModelFactory.h"
#include "DefaultAccelerators.h"
#include "ExitCode.h"
#include "Explorer++_internal.h"
#include "RegistryAppStorage.h"
#include "RegistryAppStorageFactory.h"
#include "XmlAppStorage.h"
#include "XmlAppStorageFactory.h"
#include "../Helper/Helper.h"

App::App(const CommandLine::Settings *commandLineSettings) :
	m_commandLineSettings(commandLineSettings),
	m_featureList(commandLineSettings->featuresToEnable),
	m_acceleratorManager(InitializeAcceleratorManager()),
	m_cachedIcons(MAX_CACHED_ICONS),
	m_colorRuleModel(ColorRuleModelFactory::Create()),
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

	LoadSettings();
}

void App::OnBrowserRemoved()
{
	if (m_browserList.IsEmpty())
	{
		// The last top-level browser window has been closed, so exit the application.
		PostQuitMessage(EXIT_CODE_NORMAL);
	}
}

void App::LoadSettings()
{
	BOOL loadSettingsFromXML = TestConfigFileInternal();
	std::unique_ptr<AppStorage> appStorage;

	if (loadSettingsFromXML)
	{
		appStorage = XmlAppStorageFactory::MaybeCreate();
	}
	else
	{
		appStorage = RegistryAppStorageFactory::MaybeCreate();
	}

	if (!appStorage)
	{
		return;
	}

	appStorage->LoadBookmarks(BookmarkTreeFactory::GetInstance()->GetBookmarkTree());
	appStorage->LoadColorRules(m_colorRuleModel.get());
	appStorage->LoadApplications(&m_applicationModel);
	appStorage->LoadDialogStates();
}

const CommandLine::Settings *App::GetCommandLineSettings() const
{
	return m_commandLineSettings;
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

ColorRuleModel *App::GetColorRuleModel() const
{
	return m_colorRuleModel.get();
}

Applications::ApplicationModel *App::GetApplicationModel()
{
	return &m_applicationModel;
}
