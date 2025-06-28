// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowserFactory.h"

class App;
class BrowserWindow;
class FileActionHandler;

class ShellBrowserFactoryImpl : public ShellBrowserFactory
{
public:
	ShellBrowserFactoryImpl(App *app, BrowserWindow *browser, FileActionHandler *fileActionHandler);

	// ShellBrowserFactory
	std::unique_ptr<ShellBrowser> Create(const PidlAbsolute &initialPidl,
		const FolderSettings &folderSettings, const FolderColumns *initialColumns) override;
	std::unique_ptr<ShellBrowser> CreateFromPreserved(
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
		const PreservedFolderState &preservedFolderState) override;

private:
	App *const m_app;
	BrowserWindow *const m_browser;
	FileActionHandler *const m_fileActionHandler;
};
