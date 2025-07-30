// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ShellBrowserFactory.h"

class BrowserWindow;
class NavigationEvents;

class ShellBrowserFactoryFake : public ShellBrowserFactory
{
public:
	ShellBrowserFactoryFake(BrowserWindow *browser, NavigationEvents *navigationEvents);

	std::unique_ptr<ShellBrowser> Create(const PidlAbsolute &initialPidl,
		const FolderSettings &folderSettings, const FolderColumns *initialColumns) override;
	std::unique_ptr<ShellBrowser> CreateFromPreserved(
		const PreservedShellBrowser &preservedShellBrowser) override;

private:
	BrowserWindow *const m_browser;
	NavigationEvents *const m_navigationEvents;
};
