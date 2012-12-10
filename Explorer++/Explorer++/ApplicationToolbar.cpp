/******************************************************************
 *
 * Project: Explorer++
 * File: ApplicationToolbar.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the application toolbar.
 *
 * Notes:
 * Settings structure:
 * <ApplicationToolbar>
 *	<name="App1" command="C:\...">
 *	<name="App2" command="D:\...">
 * </ApplicationToolbar>
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <boost\algorithm\string.hpp>
#include "Explorer++_internal.h"
#include "ApplicationToolbarDropHandler.h"
#include "ApplicationToolbarButtonDialog.h"
#include "ApplicationToolbar.h"
#include "MainResource.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"
#include "../Helper/Macros.h"


CApplicationToolbar::CApplicationToolbar(HWND hToolbar,UINT uIDStart,UINT uIDEnd,HINSTANCE hInstance,IExplorerplusplus *pexpp) :
m_hToolbar(hToolbar),
m_uIDStart(uIDStart),
m_uIDEnd(uIDEnd),
m_hInstance(hInstance),
m_pexpp(pexpp)
{
	Initialize();
}

CApplicationToolbar::~CApplicationToolbar()
{
	RemoveWindowSubclass(GetParent(m_hToolbar),ParentProcStub,PARENT_SUBCLASS_ID);

	RevokeDragDrop(m_hToolbar);
	m_patd->Release();
}

void CApplicationToolbar::Initialize()
{
	m_atps = &CApplicationToolbarPersistentSettings::GetInstance();

	SendMessage(m_hToolbar,TB_SETBITMAPSIZE,0,MAKELONG(16,16));
	SendMessage(m_hToolbar,TB_BUTTONSTRUCTSIZE,static_cast<WPARAM>(sizeof(TBBUTTON)),0);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(NULL,&himlSmall);
	SendMessage(m_hToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(himlSmall));

	AddButtonsToToolbar();

	m_patd = new CApplicationToolbarDropHandler(m_hToolbar);
	RegisterDragDrop(m_hToolbar,m_patd);

	SetWindowSubclass(GetParent(m_hToolbar),ParentProcStub,PARENT_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this));
}

LRESULT CALLBACK ParentProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CApplicationToolbar *pat = reinterpret_cast<CApplicationToolbar *>(dwRefData);

	return pat->ParentProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CApplicationToolbar::ParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		if(LOWORD(wParam) >= m_uIDStart &&
			LOWORD(wParam) <= m_uIDEnd)
		{
			int iIndex = static_cast<int>(SendMessage(m_hToolbar,TB_COMMANDTOINDEX,LOWORD(wParam),0));
			OpenItem(iIndex);
			return 0;
		}
		else
		{
			switch(LOWORD(wParam))
			{
			case IDM_APP_OPEN:
				OpenItem(m_RightClickItem);
				return 0;

			case IDM_APP_NEW:
				NewItem();
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
		if(reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hToolbar)
		{
			switch(reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_RCLICK:
				{
					NMMOUSE *pnmm = reinterpret_cast<NMMOUSE *>(lParam);

					if(pnmm->dwItemSpec != -1)
					{
						int iIndex = static_cast<int>(SendMessage(m_hToolbar,TB_COMMANDTOINDEX,pnmm->dwItemSpec,0));

						if(iIndex != -1)
						{
							m_RightClickItem = iIndex;

							HMENU RightClickMenu = GetSubMenu(LoadMenu(m_hInstance,
								MAKEINTRESOURCE(IDR_APPLICATIONTOOLBAR_MENU)),0);

							ClientToScreen(m_hToolbar,&pnmm->pt);
							TrackPopupMenu(RightClickMenu,TPM_LEFTALIGN,
								pnmm->pt.x,pnmm->pt.y,0,m_hToolbar,NULL);
						}

						return TRUE;
					}
				}
				break;

			case TBN_GETINFOTIP:
				{
					NMTBGETINFOTIP *pnmtbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

					int iIndex = static_cast<int>(SendMessage(m_hToolbar,TB_COMMANDTOINDEX,pnmtbgit->iItem,0));
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
	for each(auto Button in m_atps->m_Buttons)
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
	tbButton.fsStyle	= BTNS_AUTOSIZE;
	tbButton.dwData		= Button.ID;
	tbButton.iString	= NULL;

	if(Button.ShowNameOnToolbar)
	{
		tbButton.fsStyle |= BTNS_SHOWTEXT;
		tbButton.iString = reinterpret_cast<INT_PTR>(Button.Name.c_str());
	}

	SendMessage(m_hToolbar,TB_ADDBUTTONS,static_cast<WPARAM>(1),reinterpret_cast<LPARAM>(&tbButton));
	UpdateToolbarBandSizing(GetParent(m_hToolbar),m_hToolbar);
}

void CApplicationToolbar::UpdateButton(int iItem)
{
	ApplicationButton_t *Button = MapToolbarButtonToItem(iItem);

	if(Button != NULL)
	{
		ApplicationInfo_t ai = ProcessCommand(Button->Command);

		SHFILEINFO shfi;
		SHGetFileInfo(ai.Application.c_str(),0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);

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

		SendMessage(m_hToolbar,TB_SETBUTTONINFO,iItem,reinterpret_cast<LPARAM>(&tbi));
	}
}

void CApplicationToolbar::NewItem()
{
	ApplicationButton_t Button;
	Button.ShowNameOnToolbar = TRUE;

	CApplicationToolbarButtonDialog ApplicationToolbarButtonDialog(m_hInstance,
		IDD_EDITAPPLICATIONBUTTON,m_hToolbar,&Button,true);
	INT_PTR ret = ApplicationToolbarButtonDialog.ShowModalDialog();

	if(ret == 1)
	{
		Button.ID = m_atps->m_IDCounter++;
		m_atps->m_Buttons.push_back(Button);

		AddButtonToToolbar(Button);
	}
}

void CApplicationToolbar::OpenItem(int iItem)
{
	ApplicationButton_t *Button = MapToolbarButtonToItem(iItem);

	if(Button != NULL)
	{
		ApplicationInfo_t ai = ProcessCommand(Button->Command);

		LPITEMIDLIST pidl = NULL;
		HRESULT hr = GetIdlFromParsingName(ai.Application.c_str(),&pidl);

		if(SUCCEEDED(hr))
		{
			m_pexpp->OpenFileItem(pidl,ai.Parameters.c_str());
			CoTaskMemFree(pidl);
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
	ApplicationButton_t *Button = MapToolbarButtonToItem(iItem);

	if(Button != NULL)
	{
		CApplicationToolbarButtonDialog ApplicationToolbarButtonDialog(m_hInstance,
			IDD_EDITAPPLICATIONBUTTON,m_hToolbar,Button,false);
		INT_PTR ret = ApplicationToolbarButtonDialog.ShowModalDialog();

		if(ret == 1)
		{
			UpdateButton(iItem);
		}
	}
}

void CApplicationToolbar::DeleteItem(int iItem)
{
	TCHAR szInfoMsg[128];
	LoadString(m_hInstance,IDS_APPLICATIONBUTTON_DELETE,
		szInfoMsg,SIZEOF_ARRAY(szInfoMsg));

	int iMessageBoxReturn = MessageBox(m_hToolbar,szInfoMsg,
		NExplorerplusplus::WINDOW_NAME,MB_YESNO|MB_ICONINFORMATION|MB_DEFBUTTON2);

	if(iMessageBoxReturn == IDYES)
	{
		TBBUTTON tbButton;
		LRESULT lResult = SendMessage(m_hToolbar,TB_GETBUTTON,iItem,reinterpret_cast<LPARAM>(&tbButton));

		if(lResult)
		{
			int ID = static_cast<int>(tbButton.dwData);
			auto itr = std::find_if(m_atps->m_Buttons.begin(),m_atps->m_Buttons.end(),
				[ID](const ApplicationButton_t &Button){return Button.ID == ID;});

			if(itr != m_atps->m_Buttons.end())
			{
				m_atps->m_Buttons.erase(itr);
				SendMessage(m_hToolbar,TB_DELETEBUTTON,iItem,0);
				UpdateToolbarBandSizing(GetParent(m_hToolbar),m_hToolbar);
			}
		}
	}
}

ApplicationButton_t *CApplicationToolbar::MapToolbarButtonToItem(int iIndex)
{
	if(iIndex == -1)
	{
		return NULL;
	}

	TBBUTTON tbButton;
	LRESULT lResult = SendMessage(m_hToolbar,TB_GETBUTTON,iIndex,reinterpret_cast<LPARAM>(&tbButton));

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

CApplicationToolbarPersistentSettings::~CApplicationToolbarPersistentSettings()
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

		LONG lNameStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild,_T("Name"),
			szName,SIZEOF_ARRAY(szName));
		LONG lCommandStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild,_T("Command"),
			szCommand,SIZEOF_ARRAY(szCommand));
		NRegistrySettings::ReadDwordFromRegistry(hKeyChild,_T("ShowNameOnToolbar"),
			reinterpret_cast<DWORD *>(&bShowNameOnToolbar));

		if(lNameStatus == ERROR_SUCCESS && lCommandStatus == ERROR_SUCCESS)
		{
			AddButton(szName,szCommand,bShowNameOnToolbar);
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

	for each(auto Button in m_Buttons)
	{
		TCHAR szKeyName[32];
		_itow_s(index,szKeyName,SIZEOF_ARRAY(szKeyName),10);

		HKEY hKeyChild;
		LONG ReturnValue = RegCreateKeyEx(hParentKey,szKeyName,0,NULL,
			REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKeyChild,NULL);

		if(ReturnValue == ERROR_SUCCESS)
		{
			NRegistrySettings::SaveStringToRegistry(hKeyChild,_T("Name"),Button.Name.c_str());
			NRegistrySettings::SaveStringToRegistry(hKeyChild,_T("Command"),Button.Command.c_str());
			NRegistrySettings::SaveDwordToRegistry(hKeyChild,_T("ShowNameOnToolbar"),Button.ShowNameOnToolbar);

			index++;

			RegCloseKey(hKeyChild);
		}
	}
}

void CApplicationToolbarPersistentSettings::LoadXMLSettings(MSXML2::IXMLDOMNode *pNode)
{
	TCHAR szName[512];
	TCHAR szCommand[512];
	BOOL bShowNameOnToolbar = TRUE;

	BOOL bNameFound = FALSE;
	BOOL bCommandFound = FALSE;

	MSXML2::IXMLDOMNamedNodeMap *am = NULL;
	HRESULT hr = pNode->get_attributes(&am);

	if(FAILED(hr))
	{
		return;
	}

	long lChildNodes;
	am->get_length(&lChildNodes);

	for(int i = 0;i < lChildNodes;i++)
	{
		MSXML2::IXMLDOMNode *pAttributeNode = NULL;
		am->get_item(i,&pAttributeNode);

		BSTR bstrName;
		BSTR bstrValue;
		pAttributeNode->get_nodeName(&bstrName);
		pAttributeNode->get_text(&bstrValue);

		if(lstrcmpi(bstrName,L"Name") == 0)
		{
			StringCchCopy(szName,SIZEOF_ARRAY(szName),bstrValue);

			bNameFound = TRUE;
		}
		else if(lstrcmpi(bstrName,L"Command") == 0)
		{
			StringCchCopy(szCommand,SIZEOF_ARRAY(szCommand),bstrValue);

			bCommandFound = TRUE;
		}
		else if(lstrcmpi(bstrName,L"ShowNameOnToolbar") == 0)
		{
			bShowNameOnToolbar = NXMLSettings::DecodeBoolValue(bstrValue);
		}
	}

	if(bNameFound && bCommandFound)
	{
		AddButton(szName,szCommand,bShowNameOnToolbar);
	}

	MSXML2::IXMLDOMNode *pNextSibling = NULL;
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

void CApplicationToolbarPersistentSettings::SaveXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pe)
{
	BSTR bstr_wsntt = SysAllocString(L"\n\t\t");

	for each(auto Button in m_Buttons)
	{
		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);

		MSXML2::IXMLDOMElement *pParentNode = NULL;
		NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("ApplicationButton"),Button.Name.c_str());
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Command"),Button.Command.c_str());
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ShowNameOnToolbar"),NXMLSettings::EncodeBoolValue(Button.ShowNameOnToolbar));

		pParentNode->Release();
	}

	SysFreeString(bstr_wsntt);
}

void CApplicationToolbarPersistentSettings::AddButton(std::wstring Name,std::wstring Command,BOOL ShowNameOnToolbar)
{
	ApplicationButton_t Button;
	Button.Name = Name;
	Button.Command = Command;
	Button.ShowNameOnToolbar = ShowNameOnToolbar;
	Button.ID = m_IDCounter++;

	m_Buttons.push_back(Button);
}