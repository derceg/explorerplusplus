// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserWindow.h"
#include "CoreInterface.h"
#include "DefaultToolbarButtons.h"
#include "MainFontSetter.h"
#include "MainToolbarStorage.h"
#include "Tab.h"
#include "TabHistoryMenu.h"
#include "../Helper/SignalWrapper.h"
#include "../Helper/WindowSubclass.h"
#include <wil/resource.h>
#include <optional>
#include <unordered_map>

class AppServices;
class NavigationRequest;
class ResourceLoader;
class ShellBrowser;
class ShellIconLoader;

class MainToolbar
{
public:
	static MainToolbar *Create(HWND parent,
		const std::optional<MainToolbarStorage::MainToolbarButtons> &initialButtons,
		BrowserWindow *browser, AppServices *appServices, CoreInterface *coreInterface,
		ShellIconLoader *shellIconLoader);

	HWND GetHWND() const;

	void UpdateConfigDependentButtonStates();
	void UpdateToolbarButtonStates();

	MainToolbarStorage::MainToolbarButtons GetButtonsForStorage() const;

	void StartCustomization();

	// Signals
	SignalWrapper<MainToolbar, void()> sizeUpdatedSignal;

private:
	MainToolbar(HWND parent,
		const std::optional<MainToolbarStorage::MainToolbarButtons> &initialButtons,
		BrowserWindow *browser, AppServices *appServices, CoreInterface *coreInterface,
		ShellIconLoader *shellIconLoader);
	~MainToolbar() = default;

	static HWND CreateMainToolbar(HWND parent);

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent,
		const std::optional<MainToolbarStorage::MainToolbarButtons> &initialButtons);
	void SetTooolbarImageList();
	std::unordered_map<int, int> SetUpToolbarImageList(HIMAGELIST imageList, int iconSize,
		UINT dpi);
	std::vector<MainToolbarButton> GetDefaultButtons() const;
	void AddButtonsToToolbar(const std::vector<MainToolbarButton> &buttons);
	void AddButtonToToolbar(MainToolbarButton button);
	TBBUTTON GetToolbarButtonDetails(MainToolbarButton button) const;
	std::wstring GetToolbarButtonText(MainToolbarButton button) const;
	BYTE LookupToolbarButtonExtraStyles(MainToolbarButton button) const;
	int LookupToolbarButtonTextID(MainToolbarButton button) const;

	BOOL OnTBQueryInsert();
	BOOL OnTBQueryDelete();
	BOOL OnTBRestore();
	BOOL OnTBGetButtonInfo(LPARAM lParam);
	void OnTBReset();
	void OnTBChange();
	void OnTBGetInfoTip(LPARAM lParam);
	std::optional<std::wstring> MaybeGetCustomizedUpInfoTip();
	LRESULT OnDropDown(const NMTOOLBAR *nmtb);
	void ShowHistoryMenu(TabHistoryMenu::MenuType historyType, const POINT &ptScreen);
	void ShowUpNavigationMenu(const POINT &ptScreen);
	void ShowViewsMenu(const POINT &ptScreen);
	POINT GetMenuPositionForButton(MainToolbarButton button);

	void OnTabSelected(const Tab &tab);
	void OnNavigationCommitted(const NavigationRequest *request);
	void OnBrowserCommandTargetChanged();

	void UpdateToolbarButtonImageIndexes();

	void OnBrowserLifecycleStateChanged(BrowserWindow::LifecycleState updatedState);
	void OnUseLargeToolbarIconsUpdated(BOOL newValue);
	void OnShowFoldersUpdated(bool showFolders);

	void OnClipboardUpdate();
	void OnMButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keysDown);
	void OnMButtonUp(HWND hwnd, int x, int y, UINT keysDown);

	void OnFontOrDpiUpdated();

	const HWND m_hwnd;
	BrowserWindow *const m_browser;
	AppServices *const m_appServices;
	CoreInterface *const m_coreInterface;
	ShellIconLoader *const m_shellIconLoader;
	const Config *const m_config;
	const ResourceLoader *const m_resourceLoader;

	wil::unique_himagelist m_imageListSmall;
	wil::unique_himagelist m_imageListLarge;
	std::unordered_map<int, int> m_toolbarImageMapSmall;
	std::unordered_map<int, int> m_toolbarImageMapLarge;

	MainFontSetter m_fontSetter;
	MainFontSetter m_tooltipFontSetter;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	std::optional<int> m_middleButtonItem;
};
