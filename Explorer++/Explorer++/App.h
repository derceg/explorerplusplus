// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorManager.h"
#include "ApplicationModel.h"
#include "BrowserList.h"
#include "CommandLine.h"
#include "Config.h"
#include "FeatureList.h"
#include "ModelessDialogList.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/UniqueResources.h"
#include <boost/core/noncopyable.hpp>
#include <wil/resource.h>

class ColorRuleModel;

class App : private boost::noncopyable
{
public:
	App(const CommandLine::Settings *commandLineSettings);
	~App();

	const CommandLine::Settings *GetCommandLineSettings() const;
	FeatureList *GetFeatureList();
	AcceleratorManager *GetAcceleratorManager();
	Config *GetConfig();
	CachedIcons *GetCachedIcons();
	BrowserList *GetBrowserList();
	ModelessDialogList *GetModelessDialogList();
	ColorRuleModel *GetColorRuleModel() const;
	Applications::ApplicationModel *GetApplicationModel();

private:
	// Represents the maximum number of icons that can be cached. This cache is shared between
	// various components in the application.
	static constexpr int MAX_CACHED_ICONS = 1000;

	void Initialize();
	void OnBrowserRemoved();
	void LoadSettings();

	const CommandLine::Settings *const m_commandLineSettings;
	FeatureList m_featureList;
	AcceleratorManager m_acceleratorManager;
	Config m_config;
	CachedIcons m_cachedIcons;
	BrowserList m_browserList;
	ModelessDialogList m_modelessDialogList;
	std::unique_ptr<ColorRuleModel> m_colorRuleModel;
	Applications::ApplicationModel m_applicationModel;

	unique_gdiplus_shutdown m_uniqueGdiplusShutdown;
	wil::unique_hmodule m_richEditLib;
	wil::unique_oleuninitialize_call m_oleCleanup;
};
