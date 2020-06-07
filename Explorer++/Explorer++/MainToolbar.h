// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DefaultToolbarButtons.h"
#include "IconResourceLoader.h"
#include "Tab.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <wil/com.h>
#include <wil/resource.h>
#include <unordered_map>

struct Config;
__interface IExplorerplusplus;
class MainToolbar;

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
	std::vector<ToolbarButton> m_toolbarButtons;
};

class MainToolbar : public BaseWindow
{
public:

	static MainToolbar *Create(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
		std::shared_ptr<Config> config);

	void UpdateConfigDependentButtonStates();
	void UpdateToolbarButtonStates();

private:

	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	enum class HistoryType
	{
		Back,
		Forward
	};

	MainToolbar(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
		std::shared_ptr<Config> config);
	~MainToolbar() = default;

	static HWND CreateMainToolbar(HWND parent);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);
	void SetTooolbarImageList();
	static std::unordered_map<int, int> SetUpToolbarImageList(HIMAGELIST imageList,
		IconResourceLoader *iconResourceLoader, int iconSize, UINT dpi);
	void AddButtonsToToolbar(const std::vector<ToolbarButton> &buttons);
	void AddButtonToToolbar(ToolbarButton button);
	TBBUTTON GetToolbarButtonDetails(ToolbarButton button) const;
	void AddStringsToToolbar();
	void AddStringToToolbar(ToolbarButton button);
	void GetToolbarButtonText(ToolbarButton button, TCHAR *szText, int bufSize) const;
	BYTE LookupToolbarButtonExtraStyles(ToolbarButton button) const;
	int LookupToolbarButtonTextID(ToolbarButton button) const;

	BOOL OnTBQueryInsert();
	BOOL OnTBQueryDelete();
	BOOL OnTBRestore();
	BOOL OnTBGetButtonInfo(LPARAM lParam);
	void OnTBReset();
	void OnTBChange();
	void OnTBGetInfoTip(LPARAM lParam);
	LRESULT OnTbnDropDown(const NMTOOLBAR *nmtb);
	void ShowHistoryMenu(HistoryType historyType, const POINT &pt);
	void ShowToolbarViewsDropdown();
	void CreateViewsMenu(POINT *ptOrigin);

	void OnTabSelected(const Tab &tab);
	void OnNavigationCompleted(const Tab &tab);

	void UpdateToolbarButtonImageIndexes();

	void OnUseLargeToolbarIconsUpdated(BOOL newValue);

	MainToolbarPersistentSettings *m_persistentSettings;

	HINSTANCE m_instance;
	IExplorerplusplus *m_pexpp;
	std::shared_ptr<Config> m_config;

	DpiCompatibility m_dpiCompat;
	wil::com_ptr<IImageList> m_systemImageList;
	wil::unique_hbitmap m_defaultFolderIconBitmap;
	wil::unique_himagelist m_imageListSmall;
	wil::unique_himagelist m_imageListLarge;
	std::unordered_map<int, int> m_toolbarImageMapSmall;
	std::unordered_map<int, int> m_toolbarImageMapLarge;
	std::unordered_map<int, int> m_toolbarStringMap;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};