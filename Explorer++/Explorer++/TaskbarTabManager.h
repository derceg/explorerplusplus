// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Tab.h"
#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <wil/resource.h>
#include <ShObjIdl.h>
#include <functional>
#include <memory>
#include <unordered_map>

class AppServices;
class AsyncIconFetcher;
class BrowserWindow;
class NavigationRequest;
class ResourceLoader;
class ShellBrowser;
class WindowSubclass;

class TaskbarTabManager : private boost::noncopyable
{
public:
	using TaskbarListFactory = std::function<wil::com_ptr_nothrow<ITaskbarList4>()>;

	TaskbarTabManager(BrowserWindow *browser, AppServices *appServices,
		TaskbarListFactory maybeCreateTaskbarList);
	~TaskbarTabManager();

	const Tab *GetTabForProxyWindowForTesting(HWND proxyWindow) const;

private:
	class TaskbarTabProxy;

	class TaskbarTabIconProvider
	{
	public:
		enum class IconSize
		{
			Small,
			Large
		};

		TaskbarTabIconProvider(AsyncIconFetcher *iconFetcher, const ResourceLoader *resourceLoader);

		wil::unique_hicon GetIconForTab(const Tab *tab, IconSize iconSize) const;

	private:
		struct ImageListInfo
		{
			IImageList *imageList;
			int iconWidth;
			int iconHeight;
		};

		ImageListInfo GetImageListInfo(IconSize iconSize) const;

		AsyncIconFetcher *const m_iconFetcher;
		const ResourceLoader *const m_resourceLoader;
		wil::com_ptr_nothrow<IImageList> m_smallShellImageList;
		wil::com_ptr_nothrow<IImageList> m_largeShellImageList;
		int m_smallIconWidth = 0;
		int m_smallIconHeight = 0;
		int m_largeIconWidth = 0;
		int m_largeIconHeight = 0;
	};

	LRESULT BrowserWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnTaskbarButtonCreated();
	void OnShowThumbnailsChanged(bool showTaskbarThumbnails);
	void SetUpThumbnails();
	void SetUpObservers();
	void CreateTabProxy(const Tab &tab);
	void OnTabSelectionChanged(const Tab &tab);
	void OnTabMoved(const Tab &tab, int fromIndex, int toIndex);
	void OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType);
	void OnNavigationCommitted(const NavigationRequest *request);
	void OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser);
	void UpdateTabProxyDetails(const Tab *tab);
	void RemoveTabProxy(const Tab &tab);
	TaskbarTabProxy *GetProxyForTab(const Tab *tab);
	void TearDownThumbnails();

	BrowserWindow *const m_browser;
	AppServices *const m_appServices;
	TaskbarListFactory m_maybeCreateTaskbarList;
	UINT m_taskbarButtonCreatedMessage;
	bool m_taskbarButtonCreated = false;
	wil::com_ptr_nothrow<ITaskbarList4> m_taskbarList;
	bool m_thumbnailsSetUp = false;
	TaskbarTabIconProvider m_iconProvider;
	std::unordered_map<const Tab *, std::unique_ptr<TaskbarTabProxy>> m_tabProxies;
	std::vector<boost::signals2::scoped_connection> m_eventConnections;
	std::vector<boost::signals2::scoped_connection> m_connections;
	std::unique_ptr<WindowSubclass> m_bowserWindowSubclass;
};
