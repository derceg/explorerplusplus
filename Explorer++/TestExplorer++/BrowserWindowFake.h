// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserWindow.h"
#include "TabNavigationMock.h"
#include <wil/resource.h>
#include <memory>
#include <vector>

class NavigationEvents;
class Tab;
class TabEvents;

class BrowserWindowFake : public BrowserWindow
{
public:
	BrowserWindowFake(TabEvents *tabEvents, NavigationEvents *navigationEvents);
	~BrowserWindowFake();

	// BrowserWindow
	HWND GetHWND() const override;
	BrowserCommandController *GetCommandController() override;
	BrowserPane *GetActivePane() const override;
	void FocusActiveTab() override;
	void CreateTabFromPreservedTab(const PreservedTab *tab) override;
	ShellBrowser *GetActiveShellBrowser() override;
	const ShellBrowser *GetActiveShellBrowser() const override;
	void StartMainToolbarCustomization() override;
	std::optional<std::wstring> RequestMenuHelpText(HMENU menu, UINT id) const override;
	WindowStorageData GetStorageData() const override;
	bool IsActive() const override;
	void Activate() override;
	void FocusChanged() override;
	void TryClose() override;
	void Close() override;

	// Navigator
	void OpenDefaultItem(OpenFolderDisposition openFolderDisposition) override;
	void OpenItem(const std::wstring &itemPath,
		OpenFolderDisposition openFolderDisposition) override;
	void OpenItem(PCIDLIST_ABSOLUTE pidlItem, OpenFolderDisposition openFolderDisposition) override;

	// MenuHelpTextRequest
	boost::signals2::connection AddMenuHelpTextRequestObserver(
		const MenuHelpTextRequestSignal::slot_type &observer) override;

	Tab *AddTab();
	void ActivateTabAtIndex(size_t index);

private:
	static constexpr wchar_t CLASS_NAME[] = L"TestExplorer++BrowserWindowClass";

	static wil::unique_hwnd CreateBrowserWindow();
	static void RegisterBrowserWindowClass();

	void MaybeNavigateActiveShellBrowser(PCIDLIST_ABSOLUTE pidl);

	TabEvents *const m_tabEvents;
	NavigationEvents *const m_navigationEvents;
	TabNavigationMock m_tabNavigation;

	wil::unique_hwnd m_window;

	std::vector<std::unique_ptr<Tab>> m_tabs;
	size_t m_activeTabIndex = 0;
};
