// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "ShellBrowser/CachedIcons.h"
#include "ShellBrowser/iShellView.h"
#include "Tab.h"
#include "TabContainerInterface.h"
#include "TabInterface.h"
#include "../Helper/BaseWindow.h"
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <functional>
#include <unordered_map>

struct Config;

class TabContainer : public CBaseWindow
{
public:

	typedef boost::signals2::signal<void(int tabId, BOOL switchToNewTab)> TabCreatedSignal;
	typedef boost::signals2::signal<void(const Tab &tab, int fromIndex, int toIndex)> TabMovedSignal;

	static TabContainer *Create(HWND parent, TabContainerInterface *tabContainer,
		TabInterface *tabInterface, IExplorerplusplus *expp, HINSTANCE instance,
		std::shared_ptr<Config> config);

	HRESULT CreateNewTab(const TCHAR *TabDirectory, const TabSettings &tabSettings = {}, const FolderSettings *folderSettings = nullptr, boost::optional<FolderColumns> initialColumns = boost::none, int *newTabId = nullptr);
	HRESULT CreateNewTab(LPCITEMIDLIST pidlDirectory, const TabSettings &tabSettings = {}, const FolderSettings *folderSettings = nullptr, boost::optional<FolderColumns> initialColumns = boost::none, int *newTabId = nullptr);
	FolderSettings GetDefaultFolderSettings(LPCITEMIDLIST pidlDirectory) const;

	Tab &TabContainer::GetTab(int tabId);
	Tab *GetTabOptional(int tabId);
	Tab &GetSelectedTab();
	bool IsTabSelected(const Tab &tab);
	Tab &GetTabByIndex(int index);
	int GetTabIndex(const Tab &tab) const;
	int GetNumTabs() const;
	int MoveTab(const Tab &tab, int newIndex);

	// Eventually, this should be removed.
	std::unordered_map<int, Tab> &GetTabs();

	/* TODO: Ideally, there would be a method of iterating over the tabs without
	having access to the underlying container. */
	const std::unordered_map<int, Tab> &GetAllTabs() const;
	std::vector<std::reference_wrapper<const Tab>> GetAllTabsInOrder() const;

	int GetSelection();

	boost::signals2::connection AddTabCreatedObserver(const TabCreatedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back);
	boost::signals2::connection AddTabMovedObserver(const TabMovedSignal::slot_type &observer);

private:

	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static const int TAB_ICON_LOCK_INDEX = 0;

	// Represents the maximum number of icons that can be cached across
	// all tabs (as the icon cache is shared between tabs).
	static const int MAX_CACHED_ICONS = 1000;

	TabContainer(HWND parent, TabContainerInterface *tabContainer, TabInterface *tabInterface,
		IExplorerplusplus *expp, HINSTANCE instance, std::shared_ptr<Config> config);
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
	void AddImagesToTabContextMenu(HMENU menu, std::vector<HBitmapPtr> &menuImages);
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

	void OnAlwaysShowTabBarUpdated(BOOL newValue);
	void OnForceSameTabWidthUpdated(BOOL newValue);

	void OnNavigationCompleted(const Tab &tab);
	void OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType);
	void UpdateTabNameInWindow(const Tab &tab);
	void SetTabIcon(const Tab &tab);

	SortMode GetDefaultSortMode(LPCITEMIDLIST pidlDirectory) const;
	void InsertNewTab(int index, int tabId, LPCITEMIDLIST pidlDirectory, boost::optional<std::wstring> customName);

	HFONT m_hTabFont;
	HIMAGELIST m_hTabCtrlImageList;

	std::unordered_map<int, Tab> m_tabs;
	int m_tabIdCounter;
	CachedIcons m_cachedIcons;

	TabContainerInterface *m_tabContainerInterface;
	TabInterface *m_tabInterface;
	IExplorerplusplus *m_expp;

	HINSTANCE m_instance;
	std::shared_ptr<Config> m_config;

	boost::signals2::connection m_tabCreatedConnection;
	boost::signals2::connection m_tabRemovedConnection;

	boost::signals2::connection m_navigationCompletedConnection;
	boost::signals2::connection m_tabUpdatedConnection;

	boost::signals2::connection m_alwaysShowTabBarConnection;
	boost::signals2::connection m_forceSameTabWidthConnection;

	// Tab dragging
	BOOL m_bTabBeenDragged;
	int m_draggedTabStartIndex;
	int m_draggedTabEndIndex;
	RECT m_rcDraggedTab;

	// Signals
	TabCreatedSignal m_tabCreatedSignal;
	TabMovedSignal m_tabMovedSignal;
};