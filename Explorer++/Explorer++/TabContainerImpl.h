// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconFetcherImpl.h"
#include "OneShotTimer.h"
#include "OneShotTimerManager.h"
#include "ShellBrowser/FolderSettings.h"
#include "Tab.h"
#include "TabContainer.h"
#include "TabView.h"
#include "TabViewDelegate.h"
#include "../Helper/ShellDropTargetWindow.h"
#include "../Helper/WindowSubclass.h"
#include <boost/parameter.hpp>
#include <wil/resource.h>
#include <functional>
#include <optional>
#include <unordered_map>

class App;
class BookmarkTree;
class BrowserWindow;
class CachedIcons;
struct Config;
class CoreInterface;
class FileActionHandler;
class MainTabView;
struct NavigateParams;
class NavigationRequest;
struct PreservedTab;
class ShellBrowser;
class TabNavigationInterface;

BOOST_PARAMETER_NAME(name)
BOOST_PARAMETER_NAME(index)
BOOST_PARAMETER_NAME(selected)
BOOST_PARAMETER_NAME(lockState)

// The use of Boost Parameter here allows values to be set by name
// during construction. It would be better (and simpler) for this to be
// done using designated initializers, but that feature's not due to be
// introduced until C++20.
struct TabSettingsImpl
{
	template <class ArgumentPack>
	TabSettingsImpl(const ArgumentPack &args)
	{
		name = args[_name | std::nullopt];
		lockState = args[_lockState | std::nullopt];
		index = args[_index | std::nullopt];
		selected = args[_selected | std::nullopt];
	}

	std::optional<std::wstring> name;
	std::optional<Tab::LockState> lockState;
	std::optional<int> index;
	std::optional<bool> selected;

	// This is only used in tests.
	bool operator==(const TabSettingsImpl &) const = default;
};

// Used when creating a tab.
struct TabSettings : TabSettingsImpl
{
	// clang-format off
	BOOST_PARAMETER_CONSTRUCTOR(
		TabSettings,
		(TabSettingsImpl),
		tag,
		(optional
			(name, (std::wstring))
			(lockState, (Tab::LockState))
			(index, (int))
			(selected, (bool))
		)
	)
	// clang-format on
};

class TabContainerImpl :
	public TabContainer,
	public ShellDropTargetWindow<int>,
	private TabViewDelegate
{
public:
	static TabContainerImpl *Create(MainTabView *view, BrowserWindow *browser,
		TabNavigationInterface *tabNavigation, App *app, CoreInterface *coreInterface,
		FileActionHandler *fileActionHandler, CachedIcons *cachedIcons, BookmarkTree *bookmarkTree,
		HINSTANCE resourceInstance, const Config *config);

	void CreateNewTabInDefaultDirectory(const TabSettings &tabSettings);
	Tab &CreateNewTab(const std::wstring &directory, const TabSettings &tabSettings = {},
		const FolderSettings *folderSettings = nullptr,
		const FolderColumns *initialColumns = nullptr);
	Tab &CreateNewTab(const PreservedTab &preservedTab);
	Tab &CreateNewTab(NavigateParams &navigateParams, const TabSettings &tabSettings = {},
		const FolderSettings *folderSettings = nullptr,
		const FolderColumns *initialColumns = nullptr);

	Tab &GetTab(int tabId) const;
	Tab *GetTabOptional(int tabId) const;
	void SelectTab(const Tab &tab) override;
	void SelectAdjacentTab(BOOL bNextTab);
	void SelectTabAtIndex(int index);
	Tab &GetSelectedTab() const;
	int GetSelectedTabIndex() const;
	std::optional<int> GetSelectedTabIndexOptional() const;
	bool IsTabSelected(const Tab &tab) const override;
	Tab &GetTabByIndex(int index) const;
	int GetTabIndex(const Tab &tab) const;
	int GetNumTabs() const;
	int MoveTab(const Tab &tab, int newIndex);
	void DuplicateTab(const Tab &tab);
	bool CloseTab(const Tab &tab);

	// Eventually, this should be removed.
	std::unordered_map<int, std::unique_ptr<Tab>> &GetTabs();

	/* TODO: Ideally, there would be a method of iterating over the tabs without
	having access to the underlying container. */
	const std::unordered_map<int, std::unique_ptr<Tab>> &GetAllTabs() const;
	std::vector<std::reference_wrapper<const Tab>> GetAllTabsInOrder() const;

	std::vector<TabStorageData> GetStorageData() const;

private:
	// Contains data used when an item is dragged over this window.
	struct DropTargetContext
	{
		int targetIndex = -1;
		OneShotTimer switchTabTimer;
		OneShotTimer scrollTimer;
		std::optional<TabView::ScrollDirection> scrollDirection;

		DropTargetContext(OneShotTimerManager *timerManager) :
			switchTabTimer(timerManager),
			scrollTimer(timerManager)
		{
		}
	};

	static const LONG DROP_SCROLL_MARGIN_X_96DPI = 40;

	TabContainerImpl(MainTabView *view, BrowserWindow *browser,
		TabNavigationInterface *tabNavigation, App *app, CoreInterface *coreInterface,
		FileActionHandler *fileActionHandler, CachedIcons *cachedIcons, BookmarkTree *bookmarkTree,
		HINSTANCE resourceInstance, const Config *config);

	LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);

	Tab &SetUpNewTab(Tab &tab, NavigateParams &navigateParams, const TabSettings &tabSettings);

	void OnLButtonDoubleClick(const POINT &pt);

	void OnTabCtrlMButtonUp(POINT *pt);

	void OnTabCtrlRButtonUp(POINT *pt);
	void CreateTabContextMenu(Tab &tab, const POINT &pt);
	void AddImagesToTabContextMenu(HMENU menu, std::vector<wil::unique_hbitmap> &menuImages);
	void ProcessTabCommand(UINT uMenuID, Tab &tab);
	void OnOpenParentInNewTab(const Tab &tab);
	void OnRefreshTab(Tab &tab);
	void OnRefreshAllTabs();
	void OnRenameTab(Tab &tab);
	void OnLockTab(Tab &tab);
	void OnLockTabAndAddress(Tab &tab);
	void OnCloseOtherTabs(int index);
	void OnCloseTabsToRight(int index);

	void ShowBackgroundContextMenu(const POINT &ptClient);
	void OnGetDispInfo(NMTTDISPINFO *dispInfo);

	void OnTabSelected(const Tab &tab);

	void RemoveTabFromControl(const Tab &tab);

	// TabViewDelegate
	void OnTabMoved(int fromIndex, int toIndex) override;
	bool ShouldRemoveIcon(int iconIndex) override;

	// ShellDropTargetWindow
	int GetDropTargetItem(const POINT &pt) override;
	unique_pidl_absolute GetPidlForTargetItem(int targetItem) override;
	IUnknown *GetSiteForTargetItem(PCIDLIST_ABSOLUTE targetItemPidl) override;
	bool IsTargetSourceOfDrop(int targetItem, IDataObject *dataObject) override;
	void UpdateUiForDrop(int targetItem, const POINT &pt) override;
	void ResetDropUiState() override;

	void UpdateUiForTargetItem(int targetItem);
	void ScrollTabControlForDrop(const POINT &pt);
	void OnDropSwitchTabTimer();
	void OnDropScrollTimer();

	void OnWindowDestroyed();

	MainTabView *const m_view;
	BrowserWindow *const m_browser;
	TabNavigationInterface *m_tabNavigation;
	App *const m_app;
	CoreInterface *m_coreInterface;
	FileActionHandler *m_fileActionHandler;
	OneShotTimerManager m_timerManager;
	std::unordered_map<int, std::unique_ptr<Tab>> m_tabs;
	IconFetcherImpl m_iconFetcher;
	CachedIcons *m_cachedIcons;
	BookmarkTree *m_bookmarkTree;
	HINSTANCE m_resourceInstance;
	const Config *const m_config;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;

	std::vector<int> m_tabSelectionHistory;
	int m_iPreviousTabSelectionId;

	// Drop handling
	std::optional<DropTargetContext> m_dropTargetContext;
};
