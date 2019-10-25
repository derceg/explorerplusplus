// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"
#include "MenuHelper.h"
#include "PluginManager.h"
#include "../Helper/ProcessHelper.h"
#include <boost/filesystem.hpp>

static const std::wstring PLUGIN_FOLDER_NAME = L"plugins";

void Explorerplusplus::InitializePlugins()
{
	if (!g_enablePlugins)
	{
		return;
	}

	TCHAR processImageName[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), processImageName, SIZEOF_ARRAY(processImageName));

	boost::filesystem::path processDirectoryPath(processImageName);
	processDirectoryPath.remove_filename();
	processDirectoryPath.append(PLUGIN_FOLDER_NAME);

	m_pluginManager = std::make_unique<Plugins::PluginManager>(this);
	m_pluginManager->loadAllPlugins(processDirectoryPath);

	UpdateMenuAcceleratorStrings(GetMenu(m_hContainer), g_hAccl);
}