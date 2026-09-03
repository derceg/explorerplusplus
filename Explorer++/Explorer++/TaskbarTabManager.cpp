// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TaskbarTabManager.h"
#include "AppServices.h"
#include "AsyncIconFetcher.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "DarkModeColorProvider.h"
#include "DarkModeManager.h"
#include "ResourceLoader.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainer.h"
#include "TabEvents.h"
#include "TestHelper.h"
#include "../Helper/D2DHelper.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ImageInterop.h"
#include "../Helper/ImageOperations.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <dwmapi.h>

namespace
{

class ScopedTaskbarTabRegistration
{
public:
	ScopedTaskbarTabRegistration(wil::com_ptr_nothrow<ITaskbarList4> taskbarList, HWND proxyWindow,
		HWND browserWindow) :
		m_taskbarList(taskbarList),
		m_proxyWindow(proxyWindow)
	{
		HRESULT hr = m_taskbarList->RegisterTab(m_proxyWindow, browserWindow);
		DCHECK(SUCCEEDED(hr));

		m_registered = SUCCEEDED(hr);
	}

	~ScopedTaskbarTabRegistration()
	{
		if (m_registered)
		{
			HRESULT hr = m_taskbarList->UnregisterTab(m_proxyWindow);
			DCHECK(SUCCEEDED(hr));
		}
	}

private:
	const wil::com_ptr_nothrow<ITaskbarList4> m_taskbarList;
	const HWND m_proxyWindow;
	bool m_registered = false;
};

}

class TaskbarTabManager::TaskbarTabProxy
{
public:
	TaskbarTabProxy(const Tab *tab, BrowserWindow *browser,
		wil::com_ptr_nothrow<ITaskbarList4> taskbarList, TaskbarTabIconProvider *iconProvider,
		DarkModeManager *darkModeManager, DarkModeColorProvider *darkModeColorProvider);

	HWND GetWindow() const;
	const Tab *GetTab() const;
	void UpdateTitleAndIcon();
	void InvalidateIconicBitmaps();

private:
	static constexpr wchar_t CLASS_NAME[] = L"Explorer++TabProxyWindowClass";

	static ATOM RegisterProxyClass();

	void UpdateSystemMenu();

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnDwmSendIconicThumbnail(int maxWidth, int maxHeight);
	HRESULT CreateFallbackThumbnailBitmap(int width, int height,
		wil::com_ptr_nothrow<IWICBitmapSource> &output);
	HRESULT DrawFallbackThumbnail(ID2D1RenderTarget *renderTarget);
	HRESULT CreateWindowThumbnailBitmap(int maxWidth, int maxHeight,
		wil::com_ptr_nothrow<IWICBitmapSource> &output);
	HRESULT DrawWindowThumbnail(ID2D1RenderTarget *renderTarget);
	void OnDwmSendIconicLivePreviewBitmap();
	HRESULT CreateTabBitmap(wil::com_ptr_nothrow<IWICBitmapSource> &output);

	const Tab *const m_tab;
	BrowserWindow *const m_browser;
	TaskbarTabIconProvider *const m_iconProvider;
	DarkModeManager *const m_darkModeManager;
	DarkModeColorProvider *const m_darkModeColorProvider;
	wil::unique_hicon m_icon;
	wil::unique_hwnd m_hwnd;
	std::unique_ptr<WindowSubclass> m_subclass;

	// Note that this is explicitly last, since taskbar registration should be revoked before the
	// window is destroyed.
	std::unique_ptr<ScopedTaskbarTabRegistration> m_taskbarRegistration;
};

TaskbarTabManager::TaskbarTabManager(BrowserWindow *browser, AppServices *appServices,
	TaskbarListFactory maybeCreateTaskbarList) :
	m_browser(browser),
	m_appServices(appServices),
	m_maybeCreateTaskbarList(maybeCreateTaskbarList),
	m_iconProvider(appServices->GetAsyncIconFetcher(), appServices->GetResourceLoader())
{
	m_taskbarButtonCreatedMessage = RegisterWindowMessage(L"TaskbarButtonCreated");
	ChangeWindowMessageFilter(m_taskbarButtonCreatedMessage, MSGFLT_ADD);

	ChangeWindowMessageFilter(WM_DWMSENDICONICTHUMBNAIL, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_DWMSENDICONICLIVEPREVIEWBITMAP, MSGFLT_ADD);

	// Subclass the browser window until the above message (TaskbarButtonCreated) is caught.
	m_bowserWindowSubclass = std::make_unique<WindowSubclass>(m_browser->GetHWND(),
		std::bind_front(&TaskbarTabManager::BrowserWndProc, this));

	m_connections.push_back(m_appServices->GetConfig()->showTaskbarThumbnails.addObserver(
		std::bind_front(&TaskbarTabManager::OnShowThumbnailsChanged, this)));
}

TaskbarTabManager::~TaskbarTabManager() = default;

LRESULT TaskbarTabManager::BrowserWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Note that this message won't be received in environments like Windows PE, where there is no
	// shell/taskbar.
	if (uMsg == m_taskbarButtonCreatedMessage)
	{
		OnTaskbarButtonCreated();
		m_bowserWindowSubclass.reset();
		return 0;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void TaskbarTabManager::OnTaskbarButtonCreated()
{
	m_taskbarButtonCreated = true;

	if (m_appServices->GetConfig()->showTaskbarThumbnails.get())
	{
		SetUpThumbnails();
	}
}

void TaskbarTabManager::OnShowThumbnailsChanged(bool showTaskbarThumbnails)
{
	if (showTaskbarThumbnails)
	{
		SetUpThumbnails();
	}
	else
	{
		TearDownThumbnails();
	}
}

void TaskbarTabManager::SetUpThumbnails()
{
	if (!m_taskbarButtonCreated || m_thumbnailsSetUp)
	{
		return;
	}

	auto taskbarList = m_maybeCreateTaskbarList();

	if (!taskbarList)
	{
		return;
	}

	HRESULT hr = taskbarList->HrInit();

	if (FAILED(hr))
	{
		return;
	}

	m_taskbarList = taskbarList;

	auto *tabContainer = m_browser->GetActiveTabContainer();

	for (const auto *tab : tabContainer->GetAllTabsInOrder())
	{
		CreateTabProxy(*tab);

		if (tabContainer->IsTabSelected(*tab))
		{
			OnTabSelectionChanged(*tab);
		}
	}

	SetUpObservers();

	m_thumbnailsSetUp = true;
}

void TaskbarTabManager::SetUpObservers()
{
	auto *tabEvents = m_appServices->GetTabEvents();
	m_eventConnections.push_back(
		tabEvents->AddCreatedObserver(std::bind_front(&TaskbarTabManager::CreateTabProxy, this),
			TabEventScope::ForBrowser(*m_browser)));
	m_eventConnections.push_back(tabEvents->AddSelectedObserver(
		std::bind_front(&TaskbarTabManager::OnTabSelectionChanged, this),
		TabEventScope::ForBrowser(*m_browser)));
	m_eventConnections.push_back(
		tabEvents->AddMovedObserver(std::bind_front(&TaskbarTabManager::OnTabMoved, this),
			TabEventScope::ForBrowser(*m_browser)));
	m_eventConnections.push_back(
		tabEvents->AddUpdatedObserver(std::bind_front(&TaskbarTabManager::OnTabUpdated, this),
			TabEventScope::ForBrowser(*m_browser)));
	m_eventConnections.push_back(
		tabEvents->AddRemovedObserver(std::bind_front(&TaskbarTabManager::RemoveTabProxy, this),
			TabEventScope::ForBrowser(*m_browser)));

	m_eventConnections.push_back(
		m_appServices->GetShellBrowserEvents()->AddDirectoryPropertiesChangedObserver(
			std::bind_front(&TaskbarTabManager::OnDirectoryPropertiesChanged, this),
			ShellBrowserEventScope::ForBrowser(*m_browser)));

	m_eventConnections.push_back(m_appServices->GetNavigationEvents()->AddCommittedObserver(
		std::bind_front(&TaskbarTabManager::OnNavigationCommitted, this),
		NavigationEventScope::ForBrowser(*m_browser)));
}

// DWM will only interact with top-level windows. Therefore, a top-level proxy window will be
// created for every tab. This top-level window will be hidden, and will handle the thumbnail
// preview and events for the tab (e.g. activation).
//
// See:
// https://web.archive.org/web/20090817044031/http://dotnet.dzone.com/news/windows-7-taskbar-tabbed
// https://web.archive.org/web/20091213203041/http://channel9.msdn.com/learn/courses/Windows7/Taskbar/Win7TaskbarNative/Exercise-Experiment-with-the-New-Windows-7-Taskbar-Features/
void TaskbarTabManager::CreateTabProxy(const Tab &tab)
{
	auto [itr, didInsert] = m_tabProxies.insert({ &tab,
		std::make_unique<TaskbarTabProxy>(&tab, m_browser, m_taskbarList, &m_iconProvider,
			m_appServices->GetDarkModeManager(), m_appServices->GetDarkModeColorProvider()) });
	DCHECK(didInsert);
}

void TaskbarTabManager::OnTabSelectionChanged(const Tab &tab)
{
	auto *proxy = GetProxyForTab(&tab);
	m_taskbarList->SetTabActive(proxy->GetWindow(), m_browser->GetHWND(), 0);

	proxy->InvalidateIconicBitmaps();
}

void TaskbarTabManager::OnTabMoved(const Tab &tab, int fromIndex, int toIndex)
{
	UNREFERENCED_PARAMETER(fromIndex);

	const auto *tabContainer = m_browser->GetActiveTabContainer();
	int numTabs = tabContainer->GetNumTabs();

	HWND nextTabProxyWindow = nullptr;

	if (toIndex < (numTabs - 1))
	{
		const Tab &nextTab = tabContainer->GetTabByIndex(toIndex + 1);
		auto *nextTabProxy = GetProxyForTab(&nextTab);
		nextTabProxyWindow = nextTabProxy->GetWindow();
	}

	auto *proxy = GetProxyForTab(&tab);
	m_taskbarList->SetTabOrder(proxy->GetWindow(), nextTabProxyWindow);
}

void TaskbarTabManager::OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
{
	UNREFERENCED_PARAMETER(propertyType);

	UpdateTabProxyDetails(&tab);
}

void TaskbarTabManager::OnNavigationCommitted(const NavigationRequest *request)
{
	const auto *tab = request->GetShellBrowser()->GetTab();
	UpdateTabProxyDetails(tab);
}

void TaskbarTabManager::OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser)
{
	const auto *tab = shellBrowser->GetTab();
	UpdateTabProxyDetails(tab);
}

void TaskbarTabManager::UpdateTabProxyDetails(const Tab *tab)
{
	auto *proxy = GetProxyForTab(tab);
	proxy->UpdateTitleAndIcon();
	proxy->InvalidateIconicBitmaps();
}

void TaskbarTabManager::RemoveTabProxy(const Tab &tab)
{
	auto numErased = m_tabProxies.erase(&tab);
	DCHECK_EQ(numErased, 1u);
}

TaskbarTabManager::TaskbarTabProxy *TaskbarTabManager::GetProxyForTab(const Tab *tab)
{
	auto itr = m_tabProxies.find(tab);
	CHECK(itr != m_tabProxies.end());
	return itr->second.get();
}

void TaskbarTabManager::TearDownThumbnails()
{
	m_eventConnections.clear();
	m_tabProxies.clear();
	m_taskbarList.reset();
	m_thumbnailsSetUp = false;
}

const Tab *TaskbarTabManager::GetTabForProxyWindowForTesting(HWND proxyWindow) const
{
	CHECK(IsInTest());

	auto itr = std::ranges::find_if(m_tabProxies,
		[proxyWindow](const auto &item) { return item.second->GetWindow() == proxyWindow; });
	CHECK(itr != m_tabProxies.end());
	return itr->second->GetTab();
}

TaskbarTabManager::TaskbarTabProxy::TaskbarTabProxy(const Tab *tab, BrowserWindow *browser,
	wil::com_ptr_nothrow<ITaskbarList4> taskbarList, TaskbarTabIconProvider *iconProvider,
	DarkModeManager *darkModeManager, DarkModeColorProvider *darkModeColorProvider) :
	m_tab(tab),
	m_browser(browser),
	m_iconProvider(iconProvider),
	m_darkModeManager(darkModeManager),
	m_darkModeColorProvider(darkModeColorProvider)
{
	static bool classRegistered = false;

	if (!classRegistered)
	{
		auto res = RegisterProxyClass();
		CHECK_NE(res, 0);

		classRegistered = true;
	}

	m_hwnd.reset(CreateWindow(CLASS_NAME, L"", WS_CAPTION | WS_SYSMENU, 0, 0, 0, 0, nullptr,
		nullptr, GetModuleHandle(nullptr), nullptr));
	CHECK(m_hwnd);

	UpdateSystemMenu();

	BOOL forceIconicRepresentation = true;
	DwmSetWindowAttribute(m_hwnd.get(), DWMWA_FORCE_ICONIC_REPRESENTATION,
		&forceIconicRepresentation, sizeof(forceIconicRepresentation));

	BOOL hasIconicBitmap = true;
	DwmSetWindowAttribute(m_hwnd.get(), DWMWA_HAS_ICONIC_BITMAP, &hasIconicBitmap,
		sizeof(hasIconicBitmap));

	m_subclass = std::make_unique<WindowSubclass>(m_hwnd.get(),
		std::bind_front(&TaskbarTabProxy::WndProc, this));
	m_taskbarRegistration = std::make_unique<ScopedTaskbarTabRegistration>(taskbarList,
		m_hwnd.get(), m_browser->GetHWND());

	UpdateTitleAndIcon();
}

HWND TaskbarTabManager::TaskbarTabProxy::GetWindow() const
{
	return m_hwnd.get();
}

const Tab *TaskbarTabManager::TaskbarTabProxy::GetTab() const
{
	return m_tab;
}

ATOM TaskbarTabManager::TaskbarTabProxy::RegisterProxyClass()
{
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = CLASS_NAME;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.style = 0;
	return RegisterClass(&windowClass);
}

void TaskbarTabManager::TaskbarTabProxy::UpdateSystemMenu()
{
	HMENU systemMenu = GetSystemMenu(m_hwnd.get(), false);

	// None of these commands make sense for an individual tab and can therefore be removed.
	UINT commandsToRemove[] = {
		SC_RESTORE,
		SC_MOVE,
		SC_SIZE,
		SC_MINIMIZE,
		SC_MAXIMIZE,
	};

	for (UINT command : commandsToRemove)
	{
		auto res = DeleteMenu(systemMenu, command, MF_BYCOMMAND);
		DCHECK(res);
	}
}

LRESULT TaskbarTabManager::TaskbarTabProxy::WndProc(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE)
		{
			m_browser->Activate();
			m_tab->GetTabContainer()->SelectTab(*m_tab);
		}
		return 0;

	case WM_DWMSENDICONICTHUMBNAIL:
		OnDwmSendIconicThumbnail(HIWORD(lParam), LOWORD(lParam));
		return 0;

	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
		OnDwmSendIconicLivePreviewBitmap();
		return 0;

	case WM_CLOSE:
		m_tab->GetTabContainer()->CloseTab(*m_tab);
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void TaskbarTabManager::TaskbarTabProxy::OnDwmSendIconicThumbnail(int maxWidth, int maxHeight)
{
	// It's important to always generate something here, even when the window is minimized,
	// otherwise a loading spinner will be shown in place of the thumbnail and that spinner will
	// remain indefinitely, even with a call to DwmInvalidateIconicBitmaps().
	wil::com_ptr_nothrow<IWICBitmapSource> bitmap;

	if (IsIconic(m_browser->GetHWND()))
	{
		CreateFallbackThumbnailBitmap(maxWidth, maxHeight, bitmap);
	}
	else
	{
		CreateWindowThumbnailBitmap(maxWidth, maxHeight, bitmap);
	}

	if (!bitmap)
	{
		return;
	}

	wil::unique_hbitmap outputBitmap;
	HRESULT hr = ImageInterop::CreateHBITMAPFromWICBitmap(bitmap.get(), outputBitmap);

	if (FAILED(hr))
	{
		return;
	}

	hr = DwmSetIconicThumbnail(m_hwnd.get(), outputBitmap.get(), 0);
	DCHECK(SUCCEEDED(hr));
}

HRESULT TaskbarTabManager::TaskbarTabProxy::CreateFallbackThumbnailBitmap(int width, int height,
	wil::com_ptr_nothrow<IWICBitmapSource> &output)
{
	return D2DHelper::DrawToBitmap(width, height, D2DHelper::GdiUsage::WontUse,
		std::bind_front(&TaskbarTabManager::TaskbarTabProxy::DrawFallbackThumbnail, this), output);
}

// If it's not possible to capture a window thumbnail (e.g. because the browser window is
// minimized), a fallback image will be used. That fallback image consists of the tab icon drawn
// against a solid background.
HRESULT TaskbarTabManager::TaskbarTabProxy::DrawFallbackThumbnail(ID2D1RenderTarget *renderTarget)
{
	COLORREF backgroundColor = m_darkModeManager->IsDarkModeEnabled()
		? m_darkModeColorProvider->GetBackgroundColor()
		: GetSysColor(COLOR_WINDOW);
	renderTarget->Clear(D2DHelper::ColorFromColorRef(backgroundColor));

	auto icon = m_iconProvider->GetIconForTab(m_tab, TaskbarTabIconProvider::IconSize::Large);

	wil::com_ptr_nothrow<IWICBitmapSource> wicIcon;
	RETURN_IF_FAILED(ImageInterop::CreateWICBitmapFromHICON(icon.get(), wicIcon));

	wil::com_ptr_nothrow<ID2D1Bitmap> d2dIcon;
	RETURN_IF_FAILED(renderTarget->CreateBitmapFromWicBitmap(wicIcon.get(), nullptr, &d2dIcon));

	D2DHelper::DrawCenteredBitmap(renderTarget, d2dIcon.get());

	return S_OK;
}

HRESULT TaskbarTabManager::TaskbarTabProxy::CreateWindowThumbnailBitmap(int maxWidth, int maxHeight,
	wil::com_ptr_nothrow<IWICBitmapSource> &output)
{
	RECT browserRect;
	auto res = GetClientRect(m_browser->GetHWND(), &browserRect);
	CHECK(res);

	wil::com_ptr_nothrow<IWICBitmapSource> bitmap;
	RETURN_IF_FAILED(D2DHelper::DrawToBitmap(GetRectWidth(&browserRect),
		GetRectHeight(&browserRect), D2DHelper::GdiUsage::WillUse,
		std::bind_front(&TaskbarTabManager::TaskbarTabProxy::DrawWindowThumbnail, this), bitmap));

	RETURN_IF_FAILED(ImageOperations::ResizeBitmapToFit(bitmap.get(), maxWidth, maxHeight,
		WICBitmapInterpolationModeHighQualityCubic, output));

	return S_OK;
}

HRESULT TaskbarTabManager::TaskbarTabProxy::DrawWindowThumbnail(ID2D1RenderTarget *renderTarget)
{
	// If the browser window is minimized, the cursor is over one of the thumbnails (so that a peek
	// preview is being shown) and the thumbnail is clicked, the tab for that thumbnail will be
	// selected. That will then cause the bitmaps to be invalidated, which will then lead to this
	// function being called. That call can happen after the layout has been updated, but before the
	// browser window has been repainted. That then causes two issues:
	//
	// 1. The thumbnail that's generated will be incorrect (it will contain the browser window in a
	//    half drawn state).
	// 2. The listview (for whatever reason) won't be redrawn properly.
	//
	// Forcing a redraw here fixes both of those issues.
	RedrawWindow(m_browser->GetHWND(), nullptr, nullptr, RDW_ALLCHILDREN | RDW_UPDATENOW);

	RETURN_IF_FAILED(D2DHelper::DrawWindowToTarget(m_browser->GetHWND(),
		D2DHelper::PrintWindowTarget::Client, renderTarget));

	RECT tabRect;
	HWND listView = m_tab->GetShellBrowserImpl()->GetListView();
	auto res = GetWindowRect(listView, &tabRect);
	CHECK(res);
	MapWindowPoints(HWND_DESKTOP, m_browser->GetHWND(), reinterpret_cast<LPPOINT>(&tabRect), 2);
	RETURN_IF_FAILED(D2DHelper::PrintWindowToTarget(listView, D2DHelper::PrintWindowTarget::Full,
		renderTarget, { tabRect.left, tabRect.top }));

	return S_OK;
}

void TaskbarTabManager::TaskbarTabProxy::OnDwmSendIconicLivePreviewBitmap()
{
	if (IsIconic(m_browser->GetHWND()))
	{
		return;
	}

	// Only the tab needs to be captured, as the browser window will have already been captured by
	// DWM.
	wil::com_ptr_nothrow<IWICBitmapSource> bitmap;
	HRESULT hr = CreateTabBitmap(bitmap);

	if (FAILED(hr))
	{
		return;
	}

	RECT tabRect;
	auto res = GetWindowRect(m_tab->GetShellBrowserImpl()->GetListView(), &tabRect);
	CHECK(res);
	MapWindowPoints(HWND_DESKTOP, m_browser->GetHWND(), reinterpret_cast<LPPOINT>(&tabRect), 2);

	MENUBARINFO menuBarInfo;
	menuBarInfo.cbSize = sizeof(menuBarInfo);
	GetMenuBarInfo(m_browser->GetHWND(), OBJID_MENU, 0, &menuBarInfo);

	POINT ptClientOrigin = { 0, 0 };
	ClientToScreen(m_browser->GetHWND(), &ptClientOrigin);

	// DwmSetIconicLivePreviewBitmap() needs to be provided with the offset of the tab (so that it
	// knows where to draw the provided bitmap). For whatever reason, the offset can't just be in
	// the parent's client coordinates, because the menu bar isn't taken into account. That is,
	// without an adjustment, the bitmap would be shown further up than it should be.
	//
	// The adjustment here adds in the difference between the top of the menu bar and the top of the
	// client area. That difference is the height of the menu bar, in total.
	POINT ptOrigin = { tabRect.left, tabRect.top + ptClientOrigin.y - menuBarInfo.rcBar.top };

	wil::unique_hbitmap outputBitmap;
	hr = ImageInterop::CreateHBITMAPFromWICBitmap(bitmap.get(), outputBitmap);

	if (FAILED(hr))
	{
		return;
	}

	hr = DwmSetIconicLivePreviewBitmap(m_hwnd.get(), outputBitmap.get(), &ptOrigin, 0);
	DCHECK(SUCCEEDED(hr));
}

HRESULT TaskbarTabManager::TaskbarTabProxy::CreateTabBitmap(
	wil::com_ptr_nothrow<IWICBitmapSource> &output)
{
	RECT tabRect;
	HWND listView = m_tab->GetShellBrowserImpl()->GetListView();
	auto res = GetWindowRect(listView, &tabRect);
	CHECK(res);

	RETURN_IF_FAILED(D2DHelper::DrawToBitmap(
		GetRectWidth(&tabRect), GetRectHeight(&tabRect), D2DHelper::GdiUsage::WillUse,
		[listView](ID2D1RenderTarget *renderTarget)
		{
			return D2DHelper::PrintWindowToTarget(listView, D2DHelper::PrintWindowTarget::Full,
				renderTarget);
		},
		output));

	return S_OK;
}

void TaskbarTabManager::TaskbarTabProxy::UpdateTitleAndIcon()
{
	auto res = SetWindowText(m_hwnd.get(), m_tab->GetName().c_str());
	DCHECK(res);

	auto icon = m_iconProvider->GetIconForTab(m_tab, TaskbarTabIconProvider::IconSize::Small);
	SendMessage(m_hwnd.get(), WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon.get()));
	m_icon = std::move(icon);
}

void TaskbarTabManager::TaskbarTabProxy::InvalidateIconicBitmaps()
{
	HRESULT hr = DwmInvalidateIconicBitmaps(m_hwnd.get());
	DCHECK(SUCCEEDED(hr));
}

TaskbarTabManager::TaskbarTabIconProvider::TaskbarTabIconProvider(AsyncIconFetcher *iconFetcher,
	const ResourceLoader *resourceLoader) :
	m_iconFetcher(iconFetcher),
	m_resourceLoader(resourceLoader)
{
	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_SMALL, IID_PPV_ARGS(&m_smallShellImageList)));
	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_LARGE, IID_PPV_ARGS(&m_largeShellImageList)));

	FAIL_FAST_IF_FAILED(m_smallShellImageList->GetIconSize(&m_smallIconWidth, &m_smallIconHeight));
	FAIL_FAST_IF_FAILED(m_largeShellImageList->GetIconSize(&m_largeIconWidth, &m_largeIconHeight));
}

wil::unique_hicon TaskbarTabManager::TaskbarTabIconProvider::GetIconForTab(const Tab *tab,
	IconSize iconSize) const
{
	auto imageListInfo = GetImageListInfo(iconSize);
	wil::unique_hicon icon;

	if (tab->IsLocked())
	{
		icon = m_resourceLoader->LoadIconFromPNGAndScale(Icon::Lock, imageListInfo.iconWidth,
			imageListInfo.iconHeight);
	}
	else
	{
		int iconIndex = m_iconFetcher->GetCachedIconIndexOrDefault(
			tab->GetShellBrowser()->GetDirectory().Raw());
		HRESULT hr = imageListInfo.imageList->GetIcon(iconIndex, ILD_NORMAL, &icon);
		CHECK(SUCCEEDED(hr));
	}

	CHECK(icon);

	return icon;
}

TaskbarTabManager::TaskbarTabIconProvider::ImageListInfo TaskbarTabManager::TaskbarTabIconProvider::
	GetImageListInfo(IconSize iconSize) const
{
	switch (iconSize)
	{
	case IconSize::Small:
		return { m_smallShellImageList.get(), m_smallIconWidth, m_smallIconHeight };

	case IconSize::Large:
		return { m_largeShellImageList.get(), m_largeIconWidth, m_largeIconHeight };
	}

	LOG(FATAL) << "Invalid IconSize value";
}
