// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserWindow.h"
#include "Tab.h"
#include "ViewBase.h"
#include "../Helper/DropTargetWindow.h"
#include "../Helper/WinRTBaseWrapper.h"
#include "../Helper/WindowSubclass.h"
#include <boost/signals2.hpp>
#include <memory>

class AddressBarView;
class AppServices;
class BookmarksToolbarView;
class BrowserPaneView;
struct Config;
class DrivesToolbarView;
class MainRebarView;
class MainToolbarView;
class MenuView;
class NavigationRequest;
class ResourceLoader;
class ShellBrowser;
class StatusBarView;
class ThemeManager;
class ThemeWindowTracker;

namespace Applications
{

class ApplicationToolbarView;

}

class BrowserView : public ViewBase, private DropTargetInternal
{
public:
	static constexpr wchar_t WINDOW_CLASS_NAME[] = L"Explorer++";

	enum class FocusChangeDirection
	{
		Previous,
		Next
	};

	BrowserView(BrowserWindow *browser, const RECT &bounds, AppServices *appServices);
	~BrowserView();

	MenuView *GetMainMenuView() const;

	MainRebarView *GetMainRebarView() const;
	MainToolbarView *GetMainToolbarView() const;
	AddressBarView *GetAddressBarView() const;
	BookmarksToolbarView *GetBookmarksToolbarView() const;
	DrivesToolbarView *GetDrivesToolbarView() const;
	Applications::ApplicationToolbarView *GetApplicationToolbarView() const;

	BrowserPaneView *GetBrowserPaneView() const;

	StatusBarView *GetStatusBarView() const;

	void RedrawMenuBar();
	void FocusNextWindow(FocusChangeDirection direction);

private:
	static constexpr UINT WM_APP_CLOSE = WM_APP + 1;

	static HWND CreateBrowserWindow(const RECT &bounds);
	static ATOM RegisterBrowserWindowClass(HINSTANCE instance);

	void CreateMainRebarAndChildren();

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnDpiChanged(const RECT *updatedWindowRect);

	void OnBrowserLifecycleStateChanged(BrowserWindow::LifecycleState updatedState);
	void OnNavigationCommitted(const NavigationRequest *request);
	void OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser);
	void OnTabSelected(const Tab &tab);

	void OnShowFullTitlePathUpdated(BOOL newValue);
	void OnShowUserNameInTitleBarUpdated(BOOL newValue);
	void OnShowPrivilegeLevelInTitleBarUpdated(BOOL newValue);

	void UpdateWindowText();

	// DropTargetInternal
	DWORD DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;
	DWORD DragOver(DWORD keyState, POINT pt, DWORD effect) override;
	void DragLeave() override;
	DWORD Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;

	const WindowSubclass m_subclass;
	BrowserWindow *const m_browser;
	const Config *const m_config;
	ThemeManager *const m_themeManager;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;

	winrt::com_ptr<DropTargetWindow> m_dropTargetWindow;
	std::unique_ptr<ThemeWindowTracker> m_themeWindowTracker;

	std::unique_ptr<MenuView> m_mainMenuView;

	// Main rebar
	std::unique_ptr<MainRebarView> m_mainRebarView;
	std::unique_ptr<MainToolbarView> m_mainToolbarView;
	std::unique_ptr<AddressBarView> m_addressBarView;
	std::unique_ptr<BookmarksToolbarView> m_bookmarksToolbarView;
	std::unique_ptr<DrivesToolbarView> m_drivesToolbarView;
	std::unique_ptr<Applications::ApplicationToolbarView> m_applicationToolbarView;

	std::unique_ptr<BrowserPaneView> m_browserPaneView;

	std::unique_ptr<StatusBarView> m_statusBarView;
};
