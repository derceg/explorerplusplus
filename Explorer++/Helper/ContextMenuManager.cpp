/******************************************************************
 *
 * Project: Helper
 * File: ContextMenuManager.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Manages context menu extensions.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "ContextMenuManager.h"
#include "ShellHelper.h"


/* The following steps are taken when showing shell
extensions on an existing menu:
1. Load shell extensions.
2. Build and show menu.
3. Pass selection to shell extension (if necessary).
4. Release shell extensions (also free the DLL's they reside in). */
CContextMenuManager::CContextMenuManager(ContextMenuTypes ContextMenuType,
	LPCITEMIDLIST pidlDirectory,IDataObject *pDataObject,IUnknown *pUnkSite)
{
	TCHAR *pszRegContext = NULL;

	switch(ContextMenuType)
	{
	case CMT_DIRECTORY_BACKGROUND_HANDLERS:
		pszRegContext = CMH_DIRECTORY_BACKGROUND;
		break;

	case CMT_DRAGDROP_HANDLERS:
		pszRegContext = CMH_DRAGDROP_HANDLERS;
		break;
	}

	LoadContextMenuHandlers(pszRegContext,&m_ContextMenuHandlers);

	/* Initialize the shell extensions, and extract
	an IContextMenu interface. */
	for each(auto ContextMenuHandler in m_ContextMenuHandlers)
	{
		IShellExtInit *pShellExtInit = NULL;
		HRESULT hr;

		IUnknown *pUnknown = ContextMenuHandler.pUnknown;

		hr = pUnknown->QueryInterface(IID_IShellExtInit,
			(void **)&pShellExtInit);

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

				hr = pUnknown->QueryInterface(IID_IObjectWithSite,(void **)&pObjectSite);

				if(SUCCEEDED(hr))
				{
					pObjectSite->SetSite(pUnkSite);
					pObjectSite->Release();
				}
			}

			hr = pUnknown->QueryInterface(IID_IContextMenu3,
				(void **)&pContextMenu3);
			MenuHandler.pContextMenuActual = pContextMenu3;

			if(FAILED(hr))
			{
				hr = pUnknown->QueryInterface(IID_IContextMenu2,
					(void **)&pContextMenu2);
				MenuHandler.pContextMenuActual = pContextMenu2;

				if(FAILED(hr))
				{
					hr = pUnknown->QueryInterface(IID_IContextMenu,
						(void **)&pContextMenu);
					MenuHandler.pContextMenuActual = pContextMenu;
				}
			}

			MenuHandler.pContextMenu = pContextMenu;
			MenuHandler.pContextMenu2 = pContextMenu2;
			MenuHandler.pContextMenu3 = pContextMenu3;

			MenuHandler.iStartID = 0;
			MenuHandler.iEndID = 0;

			m_MenuHandlers.push_back(MenuHandler);
		}
	}
}

CContextMenuManager::~CContextMenuManager()
{
	/* Release the IContextMenu interfaces. */
	for each(auto MenuHandler in m_MenuHandlers)
	{
		if(MenuHandler.pContextMenuActual != NULL)
		{
			MenuHandler.pContextMenuActual->Release();
		}
	}

	/* ...and free the necessary DLL's. */
	for each(auto ContextMenuHandler in m_ContextMenuHandlers)
	{
		ContextMenuHandler.pUnknown->Release();

		if(ContextMenuHandler.hDLL != NULL)
		{
			FreeLibrary(ContextMenuHandler.hDLL);
		}
	}
}

void CContextMenuManager::AddMenuEntries(HMENU hMenu,
	int iStartPos,int iMinID,int iMaxID)
{
	int iOffset = 0;

	/* Loop through each of the acquired IContextMenu
	interfaces and add the required menu items. */
	for(auto itr = m_MenuHandlers.begin();itr != m_MenuHandlers.end();itr++)
	{
		MenuHandler_t MenuHandler = *itr;
		HMENU hDummyMenu = CreatePopupMenu();

		HRESULT hr = MenuHandler.pContextMenuActual->QueryContextMenu(
			hDummyMenu,0,iMinID + iOffset,iMaxID,CMF_NORMAL|CMF_EXPLORE);

		if(HRESULT_SEVERITY(hr) == SEVERITY_SUCCESS)
		{
			int iCurrentOffset = HRESULT_CODE(hr);

			if(iCurrentOffset > 0)
			{
				MENUITEMINFO mii;
				int nTotalMenuItems = 0;

				/* Need to save ID offsets for this menu handler. */
				itr->iStartID = iMinID + iOffset;
				itr->iEndID = iMinID + iOffset + iCurrentOffset;

				int nMenuItems = GetMenuItemCount(hDummyMenu);
				UINT uDefault = GetMenuDefaultItem(hDummyMenu,FALSE,0);

				/* Take items from the dummy menu, and
				insert them into the real menu. Note
				that beginning or trailing separators
				will NOT be added. */
				for(int i = 0;i < nMenuItems;i++)
				{
					TCHAR szText[256];

					mii.cbSize		= sizeof(mii);
					mii.fMask		= MIIM_BITMAP|MIIM_CHECKMARKS|MIIM_DATA|
						MIIM_FTYPE|MIIM_ID|MIIM_STATE|MIIM_STRING|MIIM_SUBMENU;
					mii.dwTypeData	= szText;
					mii.cch			= SIZEOF_ARRAY(szText);
					GetMenuItemInfo(hDummyMenu,i,TRUE,&mii);

					if((i == 0 || i == (nMenuItems - 1)) &&
						(mii.fType & MFT_SEPARATOR) == MFT_SEPARATOR)
					{
						continue;
					}

					InsertMenuItem(hMenu,iStartPos + nTotalMenuItems,TRUE,&mii);

					/* If this menu item is the default on the dummy
					menu, make it the default on the actual menu. */
					if(uDefault != -1 && mii.wID == uDefault)
					{
						SetMenuDefaultItem(hMenu,mii.wID,FALSE);
					}

					if(mii.hSubMenu != NULL)
					{
						IContextMenu2 *pContextMenu2 = NULL;

						if(MenuHandler.pContextMenu2 != NULL)
						{
							pContextMenu2 = MenuHandler.pContextMenu2;
						}
						else if(MenuHandler.pContextMenu3 != NULL)
						{
							pContextMenu2 = MenuHandler.pContextMenu3;
						}

						if(pContextMenu2 != NULL)
						{
							pContextMenu2->HandleMenuMsg(WM_INITMENUPOPUP,
								(WPARAM)GetSubMenu(hMenu,iStartPos + nTotalMenuItems),
								(LPARAM)MAKEWORD(iStartPos + nTotalMenuItems,FALSE));
						}
					}

					nTotalMenuItems++;
				}

				/* Insert a trailing separator
				after the shell extension menus. */
				mii.cbSize		= sizeof(mii);
				mii.fMask		= MIIM_FTYPE;
				mii.fType		= MFT_SEPARATOR;
				InsertMenuItem(hMenu,iStartPos + nTotalMenuItems,TRUE,&mii);

				nTotalMenuItems++;

				iOffset += iCurrentOffset;
			}
		}
	}
}

void CContextMenuManager::HandleMenuEntry(HWND hwnd,int iCmd)
{
	for each(auto MenuHandler in m_MenuHandlers)
	{
		if(iCmd >= MenuHandler.iStartID &&
			iCmd < MenuHandler.iEndID)
		{
			CMINVOKECOMMANDINFO	cmici;

			cmici.cbSize		= sizeof(CMINVOKECOMMANDINFO);
			cmici.fMask			= 0;
			cmici.hwnd			= hwnd;
			cmici.lpVerb		= (LPCSTR)MAKEWORD(iCmd - MenuHandler.iStartID,0);
			cmici.lpParameters	= NULL;
			cmici.lpDirectory	= NULL;
			cmici.nShow			= SW_SHOW;

			MenuHandler.pContextMenuActual->InvokeCommand(&cmici);
			break;
		}
	}
}