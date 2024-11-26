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
#include "FeatureList.h"
#include "ModelessDialogList.h"
#include "ProcessManager.h"
#include "Runtime.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/UniqueResources.h"
#include <boost/core/noncopyable.hpp>
#include <wil/resource.h>
#include <vector>

class ColorRuleModel;
struct WindowStorageData;

class App : private boost::noncopyable
{
public:
	App(const CommandLine::Settings *commandLineSettings);
	~App();

	int Run();

	const CommandLine::Settings *GetCommandLineSettings() const;
	bool GetSavePreferencesToXmlFile() const;
	void SetSavePreferencesToXmlFile(bool savePreferencesToXmlFile);
	FeatureList *GetFeatureList();
	AcceleratorManager *GetAcceleratorManager();
	Config *GetConfig();
	CachedIcons *GetCachedIcons();
	BrowserList *GetBrowserList();
	ModelessDialogList *GetModelessDialogList();
	BookmarkTree *GetBookmarkTree();
	ColorRuleModel *GetColorRuleModel() const;
	Applications::ApplicationModel *GetApplicationModel();

	void TryExit();
	void SessionEnding();

private:
	// Represents the maximum number of icons that can be cached. This cache is shared between
	// various components in the application.
	static constexpr int MAX_CACHED_ICONS = 1000;

	void OnBrowserRemoved();
	void SetUpSession();
	void LoadSettings(std::vector<WindowStorageData> &windows);
	void SaveSettings();
	void RestoreSession(const std::vector<WindowStorageData> &windows);
	bool IsModelessDialogMessage(MSG *msg);
	bool MaybeTranslateAccelerator(MSG *msg);

	void OnWillRemoveBrowser();
	bool ConfirmExit();
	void Exit();
	void OnExitStarted();

	const CommandLine::Settings *const m_commandLineSettings;
	bool m_savePreferencesToXmlFile = false;
	Runtime m_runtime;
	FeatureList m_featureList;
	AcceleratorManager m_acceleratorManager;
	Config m_config;
	CachedIcons m_cachedIcons;
	BrowserList m_browserList;
	ModelessDialogList m_modelessDialogList;
	BookmarkTree m_bookmarkTree;
	std::unique_ptr<ColorRuleModel> m_colorRuleModel;
	Applications::ApplicationModel m_applicationModel;
	ProcessManager m_processManager;

	concurrencpp::timer m_saveSettingsTimer;

	unique_gdiplus_shutdown m_uniqueGdiplusShutdown;
	wil::unique_hmodule m_richEditLib;
	wil::unique_oleuninitialize_call m_oleCleanup;

	bool m_exitStarted = false;
};
