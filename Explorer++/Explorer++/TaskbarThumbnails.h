// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Tab.h"
#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <wil/resource.h>
#include <memory>

class App;
class BrowserWindow;
class NavigationRequest;
class ShellBrowser;
class TabContainerImpl;
class WindowSubclass;

class TaskbarThumbnails : private boost::noncopyable
{
public:
	TaskbarThumbnails(App *app, BrowserWindow *browser, TabContainerImpl *tabContainerImpl);
	~TaskbarThumbnails();

private:
	struct TabProxyInfo
	{
		ATOM atomClass;
		HWND hProxy;
		int iTabId;
		wil::unique_hicon icon;
	};

	LRESULT MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK TabProxyWndProcStub(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK TabProxyWndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, int iTabId);

	void Initialize();
	void OnTaskbarButtonCreated();
	void SetUpObservers();
	void SetupJumplistTasks();
	ATOM RegisterTabProxyClass(const TCHAR *szClassName);
	void CreateTabProxy(const Tab &tab);
	void RegisterTab(HWND hTabProxy, const TCHAR *szDisplayName);
	void RemoveTabProxy(const Tab &tab);
	void DestroyTabProxy(TabProxyInfo &tabProxy);
	void OnDwmSendIconicThumbnail(HWND tabProxy, const Tab &tab, int maxWidth, int maxHeight);
	wil::unique_hbitmap CaptureTabScreenshot(const Tab &tab);
	wil::unique_hbitmap GetTabLivePreviewBitmap(const Tab &tab);
	void OnTabSelectionChanged(const Tab &tab);
	void OnNavigationCommitted(const NavigationRequest *request);
	void OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser);
	void SetTabProxyIcon(const Tab &tab);
	void InvalidateTaskbarThumbnailBitmap(const Tab &tab);
	void UpdateTaskbarThumbnailTitle(const Tab &tab);

	App *const m_app;
	BrowserWindow *const m_browser;
	TabContainerImpl *const m_tabContainerImpl;
	std::vector<boost::signals2::scoped_connection> m_connections;
	std::unique_ptr<WindowSubclass> m_mainWindowSubclass;

	wil::com_ptr_nothrow<ITaskbarList4> m_taskbarList;
	std::list<TabProxyInfo> m_TabProxyList;
	UINT m_uTaskbarButtonCreatedMessage;
	BOOL m_enabled;
};
