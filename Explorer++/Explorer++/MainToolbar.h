// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "DefaultToolbarButtons.h"
#include "HistoryMenu.h"
#include "IconResourceLoader.h"
#include "MainFontSetter.h"
#include "SignalWrapper.h"
#include "Tab.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <wil/com.h>
#include <wil/resource.h>
#include <optional>
#include <unordered_map>

class BrowserWindow;
struct Config;
class IconFetcher;
class MainToolbar;
struct NavigateParams;

class MainToolbarPersistentSettings
{
public:
	static MainToolbarPersistentSettings &GetInstance();

	void LoadXMLSettings(IXMLDOMNode *pNode);
	void SaveXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe);

private:
	friend MainToolbar;

	MainToolbarPersistentSettings();

	MainToolbarPersistentSettings(const MainToolbarPersistentSettings &);
	MainToolbarPersistentSettings &operator=(const MainToolbarPersistentSettings &);

	// The current set of toolbar buttons.
	std::vector<MainToolbarButton> m_toolbarButtons;
};

class MainToolbar : public BaseWindow
{
public:
	static MainToolbar *Create(HWND parent, HINSTANCE resourceInstance,
		BrowserWindow *browserWindow, CoreInterface *coreInterface, IconFetcher *iconFetcher,
		std::shared_ptr<Config> config);

	void UpdateConfigDependentButtonStates();
	void UpdateToolbarButtonStates();

	// Signals
	SignalWrapper<MainToolbar, void()> sizeUpdatedSignal;

private:
	MainToolbar(HWND parent, HINSTANCE resourceInstance, BrowserWindow *browserWindow,
		CoreInterface *coreInterface, IconFetcher *iconFetcher, std::shared_ptr<Config> config);
	~MainToolbar();

	static HWND CreateMainToolbar(HWND parent);

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);
	void SetTooolbarImageList();
	static std::unordered_map<int, int> SetUpToolbarImageList(HIMAGELIST imageList,
		IconResourceLoader *iconResourceLoader, int iconSize, UINT dpi);
	void AddButtonsToToolbar(const std::vector<MainToolbarButton> &buttons);
	void AddButtonToToolbar(MainToolbarButton button);
	TBBUTTON GetToolbarButtonDetails(MainToolbarButton button) const;
	void AddStringsToToolbar();
	void AddStringToToolbar(MainToolbarButton button);
	void GetToolbarButtonText(MainToolbarButton button, TCHAR *szText, int bufSize) const;
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
	LRESULT OnTbnDropDown(const NMTOOLBAR *nmtb);
	void ShowHistoryMenu(HistoryMenu::MenuType historyType);
	void ShowUpNavigationMenu();
	void ShowToolbarViewsMenu();
	void CreateViewsMenu(POINT *ptOrigin);

	void OnTabSelected(const Tab &tab);
	void OnNavigationCommitted(const Tab &tab, const NavigateParams &navigateParams);
	void OnFocusChanged();

	void UpdateToolbarButtonImageIndexes();

	void OnUseLargeToolbarIconsUpdated(BOOL newValue);
	void OnShowFoldersUpdated(bool showFolders);

	void OnClipboardUpdate();
	void OnMButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keysDown);
	void OnMButtonUp(HWND hwnd, int x, int y, UINT keysDown);

	void OnFontOrDpiUpdated();

	MainToolbarPersistentSettings *m_persistentSettings;

	HINSTANCE m_resourceInstance;
	BrowserWindow *m_browserWindow = nullptr;
	CoreInterface *m_coreInterface = nullptr;
	IconFetcher *m_iconFetcher = nullptr;
	std::shared_ptr<Config> m_config;

	wil::unique_himagelist m_imageListSmall;
	wil::unique_himagelist m_imageListLarge;
	std::unordered_map<int, int> m_toolbarImageMapSmall;
	std::unordered_map<int, int> m_toolbarImageMapLarge;
	std::unordered_map<int, int> m_toolbarStringMap;

	MainFontSetter m_fontSetter;
	MainFontSetter m_tooltipFontSetter;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	std::optional<int> m_middleButtonItem;
};
