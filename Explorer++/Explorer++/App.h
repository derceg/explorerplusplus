// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "AcceleratorManager.h"
#include "CommandLine.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/UniqueResources.h"
#include <boost/core/noncopyable.hpp>

class App : private boost::noncopyable
{
public:
	App(const CommandLine::Settings *commandLineSettings);

	const CommandLine::Settings *GetCommandLineSettings() const;
	AcceleratorManager *GetAcceleratorManager();
	Config *GetConfig();
	CachedIcons *GetCachedIcons();

private:
	// Represents the maximum number of icons that can be cached. This cache is shared between
	// various components in the application.
	static constexpr int MAX_CACHED_ICONS = 1000;

	const CommandLine::Settings *const m_commandLineSettings;
	AcceleratorManager m_acceleratorManager;
	Config m_config;
	CachedIcons m_cachedIcons;

	unique_gdiplus_shutdown m_uniqueGdiplusShutdown;
};
