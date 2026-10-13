// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserWindow.h"
#include "Tab.h"
#include "../Helper/DropTargetWindow.h"
#include "../Helper/WinRTBaseWrapper.h"
#include <boost/signals2.hpp>

struct Config;
class NavigationEvents;
class NavigationRequest;
class ResourceLoader;
class ShellBrowser;
class ShellBrowserEvents;
class TabEvents;
class WindowSubclass;

class BrowserView : private DropTargetInternal
{
public:
	enum class FocusChangeDirection
	{
		Previous,
		Next
	};

	static BrowserView *Create(HWND hwnd, BrowserWindow *browser, const Config *config,
		TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
		NavigationEvents *navigationEvents, const ResourceLoader *resourceLoader);

	void RedrawMenuBar();
	void FocusNextWindow(FocusChangeDirection direction);

private:
	BrowserView(HWND hwnd, BrowserWindow *browser, const Config *config, TabEvents *tabEvents,
		ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
		const ResourceLoader *resourceLoader);
	~BrowserView() = default;

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

	void OnNcDestroy();

	const HWND m_hwnd;
	BrowserWindow *const m_browser;
	const Config *const m_config;
	const ResourceLoader *const m_resourceLoader;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	winrt::com_ptr<DropTargetWindow> m_dropTargetWindow;
};
