// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconFetcherImpl.h"
#include "MouseEvent.h"
#include "OneShotTimer.h"
#include "OneShotTimerManager.h"
#include "ShellBrowser/FolderSettings.h"
#include "Tab.h"
#include "TabView.h"
#include "TabViewDelegate.h"
#include "../Helper/ShellDropTargetWindow.h"
#include "../Helper/WindowSubclass.h"
#include <functional>
#include <optional>
#include <unordered_map>

class AcceleratorManager;
class BookmarkTree;
class BrowserWindow;
class CachedIcons;
class ClipboardStore;
struct Config;
class MainTabView;
struct NavigateParams;
class NavigationEvents;
class NavigationRequest;
struct PreservedTab;
class ResourceLoader;
class ShellBrowserEvents;
class ShellBrowserFactory;
class TabRestorer;

// Used when creating a tab.
struct TabSettings
{
	std::optional<std::wstring> name;
	std::optional<Tab::LockState> lockState;
	std::optional<int> index;
	std::optional<bool> selected;

	// This is only used in tests.
	bool operator==(const TabSettings &) const = default;
};

class TabContainer : public ShellDropTargetWindow<int>, private TabViewDelegate
{
public:
	// When selecting an adjacent tab, indicates whether the previous or next tab should be
	// selected.
	enum class SelectionDirection
	{
		Previous,
		Next
	};

	static TabContainer *Create(MainTabView *view, BrowserWindow *browser,
		ShellBrowserFactory *shellBrowserFactory, TabEvents *tabEvents,
		ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
		TabRestorer *tabRestorer, CachedIcons *cachedIcons, BookmarkTree *bookmarkTree,
		ClipboardStore *clipboardStore, const AcceleratorManager *acceleratorManager,
		const Config *config, const ResourceLoader *resourceLoader);

	MainTabView *GetView();

	void CreateNewTabInDefaultDirectory(const TabSettings &tabSettings);
	Tab &CreateNewTab(const std::wstring &directory, const TabSettings &tabSettings = {},
		const FolderSettings *folderSettings = nullptr,
		const FolderColumns *initialColumns = nullptr);
	Tab &CreateNewTab(const PreservedTab &preservedTab);
	Tab &CreateNewTab(NavigateParams &navigateParams, const TabSettings &tabSettings = {},
		const FolderSettings *folderSettings = nullptr,
		const FolderColumns *initialColumns = nullptr);

	Tab &GetTab(int tabId) const;
	Tab *MaybeGetTab(int tabId) const;
	void SelectTab(const Tab &tab);
	void SelectAdjacentTab(SelectionDirection selectionDirection);
	void SelectTabAtIndex(int index);
	Tab &GetSelectedTab() const;
	int GetSelectedTabIndex() const;
	bool IsTabSelected(const Tab &tab) const;
	Tab &GetTabByIndex(int index) const;
	int GetTabIndex(const Tab &tab) const;
	int GetNumTabs() const;
	int MoveTab(const Tab &tab, int newIndex);
	Tab &DuplicateTab(const Tab &tab);

	// Unlike CloseTab(), which will only close tabs that aren't locked, this will close all tabs,
	// regardless of the lock state of any individual tab. This is needed when the parent window is
	// being closed.
	void CloseAllTabs();

	bool CloseTab(const Tab &tab);

	/* TODO: Ideally, there would be a method of iterating over the tabs without
	having access to the underlying container. */
	const std::unordered_map<int, std::unique_ptr<Tab>> &GetAllTabs() const;
	std::vector<Tab *> GetAllTabsInOrder() const;

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

	enum class CloseMode
	{
		Normal,
		Force
	};

	static const LONG DROP_SCROLL_MARGIN_X_96DPI = 40;

	TabContainer(MainTabView *view, BrowserWindow *browser,
		ShellBrowserFactory *shellBrowserFactory, TabEvents *tabEvents,
		ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
		TabRestorer *tabRestorer, CachedIcons *cachedIcons, BookmarkTree *bookmarkTree,
		ClipboardStore *clipboardStore, const AcceleratorManager *acceleratorManager,
		const Config *config, const ResourceLoader *resourceLoader);

	LRESULT ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);

	Tab &SetUpNewTab(Tab &tab, NavigateParams &navigateParams, const TabSettings &tabSettings);

	void OnTabDoubleClicked(Tab *tab, const MouseEvent &event);
	void OnTabMiddleClicked(Tab *tab, const MouseEvent &event);
	void OnTabRightClicked(Tab *tab, const MouseEvent &event);

	void ShowBackgroundContextMenu(const POINT &ptClient);

	void OnTabSelected(const Tab &tab);

	bool CloseTab(const Tab &tab, CloseMode closeMode);
	void RemoveTabFromControl(const Tab &tab);

	// TabViewDelegate
	void OnTabMoved(int fromIndex, int toIndex) override;
	bool ShouldRemoveIcon(int iconIndex) override;
	void OnSelectionChanged() override;

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
	ShellBrowserFactory *const m_shellBrowserFactory;
	TabEvents *const m_tabEvents;
	ShellBrowserEvents *const m_shellBrowserEvents;
	NavigationEvents *const m_navigationEvents;
	TabRestorer *const m_tabRestorer;
	OneShotTimerManager m_timerManager;
	std::unordered_map<int, std::unique_ptr<Tab>> m_tabs;
	IconFetcherImpl m_iconFetcher;
	CachedIcons *const m_cachedIcons;
	BookmarkTree *const m_bookmarkTree;
	ClipboardStore *const m_clipboardStore;
	const AcceleratorManager *const m_acceleratorManager;
	const Config *const m_config;
	const ResourceLoader *const m_resourceLoader;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;

	std::vector<int> m_tabSelectionHistory;
	int m_iPreviousTabSelectionId;

	// Drop handling
	std::optional<DropTargetContext> m_dropTargetContext;
};
