// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "ShellBrowser/CachedIcons.h"
#include "ShellBrowser/iShellView.h"
#include "SignalWrapper.h"
#include "Tab.h"
#include "TabContainerInterface.h"
#include "TabInterface.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/DpiCompatibility.h"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <functional>
#include <unordered_map>

struct Config;
class Navigation;

class TabContainer : public CBaseWindow
{
public:

	static TabContainer *Create(HWND parent, TabContainerInterface *tabContainer,
		TabInterface *tabInterface, Navigation *navigation, IExplorerplusplus *expp,
		HINSTANCE instance, std::shared_ptr<Config> config);

	HRESULT CreateNewTab(const TCHAR *TabDirectory, const TabSettings &tabSettings = {}, const FolderSettings *folderSettings = nullptr, boost::optional<FolderColumns> initialColumns = boost::none, int *newTabId = nullptr);
	HRESULT CreateNewTab(LPCITEMIDLIST pidlDirectory, const TabSettings &tabSettings = {}, const FolderSettings *folderSettings = nullptr, boost::optional<FolderColumns> initialColumns = boost::none, int *newTabId = nullptr);
	FolderSettings GetDefaultFolderSettings(LPCITEMIDLIST pidlDirectory) const;

	Tab &GetTab(int tabId);
	Tab *GetTabOptional(int tabId);
	void SelectTab(const Tab &tab);
	void SelectAdjacentTab(BOOL bNextTab);
	void SelectTabAtIndex(int index);
	Tab &GetSelectedTab();
	int GetSelectedTabIndex() const;
	bool IsTabSelected(const Tab &tab);
	Tab &GetTabByIndex(int index);
	int GetTabIndex(const Tab &tab) const;
	int GetNumTabs() const;
	int MoveTab(const Tab &tab, int newIndex);
	void DuplicateTab(const Tab &tab);
	bool CloseTab(const Tab &tab);

	// Eventually, this should be removed.
	std::unordered_map<int, Tab> &GetTabs();

	/* TODO: Ideally, there would be a method of iterating over the tabs without
	having access to the underlying container. */
	const std::unordered_map<int, Tab> &GetAllTabs() const;
	std::vector<std::reference_wrapper<const Tab>> GetAllTabsInOrder() const;

	// Signals
	SignalWrapper<TabContainer, void(int tabId, BOOL switchToNewTab)> tabCreatedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab, Tab::PropertyType propertyType)> tabUpdatedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab, int fromIndex, int toIndex)> tabMovedSignal;
	SignalWrapper<TabContainer, void(const Tab &tab)> tabSelectedSignal;
	SignalWrapper<TabContainer, void(int tabId)> tabRemovedSignal;

private:

	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	// Represents the maximum number of icons that can be cached across
	// all tabs (as the icon cache is shared between tabs).
	static const int MAX_CACHED_ICONS = 1000;

	static const int ICON_SIZE_96DPI = 16;

	TabContainer(HWND parent, TabContainerInterface *tabContainer, TabInterface *tabInterface,
		Navigation *navigation, IExplorerplusplus *expp, HINSTANCE instance,
		std::shared_ptr<Config> config);
	~TabContainer();

	static HWND CreateTabControl(HWND parent, BOOL forceSameTabWidth);

	static LRESULT CALLBACK WndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);
	void AddDefaultTabIcons(HIMAGELIST himlTab);

	void OnTabCtrlLButtonDown(POINT *pt);
	void OnTabCtrlLButtonUp(void);
	void OnTabCtrlMouseMove(POINT *pt);

	void OnLButtonDoubleClick(const POINT &pt);

	void OnTabCtrlMButtonUp(POINT *pt);

	void OnTabCtrlRButtonUp(POINT *pt);
	void CreateTabContextMenu(Tab &tab, const POINT &pt);
	void AddImagesToTabContextMenu(HMENU menu, std::vector<wil::unique_hbitmap> &menuImages);
	void ProcessTabCommand(UINT uMenuID, Tab &tab);
	void OnOpenParentInNewTab(const Tab &tab);
	void OnRefreshAllTabs();
	void OnRenameTab(const Tab &tab);
	void OnLockTab(Tab &tab);
	void OnLockTabAndAddress(Tab &tab);
	void OnCloseOtherTabs(int index);
	void OnCloseTabsToRight(int index);

	void OnGetDispInfo(NMTTDISPINFO *dispInfo);

	void OnTabCreated(int tabId, BOOL switchToNewTab);
	void OnTabRemoved(int tabId);

	void OnTabSelected(const Tab &tab);

	void OnAlwaysShowTabBarUpdated(BOOL newValue);
	void OnForceSameTabWidthUpdated(BOOL newValue);

	void OnNavigationCompleted(const Tab &tab);
	void OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType);
	void UpdateTabNameInWindow(const Tab &tab);
	void SetTabIcon(const Tab &tab);

	SortMode GetDefaultSortMode(LPCITEMIDLIST pidlDirectory) const;
	void InsertNewTab(int index, int tabId, LPCITEMIDLIST pidlDirectory, boost::optional<std::wstring> customName);

	void RemoveTabFromControl(const Tab &tab);

	HFONT m_hTabFont;
	HIMAGELIST m_hTabCtrlImageList;

	std::unordered_map<int, Tab> m_tabs;
	int m_tabIdCounter;
	CachedIcons m_cachedIcons;
	int m_tabIconLockIndex;

	TabContainerInterface *m_tabContainerInterface;
	TabInterface *m_tabInterface;
	Navigation *m_navigation;
	IExplorerplusplus *m_expp;

	HINSTANCE m_instance;
	std::shared_ptr<Config> m_config;

	std::vector<boost::signals2::scoped_connection> m_connections;

	DpiCompatibility m_dpiCompat;

	std::vector<int> m_tabSelectionHistory;
	int m_iPreviousTabSelectionId;

	// Tab dragging
	BOOL m_bTabBeenDragged;
	int m_draggedTabStartIndex;
	int m_draggedTabEndIndex;
	RECT m_rcDraggedTab;
};