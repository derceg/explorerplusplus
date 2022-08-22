// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/FolderSettings.h"
#include "SignalWrapper.h"
#include "Tab.h"
#include "../Helper/IconFetcher.h"
#include "../Helper/ShellDropTargetWindow.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <boost/parameter.hpp>
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <wil/resource.h>
#include <functional>
#include <optional>
#include <unordered_map>

class BookmarkTree;
class CachedIcons;
struct Config;
class FileActionHandler;
__interface IExplorerplusplus;
class Navigation;
struct PreservedTab;

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

class TabContainer : public ShellDropTargetWindow<int>
{
public:
	static TabContainer *Create(HWND parent, TabNavigationInterface *tabNavigation,
		IExplorerplusplus *expp, FileActionHandler *fileActionHandler, CachedIcons *cachedIcons,
		BookmarkTree *bookmarkTree, HINSTANCE instance, std::shared_ptr<Config> config);

	void CreateNewTabInDefaultDirectory(const TabSettings &tabSettings);
	void CreateNewTab(const TCHAR *TabDirectory, const TabSettings &tabSettings = {},
		const FolderSettings *folderSettings = nullptr,
		const FolderColumns *initialColumns = nullptr, int *newTabId = nullptr);
	void CreateNewTab(const PreservedTab &preservedTab, int *newTabId = nullptr);
	void CreateNewTab(PCIDLIST_ABSOLUTE pidlDirectory, const TabSettings &tabSettings = {},
		const FolderSettings *folderSettings = nullptr,
		const FolderColumns *initialColumns = nullptr, int *newTabId = nullptr);

	Tab &GetTab(int tabId);
	Tab *GetTabOptional(int tabId);
	void SelectTab(const Tab &tab);
	void SelectAdjacentTab(BOOL bNextTab);
	void SelectTabAtIndex(int index);
	Tab &GetSelectedTab();
	int GetSelectedTabIndex() const;
	std::optional<int> GetSelectedTabIndexOptional() const;
	bool IsTabSelected(const Tab &tab);
	Tab &GetTabByIndex(int index);
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

	// Signals
	SignalWrapper<TabContainer, void(int tabId, BOOL switchToNewTab)> tabCreatedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab, PCIDLIST_ABSOLUTE pidl)>
		tabNavigationStartedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab, PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry)>
		tabNavigationCommittedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab)> tabNavigationCompletedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab)> tabNavigationFailedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab, Tab::PropertyType propertyType)>
		tabUpdatedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab, int fromIndex, int toIndex)> tabMovedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab)> tabSelectedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab)> tabPreRemovalSignal;
	SignalWrapper<TabContainer, void(int tabId)> tabRemovedSignal;

	SignalWrapper<TabContainer, void(const Tab &tab)> tabDirectoryModifiedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab)> tabListViewSelectionChangedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab)> tabColumnsChangedSignal;

private:
	enum class ScrollDirection
	{
		Left,
		Right
	};

	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static constexpr int ICON_SIZE_96DPI = 16;

	static constexpr UINT DROP_SWITCH_TAB_TIMER_ID = 1;
	static constexpr UINT DROP_SWITCH_TAB_TIMER_ELAPSE = 500;

	static constexpr UINT DROP_SCROLL_TIMER_ID = 2;
	static constexpr UINT DROP_SCROLL_TIMER_ELAPSE = 1000;

	static constexpr LONG DROP_SCROLL_MARGIN_X_96DPI = 40;

	TabContainer(HWND parent, TabNavigationInterface *tabNavigation, IExplorerplusplus *expp,
		FileActionHandler *fileActionHandler, CachedIcons *cachedIcons, BookmarkTree *bookmarkTree,
		HINSTANCE instance, std::shared_ptr<Config> config);
	~TabContainer();

	static HWND CreateTabControl(HWND parent, BOOL forceSameTabWidth);

	static LRESULT CALLBACK WndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);
	void AddDefaultTabIcons(HIMAGELIST himlTab);
	bool IsDefaultIcon(int iconIndex);

	void SetUpNewTab(Tab &tab, PCIDLIST_ABSOLUTE pidlDirectory, const TabSettings &tabSettings,
		bool addHistoryEntry, int *newTabId);

	void OnTabCtrlLButtonDown(POINT *pt);
	void OnTabCtrlLButtonUp();
	void OnTabCtrlMouseMove(POINT *pt);
	void OnMouseWheel(HWND hwnd, int xPos, int yPos, int delta, UINT keys);

	void OnLButtonDoubleClick(const POINT &pt);

	void OnTabCtrlMButtonUp(POINT *pt);

	void OnTabCtrlRButtonUp(POINT *pt);
	void CreateTabContextMenu(Tab &tab, const POINT &pt);
	void AddImagesToTabContextMenu(HMENU menu, std::vector<wil::unique_hbitmap> &menuImages);
	void ProcessTabCommand(UINT uMenuID, Tab &tab);
	void OnOpenParentInNewTab(const Tab &tab);
	void OnRefreshTab(Tab &tab);
	void OnRefreshAllTabs();
	void OnRenameTab(const Tab &tab);
	void OnLockTab(Tab &tab);
	void OnLockTabAndAddress(Tab &tab);
	void OnCloseOtherTabs(int index);
	void OnCloseTabsToRight(int index);

	void ShowBackgroundContextMenu(const POINT &ptClient);
	void OnBackgroundMenuItemSelected(int menuItemId);
	void OnGetDispInfo(NMTTDISPINFO *dispInfo);

	void OnTabCreated(int tabId, BOOL switchToNewTab);
	void OnTabRemoved(int tabId);

	void OnTabSelected(const Tab &tab);

	void OnAlwaysShowTabBarUpdated(BOOL newValue);
	void OnForceSameTabWidthUpdated(BOOL newValue);

	void OnNavigationCommitted(const Tab &tab, PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry);
	void OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType);
	void UpdateTabNameInWindow(const Tab &tab);
	void SetTabIcon(const Tab &tab);
	void SetTabIconFromSystemImageList(const Tab &tab, int systemIconIndex);
	void SetTabIconFromImageList(const Tab &tab, int imageIndex);

	void InsertNewTab(int index, int tabId, PCIDLIST_ABSOLUTE pidlDirectory,
		std::optional<std::wstring> customName);

	void RemoveTabFromControl(const Tab &tab);

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
	void ScrollTabControl(ScrollDirection direction);

	wil::unique_hfont m_tabFont;
	wil::unique_himagelist m_tabCtrlImageList;

	std::unordered_map<int, std::unique_ptr<Tab>> m_tabs;

	IconFetcher m_iconFetcher;
	CachedIcons *m_cachedIcons;
	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	int m_defaultFolderIconSystemImageListIndex;
	int m_defaultFolderIconIndex;
	int m_tabIconLockIndex;

	TabNavigationInterface *m_tabNavigation;
	IExplorerplusplus *m_expp;
	FileActionHandler *m_fileActionHandler;

	HINSTANCE m_instance;
	std::shared_ptr<Config> m_config;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	std::vector<int> m_tabSelectionHistory;
	int m_iPreviousTabSelectionId;

	// Tab dragging
	BOOL m_bTabBeenDragged;
	int m_draggedTabStartIndex;
	int m_draggedTabEndIndex;
	RECT m_rcDraggedTab;

	// Drop handling
	int m_dropTargetIndex;
	std::optional<ScrollDirection> m_dropScrollDirection;

	BookmarkTree *m_bookmarkTree;
};
