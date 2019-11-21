// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationToolbarDropHandler.h"
#include "CoreInterface.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <MsXml2.h>
#include <objbase.h>
#include <vector>

class CApplicationToolbar;
class CApplicationToolbarDropHandler;

struct ApplicationButton_t
{
	std::wstring	Name;
	std::wstring	Command;
	BOOL			ShowNameOnToolbar;

	int				ID;
};

class CApplicationToolbarPersistentSettings
{
public:

	static CApplicationToolbarPersistentSettings &GetInstance();

	void	SaveRegistrySettings(HKEY hParentKey);
	void	LoadRegistrySettings(HKEY hParentKey);

	void	SaveXMLSettings(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pe);
	void	LoadXMLSettings(IXMLDOMNode *pNode);

private:

	friend CApplicationToolbar;

	static const TCHAR SETTING_NAME[];
	static const TCHAR SETTING_COMMAND[];
	static const TCHAR SETTING_SHOW_NAME_ON_TOOLBAR[];

	CApplicationToolbarPersistentSettings();

	CApplicationToolbarPersistentSettings(const CApplicationToolbarPersistentSettings &);
	CApplicationToolbarPersistentSettings & operator=(const CApplicationToolbarPersistentSettings &);

	bool AddButton(const std::wstring &name, const std::wstring &command,
		BOOL showNameOnToolbar, ApplicationButton_t *buttonOut);

	std::vector<ApplicationButton_t> m_Buttons;
	int m_IDCounter;
};

class CApplicationToolbar : public CBaseWindow
{
public:

	static CApplicationToolbar *Create(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance, IExplorerplusplus *pexpp);

	void				ShowNewItemDialog();
	void				AddNewItem(const std::wstring &name, const std::wstring &command, BOOL showNameOnToolbar);
	void				OpenItem(int iItem, std::wstring *parameters);
	void				ShowItemProperties(int iItem);
	void				DeleteItem(int iItem);

private:

	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	struct ApplicationInfo_t
	{
		std::wstring Application;
		std::wstring Parameters;
	};

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	CApplicationToolbar(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance, IExplorerplusplus *pexpp);
	~CApplicationToolbar();

	static HWND			CreateApplicationToolbar(HWND hParent);

	void				Initialize(HWND hParent);

	void				AddButtonsToToolbar();
	void				AddButtonToToolbar(const ApplicationButton_t &Button);
	void				UpdateButton(int iItem);

	void				OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow);

	ApplicationInfo_t	ProcessCommand(const std::wstring &Command);
	ApplicationButton_t	*MapToolbarButtonToItem(int index);

	HINSTANCE			m_hInstance;

	UINT				m_uIDStart;
	UINT				m_uIDEnd;

	int					m_RightClickItem;

	IExplorerplusplus	*m_pexpp;

	CApplicationToolbarDropHandler	*m_patd;

	CApplicationToolbarPersistentSettings	*m_atps;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection>	m_connections;
};