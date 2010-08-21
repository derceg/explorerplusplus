/******************************************************************
 *
 * Project: Explorer++
 * File: ShellContextMenuHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles shell context menus (such as file
 * context menus, and the new menu).
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Misc.h"
#include "Explorer++.h"


#define MENU_OPEN_IN_NEW_TAB	(MAX_SHELL_MENU_ID + 1)
#define MENU_OPEN_FILE_LOCATION	(MAX_SHELL_MENU_ID + 2)

CContainer	*g_pContainer = NULL;

HRESULT CContainer::CreateFileContextMenu(HWND hwnd,LPITEMIDLIST pidlParent,
POINT MousePos,UINT uFrom,LPCITEMIDLIST *ppidl,int nFiles,BOOL bRename,BOOL bExtended)
{
	assert(pidlParent != NULL);
	assert(nFiles > 0);

	IShellFolder		*pDesktopFolder = NULL;
	IShellFolder		*pShellParentFolder = NULL;
	IShellFolder		*pShellFolder = NULL;
	IContextMenu		*pContext = NULL;
	IContextMenu		*pActualContext = NULL;
	LPITEMIDLIST		pidlRelative = NULL;
	HMENU				hMenu;
	MENUITEMINFO		mii;
	CMINVOKECOMMANDINFO	cmici;
	TCHAR				szCmd[64];
	SFGAOF				FileAttributes = 0;
	HRESULT				hr;
	INT					Cmd;

	if(ppidl == NULL)
	{
		hr = SHBindToParent(pidlParent,IID_IShellFolder,(void **)&pShellParentFolder,
		(LPCITEMIDLIST *)&pidlRelative);

		if(SUCCEEDED(hr))
		{
			hr = pShellParentFolder->GetUIObjectOf(m_hContainer,1,
			(LPCITEMIDLIST *)&pidlRelative,IID_IContextMenu,0,
			(LPVOID *)&pContext);

			if(SUCCEEDED(hr))
			{
				FileAttributes = SFGAO_FOLDER;

				pShellParentFolder->Release();
				pShellParentFolder = NULL;
			}
		}
	}
	else
	{
		if(IsNamespaceRoot(pidlParent))
		{
			hr = SHGetDesktopFolder(&pShellFolder);
		}
		else
		{
			SHGetDesktopFolder(&pDesktopFolder);
			
			hr = pDesktopFolder->BindToObject(pidlParent,NULL,IID_IShellFolder,(LPVOID *)&pShellFolder);

			pDesktopFolder->Release();
			pDesktopFolder = NULL;
		}

		if(SUCCEEDED(hr))
		{
			hr = pShellFolder->GetUIObjectOf(m_hContainer,nFiles,
			(LPCITEMIDLIST *)ppidl,IID_IContextMenu,0,
			(LPVOID *)&pContext);

			if(SUCCEEDED(hr))
			{
				if(nFiles == 1)
				{
					FileAttributes = SFGAO_FOLDER;

					pShellFolder->GetAttributesOf(1,(LPCITEMIDLIST *)ppidl,&FileAttributes);
				}

				pShellFolder->Release();
				pShellFolder = NULL;
			}
		}
	}

	if(SUCCEEDED(hr))
	{
		m_pShellContext3 = NULL;
		m_pShellContext2 = NULL;
		m_pShellContext = NULL;

		/* First, try to get IContextMenu3, then IContextMenu2, and if neither of these
		are available, IContextMenu. */
		hr = pContext->QueryInterface(IID_IContextMenu3,(LPVOID *)&m_pShellContext3);
		pActualContext = m_pShellContext3;

		if(FAILED(hr))
		{
			hr = pContext->QueryInterface(IID_IContextMenu2,(LPVOID *)&m_pShellContext2);
			pActualContext = m_pShellContext2;

			if(FAILED(hr))
			{
				hr = pContext->QueryInterface(IID_IContextMenu,(LPVOID *)&m_pShellContext);
				pActualContext = m_pShellContext;
			}
		}

		if(SUCCEEDED(hr))
		{
			hMenu = CreatePopupMenu();
			m_hMenu = hMenu;
			m_bMixedMenu = FALSE;

			UINT uFlags;

			uFlags = CMF_NORMAL;

			if(bExtended)
				uFlags |= CMF_EXTENDEDVERBS;

			if(bRename)
				uFlags |= CMF_CANRENAME;

			pActualContext->QueryContextMenu(hMenu,0,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,
				uFlags);

			if(uFrom == FROM_LISTVIEW || uFrom == FROM_TREEVIEW ||
				uFrom == FROM_DRIVEBAR)
			{
				if(nFiles == 1)
				{
					if(FileAttributes & SFGAO_FOLDER)
					{
						mii.cbSize		= sizeof(MENUITEMINFO);
						mii.fMask		= MIIM_STRING|MIIM_ID;
						mii.wID			= MENU_OPEN_IN_NEW_TAB;
						mii.dwTypeData	= _T("Open in New Tab");
						InsertMenuItem(hMenu,1,TRUE,&mii);
					}
				}
			}
			else if(uFrom == FROM_SEARCH)
			{
				mii.cbSize		= sizeof(MENUITEMINFO);
				mii.fMask		= MIIM_STRING|MIIM_ID;
				mii.wID			= MENU_OPEN_FILE_LOCATION;
				mii.dwTypeData	= _T("Open file location");
				InsertMenuItem(hMenu,1,TRUE,&mii);
			}

			g_pContainer = this;

			if(m_pShellContext3 != NULL || m_pShellContext2 != NULL)
			{
				/* Subclass the owner window, so that the shell can handle menu messages. */
				DefaultMainWndProc = (WNDPROC)SetWindowLongPtr(hwnd,GWLP_WNDPROC,
					(LONG_PTR)ShellMenuHookProcStub);
			}

			Cmd = TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RETURNCMD,MousePos.x,MousePos.y,
			0,hwnd,NULL);

			if(m_pShellContext3 != NULL || m_pShellContext2 != NULL)
			{
				/* Restore previous window procedure. */
				SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG_PTR)DefaultMainWndProc);
			}

			if(Cmd >= MIN_SHELL_MENU_ID && Cmd <= MAX_SHELL_MENU_ID)
			{
				hr = pActualContext->GetCommandString(Cmd - MIN_SHELL_MENU_ID,GCS_VERB,
				NULL,(LPSTR)szCmd,SIZEOF_ARRAY(szCmd));

				if(StrCmpI(szCmd,_T("open")) == 0)
				{
					/* If ppidl is NULL, open the item specified by pidlParent
					in the current listview. If ppidl is not NULL, open each
					of the items specified in ppidl. */
					if(ppidl == NULL)
					{
						OpenItem(pidlParent,FALSE,FALSE);
					}
					else
					{
						LPITEMIDLIST	pidlComplete = NULL;
						int				i = 0;

						for(i = 0;i < nFiles;i++)
						{
							pidlComplete = ILCombine(pidlParent,ppidl[i]);

							OpenItem(pidlComplete,FALSE,FALSE);

							CoTaskMemFree(pidlComplete);
						}
					}

					m_bTreeViewOpenInNewTab = TRUE;

					return 0;
				}
				else if(StrCmpI(szCmd,_T("rename")) == 0)
				{
					if(uFrom == FROM_LISTVIEW)
					{
						OnFileRename();
					}
					else if(uFrom == FROM_TREEVIEW)
					{
						OnTreeViewFileRename();
					}
				}
				else if(StrCmpI(szCmd,_T("copy")) == 0)
				{
					if(uFrom == FROM_LISTVIEW)
					{
						OnListViewCopy(TRUE);
					}
					else if(uFrom == FROM_TREEVIEW)
					{
						OnTreeViewCopy(TRUE);
					}
				}
				else if(StrCmpI(szCmd,_T("cut")) == 0)
				{
					if(uFrom == FROM_LISTVIEW)
					{
						OnListViewCopy(FALSE);
					}
					else if(uFrom == FROM_TREEVIEW)
					{
						OnTreeViewCopy(FALSE);
					}
				}
				else
				{
					cmici.cbSize		= sizeof(CMINVOKECOMMANDINFO);
					cmici.fMask			= 0;
					cmici.hwnd			= m_hContainer;
					cmici.lpVerb		= (LPCSTR)MAKEWORD(Cmd - MIN_SHELL_MENU_ID,0);
					cmici.lpParameters	= NULL;
					cmici.lpDirectory	= NULL;
					cmici.nShow			= SW_SHOW;

					hr = pActualContext->InvokeCommand(&cmici);
				}
			}
			else
			{
				/* This is a custom menu item (e.g. Open
				in New Tab). */
				switch(Cmd)
				{
					case MENU_OPEN_IN_NEW_TAB:
						{
							LPITEMIDLIST pidlComplete;
							TCHAR szParsingPath[MAX_PATH];
							BOOL bOpenInNewTab;

							if(ppidl != NULL)
							{
								pidlComplete = ILCombine(pidlParent,*ppidl);

								bOpenInNewTab = FALSE;
							}
							else
							{
								pidlComplete = ILClone(pidlParent);

								bOpenInNewTab = TRUE;
							}

							GetDisplayName(pidlComplete,szParsingPath,SHGDN_FORPARSING);
							BrowseFolder(szParsingPath,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);

							m_bTreeViewOpenInNewTab = TRUE;

							CoTaskMemFree(pidlComplete);
						}
						break;

					case MENU_OPEN_FILE_LOCATION:
						{
							TCHAR szFileName[MAX_PATH];

							BrowseFolder(pidlParent,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);

							GetDisplayName((LPITEMIDLIST)ppidl[0],szFileName,SHGDN_INFOLDER|SHGDN_FORPARSING);

							m_pActiveShellBrowser->SelectFiles(szFileName);
						}
						break;
				}
			}

			if(m_pShellContext3 != NULL)
			{
				m_pShellContext3->Release();
				m_pShellContext3 = NULL;
			}
			else if(m_pShellContext2 != NULL)
			{
				m_pShellContext2->Release();
				m_pShellContext2 = NULL;
			}
			else if(m_pShellContext != NULL)
			{
				m_pShellContext->Release();
				m_pShellContext = NULL;
			}

			/* Do NOT destroy the menu until AFTER
			the command has been executed. Items
			on the "Send to" submenu may not work,
			for example, if this item is destroyed
			earlier. */
			DestroyMenu(hMenu);
		}
		pContext->Release();
		pContext = NULL;
	}

	return S_OK;
}

HRESULT CContainer::ShowMultipleFileProperties(LPITEMIDLIST pidlDirectory,
LPCITEMIDLIST *ppidl,int nFiles)
{
	return ExecuteActionFromContextMenu(pidlDirectory,ppidl,nFiles,_T("properties"),0);
}

HRESULT CContainer::ExecuteActionFromContextMenu(LPITEMIDLIST pidlDirectory,
LPCITEMIDLIST *ppidl,int nFiles,TCHAR *szAction,DWORD fMask)
{
	assert(pidlDirectory != NULL);
	assert(szAction != NULL);

	IShellFolder		*pDesktopFolder = NULL;
	IShellFolder		*pShellParentFolder = NULL;
	IShellFolder		*pShellFolder = NULL;
	IContextMenu		*pContext = NULL;
	LPITEMIDLIST		pidlRelative = NULL;
	CMINVOKECOMMANDINFO	cmici;
	HRESULT				hr = S_FALSE;
	char				szActionA[32];

	if(nFiles == 0)
	{
		hr = SHBindToParent(pidlDirectory,IID_IShellFolder,(void **)&pShellParentFolder,
		(LPCITEMIDLIST *)&pidlRelative);

		if(SUCCEEDED(hr))
		{
			hr = pShellParentFolder->GetUIObjectOf(m_hContainer,1,
				(LPCITEMIDLIST *)&pidlRelative,IID_IContextMenu,0,
				(LPVOID *)&pContext);

			pShellParentFolder->Release();
			pShellParentFolder = NULL;
		}
	}
	else
	{
		hr = SHGetDesktopFolder(&pDesktopFolder);

		if(SUCCEEDED(hr))
		{
			if(IsNamespaceRoot(pidlDirectory))
			{
				hr = pDesktopFolder->GetUIObjectOf(m_hContainer,nFiles,
				(LPCITEMIDLIST *)ppidl,IID_IContextMenu,0,(LPVOID *)&pContext);
			}
			else
			{
				hr = pDesktopFolder->BindToObject(pidlDirectory,NULL,
				IID_IShellFolder,(LPVOID *)&pShellFolder);

				if(SUCCEEDED(hr))
				{
					hr = pShellFolder->GetUIObjectOf(m_hContainer,nFiles,
						(LPCITEMIDLIST *)ppidl,IID_IContextMenu,0,(LPVOID *)&pContext);

					pShellFolder->Release();
					pShellFolder = NULL;
				}
			}

			pDesktopFolder->Release();
			pDesktopFolder = NULL;
		}
	}

	if(pContext != NULL)
	{
		/* Action string MUST be ANSI. */
		#ifdef UNICODE
		WideCharToMultiByte(CP_ACP,0,szAction,-1,szActionA,
		SIZEOF_ARRAY(szActionA),NULL,NULL);
		#else
		StringCchCopy(szActionA,SIZEOF_ARRAY(szActionA),szAction);
		#endif

		cmici.cbSize		= sizeof(CMINVOKECOMMANDINFO);
		cmici.fMask			= fMask;
		cmici.hwnd			= m_hContainer;
		cmici.lpVerb		= szActionA;
		cmici.lpParameters	= NULL;
		cmici.lpDirectory	= NULL;
		cmici.nShow			= SW_SHOW;

		hr = pContext->InvokeCommand(&cmici);

		pContext->Release();
		pContext = NULL;
	}

	return hr;
}

LRESULT CALLBACK ShellMenuHookProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	return g_pContainer->ShellMenuHookProc(hwnd,Msg,wParam,lParam);
}

LRESULT CALLBACK ShellMenuHookProcStubMainWindow(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	CContainer *pContainer = (CContainer *)GetWindowLongPtr(hwnd,GWLP_USERDATA);

	return pContainer->ShellMenuHookProc(hwnd,Msg,wParam,lParam);
}

LRESULT CALLBACK CContainer::ShellMenuHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_MEASUREITEM:
			/* wParam is 0 if this item was sent by a menu. */
			if(wParam == 0)
			{
				MEASUREITEMSTRUCT *pmi;

				pmi = (MEASUREITEMSTRUCT *)lParam;

				if(m_bMixedMenu)
				{
					if(pmi->itemID >= MIN_SHELL_MENU_ID && pmi->itemID <= MAX_SHELL_MENU_ID)
					{
						if(m_pShellContext3 != NULL)
							m_pShellContext3->HandleMenuMsg2(uMsg,wParam,lParam,NULL);
						else if(m_pShellContext2 != NULL)
							m_pShellContext2->HandleMenuMsg(uMsg,wParam,lParam);

						pmi->itemHeight	= 20;
					}
					else
					{
						m_pCustomMenu->OnMeasureItem(wParam,lParam);
					}
				}
				else
				{
					if(m_pShellContext3 != NULL)
						m_pShellContext3->HandleMenuMsg2(uMsg,wParam,lParam,NULL);
					else if(m_pShellContext2 != NULL)
						m_pShellContext2->HandleMenuMsg(uMsg,wParam,lParam);
				}

				return TRUE;
			}
			break;

		case WM_DRAWITEM:
			if(wParam == 0)
			{
				DRAWITEMSTRUCT *pdi;

				pdi = (DRAWITEMSTRUCT *)lParam;

				if(m_bMixedMenu)
				{
					if(pdi->itemID >= MIN_SHELL_MENU_ID && pdi->itemID <= MAX_SHELL_MENU_ID)
					{
						if(m_pShellContext3 != NULL)
							m_pShellContext3->HandleMenuMsg2(uMsg,wParam,lParam,NULL);
						else if(m_pShellContext2 != NULL)
							m_pShellContext2->HandleMenuMsg(uMsg,wParam,lParam);

						if(pdi->itemState & ODS_SELECTED)
						{
							COLORREF BorderColor = RGB(0,0,0);
							HBRUSH hBrush;

							hBrush = CreateSolidBrush(BorderColor);

							/* Draw the frame around the menu. */
							FrameRect(pdi->hDC,&pdi->rcItem,hBrush);

							DeleteObject(hBrush);
						}
					}
					else
					{
						m_pCustomMenu->OnDrawItem(wParam,lParam);
					}
				}
				else
				{
					if(m_pShellContext3 != NULL)
						m_pShellContext3->HandleMenuMsg2(uMsg,wParam,lParam,NULL);
					else if(m_pShellContext2 != NULL)
						m_pShellContext2->HandleMenuMsg(uMsg,wParam,lParam);
				}
			}
			return TRUE;
			break;

		case WM_INITMENUPOPUP:
			{
				/* File context menu. */
				if(!m_bMixedMenu)
				{
					if(m_pShellContext3 != NULL)
						m_pShellContext3->HandleMenuMsg2(uMsg,wParam,lParam,NULL);
					else if(m_pShellContext2 != NULL)
						m_pShellContext2->HandleMenuMsg(uMsg,wParam,lParam);
				}
				else
				{
					/* Right click menu. */
					if((HMENU)wParam == GetSubMenu(m_hMenu,m_MenuPos))
					{
						if(m_pShellContext3 != NULL)
							m_pShellContext3->HandleMenuMsg2(uMsg,wParam,lParam,NULL);
						else if(m_pShellContext2 != NULL)
							m_pShellContext2->HandleMenuMsg(uMsg,wParam,lParam);
					}
				}
			}
			break;

		case WM_MENUSELECT:
			{
				UINT CmdID;
				TCHAR HelpString[MAX_PATH];
				HRESULT hr = E_FAIL;

				if(HIWORD(wParam) == 0xFFFF && lParam == 0)
				{
					HandleStatusBarMenuClose();
				}
				else
				{
					HandleStatusBarMenuOpen();

					CmdID = (UINT)LOWORD(wParam);

					if(!((HIWORD(wParam) & MF_POPUP) == MF_POPUP) && CmdID <= MAX_SHELL_MENU_ID
						&& CmdID >= MIN_SHELL_MENU_ID)
					{
						/* Ask for the help string for the currently selected menu item. */
						if(m_pShellContext3 != NULL)
						{
							hr = m_pShellContext3->GetCommandString(CmdID - MIN_SHELL_MENU_ID,GCS_HELPTEXT,
							NULL,(LPSTR)HelpString,SIZEOF_ARRAY(HelpString));
						}
						else if(m_pShellContext2 != NULL)
						{
							hr = m_pShellContext2->GetCommandString(CmdID - MIN_SHELL_MENU_ID,GCS_HELPTEXT,
								NULL,(LPSTR)HelpString,SIZEOF_ARRAY(HelpString));
						}

						/* If the help string was found, send it to the status bar. */
						if(hr == NOERROR)
						{
							SendMessage(m_hStatusBar,SB_SETTEXT,(WPARAM)0|0,(LPARAM)HelpString);
						}

						/* Prevent the message from been passed onto the main window. */
						return 0;
					}
				}
			}
			break;
	}

	return CallWindowProc(DefaultMainWndProc,hwnd,uMsg,wParam,lParam);
}

HRESULT CContainer::ProcessShellMenuCommand(IContextMenu *pContextMenu,UINT CmdIDOffset)
{
	assert(pContextMenu != NULL);

	CMINVOKECOMMANDINFO	cmici;

	cmici.cbSize		= sizeof(CMINVOKECOMMANDINFO);
	cmici.fMask			= 0;
	cmici.hwnd			= m_hContainer;
	cmici.lpVerb		= (LPCSTR)MAKEWORD(CmdIDOffset - MIN_SHELL_MENU_ID,0);
	cmici.lpParameters	= NULL;
	cmici.lpDirectory	= NULL;
	cmici.nShow			= SW_SHOW;

	return pContextMenu->InvokeCommand(&cmici);
}