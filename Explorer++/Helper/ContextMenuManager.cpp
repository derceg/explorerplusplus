// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * This class is designed to show an existing menu, with
 * has been modified to also show menu items inserted
 * by shell extensions.
 */

#include "stdafx.h"
#include <vector>
#include "Helper.h"
#include "ContextMenuManager.h"
#include "ShellHelper.h"
#include "Macros.h"


const TCHAR CContextMenuManager::CMH_FOLDER_DRAG_AND_DROP[] = _T("Folder\\ShellEx\\DragDropHandlers");
const TCHAR CContextMenuManager::CMH_DIRECTORY_BACKGROUND[] = _T("Directory\\Background\\shellex\\ContextMenuHandlers");
const TCHAR CContextMenuManager::CMH_DIRECTORY_DRAG_AND_DROP[] = _T("Directory\\shellex\\DragDropHandlers");

/* The following steps are taken when showing shell
extensions on an existing menu:
1. Load shell extensions.
2. Build and show menu.
3. Pass selection to shell extension (if necessary).
4. Release shell extensions (also free the DLL's they reside in). */
CContextMenuManager::CContextMenuManager(ContextMenuType_t ContextMenuType,
	PCIDLIST_ABSOLUTE pidlDirectory,IDataObject *pDataObject,IUnknown *pUnkSite,
	const std::vector<std::wstring> &blacklistedCLSIDEntries)
{
	ItemType_t ItemType = GetItemType(pidlDirectory);

	const TCHAR *pszRegContext = NULL;

	switch(ItemType)
	{
	case ITEM_TYPE_FOLDER:
		if(ContextMenuType == CONTEXT_MENU_TYPE_DRAG_AND_DROP)
		{
			pszRegContext = CMH_FOLDER_DRAG_AND_DROP;
		}
		break;

	case ITEM_TYPE_DIRECTORY:
		if(ContextMenuType == CONTEXT_MENU_TYPE_BACKGROUND)
		{
			pszRegContext = CMH_DIRECTORY_BACKGROUND;
		}
		else if(ContextMenuType == CONTEXT_MENU_TYPE_DRAG_AND_DROP)
		{
			pszRegContext = CMH_DIRECTORY_DRAG_AND_DROP;
		}
		break;
	}

	if(pszRegContext == NULL)
	{
		return;
	}

	BOOL bRet = LoadContextMenuHandlers(pszRegContext, m_ContextMenuHandlers, blacklistedCLSIDEntries);

	if(!bRet)
	{
		return;
	}

	/* Initialize the shell extensions, and extract
	an IContextMenu interface. */
	for(const auto &ContextMenuHandler : m_ContextMenuHandlers)
	{
		IShellExtInit *pShellExtInit = NULL;
		HRESULT hr;

		IUnknown *pUnknown = ContextMenuHandler.pUnknown;

		hr = pUnknown->QueryInterface(IID_PPV_ARGS(&pShellExtInit));

		if(SUCCEEDED(hr))
		{
			MenuHandler_t MenuHandler;
			IContextMenu *pContextMenu = NULL;
			IContextMenu2 *pContextMenu2 = NULL;
			IContextMenu3 *pContextMenu3 = NULL;

			pShellExtInit->Initialize(pidlDirectory,pDataObject,NULL);
			pShellExtInit->Release();

			if(pUnkSite != NULL)
			{
				IObjectWithSite *pObjectSite = NULL;

				hr = pUnknown->QueryInterface(IID_PPV_ARGS(&pObjectSite));

				if(SUCCEEDED(hr))
				{
					pObjectSite->SetSite(pUnkSite);
					pObjectSite->Release();
				}
			}

			hr = pUnknown->QueryInterface(IID_PPV_ARGS(&pContextMenu3));
			MenuHandler.pContextMenuActual = pContextMenu3;

			if(FAILED(hr) || pContextMenu3 == NULL)
			{
				hr = pUnknown->QueryInterface(IID_PPV_ARGS(&pContextMenu2));
				MenuHandler.pContextMenuActual = pContextMenu2;

				if(FAILED(hr) || pContextMenu2 == NULL)
				{
					hr = pUnknown->QueryInterface(IID_PPV_ARGS(&pContextMenu));
					MenuHandler.pContextMenuActual = pContextMenu;
				}
			}

			MenuHandler.pContextMenu = pContextMenu;
			MenuHandler.pContextMenu2 = pContextMenu2;
			MenuHandler.pContextMenu3 = pContextMenu3;

			MenuHandler.uStartID = 0;
			MenuHandler.uEndID = 0;

			m_MenuHandlers.push_back(MenuHandler);
		}
	}
}

CContextMenuManager::~CContextMenuManager()
{
	/* Release the IContextMenu interfaces. */
	for(auto MenuHandler : m_MenuHandlers)
	{
		if(MenuHandler.pContextMenuActual != NULL)
		{
			MenuHandler.pContextMenuActual->Release();
		}
	}

	/* ...and free the necessary DLL's. */
	for(auto ContextMenuHandler : m_ContextMenuHandlers)
	{
		ContextMenuHandler.pUnknown->Release();

		if(ContextMenuHandler.hDLL != NULL)
		{
			FreeLibrary(ContextMenuHandler.hDLL);
		}
	}
}

bool CContextMenuManager::ShowMenu(HWND hwnd,HMENU hMenu,
	UINT uIDPrevious,UINT uMinID,UINT uMaxID,const POINT &pt,
	CStatusBar &StatusBar)
{
	if(hwnd == NULL ||
		hMenu == NULL ||
		uIDPrevious == 0 ||
		uMaxID <= uMinID)
	{
		return false;
	}

	m_uMinID = uMinID;
	m_uMaxID = uMaxID;
	m_pStatusBar = &StatusBar;

	AddMenuEntries(hMenu,uIDPrevious,uMinID,uMaxID);

	BOOL bRet = SetWindowSubclass(hwnd,ContextMenuHookProc,CONTEXT_MENU_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this));

	if(!bRet)
	{
		return false;
	}

	UINT uCmd = TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_RIGHTBUTTON|
		TPM_VERTICAL|TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL);

	RemoveWindowSubclass(hwnd,ContextMenuHookProc,CONTEXT_MENU_SUBCLASS_ID);

	if(uCmd != 0)
	{
		if(uCmd >= uMinID && uCmd <= uMaxID)
		{
			InvokeMenuEntry(hwnd,uCmd);
		}
		else
		{
			SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(uCmd,0),NULL);
		}
	}

	return true;
}

void CContextMenuManager::AddMenuEntries(HMENU hMenu,
	UINT uIDPrevious,int iMinID,int iMaxID)
{
	/* Shell extension menu items will be inserted
	directly *after* this item. This is used rather
	than a direct position, as some shell extensions
	(such as the "Catalyst Context Menu") insert themselves
	at positions other than the one specified. */
	int iStartPos = GetMenuItemPos(hMenu,uIDPrevious) + 1;

	/* First, add two separators to the menu; these
	will "bracket" the shell extensions. */
	MENUITEMINFO mii;
	mii.cbSize	= sizeof(mii);
	mii.fMask	= MIIM_FTYPE;
	mii.fType	= MFT_SEPARATOR;
	InsertMenuItem(hMenu,iStartPos,TRUE,&mii);
	InsertMenuItem(hMenu,iStartPos + 1,TRUE,&mii);

	iStartPos++;

	int iOffset = 0;

	/* Loop through each of the acquired IContextMenu interfaces and add
	the required menu items. */
	for(auto itr = m_MenuHandlers.begin();itr != m_MenuHandlers.end();itr++)
	{
		if(itr->pContextMenuActual != NULL)
		{
			HRESULT hr = itr->pContextMenuActual->QueryContextMenu(
				hMenu,iStartPos,iMinID + iOffset,iMaxID,
				CMF_NORMAL|CMF_EXPLORE);

			if(HRESULT_SEVERITY(hr) == SEVERITY_SUCCESS)
			{
				int iCurrentOffset = HRESULT_CODE(hr);

				if(iCurrentOffset > 0)
				{
					/* Need to save ID offsets for this menu handler. */
					itr->uStartID = iMinID + iOffset;
					itr->uEndID = iMinID + iOffset + iCurrentOffset;

					iOffset += iCurrentOffset;

					iStartPos = GetMenuItemPos(hMenu,uIDPrevious) + 2;
				}
			}
		}
	}

	RemoveDuplicateSeperators(hMenu);
}

void CContextMenuManager::RemoveDuplicateSeperators(HMENU hMenu)
{
	std::vector<int> DeletionVector;

	bool bPreviousItemSeperator = false;

	for(int i = 0;i < GetMenuItemCount(hMenu);i++)
	{
		MENUITEMINFO mii;
		mii.cbSize	= sizeof(mii);
		mii.fMask	= MIIM_FTYPE;
		GetMenuItemInfo(hMenu,i,TRUE,&mii);

		bool bCurrentItemSeparator = (mii.fType & MFT_SEPARATOR) ? true : false;

		if(bPreviousItemSeperator && bCurrentItemSeparator)
		{
			DeletionVector.push_back(i);
		}

		bPreviousItemSeperator = bCurrentItemSeparator;
	}

	for(auto itr = DeletionVector.rbegin();itr != DeletionVector.rend();itr++)
	{
		DeleteMenu(hMenu,*itr,MF_BYPOSITION);
	}
}

LRESULT CALLBACK CContextMenuManager::ContextMenuHookProc(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CContextMenuManager *pcmm = reinterpret_cast<CContextMenuManager *>(dwRefData);

	switch(uMsg)
	{
	case WM_INITMENUPOPUP:
	case WM_MEASUREITEM:
	case WM_DRAWITEM:
	case WM_MENUCHAR:
		{
			LRESULT lRes;
			HRESULT hr = pcmm->HandleMenuMessage(uMsg,wParam,lParam,lRes);

			if(SUCCEEDED(hr))
			{
				if(uMsg == WM_INITMENUPOPUP)
				{
					return 0;
				}
				else if(uMsg == WM_MEASUREITEM ||
					uMsg == WM_DRAWITEM)
				{
					return TRUE;
				}
				else
				{
					return lRes;
				}
			}
		}
		break;

	case WM_MENUSELECT:
		{
			if(HIWORD(wParam) == 0xFFFF && lParam == 0)
			{
				pcmm->m_pStatusBar->HandleStatusBarMenuClose();
			}
			else
			{
				pcmm->m_pStatusBar->HandleStatusBarMenuOpen();

				UINT uCmd = LOWORD(wParam);

				if(uCmd >= pcmm->m_uMinID && uCmd <= pcmm->m_uMaxID)
				{
					TCHAR szHelperText[512];

					HRESULT hr = pcmm->GetMenuHelperText(LOWORD(wParam),szHelperText,SIZEOF_ARRAY(szHelperText));

					if(hr == S_OK)
					{
						pcmm->m_pStatusBar->SetPartText(0,szHelperText);
						return 0;
					}
				}
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

HRESULT CContextMenuManager::HandleMenuMessage(UINT uMsg,WPARAM wParam,
	LPARAM lParam,LRESULT &lRes)
{
	UINT uItemID = static_cast<UINT>(-1);
	bool bContextMenu3Required = false;

	switch(uMsg)
	{
	case WM_INITMENUPOPUP:
		uItemID = GetMenuItemID(reinterpret_cast<HMENU>(wParam),0);
		break;

	case WM_MEASUREITEM:
		uItemID = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam)->itemID;
		break;

	case WM_DRAWITEM:
		uItemID = reinterpret_cast<DRAWITEMSTRUCT *>(lParam)->itemID;
		break;

	case WM_MENUCHAR:
		uItemID = GetMenuItemID(reinterpret_cast<HMENU>(lParam),0);
		bContextMenu3Required = true;
		break;
	}

	HRESULT hr = E_FAIL;

	if(uItemID != -1)
	{
		for(auto MenuHandler : m_MenuHandlers)
		{
			if(uItemID >= MenuHandler.uStartID &&
				uItemID < MenuHandler.uEndID)
			{
				if(MenuHandler.pContextMenu3 != NULL)
				{
					hr = MenuHandler.pContextMenu3->HandleMenuMsg2(uMsg,wParam,lParam,&lRes);
				}
				else if(MenuHandler.pContextMenu2 != NULL && !bContextMenu3Required)
				{
					hr = MenuHandler.pContextMenu2->HandleMenuMsg(uMsg,wParam,lParam);
				}

				break;
			}
		}
	}

	return hr;
}

/* See http://blogs.msdn.com/b/oldnewthing/archive/2004/09/28/235242.aspx for potential
issues with this. */
HRESULT CContextMenuManager::GetMenuHelperText(UINT uID,TCHAR *szText,UINT cchMax)
{
	HRESULT hr = E_FAIL;

	for(auto MenuHandler : m_MenuHandlers)
	{
		if(uID >= MenuHandler.uStartID &&
			uID < MenuHandler.uEndID)
		{
			if(MenuHandler.pContextMenuActual != NULL)
			{
				hr = MenuHandler.pContextMenuActual->GetCommandString(uID - MenuHandler.uStartID,
					GCS_HELPTEXT,NULL,reinterpret_cast<LPSTR>(szText),cchMax);
			}

			break;
		}
	}

	return hr;
}

int CContextMenuManager::GetMenuItemPos(HMENU hMenu,UINT uID)
{
	int iPos = -1;

	for(int i = 0;i < GetMenuItemCount(hMenu);i++)
	{
		if(GetMenuItemID(hMenu,i) == uID)
		{
			iPos = i;
			break;
		}
	}

	return iPos;
}

void CContextMenuManager::InvokeMenuEntry(HWND hwnd,UINT uCmd)
{
	for(auto MenuHandler : m_MenuHandlers)
	{
		if(uCmd >= MenuHandler.uStartID &&
			uCmd < MenuHandler.uEndID)
		{
			CMINVOKECOMMANDINFO	cmici;
			cmici.cbSize		= sizeof(CMINVOKECOMMANDINFO);
			cmici.fMask			= 0;
			cmici.hwnd			= hwnd;
			cmici.lpVerb		= reinterpret_cast<LPCSTR>(MAKEWORD(uCmd - MenuHandler.uStartID,0));
			cmici.lpParameters	= NULL;
			cmici.lpDirectory	= NULL;
			cmici.nShow			= SW_SHOW;

			MenuHandler.pContextMenuActual->InvokeCommand(&cmici);
			break;
		}
	}
}

CContextMenuManager::ItemType_t CContextMenuManager::GetItemType(PCIDLIST_ABSOLUTE pidl)
{
	SFGAOF Attributes = SFGAO_FOLDER|SFGAO_FILESYSTEM;
	GetItemAttributes(pidl,&Attributes);

	if((Attributes & SFGAO_FOLDER) &&
		(Attributes & SFGAO_FILESYSTEM))
	{
		return ITEM_TYPE_DIRECTORY;
	}
	else if((Attributes & SFGAO_FOLDER) &&
		!(Attributes & SFGAO_FILESYSTEM))
	{
		return ITEM_TYPE_FOLDER;
	}
	else
	{
		return ITEM_TYPE_FILE;
	}
}