// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowserFactory.h"

class AppServices;
class BrowserWindow;

class ShellBrowserFactoryImpl : public ShellBrowserFactory
{
public:
	ShellBrowserFactoryImpl(BrowserWindow *browser, AppServices *appServices,
		HINSTANCE resourceInstance);

	// ShellBrowserFactory
	std::unique_ptr<ShellBrowser> Create(const PidlAbsolute &initialPidl,
		const FolderSettings &folderSettings, const FolderColumns *initialColumns) override;
	std::unique_ptr<ShellBrowser> CreateFromPreserved(
		const PreservedShellBrowser &preservedShellBrowser) override;

private:
	BrowserWindow *const m_browser;
	AppServices *const m_appServices;
	const HINSTANCE m_resourceInstance;
};
