// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AcceleratorHelper.h"
#include "Explorer++_internal.h"
#include "FeatureList.h"
#include "Plugins/PluginManager.h"
#include "../Helper/ProcessHelper.h"
#include <filesystem>

static const std::wstring PLUGIN_FOLDER_NAME = L"plugins";

void Explorerplusplus::InitializePlugins()
{
	if (!FeatureList::GetInstance()->IsEnabled(Feature::Plugins))
	{
		return;
	}

	TCHAR processImageName[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), processImageName, SIZEOF_ARRAY(processImageName));

	std::filesystem::path processDirectoryPath(processImageName);
	processDirectoryPath.remove_filename();
	processDirectoryPath.append(PLUGIN_FOLDER_NAME);

	m_pluginManager = std::make_unique<Plugins::PluginManager>(this);
	m_pluginManager->loadAllPlugins(processDirectoryPath);

	UpdateMenuAcceleratorStrings(GetMenu(m_hContainer), m_acceleratorManager);
}
