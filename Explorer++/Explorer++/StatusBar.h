// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainFontSetter.h"
#include <boost/signals2.hpp>
#include <memory>
#include <string>
#include <vector>

class BrowserWindow;
struct Config;
class NavigationEvents;
class NavigationRequest;
class ResourceLoader;
class ShellBrowser;
class ShellBrowserEvents;
class Tab;
class TabEvents;

class StatusBar
{
public:
	static StatusBar *Create(HWND parent, const BrowserWindow *browser, const Config *config,
		TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
		NavigationEvents *navigationEvents, const ResourceLoader *resourceLoader);

	HWND GetHWND() const;
	void OnMenuSelect(HMENU menu, UINT itemId, UINT flags);

private:
	StatusBar(HWND parent, const BrowserWindow *browser, const Config *config, TabEvents *tabEvents,
		ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
		const ResourceLoader *resourceLoader);

	void SetParts();

	// This function should be called whenever the font is updated for the status bar control.
	void UpdateMinHeight();

	LRESULT StatusBarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

	void OnNcDestroy();

	const HWND m_hwnd;
	const BrowserWindow *const m_browser;
	const Config *const m_config;
	MainFontSetter m_fontSetter;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
};
