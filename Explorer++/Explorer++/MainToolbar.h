// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "DefaultToolbarButtons.h"
#include "Navigation.h"
#include "TabContainerInterface.h"
#include "../Helper/BaseWindow.h"
#include <list>
#include <unordered_map>

struct Config;

class MainToolbar : public CBaseWindow
{
public:

	static MainToolbar *Create(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
		TabContainerInterface *tabContainerInterface, Navigation *navigation, std::shared_ptr<Config> config);

	void UpdateToolbarSize();
	void UpdateToolbarButtonStates();

private:

	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	MainToolbar(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
		TabContainerInterface *tabContainerInterface, Navigation *navigation,
		std::shared_ptr<Config> config);
	~MainToolbar();

	static HWND CreateMainToolbar(HWND parent);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);
	void SetInitialToolbarButtons();
	void AddButtonsToToolbar();
	void AddButtonToToolbar(int iButtonId);
	TBBUTTON GetToolbarButtonDetails(int iButtonId) const;
	void AddStringsToToolbar();
	void AddStringToToolbar(int iButtonId);
	void GetToolbarButtonText(int iButtonId, TCHAR *szText, int bufSize) const;
	int LookupToolbarButtonImage(int iButtonID) const;
	BYTE LookupToolbarButtonExtraStyles(int iButtonID) const;
	int LookupToolbarButtonTextID(int iButtonID) const;

	BOOL OnTBQueryInsert();
	BOOL OnTBQueryDelete();
	BOOL OnTBRestore();
	BOOL OnTBGetButtonInfo(LPARAM lParam);
	void OnTBReset();
	void OnTBGetInfoTip(LPARAM lParam);
	LRESULT OnTbnDropDown(LPARAM lParam);
	void ShowToolbarViewsDropdown();
	void CreateViewsMenu(POINT *ptOrigin);

	void OnTabSelected(const Tab &tab);
	void OnNavigationCompleted(const Tab &tab);

	HINSTANCE m_instance;
	IExplorerplusplus *m_pexpp;
	TabContainerInterface *m_tabContainerInterface;
	Navigation *m_navigation;
	std::shared_ptr<Config> m_config;

	HIMAGELIST m_himlSmall;
	HIMAGELIST m_himlLarge;
	std::unordered_map<int, int> m_toolbarStringMap;

	std::list<ToolbarButton_t> m_tbInitial;

	boost::signals2::connection m_tabSelectedConnection;
	boost::signals2::connection m_navigationCompletedConnection;
};