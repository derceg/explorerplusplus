// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationToolbarDropHandler.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <objbase.h>
#include <vector>

class ApplicationToolbar;
class ApplicationToolbarDropHandler;
__interface IExplorerplusplus;

struct ApplicationButton
{
	std::wstring Name;
	std::wstring Command;
	BOOL ShowNameOnToolbar;

	int ID;
};

class ApplicationToolbarPersistentSettings
{
public:
	static ApplicationToolbarPersistentSettings &GetInstance();

	void SaveRegistrySettings(HKEY hParentKey);
	void LoadRegistrySettings(HKEY hParentKey);

	void SaveXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe);
	void LoadXMLSettings(IXMLDOMNode *pNode);

private:
	friend ApplicationToolbar;

	static const TCHAR SETTING_NAME[];
	static const TCHAR SETTING_COMMAND[];
	static const TCHAR SETTING_SHOW_NAME_ON_TOOLBAR[];

	ApplicationToolbarPersistentSettings();

	ApplicationToolbarPersistentSettings(const ApplicationToolbarPersistentSettings &);
	ApplicationToolbarPersistentSettings &operator=(const ApplicationToolbarPersistentSettings &);

	bool AddButton(const std::wstring &name, const std::wstring &command, BOOL showNameOnToolbar,
		ApplicationButton *buttonOut);

	std::vector<ApplicationButton> m_Buttons;
	int m_IDCounter;
};

class ApplicationToolbar : public BaseWindow
{
public:
	static ApplicationToolbar *Create(
		HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance, IExplorerplusplus *pexpp);

	void ShowNewItemDialog();
	void AddNewItem(const std::wstring &name, const std::wstring &command, BOOL showNameOnToolbar);
	void OpenItem(int iItem, std::wstring *parameters);
	void ShowItemProperties(int iItem);
	void DeleteItem(int iItem);

private:
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	ApplicationToolbar(
		HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance, IExplorerplusplus *pexpp);
	~ApplicationToolbar();

	static HWND CreateApplicationToolbar(HWND hParent);

	void Initialize(HWND hParent);

	void AddButtonsToToolbar();
	void AddButtonToToolbar(const ApplicationButton &Button);
	void UpdateButton(int iItem);

	void OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow, const POINT &pt);

	ApplicationButton *MapToolbarButtonToItem(int index);

	HINSTANCE m_hInstance;

	UINT m_uIDStart;
	UINT m_uIDEnd;

	int m_RightClickItem;

	IExplorerplusplus *m_pexpp;

	ApplicationToolbarDropHandler *m_patd;

	ApplicationToolbarPersistentSettings *m_atps;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};