// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "ShellBrowser/iShellView.h"
#include "Tab.h"
#include "TabContainerInterface.h"
#include "boost/signals2.hpp"
#include <unordered_map>

struct Config;

class CTabContainer
{
public:

	typedef boost::signals2::signal<void(const Tab &tab, int fromIndex, int toIndex)> TabMovedSignal;

	CTabContainer(HWND hTabCtrl, std::unordered_map<int, Tab>* tabInfo, TabContainerInterface* tabContainer,
		IExplorerplusplus* expp, std::shared_ptr<Config> config);
	~CTabContainer();

	void InsertTab();
	void RemoveTab();

	int GetSelection();

	boost::signals2::connection AddTabMovedObserver(const TabMovedSignal::slot_type &observer);

private:

	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static const int TAB_ICON_LOCK_INDEX = 0;

	static LRESULT CALLBACK WndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnTabCtrlLButtonDown(POINT *pt);
	void OnTabCtrlLButtonUp(void);
	void OnTabCtrlMouseMove(POINT *pt);
	void OnTabCtrlMButtonUp(POINT *pt);

	void OnGetDispInfo(NMTTDISPINFO *dispInfo);

	void OnTabCreated(int tabId, BOOL switchToNewTab);
	void OnTabRemoved(int tabId);

	void OnAlwaysShowTabBarUpdated(BOOL newValue);

	void OnNavigationCompleted(const Tab &tab);
	void OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType);
	void UpdateTabNameInWindow(const Tab &tab);
	void SetTabIcon(const Tab &tab);

	HWND m_hTabCtrl;

	std::unordered_map<int, Tab> *m_tabInfo;
	TabContainerInterface *m_tabContainer;
	IExplorerplusplus *m_expp;

	std::shared_ptr<Config> m_config;

	boost::signals2::connection m_tabCreatedConnection;
	boost::signals2::connection m_tabRemovedConnection;

	boost::signals2::connection m_navigationCompletedConnection;
	boost::signals2::connection m_tabUpdatedConnection;

	boost::signals2::connection m_alwaysShowTabBarConnection;

	// Tab dragging
	BOOL m_bTabBeenDragged;
	int m_draggedTabStartIndex;
	int m_draggedTabEndIndex;
	RECT m_rcDraggedTab;

	TabMovedSignal m_tabMovedSignal;
};