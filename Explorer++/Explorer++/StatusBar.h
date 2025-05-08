// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <string>
#include <vector>

class BrowserWindow;
struct Config;
class NavigationEvents;
class NavigationRequest;
class ResourceLoader;
class ShellBrowser;
class ShellBrowserEvents;
class StatusBarView;
class Tab;
class TabEvents;

class StatusBar
{
public:
	static StatusBar *Create(StatusBarView *view, const BrowserWindow *browser,
		const Config *config, TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
		NavigationEvents *navigationEvents, const ResourceLoader *resourceLoader);

	StatusBarView *GetView();
	void OnMenuSelect(HMENU menu, UINT itemId, UINT flags);

private:
	StatusBar(StatusBarView *view, const BrowserWindow *browser, const Config *config,
		TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
		NavigationEvents *navigationEvents, const ResourceLoader *resourceLoader);

	void SetStandardParts();

	void OnTabSelected(const Tab &tab);
	void OnDirectoryContentsChanged(const ShellBrowser *shellBrowser);
	void OnListViewSelectionChanged(const ShellBrowser *shellBrowser);
	void UpdateTextForNavigation(const NavigationRequest *request);
	void OnNavigationsStopped(const ShellBrowser *shellBrowser);

	void OnMenuClose();
	void OnMenuItemSelected(HMENU menu, UINT itemId, UINT flags);

	void UpdateText(const Tab &tab);
	void SetLoadingText(PCIDLIST_ABSOLUTE pidl);
	std::wstring CreateDriveFreeSpaceString(const std::wstring &path);

	void OnWindowDestroyed();

	StatusBarView *const m_view;
	const BrowserWindow *const m_browser;
	const Config *const m_config;
	const ResourceLoader *const m_resourceLoader;
	bool m_showingMenuHelpText = false;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
