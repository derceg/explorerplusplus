// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "DefaultToolbarButtons.h"
#include "IconResourceLoader.h"
#include "Navigation.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/DpiCompatibility.h"
#include <wil/resource.h>
#include <unordered_map>

struct Config;

class MainToolbar : public CBaseWindow
{
public:

	static MainToolbar *Create(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
		Navigation *navigation, std::shared_ptr<Config> config);

	void UpdateToolbarSize();
	void UpdateToolbarButtonStates();

private:

	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	MainToolbar(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
		Navigation *navigation, std::shared_ptr<Config> config);
	~MainToolbar();

	static HWND CreateMainToolbar(HWND parent);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);
	void SetTooolbarImageList();
	static std::unordered_map<int, int> SetUpToolbarImageList(HIMAGELIST imageList,
		IconResourceLoader *iconResourceLoader, int iconSize, UINT dpi);
	void AddButtonsToToolbar(const std::vector<ToolbarButton_t> &buttons);
	void AddButtonToToolbar(int iButtonId);
	TBBUTTON GetToolbarButtonDetails(int iButtonId) const;
	void AddStringsToToolbar();
	void AddStringToToolbar(int iButtonId);
	void GetToolbarButtonText(int iButtonId, TCHAR *szText, int bufSize) const;
	BYTE LookupToolbarButtonExtraStyles(int iButtonID) const;
	int LookupToolbarButtonTextID(int iButtonID) const;

	BOOL OnTBQueryInsert();
	BOOL OnTBQueryDelete();
	BOOL OnTBRestore();
	BOOL OnTBGetButtonInfo(LPARAM lParam);
	void OnTBReset();
	void OnTBChange();
	void OnTBGetInfoTip(LPARAM lParam);
	LRESULT OnTbnDropDown(LPARAM lParam);
	void ShowToolbarViewsDropdown();
	void CreateViewsMenu(POINT *ptOrigin);

	void OnTabSelected(const Tab &tab);
	void OnNavigationCompleted(const Tab &tab);

	void UpdateToolbarButtonImageIndexes();

	HINSTANCE m_instance;
	IExplorerplusplus *m_pexpp;
	Navigation *m_navigation;
	std::shared_ptr<Config> m_config;

	DpiCompatibility m_dpiCompat;
	wil::unique_himagelist m_imageListSmall;
	wil::unique_himagelist m_imageListLarge;
	std::unordered_map<int, int> m_toolbarImageMapSmall;
	std::unordered_map<int, int> m_toolbarImageMapLarge;
	std::unordered_map<int, int> m_toolbarStringMap;

	// The current set of toolbar buttons.
	std::vector<ToolbarButton_t> m_toolbarButtons;

	std::vector<boost::signals2::scoped_connection> m_connections;
};