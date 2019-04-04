// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/iShellView.h"
#include "Tab.h"
#include "TabContainerInterface.h"
#include <unordered_map>

class CTabContainer
{
public:

	CTabContainer(HWND hTabCtrl, std::unordered_map<int, Tab> *tabInfo, TabContainerInterface *tabContainer);
	~CTabContainer();

	void InsertTab();
	void RemoveTab();

	int GetSelection();

private:

	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static const int TAB_ICON_LOCK_INDEX = 0;

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnGetDispInfo(NMTTDISPINFO *dispInfo);

	void OnNavigationCompleted(const Tab &tab);
	void OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType);
	void UpdateTabNameInWindow(const Tab &tab);
	void SetTabIcon(const Tab &tab);

	HWND m_hTabCtrl;

	std::unordered_map<int, Tab> *m_tabInfo;
	TabContainerInterface *m_tabContainer;

	boost::signals2::connection m_navigationCompletedConnection;
	boost::signals2::connection m_tabUpdatedConnection;
};