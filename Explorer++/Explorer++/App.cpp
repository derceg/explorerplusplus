// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "App.h"
#include "DefaultAccelerators.h"

App::App(const CommandLine::Settings *commandLineSettings) :
	m_commandLineSettings(commandLineSettings),
	m_acceleratorManager(InitializeAcceleratorManager()),
	m_cachedIcons(MAX_CACHED_ICONS)
{
}

const CommandLine::Settings *App::GetCommandLineSettings() const
{
	return m_commandLineSettings;
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
