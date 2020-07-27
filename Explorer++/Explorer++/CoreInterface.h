// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>

using TabsInitializedSignal = boost::signals2::signal<void()>;
using MainMenuPreShowSignal = boost::signals2::signal<void(HMENU mainMenu)>;
using ToolbarContextMenuSignal =
	boost::signals2::signal<void(HMENU menu, HWND sourceWindow, const POINT &pt)>;
using ApplicationShuttingDownSignal = boost::signals2::signal<void()>;

enum class MousewheelSource
{
	ListView,
	TreeView,
	Other
};

class CachedIcons;
struct Config;
class IconResourceLoader;
__interface IDirectoryMonitor;
class ShellBrowser;
class StatusBar;
class TabContainer;
class TabRestorer;

/* Basic interface between Explorerplusplus
and some of the other components (such as the
dialogs and toolbars). */
__interface IExplorerplusplus
{
	const Config *GetConfig() const;
	HMODULE GetLanguageModule() const;

	HWND GetMainWindow() const;

	HWND GetActiveListView() const;
	ShellBrowser *GetActiveShellBrowser() const;

	TabContainer *GetTabContainer() const;
	TabRestorer *GetTabRestorer() const;
	IDirectoryMonitor *GetDirectoryMonitor() const;

	IconResourceLoader *GetIconResourceLoader() const;
	CachedIcons *GetCachedIcons();

	HWND GetTreeView() const;

	void OpenItem(const TCHAR *szItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow);
	void OpenItem(PCIDLIST_ABSOLUTE pidlItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow);

	StatusBar *GetStatusBar();

	void OpenFileItem(PCIDLIST_ABSOLUTE pidlItem, const TCHAR *szParameters);

	HMENU BuildViewsMenu();

	bool CanCreate() const;
	BOOL CanCut() const;
	BOOL CanCopy() const;
	BOOL CanRename() const;
	BOOL CanDelete() const;
	BOOL CanShowFileProperties() const;
	BOOL CanPaste() const;

	BOOL OnMouseWheel(MousewheelSource mousewheelSource, WPARAM wParam, LPARAM lParam);

	void ShowTabBar();
	void HideTabBar();

	void SetListViewInitialPosition(HWND hListView);

	// Used to support the options dialog.
	void SaveAllSettings();
	BOOL GetSavePreferencesToXmlFile() const;
	void SetSavePreferencesToXmlFile(BOOL savePreferencesToXmlFile);

	boost::signals2::connection AddTabsInitializedObserver(
		const TabsInitializedSignal::slot_type &observer);
	boost::signals2::connection AddMainMenuPreShowObserver(
		const MainMenuPreShowSignal::slot_type &observer);
	boost::signals2::connection AddToolbarContextMenuObserver(
		const ToolbarContextMenuSignal::slot_type &observer);
	boost::signals2::connection AddApplicationShuttingDownObserver(
		const ApplicationShuttingDownSignal::slot_type &observer);
};