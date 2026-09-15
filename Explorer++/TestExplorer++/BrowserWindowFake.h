// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserWindow.h"
#include "ShellBrowserFactoryFake.h"
#include "TabContainer.h"
#include "../Helper/Pidl.h"
#include <wil/resource.h>

class AppServices;
struct Config;
struct WindowStorageData;

class BrowserWindowFake : public BrowserWindow
{
public:
	BrowserWindowFake(const WindowStorageData &storageData, AppServices *appServices);

	// BrowserWindow
	HWND GetHWND() const override;
	BrowserCommandController *GetCommandController() override;
	BrowserPane *GetActivePane() const override;
	TabContainer *GetActiveTabContainer() override;
	const TabContainer *GetActiveTabContainer() const override;
	void FocusActiveTab() override;
	Tab *CreateTabFromPreservedTab(const PreservedTab *tab) override;
	void OpenDefaultItem(OpenFolderDisposition openFolderDisposition) override;
	void OpenItem(const std::wstring &itemPath,
		OpenFolderDisposition openFolderDisposition) override;
	void OpenItem(PCIDLIST_ABSOLUTE pidlItem, OpenFolderDisposition openFolderDisposition) override;
	void OpenFileItem(const std::wstring &itemPath, const std::wstring &parameters) override;
	void OpenFileItem(PCIDLIST_ABSOLUTE pidlItem, const std::wstring &parameters) override;
	ShellBrowser *GetActiveShellBrowser() override;
	const ShellBrowser *GetActiveShellBrowser() const override;
	void StartMainToolbarCustomization() override;
	std::optional<std::wstring> RequestMenuHelpText(HMENU menu, UINT id) const override;
	WindowStorageData GetStorageData() const override;
	bool IsActive() const override;
	void Activate() override;
	void TryClose() override;
	void Close() override;

	void NotifyBrowserClosing();

	using BrowserWindow::SetLifecycleState;

	// MenuHelpTextHost
	void NotifyTopLevelMenuShown() override;
	void NotifyTopLevelMenuClosed() override;
	void MenuItemSelected(HMENU menu, UINT itemId, UINT flags) override;
	boost::signals2::connection AddMenuHelpTextRequestObserver(
		const MenuHelpTextRequestSignal::slot_type &observer) override;

	void SetWorkspaceBounds(const RECT &bounds);
	[[nodiscard]] int AddTabAndReturnId(const std::wstring &path,
		const TabSettings &tabSettings = {}, PidlAbsolute *outputPidl = nullptr);
	Tab *AddTab(const std::wstring &path, const TabSettings &tabSettings = {},
		PidlAbsolute *outputPidl = nullptr);
	Tab *AddTab(const PidlAbsolute &pidl, const TabSettings &tabSettings = {});

private:
	static constexpr wchar_t CLASS_NAME[] = L"TestExplorer++BrowserWindowClass";

	static wil::unique_hwnd CreateBrowserWindow();
	static void RegisterBrowserWindowClass();

	const Config *const m_config;

	wil::unique_hwnd m_window;

	ShellBrowserFactoryFake m_shellBrowserFactory;
	TabContainer *const m_tabContainer;
};
