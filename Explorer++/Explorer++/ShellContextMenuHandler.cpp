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
#include "../Helper/ShellHelper.h"


#define MENU_OPEN_IN_NEW_TAB	(MAX_SHELL_MENU_ID + 1)
#define MENU_OPEN_FILE_LOCATION	(MAX_SHELL_MENU_ID + 2)

/*if(uFrom == FROM_LISTVIEW || uFrom == FROM_TREEVIEW ||
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
}*/

//if(StrCmpI(szCmd,_T("open")) == 0)
//{
//	/* If ppidl is NULL, open the item specified by pidlParent
//	in the current listview. If ppidl is not NULL, open each
//	of the items specified in ppidl. */
//	if(ppidl == NULL)
//	{
//		OpenItem(pidlParent,FALSE,FALSE);
//	}
//	else
//	{
//		LPITEMIDLIST	pidlComplete = NULL;
//		int				i = 0;
//
//		for(i = 0;i < nFiles;i++)
//		{
//			pidlComplete = ILCombine(pidlParent,ppidl[i]);
//
//			OpenItem(pidlComplete,FALSE,FALSE);
//
//			CoTaskMemFree(pidlComplete);
//		}
//	}
//
//	m_bTreeViewOpenInNewTab = TRUE;
//
//	return 0;
//}
//else if(StrCmpI(szCmd,_T("rename")) == 0)
//{
//	if(uFrom == FROM_LISTVIEW)
//	{
//		OnFileRename();
//	}
//	else if(uFrom == FROM_TREEVIEW)
//	{
//		OnTreeViewFileRename();
//	}
//}
//else if(StrCmpI(szCmd,_T("copy")) == 0)
//{
//	if(uFrom == FROM_LISTVIEW)
//	{
//		OnListViewCopy(TRUE);
//	}
//	else if(uFrom == FROM_TREEVIEW)
//	{
//		OnTreeViewCopy(TRUE);
//	}
//}
//else if(StrCmpI(szCmd,_T("cut")) == 0)
//{
//	if(uFrom == FROM_LISTVIEW)
//	{
//		OnListViewCopy(FALSE);
//	}
//	else if(uFrom == FROM_TREEVIEW)
//	{
//		OnTreeViewCopy(FALSE);
//	}
//}

///* This is a custom menu item (e.g. Open
//in New Tab). */
//switch(Cmd)
//{
//case MENU_OPEN_IN_NEW_TAB:
//	{
//		LPITEMIDLIST pidlComplete;
//		TCHAR szParsingPath[MAX_PATH];
//		BOOL bOpenInNewTab;
//
//		if(ppidl != NULL)
//		{
//			pidlComplete = ILCombine(pidlParent,*ppidl);
//
//			bOpenInNewTab = FALSE;
//		}
//		else
//		{
//			pidlComplete = ILClone(pidlParent);
//
//			bOpenInNewTab = TRUE;
//		}
//
//		GetDisplayName(pidlComplete,szParsingPath,SHGDN_FORPARSING);
//		BrowseFolder(szParsingPath,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
//
//		m_bTreeViewOpenInNewTab = TRUE;
//
//		CoTaskMemFree(pidlComplete);
//	}
//	break;
//
//case MENU_OPEN_FILE_LOCATION:
//	{
//		TCHAR szFileName[MAX_PATH];
//
//		BrowseFolder(pidlParent,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
//
//		GetDisplayName((LPITEMIDLIST)ppidl[0],szFileName,SHGDN_INFOLDER|SHGDN_FORPARSING);
//
//		m_pActiveShellBrowser->SelectFiles(szFileName);
//	}
//	break;
//}

HRESULT Explorerplusplus::ShowMultipleFileProperties(LPITEMIDLIST pidlDirectory,
LPCITEMIDLIST *ppidl,int nFiles)
{
	return ExecuteActionFromContextMenu(pidlDirectory,ppidl,nFiles,_T("properties"),0);
}

HRESULT Explorerplusplus::ExecuteActionFromContextMenu(LPITEMIDLIST pidlDirectory,
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

HRESULT Explorerplusplus::ProcessShellMenuCommand(IContextMenu *pContextMenu,
	UINT CmdIDOffset,UINT iStartOffset)
{
	assert(pContextMenu != NULL);

	CMINVOKECOMMANDINFO	cmici;

	cmici.cbSize		= sizeof(CMINVOKECOMMANDINFO);
	cmici.fMask			= 0;
	cmici.hwnd			= m_hContainer;
	cmici.lpVerb		= (LPCSTR)MAKEWORD(CmdIDOffset - iStartOffset,0);
	cmici.lpParameters	= NULL;
	cmici.lpDirectory	= NULL;
	cmici.nShow			= SW_SHOW;

	return pContextMenu->InvokeCommand(&cmici);
}