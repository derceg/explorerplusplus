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
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/XMLSettings.h"
#include <boost\algorithm\string.hpp>

const TCHAR CApplicationToolbarPersistentSettings::SETTING_NAME[] = _T("Name");
const TCHAR CApplicationToolbarPersistentSettings::SETTING_COMMAND[] = _T("Command");
const TCHAR CApplicationToolbarPersistentSettings::SETTING_SHOW_NAME_ON_TOOLBAR[] = _T("ShowNameOnToolbar");

CApplicationToolbar *CApplicationToolbar::Create(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance, IExplorerplusplus *pexpp)
{
	return new CApplicationToolbar(hParent, uIDStart, uIDEnd, hInstance, pexpp);
}

CApplicationToolbar::CApplicationToolbar(HWND hParent,UINT uIDStart,UINT uIDEnd,HINSTANCE hInstance,IExplorerplusplus *pexpp) :
CBaseWindow(CreateApplicationToolbar(hParent)),
m_hInstance(hInstance),
m_uIDStart(uIDStart),
m_uIDEnd(uIDEnd),
m_pexpp(pexpp)
{
	Initialize(hParent);
}

HWND CApplicationToolbar::CreateApplicationToolbar(HWND hParent)
{
	return CreateToolbar(hParent, WS_CHILD | WS_VISIBLE |
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_LIST |
		TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS |
		TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
}

void CApplicationToolbar::Initialize(HWND hParent)
{
	m_atps = &CApplicationToolbarPersistentSettings::GetInstance();

	SendMessage(m_hwnd,TB_BUTTONSTRUCTSIZE,static_cast<WPARAM>(sizeof(TBBUTTON)),0);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(NULL,&himlSmall);

	int iconWidth;
	int iconHeight;
	ImageList_GetIconSize(himlSmall, &iconWidth, &iconHeight);
	SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));

	SendMessage(m_hwnd,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(himlSmall));

	m_patd = new CApplicationToolbarDropHandler(m_hwnd, this);
	RegisterDragDrop(m_hwnd,m_patd);

	m_windowSubclasses.push_back(WindowSubclassWrapper(hParent, ParentWndProcStub,
		PARENT_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	AddButtonsToToolbar();

	m_connections.push_back(m_pexpp->AddToolbarContextMenuObserver(boost::bind(&CApplicationToolbar::OnToolbarContextMenuPreShow, this, _1, _2)));
}

CApplicationToolbar::~CApplicationToolbar()
{
	RevokeDragDrop(m_hwnd);
	m_patd->Release();
}

LRESULT CALLBACK CApplicationToolbar::ParentWndProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CApplicationToolbar *pat = reinterpret_cast<CApplicationToolbar *>(dwRefData);
	return pat->ParentWndProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CApplicationToolbar::ParentWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		if(LOWORD(wParam) >= m_uIDStart &&
			LOWORD(wParam) <= m_uIDEnd)
		{
			int iIndex = static_cast<int>(SendMessage(m_hwnd,TB_COMMANDTOINDEX,LOWORD(wParam),0));
			OpenItem(iIndex, NULL);
			return 0;
		}
		else
		{
			switch(LOWORD(wParam))
			{
			case IDM_APP_OPEN:
				OpenItem(m_RightClickItem, NULL);
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
		if(reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch(reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_RCLICK:
				{
					NMMOUSE *pnmm = reinterpret_cast<NMMOUSE *>(lParam);

					if(pnmm->dwItemSpec != -1)
					{
						int iIndex = static_cast<int>(SendMessage(m_hwnd,TB_COMMANDTOINDEX,pnmm->dwItemSpec,0));

						if(iIndex != -1)
						{
							m_RightClickItem = iIndex;

							HMENU RightClickMenu = GetSubMenu(LoadMenu(m_hInstance,
								MAKEINTRESOURCE(IDR_APPLICATIONTOOLBAR_MENU)),0);

							ClientToScreen(m_hwnd,&pnmm->pt);
							TrackPopupMenu(RightClickMenu,TPM_LEFTALIGN,
								pnmm->pt.x,pnmm->pt.y,0,m_hwnd,NULL);
						}

						return TRUE;
					}
				}
				break;

			case TBN_GETINFOTIP:
				{
					NMTBGETINFOTIP *pnmtbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

					int iIndex = static_cast<int>(SendMessage(m_hwnd,TB_COMMANDTOINDEX,pnmtbgit->iItem,0));
					ApplicationButton_t *Button = MapToolbarButtonToItem(iIndex);

					if(Button != NULL)
					{
						StringCchPrintf(pnmtbgit->pszText,pnmtbgit->cchTextMax,_T("%s\n%s"),
							Button->Name.c_str(),Button->Command.c_str());
					}

					return 0;
				}
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void CApplicationToolbar::AddButtonsToToolbar()
{
	for(const auto &Button : m_atps->m_Buttons)
	{
		AddButtonToToolbar(Button);
	}
}

void CApplicationToolbar::AddButtonToToolbar(const ApplicationButton_t &Button)
{
	ApplicationInfo_t ai = ProcessCommand(Button.Command);

	SHFILEINFO shfi;
	DWORD_PTR ret = SHGetFileInfo(ai.Application.c_str(),0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);

	/* Assign a generic icon if the file was not found. */
	if(ret == 0)
	{
		shfi.iIcon = 0;
	}

	TBBUTTON tbButton;
	tbButton.iBitmap	= shfi.iIcon;
	tbButton.idCommand	= m_uIDStart + Button.ID;
	tbButton.fsState	= TBSTATE_ENABLED;
	tbButton.fsStyle	= BTNS_AUTOSIZE|BTNS_SHOWTEXT;
	tbButton.dwData		= Button.ID;
	tbButton.iString	= NULL;

	if(Button.ShowNameOnToolbar)
	{
		tbButton.iString = reinterpret_cast<INT_PTR>(Button.Name.c_str());
	}
	else
	{
		tbButton.iString = reinterpret_cast<INT_PTR>(EMPTY_STRING);
	}

	SendMessage(m_hwnd,TB_ADDBUTTONS,static_cast<WPARAM>(1),reinterpret_cast<LPARAM>(&tbButton));
	UpdateToolbarBandSizing(GetParent(m_hwnd),m_hwnd);
}

void CApplicationToolbar::UpdateButton(int iItem)
{
	ApplicationButton_t *Button = MapToolbarButtonToItem(iItem);

	if(Button != NULL)
	{
		ApplicationInfo_t ai = ProcessCommand(Button->Command);

		SHFILEINFO shfi;
		DWORD_PTR ret = SHGetFileInfo(ai.Application.c_str(),0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);

		if(ret == 0)
		{
			shfi.iIcon = 0;
		}

		TBBUTTONINFO tbi;
		TCHAR Name[512];
		tbi.cbSize = sizeof(tbi);
		tbi.dwMask = TBIF_BYINDEX|TBIF_IMAGE|TBIF_TEXT;
		tbi.iImage = shfi.iIcon;

		if(Button->ShowNameOnToolbar)
		{
			StringCchCopy(Name,SIZEOF_ARRAY(Name),Button->Name.c_str());
			tbi.pszText = Name;
		}
		else
		{
			tbi.pszText = EMPTY_STRING;
		}

		SendMessage(m_hwnd,TB_SETBUTTONINFO,iItem,reinterpret_cast<LPARAM>(&tbi));
	}
}

void CApplicationToolbar::ShowNewItemDialog()
{
	ApplicationButton_t Button;
	Button.ShowNameOnToolbar = TRUE;

	CApplicationToolbarButtonDialog ApplicationToolbarButtonDialog(m_hInstance, m_hwnd, &Button, true);
	INT_PTR ret = ApplicationToolbarButtonDialog.ShowModalDialog();

	if(ret == 1)
	{
		Button.ID = m_atps->m_IDCounter++;
		m_atps->m_Buttons.push_back(Button);

		AddButtonToToolbar(Button);
	}
}

void CApplicationToolbar::AddNewItem(const std::wstring &name, const std::wstring &command,
	BOOL showNameOnToolbar)
{
	ApplicationButton_t button;
	bool success = m_atps->AddButton(name, command, showNameOnToolbar, &button);

	if(success)
	{
		AddButtonToToolbar(button);
	}
}

/* If any parameters are provided to this method,
they will be passed to the application along
with the default parameters (i.e. those attached
to the button). */
void CApplicationToolbar::OpenItem(int iItem, std::wstring *parameters)
{
	assert(iItem >= 0 && static_cast<size_t>(iItem) < m_atps->m_Buttons.size());

	ApplicationButton_t *Button = MapToolbarButtonToItem(iItem);

	if(Button != NULL)
	{
		ApplicationInfo_t ai = ProcessCommand(Button->Command);

		unique_pidl_absolute pidl;
		HRESULT hr = SHParseDisplayName(ai.Application.c_str(), nullptr, wil::out_param(pidl), 0, nullptr);

		if(SUCCEEDED(hr))
		{
			std::wstring combinedParameters = ai.Parameters;

			if(parameters != NULL && parameters->length() > 0)
			{
				combinedParameters.append(_T(" "));
				combinedParameters.append(*parameters);
			}

			m_pexpp->OpenFileItem(pidl.get(), combinedParameters.c_str());
		}
	}
}

/* Takes a command string inputted by the user, and splits it
up into two components: an application path (with any environment strings
expanded) and a parameter list.

Two supported styles:
1. "[command]" [parameters] (used if the command contains spaces)
2. [command] [parameters] */
CApplicationToolbar::ApplicationInfo_t CApplicationToolbar::ProcessCommand(const std::wstring &Command)
{
	ApplicationInfo_t ai;

	ai.Application = EMPTY_STRING;
	ai.Parameters = EMPTY_STRING;

	std::wstring TempCommand = Command;

	/* Remove leading/trailing spaces. */
	boost::trim(TempCommand);

	if(TempCommand.length() == 0)
	{
		return ai;
	}

	size_t SubstringStart;
	size_t SubstringEnd;
	size_t SubstringLength;

	if(TempCommand.at(0) == _T('\"'))
	{
		SubstringStart = 1;
		SubstringEnd = TempCommand.find(_T("\""),1);
	}
	else
	{
		SubstringStart = 0;
		SubstringEnd = TempCommand.find_first_of(_T(" "));
	}

	if(SubstringEnd != std::wstring::npos)
	{
		SubstringLength = SubstringEnd - SubstringStart;
	}
	else
	{
		SubstringLength = std::wstring::npos;
	}

	std::wstring TempApplication = TempCommand.substr(SubstringStart,SubstringLength);
	boost::trim(TempApplication);

	TCHAR ExpandedApplicationPath[MAX_PATH];
	MyExpandEnvironmentStrings(TempApplication.c_str(),
		ExpandedApplicationPath,SIZEOF_ARRAY(ExpandedApplicationPath));

	ai.Application = ExpandedApplicationPath;

	if(SubstringEnd != std::wstring::npos)
	{
		ai.Parameters = TempCommand.substr(SubstringEnd + 1);
		boost::trim(ai.Parameters);
	}
	else
	{
		ai.Parameters = EMPTY_STRING;
	}

	return ai;
}

void CApplicationToolbar::ShowItemProperties(int iItem)
{
	assert(iItem >= 0 && static_cast<size_t>(iItem) < m_atps->m_Buttons.size());

	ApplicationButton_t *Button = MapToolbarButtonToItem(iItem);

	if(Button != NULL)
	{
		CApplicationToolbarButtonDialog ApplicationToolbarButtonDialog(m_hInstance,
			m_hwnd, Button, false);
		INT_PTR ret = ApplicationToolbarButtonDialog.ShowModalDialog();

		if(ret == 1)
		{
			UpdateButton(iItem);
		}
	}
}

void CApplicationToolbar::DeleteItem(int iItem)
{
	assert(iItem >= 0 && static_cast<size_t>(iItem) < m_atps->m_Buttons.size());

	std::wstring message = ResourceHelper::LoadString(m_hInstance, IDS_APPLICATIONBUTTON_DELETE);

	int iMessageBoxReturn = MessageBox(m_hwnd,message.c_str(),
		NExplorerplusplus::APP_NAME,MB_YESNO|MB_ICONINFORMATION|MB_DEFBUTTON2);

	if(iMessageBoxReturn == IDYES)
	{
		TBBUTTON tbButton;
		LRESULT lResult = SendMessage(m_hwnd,TB_GETBUTTON,iItem,reinterpret_cast<LPARAM>(&tbButton));

		if(lResult)
		{
			int ID = static_cast<int>(tbButton.dwData);
			auto itr = std::find_if(m_atps->m_Buttons.begin(),m_atps->m_Buttons.end(),
				[ID](const ApplicationButton_t &Button){return Button.ID == ID;});

			if(itr != m_atps->m_Buttons.end())
			{
				m_atps->m_Buttons.erase(itr);
				SendMessage(m_hwnd,TB_DELETEBUTTON,iItem,0);
				UpdateToolbarBandSizing(GetParent(m_hwnd),m_hwnd);
			}
		}
	}
}

void CApplicationToolbar::OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow)
{
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

ApplicationButton_t *CApplicationToolbar::MapToolbarButtonToItem(int iIndex)
{
	if(iIndex == -1)
	{
		return NULL;
	}

	TBBUTTON tbButton;
	LRESULT lResult = SendMessage(m_hwnd,TB_GETBUTTON,iIndex,reinterpret_cast<LPARAM>(&tbButton));

	if(lResult)
	{
		int ID = static_cast<int>(tbButton.dwData);
		auto itr = std::find_if(m_atps->m_Buttons.begin(),m_atps->m_Buttons.end(),
			[ID](const ApplicationButton_t &Button){return Button.ID == ID;});

		if(itr != m_atps->m_Buttons.end())
		{
			return &(*itr);
		}
	}

	return NULL;
}

CApplicationToolbarPersistentSettings::CApplicationToolbarPersistentSettings() :
m_IDCounter(0)
{

}

CApplicationToolbarPersistentSettings& CApplicationToolbarPersistentSettings::GetInstance()
{
	static CApplicationToolbarPersistentSettings atps;
	return atps;
}

void CApplicationToolbarPersistentSettings::LoadRegistrySettings(HKEY hParentKey)
{
	TCHAR	szItemKey[256];
	int		i = 0;
	StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),_T("%d"),i);

	HKEY hKeyChild;
	LONG ReturnValue = RegOpenKeyEx(hParentKey,szItemKey,0,KEY_READ,&hKeyChild);

	while(ReturnValue == ERROR_SUCCESS)
	{
		TCHAR szName[512];
		TCHAR szCommand[512];
		BOOL bShowNameOnToolbar = TRUE;

		LONG lNameStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild, SETTING_NAME,
			szName, SIZEOF_ARRAY(szName));
		LONG lCommandStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild, SETTING_COMMAND,
			szCommand, SIZEOF_ARRAY(szCommand));
		NRegistrySettings::ReadDwordFromRegistry(hKeyChild, SETTING_SHOW_NAME_ON_TOOLBAR,
			reinterpret_cast<DWORD *>(&bShowNameOnToolbar));

		if(lNameStatus == ERROR_SUCCESS && lCommandStatus == ERROR_SUCCESS)
		{
			AddButton(szName,szCommand,bShowNameOnToolbar,NULL);
		}

		RegCloseKey(hKeyChild);

		i++;

		StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),_T("%d"),i);
		ReturnValue = RegOpenKeyEx(hParentKey,szItemKey,0,KEY_READ,&hKeyChild);
	}
}

void CApplicationToolbarPersistentSettings::SaveRegistrySettings(HKEY hParentKey)
{
	int index = 0;

	for(const auto &Button : m_Buttons)
	{
		TCHAR szKeyName[32];
		_itow_s(index,szKeyName,SIZEOF_ARRAY(szKeyName),10);

		HKEY hKeyChild;
		LONG ReturnValue = RegCreateKeyEx(hParentKey,szKeyName,0,NULL,
			REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKeyChild,NULL);

		if(ReturnValue == ERROR_SUCCESS)
		{
			NRegistrySettings::SaveStringToRegistry(hKeyChild, SETTING_NAME, Button.Name.c_str());
			NRegistrySettings::SaveStringToRegistry(hKeyChild, SETTING_COMMAND, Button.Command.c_str());
			NRegistrySettings::SaveDwordToRegistry(hKeyChild, SETTING_SHOW_NAME_ON_TOOLBAR, Button.ShowNameOnToolbar);

			index++;

			RegCloseKey(hKeyChild);
		}
	}
}

void CApplicationToolbarPersistentSettings::LoadXMLSettings(IXMLDOMNode *pNode)
{
	TCHAR szName[512];
	TCHAR szCommand[512];
	BOOL bShowNameOnToolbar = TRUE;

	BOOL bNameFound = FALSE;
	BOOL bCommandFound = FALSE;

	IXMLDOMNamedNodeMap *am = NULL;
	HRESULT hr = pNode->get_attributes(&am);

	if(FAILED(hr))
	{
		return;
	}

	long lChildNodes;
	am->get_length(&lChildNodes);

	for(int i = 0;i < lChildNodes;i++)
	{
		IXMLDOMNode *pAttributeNode = NULL;
		am->get_item(i,&pAttributeNode);

		BSTR bstrName;
		BSTR bstrValue;
		pAttributeNode->get_nodeName(&bstrName);
		pAttributeNode->get_text(&bstrValue);

		if(lstrcmpi(bstrName, SETTING_NAME) == 0)
		{
			StringCchCopy(szName,SIZEOF_ARRAY(szName),bstrValue);

			bNameFound = TRUE;
		}
		else if(lstrcmpi(bstrName, SETTING_COMMAND) == 0)
		{
			StringCchCopy(szCommand,SIZEOF_ARRAY(szCommand),bstrValue);

			bCommandFound = TRUE;
		}
		else if(lstrcmpi(bstrName, SETTING_SHOW_NAME_ON_TOOLBAR) == 0)
		{
			bShowNameOnToolbar = NXMLSettings::DecodeBoolValue(bstrValue);
		}
	}

	if(bNameFound && bCommandFound)
	{
		AddButton(szName,szCommand,bShowNameOnToolbar,NULL);
	}

	IXMLDOMNode *pNextSibling = NULL;
	hr = pNode->get_nextSibling(&pNextSibling);

	if(hr == S_OK)
	{
		hr = pNextSibling->get_nextSibling(&pNextSibling);

		if(hr == S_OK)
		{
			LoadXMLSettings(pNextSibling);
		}
	}
}

void CApplicationToolbarPersistentSettings::SaveXMLSettings(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pe)
{
	BSTR bstr_wsntt = SysAllocString(L"\n\t\t");

	for(const auto &Button : m_Buttons)
	{
		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);

		IXMLDOMElement *pParentNode = NULL;
		NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("ApplicationButton"),Button.Name.c_str());
		NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_COMMAND, Button.Command.c_str());
		NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SHOW_NAME_ON_TOOLBAR, NXMLSettings::EncodeBoolValue(Button.ShowNameOnToolbar));

		pParentNode->Release();
	}

	SysFreeString(bstr_wsntt);
}

bool CApplicationToolbarPersistentSettings::AddButton(const std::wstring &name, const std::wstring &command,
	BOOL showNameOnToolbar, ApplicationButton_t *buttonOut)
{
	if(name.length() == 0 ||
		command.length() == 0)
	{
		return false;
	}

	ApplicationButton_t button;
	button.Name = name;
	button.Command = command;
	button.ShowNameOnToolbar = showNameOnToolbar;
	button.ID = m_IDCounter++;
	m_Buttons.push_back(button);

	if(buttonOut != NULL)
	{
		*buttonOut = button;
	}

	return true;
}