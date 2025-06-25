// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserWindow.h"
#include "ShellBrowserFactoryFake.h"
#include "TabContainer.h"
#include "TabNavigationMock.h"
#include "../Helper/PidlHelper.h"
#include <wil/resource.h>

class AcceleratorManager;
class BookmarkTree;
class CachedIcons;
struct Config;
class NavigationEvents;
class ResourceLoader;
class ShellBrowserEvents;
class Tab;
class TabEvents;

class BrowserWindowFake : public BrowserWindow
{
public:
	BrowserWindowFake(const Config *config, TabEvents *tabEvents,
		ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
		CachedIcons *cachedIcons, BookmarkTree *bookmarkTree,
		const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader);

	// BrowserWindow
	HWND GetHWND() const override;
	BrowserCommandController *GetCommandController() override;
	BrowserPane *GetActivePane() const override;
	TabContainer *GetActiveTabContainer() override;
	const TabContainer *GetActiveTabContainer() const override;
	void FocusActiveTab() override;
	Tab *CreateTabFromPreservedTab(const PreservedTab *tab) override;
	ShellBrowser *GetActiveShellBrowser() override;
	const ShellBrowser *GetActiveShellBrowser() const override;
	void StartMainToolbarCustomization() override;
	std::optional<std::wstring> RequestMenuHelpText(HMENU menu, UINT id) const override;
	WindowStorageData GetStorageData() const override;
	bool IsActive() const override;
	void Activate() override;
	void TryClose() override;
	void Close() override;

	using BrowserWindow::SetLifecycleState;

	// Navigator
	void OpenDefaultItem(OpenFolderDisposition openFolderDisposition) override;
	void OpenItem(const std::wstring &itemPath,
		OpenFolderDisposition openFolderDisposition) override;
	void OpenItem(PCIDLIST_ABSOLUTE pidlItem, OpenFolderDisposition openFolderDisposition) override;

	// MenuHelpTextHost
	boost::signals2::connection AddMenuHelpTextRequestObserver(
		const MenuHelpTextRequestSignal::slot_type &observer) override;

	[[nodiscard]] int AddTabAndReturnId(const std::wstring &path,
		const TabSettings &tabSettings = {}, PidlAbsolute *outputPidl = nullptr);
	Tab *AddTab(const std::wstring &path, const TabSettings &tabSettings = {},
		PidlAbsolute *outputPidl = nullptr);

private:
	static constexpr wchar_t CLASS_NAME[] = L"TestExplorer++BrowserWindowClass";

	static wil::unique_hwnd CreateBrowserWindow();
	static void RegisterBrowserWindowClass();

	const Config *const m_config;
	TabNavigationMock m_tabNavigation;

	wil::unique_hwnd m_window;

	ShellBrowserFactoryFake m_shellBrowserFactory;
	TabContainer *const m_tabContainer;
};
