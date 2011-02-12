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
#include "Explorer++.h"
#include "../Helper/ShellHelper.h"


#define MENU_OPEN_IN_NEW_TAB	(MAX_SHELL_MENU_ID + 1)

void Explorerplusplus::AddMenuEntries(LPITEMIDLIST pidlParent,
	list<LPITEMIDLIST> pidlItemList,DWORD_PTR dwData,HMENU hMenu)
{
	assert(dwData != NULL);

	FileContextMenuInfo_t *pfcmi = reinterpret_cast<FileContextMenuInfo_t *>(dwData);

	if(pfcmi->uFrom == FROM_LISTVIEW || pfcmi->uFrom == FROM_TREEVIEW ||
		pfcmi->uFrom == FROM_DRIVEBAR)
	{
		if(pidlItemList.size() == 1)
		{
			SFGAOF FileAttributes = SFGAO_FOLDER;

			vector<LPITEMIDLIST> pidlItemVector(pidlItemList.begin(),pidlItemList.end());

			/* TODO: Convert first argument of function
			to const. */
			GetItemAttributes(pidlItemVector[0],&FileAttributes);

			if(FileAttributes & SFGAO_FOLDER)
			{
				MENUITEMINFO mii;
				mii.cbSize		= sizeof(MENUITEMINFO);
				mii.fMask		= MIIM_STRING|MIIM_ID;
				mii.wID			= MENU_OPEN_IN_NEW_TAB;
				mii.dwTypeData	= _T("Open in New Tab");
				InsertMenuItem(hMenu,1,TRUE,&mii);
			}
		}
	}
}

BOOL Explorerplusplus::HandleShellMenuItem(LPITEMIDLIST pidlParent,
	list<LPITEMIDLIST> pidlItemList,DWORD_PTR dwData,TCHAR *szCmd)
{
	FileContextMenuInfo_t *pfcmi = reinterpret_cast<FileContextMenuInfo_t *>(dwData);

	if(StrCmpI(szCmd,_T("open")) == 0)
	{
		/* If ppidl is NULL, open the item specified by pidlParent
		in the current listview. If ppidl is not NULL, open each
		of the items specified in ppidl. */
		if(pidlItemList.size() == 0)
		{
			OpenItem(pidlParent,FALSE,FALSE);
		}
		else
		{
			LPITEMIDLIST pidlComplete = NULL;

			for each(auto pidl in pidlItemList)
			{
				pidlComplete = ILCombine(pidlParent,pidl);

				OpenItem(pidlComplete,FALSE,FALSE);

				CoTaskMemFree(pidlComplete);
			}
		}

		m_bTreeViewOpenInNewTab = TRUE;

		return TRUE;
	}
	else if(StrCmpI(szCmd,_T("rename")) == 0)
	{
		if(pfcmi->uFrom == FROM_LISTVIEW)
		{
			OnFileRename();
		}
		else if(pfcmi->uFrom == FROM_TREEVIEW)
		{
			OnTreeViewFileRename();
		}

		return TRUE;
	}
	else if(StrCmpI(szCmd,_T("copy")) == 0)
	{
		if(pfcmi->uFrom == FROM_LISTVIEW)
		{
			OnListViewCopy(TRUE);
		}
		else if(pfcmi->uFrom == FROM_TREEVIEW)
		{
			OnTreeViewCopy(TRUE);
		}

		return TRUE;
	}
	else if(StrCmpI(szCmd,_T("cut")) == 0)
	{
		if(pfcmi->uFrom == FROM_LISTVIEW)
		{
			OnListViewCopy(FALSE);
		}
		else if(pfcmi->uFrom == FROM_TREEVIEW)
		{
			OnTreeViewCopy(FALSE);
		}

		return TRUE;
	}

	return FALSE;
}

void Explorerplusplus::HandleCustomMenuItem(LPITEMIDLIST pidlParent,
	list<LPITEMIDLIST> pidlItemList,int iCmd)
{
	switch(iCmd)
	{
		case MENU_OPEN_IN_NEW_TAB:
			{
				LPITEMIDLIST pidlComplete;
				TCHAR szParsingPath[MAX_PATH];
				BOOL bOpenInNewTab;

				if(pidlItemList.size() != 0)
				{
					vector<LPITEMIDLIST> pidlItemVector(pidlItemList.begin(),pidlItemList.end());
					pidlComplete = ILCombine(pidlParent,pidlItemVector[0]);

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
	}
}

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