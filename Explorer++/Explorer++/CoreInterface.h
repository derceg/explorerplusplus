// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconResourceLoader.h"
#include "../Helper/StatusBar.h"
#include <boost/signals2.hpp>

typedef boost::signals2::signal<void()> TabsInitializedSignal;
typedef boost::signals2::signal<void(HMENU mainMenu)> MainMenuPreShowSignal;
typedef boost::signals2::signal<void(HMENU menu, HWND sourceWindow)> ToolbarContextMenuSignal;

enum MousewheelSource_t
{
	MOUSEWHEEL_SOURCE_LISTVIEW,
	MOUSEWHEEL_SOURCE_TREEVIEW,
	MOUSEWHEEL_SOURCE_OTHER
};

class CachedIcons;
struct Config;
class CShellBrowser;
__interface IDirectoryMonitor;
class TabContainer;

/* Basic interface between Explorerplusplus
and some of the other components (such as the
dialogs and toolbars). */
__interface IExplorerplusplus
{
	Config			*GetConfig() const;
	HMODULE			GetLanguageModule() const;

	HWND			GetMainWindow() const;

	HWND			GetActiveListView() const;
	CShellBrowser	*GetActiveShellBrowser() const;

	TabContainer	*GetTabContainer() const;
	IDirectoryMonitor	*GetDirectoryMonitor() const;

	IconResourceLoader	*GetIconResourceLoader() const;
	CachedIcons		*GetCachedIcons();

	HWND			GetTreeView() const;

	void			OpenItem(const TCHAR *szItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow);
	void			OpenItem(PCIDLIST_ABSOLUTE pidlItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow);

	CStatusBar		*GetStatusBar();

	void			OpenFileItem(PCIDLIST_ABSOLUTE pidlItem, const TCHAR *szParameters);

	HMENU			BuildViewsMenu();

	bool			CanCreate() const;
	BOOL			CanCut() const;
	BOOL			CanCopy() const;
	BOOL			CanRename() const;
	BOOL			CanDelete() const;
	BOOL			CanShowFileProperties() const;
	BOOL			CanPaste() const;

	BOOL			OnMouseWheel(MousewheelSource_t MousewheelSource, WPARAM wParam, LPARAM lParam);

	void			ShowTabBar();
	void			HideTabBar();

	void			SetListViewInitialPosition(HWND hListView);

	// Used to support the options dialog.
	void			SaveAllSettings();
	BOOL			GetSavePreferencesToXmlFile() const;
	void			SetSavePreferencesToXmlFile(BOOL savePreferencesToXmlFile);

	boost::signals2::connection	AddTabsInitializedObserver(const TabsInitializedSignal::slot_type &observer);
	boost::signals2::connection	AddMainMenuPreShowObserver(const MainMenuPreShowSignal::slot_type &observer);
	boost::signals2::connection	AddToolbarContextMenuObserver(const ToolbarContextMenuSignal::slot_type &observer);
};