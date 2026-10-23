// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserWindow.h"
#include "Tab.h"
#include "ViewBase.h"
#include "../Helper/DropTargetWindow.h"
#include "../Helper/WinRTBaseWrapper.h"
#include <boost/signals2.hpp>
#include <memory>

class AddressBarView;
class BookmarksToolbarView;
struct Config;
class DrivesToolbarView;
class MainRebarView;
class MenuView;
class NavigationEvents;
class NavigationRequest;
class ResourceLoader;
class ShellBrowser;
class ShellBrowserEvents;
class StatusBarView;
class TabEvents;
class ThemeManager;
class ThemeWindowTracker;

namespace Applications
{

class ApplicationToolbarView;

}

class BrowserView : public ViewBase, private DropTargetInternal
{
public:
	enum class FocusChangeDirection
	{
		Previous,
		Next
	};

	BrowserView(HWND hwnd, BrowserWindow *browser, const Config *config, TabEvents *tabEvents,
		ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
		ThemeManager *themeManager, const ResourceLoader *resourceLoader);
	~BrowserView();

	MenuView *GetMainMenuView();

	MainRebarView *GetMainRebarView();
	AddressBarView *GetAddressBarView();
	BookmarksToolbarView *GetBookmarksToolbarView();
	DrivesToolbarView *GetDrivesToolbarView();
	Applications::ApplicationToolbarView *GetApplicationToolbarView();

	StatusBarView *GetStatusBarView();

	void RedrawMenuBar();
	void FocusNextWindow(FocusChangeDirection direction);

private:
	void CreateMainRebarAndChildren();

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
	std::unique_ptr<AddressBarView> m_addressBarView;
	std::unique_ptr<BookmarksToolbarView> m_bookmarksToolbarView;
	std::unique_ptr<DrivesToolbarView> m_drivesToolbarView;
	std::unique_ptr<Applications::ApplicationToolbarView> m_applicationToolbarView;

	std::unique_ptr<StatusBarView> m_statusBarView;
};
