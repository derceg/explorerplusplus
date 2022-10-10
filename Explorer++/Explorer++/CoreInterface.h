// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>

enum class MousewheelSource
{
	ListView,
	TreeView,
	Other
};

enum class OpenFolderDisposition
{
	CurrentTab,
	BackgroundTab,
	ForegroundTab,
	NewWindow
};

enum class WindowFocusSource
{
	AddressBar,
	TreeView,
	ListView
};

using TabsInitializedSignal = boost::signals2::signal<void()>;
using MainMenuPreShowSignal = boost::signals2::signal<void(HMENU mainMenu)>;
using ToolbarContextMenuSignal =
	boost::signals2::signal<void(HMENU menu, HWND sourceWindow, const POINT &pt)>;
using ToolbarContextMenuSelectedSignal =
	boost::signals2::signal<void(HWND sourceWindow, int menuItemId)>;
using FocusChangedSignal = boost::signals2::signal<void(WindowFocusSource windowFocusSource)>;
using ApplicationShuttingDownSignal = boost::signals2::signal<void()>;

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
class CoreInterface
{
public:
	virtual ~CoreInterface() = default;

	virtual const Config *GetConfig() const = 0;
	virtual HMODULE GetResourceModule() const = 0;
	virtual HACCEL *GetAcceleratorTable() const = 0;

	virtual HWND GetMainWindow() const = 0;

	virtual HWND GetActiveListView() const = 0;
	virtual ShellBrowser *GetActiveShellBrowser() const = 0;

	virtual TabContainer *GetTabContainer() const = 0;
	virtual TabRestorer *GetTabRestorer() const = 0;
	virtual IDirectoryMonitor *GetDirectoryMonitor() const = 0;

	virtual IconResourceLoader *GetIconResourceLoader() const = 0;
	virtual CachedIcons *GetCachedIcons() = 0;

	virtual HWND GetTreeView() const = 0;

	virtual void OpenItem(const std::wstring &itemPath,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab) = 0;
	virtual void OpenItem(PCIDLIST_ABSOLUTE pidlItem,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab) = 0;
	virtual OpenFolderDisposition DetermineOpenDisposition(bool isMiddleButtonDown,
		bool isCtrlKeyDown, bool isShiftKeyDown) = 0;

	virtual StatusBar *GetStatusBar() = 0;

	virtual void OpenFileItem(PCIDLIST_ABSOLUTE pidlItem, const TCHAR *szParameters) = 0;

	virtual wil::unique_hmenu BuildViewsMenu() = 0;

	virtual bool CanCreate() const = 0;
	virtual BOOL CanCut() const = 0;
	virtual BOOL CanCopy() const = 0;
	virtual BOOL CanRename() const = 0;
	virtual BOOL CanDelete() const = 0;
	virtual BOOL CanShowFileProperties() const = 0;
	virtual BOOL CanPaste() const = 0;

	virtual BOOL OnMouseWheel(MousewheelSource mousewheelSource, WPARAM wParam, LPARAM lParam) = 0;

	virtual void ShowTabBar() = 0;
	virtual void HideTabBar() = 0;

	virtual void SetListViewInitialPosition(HWND hListView) = 0;

	virtual void FocusChanged(WindowFocusSource windowFocusSource) = 0;
	virtual void FocusActiveTab() = 0;

	// Used to support the options dialog.
	virtual void SaveAllSettings() = 0;
	virtual BOOL GetSavePreferencesToXmlFile() const = 0;
	virtual void SetSavePreferencesToXmlFile(BOOL savePreferencesToXmlFile) = 0;

	virtual int CloseApplication() = 0;

	virtual boost::signals2::connection AddTabsInitializedObserver(
		const TabsInitializedSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddMainMenuPreShowObserver(
		const MainMenuPreShowSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddToolbarContextMenuObserver(
		const ToolbarContextMenuSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddToolbarContextMenuSelectedObserver(
		const ToolbarContextMenuSelectedSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddFocusChangeObserver(
		const FocusChangedSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddApplicationShuttingDownObserver(
		const ApplicationShuttingDownSignal::slot_type &observer) = 0;
};
