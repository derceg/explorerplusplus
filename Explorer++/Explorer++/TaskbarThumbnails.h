// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "Navigation.h"
#include "Tab.h"
#include "TabContainer.h"
#include "../Helper/Macros.h"
#include <wil/resource.h>

struct Config;

class TaskbarThumbnails
{
public:

	static TaskbarThumbnails *Create(IExplorerplusplus *expp, TabContainer *tabContainer,
		Navigation *navigation, HINSTANCE instance, std::shared_ptr<Config> config);

private:

	DISALLOW_COPY_AND_ASSIGN(TaskbarThumbnails);

	struct TabProxyInfo_t
	{
		ATOM atomClass;
		HWND hProxy;
		int iTabId;
	};

	TaskbarThumbnails(IExplorerplusplus *expp, TabContainer *tabContainer,
		Navigation *navigation, HINSTANCE instance, std::shared_ptr<Config> config);
	~TaskbarThumbnails();

	static LRESULT CALLBACK MainWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK TabProxyWndProcStub(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK TabProxyWndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, int iTabId);

	void Initialize();
	void SetupJumplistTasks();
	ATOM RegisterTabProxyClass(const TCHAR *szClassName);
	void CreateTabProxy(int iTabId, BOOL bSwitchToNewTab);
	void RegisterTab(HWND hTabProxy, const TCHAR *szDisplayName, BOOL bTabActive);
	void RemoveTabProxy(int iTabId);
	wil::unique_hbitmap CaptureTabScreenshot(const Tab &tab);
	wil::unique_hbitmap GetTabLivePreviewBitmap(const Tab &tab);
	void OnTabSelectionChanged(const Tab &tab);
	void OnNavigationCompleted(const Tab &tab);
	void SetTabProxyIcon(const Tab &tab);
	void InvalidateTaskbarThumbnailBitmap(const Tab &tab);
	void UpdateTaskbarThumbnailTtitle(const Tab &tab);

	IExplorerplusplus *m_expp;
	TabContainer *m_tabContainer;
	Navigation *m_navigation;
	HINSTANCE m_instance;

	ITaskbarList4 *m_pTaskbarList;
	std::list<TabProxyInfo_t> m_TabProxyList;
	UINT m_uTaskbarButtonCreatedMessage;
	BOOL m_bTaskbarInitialised;
	BOOL m_enabled;
};