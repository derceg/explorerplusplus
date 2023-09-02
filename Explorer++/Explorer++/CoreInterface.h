// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>

// Stops signal propagation after the first successful handler (i.e. the first handler that returns
// a result that evaluates to true).
template <typename T>
struct FirstSuccessfulRequestCombiner
{
	typedef T result_type;

	template <typename InputIterator>
	T operator()(InputIterator first, InputIterator last) const
	{
		while (first != last)
		{
			if (T fullfilled = *first)
			{
				return fullfilled;
			}

			++first;
		}

		return T();
	}
};

using TabsInitializedSignal = boost::signals2::signal<void()>;
using MainMenuPreShowSignal = boost::signals2::signal<void(HMENU mainMenu)>;
using MainMenuItemRightClickedSignal =
	boost::signals2::signal<bool(HMENU menu, int index, const POINT &pt),
		FirstSuccessfulRequestCombiner<bool>>;
using MainMenuItemMiddleClickedSignal =
	boost::signals2::signal<bool(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown),
		FirstSuccessfulRequestCombiner<bool>>;
using GetMenuItemHelperTextSignal =
	boost::signals2::signal<std::optional<std::wstring>(HMENU menu, int id),
		FirstSuccessfulRequestCombiner<std::optional<std::wstring>>>;
using ToolbarContextMenuSignal =
	boost::signals2::signal<void(HMENU menu, HWND sourceWindow, const POINT &pt)>;
using ToolbarContextMenuSelectedSignal =
	boost::signals2::signal<void(HWND sourceWindow, int menuItemId)>;
using FocusChangedSignal = boost::signals2::signal<void()>;
using DeviceChangeSignal = boost::signals2::signal<void(UINT eventType, LONG_PTR eventData)>;
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
	virtual HINSTANCE GetResourceInstance() const = 0;
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

	virtual void ShowTabBar() = 0;
	virtual void HideTabBar() = 0;

	virtual void SetListViewInitialPosition(HWND hListView) = 0;

	virtual void FocusChanged() = 0;
	virtual void FocusActiveTab() = 0;

	// Used to support the options dialog.
	virtual void SaveAllSettings() = 0;
	virtual BOOL GetSavePreferencesToXmlFile() const = 0;
	virtual void SetSavePreferencesToXmlFile(BOOL savePreferencesToXmlFile) = 0;

	virtual void RequestCloseApplication() = 0;

	virtual boost::signals2::connection AddTabsInitializedObserver(
		const TabsInitializedSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddMainMenuPreShowObserver(
		const MainMenuPreShowSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddMainMenuItemMiddleClickedObserver(
		const MainMenuItemMiddleClickedSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddMainMenuItemRightClickedObserver(
		const MainMenuItemRightClickedSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddGetMenuItemHelperTextObserver(
		const GetMenuItemHelperTextSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddToolbarContextMenuObserver(
		const ToolbarContextMenuSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddToolbarContextMenuSelectedObserver(
		const ToolbarContextMenuSelectedSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddFocusChangeObserver(
		const FocusChangedSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddDeviceChangeObserver(
		const DeviceChangeSignal::slot_type &observer) = 0;
	virtual boost::signals2::connection AddApplicationShuttingDownObserver(
		const ApplicationShuttingDownSignal::slot_type &observer) = 0;
};
