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

class AppServices;
class BrowserWindow;
class NavigationRequest;
class ShellBrowser;
class WindowSubclass;

class TaskbarThumbnails : private boost::noncopyable
{
public:
	TaskbarThumbnails(BrowserWindow *browser, AppServices *appServices);
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
	void OnShowThumbnailsChanged(bool showTaskbarThumbnails);
	void SetUpThumbnails();
	void SetUpObservers();
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
	void TearDownThumbnails();

	BrowserWindow *const m_browser;
	AppServices *const m_appServices;
	std::vector<boost::signals2::scoped_connection> m_eventConnections;
	std::vector<boost::signals2::scoped_connection> m_connections;
	std::unique_ptr<WindowSubclass> m_mainWindowSubclass;

	wil::com_ptr_nothrow<ITaskbarList4> m_taskbarList;
	std::list<TabProxyInfo> m_tabProxyList;
	UINT m_taskbarButtonCreatedMessage;
	bool m_taskbarButtonCreated = false;
	bool m_thumbnailsSetUp = false;
};
