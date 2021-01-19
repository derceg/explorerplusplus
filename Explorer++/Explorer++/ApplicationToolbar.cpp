// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Handles the application toolbar.
 *
 * Notes:
 * Settings structure:
 * <ApplicationToolbar>
 *	<name="App1" command="C:\...">
 *	<name="App2" command="D:\...">
 * </ApplicationToolbar>
 */

#include "stdafx.h"
#include "ApplicationToolbar.h"
#include "ApplicationToolbarButtonDialog.h"
#include "ApplicationToolbarDropHandler.h"
#include "ApplicationToolbarHelper.h"
#include "CoreInterface.h"
#include "DarkModeHelper.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/XMLSettings.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <wil/com.h>
#include <wil/resource.h>
#include <comdef.h>

const TCHAR ApplicationToolbarPersistentSettings::SETTING_NAME[] = _T("Name");
const TCHAR ApplicationToolbarPersistentSettings::SETTING_COMMAND[] = _T("Command");
const TCHAR ApplicationToolbarPersistentSettings::SETTING_SHOW_NAME_ON_TOOLBAR[] =
	_T("ShowNameOnToolbar");

using namespace ApplicationToolbarHelper;

ApplicationToolbar *ApplicationToolbar::Create(
	HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance, IExplorerplusplus *pexpp)
{
	return new ApplicationToolbar(hParent, uIDStart, uIDEnd, hInstance, pexpp);
}

ApplicationToolbar::ApplicationToolbar(
	HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance, IExplorerplusplus *pexpp) :
	BaseWindow(CreateApplicationToolbar(hParent)),
	m_hInstance(hInstance),
	m_uIDStart(uIDStart),
	m_uIDEnd(uIDEnd),
	m_pexpp(pexpp)
{
	Initialize(hParent);
}

HWND ApplicationToolbar::CreateApplicationToolbar(HWND hParent)
{
	return CreateToolbar(hParent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_DOUBLEBUFFER
			| TBSTYLE_EX_HIDECLIPPEDBUTTONS);
}

void ApplicationToolbar::Initialize(HWND hParent)
{
	m_atps = &ApplicationToolbarPersistentSettings::GetInstance();

	SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, static_cast<WPARAM>(sizeof(TBBUTTON)), 0);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(nullptr, &himlSmall);

	int iconWidth;
	int iconHeight;
	ImageList_GetIconSize(himlSmall, &iconWidth, &iconHeight);
	SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));

	SendMessage(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(himlSmall));

	m_patd = new ApplicationToolbarDropHandler(m_hwnd, this);
	RegisterDragDrop(m_hwnd, m_patd);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(
		hParent, ParentWndProcStub, PARENT_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	AddButtonsToToolbar();

	m_connections.push_back(m_pexpp->AddToolbarContextMenuObserver(
		boost::bind(&ApplicationToolbar::OnToolbarContextMenuPreShow, this, _1, _2, _3)));

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (darkModeHelper.IsDarkModeEnabled())
	{
		darkModeHelper.SetDarkModeForToolbarTooltips(m_hwnd);
	}
}

ApplicationToolbar::~ApplicationToolbar()
{
	RevokeDragDrop(m_hwnd);
	m_patd->Release();
}

LRESULT CALLBACK ApplicationToolbar::ParentWndProcStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pat = reinterpret_cast<ApplicationToolbar *>(dwRefData);
	return pat->ParentWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ApplicationToolbar::ParentWndProc(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) >= m_uIDStart && LOWORD(wParam) <= m_uIDEnd)
		{
			int iIndex =
				static_cast<int>(SendMessage(m_hwnd, TB_COMMANDTOINDEX, LOWORD(wParam), 0));
			OpenItem(iIndex, nullptr);
			return 0;
		}
		else
		{
			switch (LOWORD(wParam))
			{
			case IDM_APP_OPEN:
				OpenItem(m_RightClickItem, nullptr);
				return 0;

			case IDM_APP_NEW:
				ShowNewItemDialog();
				return 0;

			case IDM_APP_DELETE:
				DeleteItem(m_RightClickItem);
				return 0;

			case IDM_APP_PROPERTIES:
				ShowItemProperties(m_RightClickItem);
				return 0;
			}
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_RCLICK:
			{
				auto *pnmm = reinterpret_cast<NMMOUSE *>(lParam);

				if (pnmm->dwItemSpec != -1)
				{
					int iIndex = static_cast<int>(
						SendMessage(m_hwnd, TB_COMMANDTOINDEX, pnmm->dwItemSpec, 0));

					if (iIndex != -1)
					{
						m_RightClickItem = iIndex;

						HMENU rightClickMenu = GetSubMenu(
							LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_APPLICATIONTOOLBAR_MENU)), 0);

						ClientToScreen(m_hwnd, &pnmm->pt);
						TrackPopupMenu(rightClickMenu, TPM_LEFTALIGN, pnmm->pt.x, pnmm->pt.y, 0,
							m_hwnd, nullptr);
					}

					return TRUE;
				}
			}
			break;

			case TBN_GETINFOTIP:
			{
				auto *pnmtbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

				int iIndex =
					static_cast<int>(SendMessage(m_hwnd, TB_COMMANDTOINDEX, pnmtbgit->iItem, 0));
				ApplicationButton *button = MapToolbarButtonToItem(iIndex);

				if (button != nullptr)
				{
					StringCchPrintf(pnmtbgit->pszText, pnmtbgit->cchTextMax, _T("%s\n%s"),
						button->Name.c_str(), button->Command.c_str());
				}

				return 0;
			}
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void ApplicationToolbar::AddButtonsToToolbar()
{
	for (const auto &button : m_atps->m_Buttons)
	{
		AddButtonToToolbar(button);
	}
}

void ApplicationToolbar::AddButtonToToolbar(const ApplicationButton &Button)
{
	ApplicationInfo ai = ParseCommandString(Button.Command);

	SHFILEINFO shfi;
	DWORD_PTR ret =
		SHGetFileInfo(ai.application.c_str(), 0, &shfi, sizeof(shfi), SHGFI_SYSICONINDEX);

	/* Assign a generic icon if the file was not found. */
	if (ret == 0)
	{
		shfi.iIcon = 0;
	}

	TBBUTTON tbButton;
	tbButton.iBitmap = shfi.iIcon;
	tbButton.idCommand = m_uIDStart + Button.ID;
	tbButton.fsState = TBSTATE_ENABLED;
	tbButton.fsStyle = BTNS_AUTOSIZE | BTNS_SHOWTEXT;
	tbButton.dwData = Button.ID;

	if (Button.ShowNameOnToolbar)
	{
		tbButton.iString = reinterpret_cast<INT_PTR>(Button.Name.c_str());
	}
	else
	{
		tbButton.iString = reinterpret_cast<INT_PTR>(EMPTY_STRING);
	}

	SendMessage(m_hwnd, TB_ADDBUTTONS, static_cast<WPARAM>(1), reinterpret_cast<LPARAM>(&tbButton));
	UpdateToolbarBandSizing(GetParent(m_hwnd), m_hwnd);
}

void ApplicationToolbar::UpdateButton(int iItem)
{
	ApplicationButton *button = MapToolbarButtonToItem(iItem);

	if (button != nullptr)
	{
		ApplicationInfo ai = ParseCommandString(button->Command);

		SHFILEINFO shfi;
		DWORD_PTR ret =
			SHGetFileInfo(ai.application.c_str(), 0, &shfi, sizeof(shfi), SHGFI_SYSICONINDEX);

		if (ret == 0)
		{
			shfi.iIcon = 0;
		}

		TBBUTTONINFO tbi;
		TCHAR name[512];
		tbi.cbSize = sizeof(tbi);
		tbi.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
		tbi.iImage = shfi.iIcon;

		if (button->ShowNameOnToolbar)
		{
			WI_SetFlag(tbi.dwMask, TBIF_TEXT);

			StringCchCopy(name, SIZEOF_ARRAY(name), button->Name.c_str());
			tbi.pszText = name;
		}

		SendMessage(m_hwnd, TB_SETBUTTONINFO, iItem, reinterpret_cast<LPARAM>(&tbi));
	}
}

void ApplicationToolbar::ShowNewItemDialog()
{
	ApplicationButton button;
	button.ShowNameOnToolbar = TRUE;

	ApplicationToolbarButtonDialog applicationToolbarButtonDialog(
		m_hInstance, m_hwnd, &button, true);
	INT_PTR ret = applicationToolbarButtonDialog.ShowModalDialog();

	if (ret == 1)
	{
		button.ID = m_atps->m_IDCounter++;
		m_atps->m_Buttons.push_back(button);

		AddButtonToToolbar(button);
	}
}

void ApplicationToolbar::AddNewItem(
	const std::wstring &name, const std::wstring &command, BOOL showNameOnToolbar)
{
	ApplicationButton button;
	bool success = m_atps->AddButton(name, command, showNameOnToolbar, &button);

	if (success)
	{
		AddButtonToToolbar(button);
	}
}

/* If any parameters are provided to this method,
they will be passed to the application along
with the default parameters (i.e. those attached
to the button). */
void ApplicationToolbar::OpenItem(int iItem, std::wstring *parameters)
{
	assert(iItem >= 0 && static_cast<size_t>(iItem) < m_atps->m_Buttons.size());

	ApplicationButton *button = MapToolbarButtonToItem(iItem);

	if (!button)
	{
		return;
	}

	ApplicationInfo ai = ParseCommandString(button->Command);

	unique_pidl_absolute pidl;
	HRESULT hr =
		SHParseDisplayName(ai.application.c_str(), nullptr, wil::out_param(pidl), 0, nullptr);

	if (FAILED(hr))
	{
		std::wstring messageTemplate =
			ResourceHelper::LoadString(m_hInstance, IDS_APPLICATION_TOOLBAR_OPEN_ERROR);
		_com_error error(hr);
		std::wstring message =
			(boost::wformat(messageTemplate) % ai.application % error.ErrorMessage()).str();

		MessageBox(m_hwnd, message.c_str(), NExplorerplusplus::APP_NAME, MB_ICONWARNING | MB_OK);
		return;
	}

	std::wstring combinedParameters = ai.parameters;

	if (parameters != nullptr && parameters->length() > 0)
	{
		combinedParameters.append(_T(" "));
		combinedParameters.append(*parameters);
	}

	m_pexpp->OpenFileItem(pidl.get(), combinedParameters.c_str());
}

void ApplicationToolbar::ShowItemProperties(int iItem)
{
	assert(iItem >= 0 && static_cast<size_t>(iItem) < m_atps->m_Buttons.size());

	ApplicationButton *button = MapToolbarButtonToItem(iItem);

	if (button != nullptr)
	{
		ApplicationToolbarButtonDialog applicationToolbarButtonDialog(
			m_hInstance, m_hwnd, button, false);
		INT_PTR ret = applicationToolbarButtonDialog.ShowModalDialog();

		if (ret == 1)
		{
			UpdateButton(iItem);
		}
	}
}

void ApplicationToolbar::DeleteItem(int iItem)
{
	assert(iItem >= 0 && static_cast<size_t>(iItem) < m_atps->m_Buttons.size());

	std::wstring message = ResourceHelper::LoadString(m_hInstance, IDS_APPLICATIONBUTTON_DELETE);

	int iMessageBoxReturn = MessageBox(m_hwnd, message.c_str(), NExplorerplusplus::APP_NAME,
		MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);

	if (iMessageBoxReturn == IDYES)
	{
		TBBUTTON tbButton;
		LRESULT lResult =
			SendMessage(m_hwnd, TB_GETBUTTON, iItem, reinterpret_cast<LPARAM>(&tbButton));

		if (lResult)
		{
			int id = static_cast<int>(tbButton.dwData);
			auto itr = std::find_if(m_atps->m_Buttons.begin(), m_atps->m_Buttons.end(),
				[id](const ApplicationButton &Button) {
					return Button.ID == id;
				});

			if (itr != m_atps->m_Buttons.end())
			{
				m_atps->m_Buttons.erase(itr);
				SendMessage(m_hwnd, TB_DELETEBUTTON, iItem, 0);
				UpdateToolbarBandSizing(GetParent(m_hwnd), m_hwnd);
			}
		}
	}
}

void ApplicationToolbar::OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow, const POINT &pt)
{
	UNREFERENCED_PARAMETER(pt);

	if (sourceWindow != m_hwnd)
	{
		return;
	}

	std::wstring newText = ResourceHelper::LoadString(m_hInstance, IDS_APPLICATIONBUTTON_NEW);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.dwTypeData = newText.data();
	mii.wID = IDM_APP_NEW;

	InsertMenuItem(menu, IDM_TOOLBARS_CUSTOMIZE, FALSE, &mii);
}

ApplicationButton *ApplicationToolbar::MapToolbarButtonToItem(int iIndex)
{
	if (iIndex == -1)
	{
		return nullptr;
	}

	TBBUTTON tbButton;
	LRESULT lResult =
		SendMessage(m_hwnd, TB_GETBUTTON, iIndex, reinterpret_cast<LPARAM>(&tbButton));

	if (lResult)
	{
		int id = static_cast<int>(tbButton.dwData);
		auto itr = std::find_if(m_atps->m_Buttons.begin(), m_atps->m_Buttons.end(),
			[id](const ApplicationButton &Button) {
				return Button.ID == id;
			});

		if (itr != m_atps->m_Buttons.end())
		{
			return &(*itr);
		}
	}

	return nullptr;
}

ApplicationToolbarPersistentSettings::ApplicationToolbarPersistentSettings() : m_IDCounter(0)
{
}

ApplicationToolbarPersistentSettings &ApplicationToolbarPersistentSettings::GetInstance()
{
	static ApplicationToolbarPersistentSettings atps;
	return atps;
}

void ApplicationToolbarPersistentSettings::LoadRegistrySettings(HKEY hParentKey)
{
	TCHAR szItemKey[256];
	int i = 0;
	StringCchPrintf(szItemKey, SIZEOF_ARRAY(szItemKey), _T("%d"), i);

	HKEY hKeyChild;
	LONG returnValue = RegOpenKeyEx(hParentKey, szItemKey, 0, KEY_READ, &hKeyChild);

	while (returnValue == ERROR_SUCCESS)
	{
		TCHAR szName[512];
		TCHAR szCommand[512];
		BOOL bShowNameOnToolbar = TRUE;

		LONG lNameStatus =
			RegistrySettings::ReadString(hKeyChild, SETTING_NAME, szName, SIZEOF_ARRAY(szName));
		LONG lCommandStatus = RegistrySettings::ReadString(
			hKeyChild, SETTING_COMMAND, szCommand, SIZEOF_ARRAY(szCommand));
		RegistrySettings::ReadDword(hKeyChild, SETTING_SHOW_NAME_ON_TOOLBAR,
			reinterpret_cast<DWORD *>(&bShowNameOnToolbar));

		if (lNameStatus == ERROR_SUCCESS && lCommandStatus == ERROR_SUCCESS)
		{
			AddButton(szName, szCommand, bShowNameOnToolbar, nullptr);
		}

		RegCloseKey(hKeyChild);

		i++;

		StringCchPrintf(szItemKey, SIZEOF_ARRAY(szItemKey), _T("%d"), i);
		returnValue = RegOpenKeyEx(hParentKey, szItemKey, 0, KEY_READ, &hKeyChild);
	}
}

void ApplicationToolbarPersistentSettings::SaveRegistrySettings(HKEY hParentKey)
{
	int index = 0;

	for (const auto &button : m_Buttons)
	{
		TCHAR szKeyName[32];
		_itow_s(index, szKeyName, SIZEOF_ARRAY(szKeyName), 10);

		HKEY hKeyChild;
		LONG returnValue = RegCreateKeyEx(hParentKey, szKeyName, 0, nullptr,
			REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKeyChild, nullptr);

		if (returnValue == ERROR_SUCCESS)
		{
			RegistrySettings::SaveString(hKeyChild, SETTING_NAME, button.Name.c_str());
			RegistrySettings::SaveString(hKeyChild, SETTING_COMMAND, button.Command.c_str());
			RegistrySettings::SaveDword(
				hKeyChild, SETTING_SHOW_NAME_ON_TOOLBAR, button.ShowNameOnToolbar);

			index++;

			RegCloseKey(hKeyChild);
		}
	}
}

void ApplicationToolbarPersistentSettings::LoadXMLSettings(IXMLDOMNode *pNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
	HRESULT hr = pNode->get_attributes(&am);

	if (FAILED(hr))
	{
		return;
	}

	BOOL bNameFound = FALSE;
	BOOL bCommandFound = FALSE;
	TCHAR szName[512];
	TCHAR szCommand[512];
	BOOL bShowNameOnToolbar = TRUE;

	long lChildNodes;
	am->get_length(&lChildNodes);

	for (int i = 0; i < lChildNodes; i++)
	{
		wil::com_ptr_nothrow<IXMLDOMNode> pAttributeNode;
		am->get_item(i, &pAttributeNode);

		wil::unique_bstr bstrName;
		pAttributeNode->get_nodeName(&bstrName);

		wil::unique_bstr bstrValue;
		pAttributeNode->get_text(&bstrValue);

		if (lstrcmpi(bstrName.get(), SETTING_NAME) == 0)
		{
			StringCchCopy(szName, SIZEOF_ARRAY(szName), bstrValue.get());

			bNameFound = TRUE;
		}
		else if (lstrcmpi(bstrName.get(), SETTING_COMMAND) == 0)
		{
			StringCchCopy(szCommand, SIZEOF_ARRAY(szCommand), bstrValue.get());

			bCommandFound = TRUE;
		}
		else if (lstrcmpi(bstrName.get(), SETTING_SHOW_NAME_ON_TOOLBAR) == 0)
		{
			bShowNameOnToolbar = NXMLSettings::DecodeBoolValue(bstrValue.get());
		}
	}

	if (bNameFound && bCommandFound)
	{
		AddButton(szName, szCommand, bShowNameOnToolbar, nullptr);
	}

	wil::com_ptr_nothrow<IXMLDOMNode> pNextSibling;
	hr = pNode->get_nextSibling(&pNextSibling);

	if (hr == S_OK)
	{
		wil::com_ptr_nothrow<IXMLDOMNode> secondSibling;
		hr = pNextSibling->get_nextSibling(&secondSibling);

		if (hr == S_OK)
		{
			LoadXMLSettings(secondSibling.get());
		}
	}
}

void ApplicationToolbarPersistentSettings::SaveXMLSettings(
	IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe)
{
	auto bstr_wsntt = wil::make_bstr_nothrow(L"\n\t\t");

	for (const auto &button : m_Buttons)
	{
		NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe);

		wil::com_ptr_nothrow<IXMLDOMElement> pParentNode;
		NXMLSettings::CreateElementNode(
			pXMLDom, &pParentNode, pe, _T("ApplicationButton"), button.Name.c_str());
		NXMLSettings::AddAttributeToNode(
			pXMLDom, pParentNode.get(), SETTING_COMMAND, button.Command.c_str());
		NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), SETTING_SHOW_NAME_ON_TOOLBAR,
			NXMLSettings::EncodeBoolValue(button.ShowNameOnToolbar));
	}
}

bool ApplicationToolbarPersistentSettings::AddButton(const std::wstring &name,
	const std::wstring &command, BOOL showNameOnToolbar, ApplicationButton *buttonOut)
{
	if (name.length() == 0 || command.length() == 0)
	{
		return false;
	}

	ApplicationButton button;
	button.Name = name;
	button.Command = command;
	button.ShowNameOnToolbar = showNameOnToolbar;
	button.ID = m_IDCounter++;
	m_Buttons.push_back(button);

	if (buttonOut != nullptr)
	{
		*buttonOut = button;
	}

	return true;
}