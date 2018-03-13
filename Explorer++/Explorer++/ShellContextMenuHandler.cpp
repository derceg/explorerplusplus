/******************************************************************
 *
 * Project: Explorer++
 * File: ShellContextMenuHandler.cpp
 * License: GPL - See LICENSE in the top level directory
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
#include "MainResource.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/Macros.h"


#define MENU_OPEN_IN_NEW_TAB	(MAX_SHELL_MENU_ID + 1)

void Explorerplusplus::AddMenuEntries(LPCITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,HMENU hMenu)
{
	assert(dwData != NULL);

	FileContextMenuInfo_t *pfcmi = reinterpret_cast<FileContextMenuInfo_t *>(dwData);

	bool AddNewTabMenuItem = false;

	if(pfcmi->uFrom == FROM_LISTVIEW)
	{
		if(pidlItemList.size() == 1)
		{
			SFGAOF FileAttributes = SFGAO_FOLDER;

			LPITEMIDLIST pidlComplete = ILCombine(pidlParent,pidlItemList.front());
			GetItemAttributes(pidlComplete,&FileAttributes);
			CoTaskMemFree(pidlComplete);

			if(FileAttributes & SFGAO_FOLDER)
			{
				AddNewTabMenuItem = true;
			}
		}
	}
	else if(pfcmi->uFrom == FROM_TREEVIEW)
	{
		/* The treeview only contains folders,
		so the new tab menu item will always
		be shown. */
		AddNewTabMenuItem = true;
	}

	if(AddNewTabMenuItem)
	{
		MENUITEMINFO mii;
		TCHAR szTemp[64];

		LoadString(m_hLanguageModule,IDS_GENERAL_OPEN_IN_NEW_TAB,szTemp,SIZEOF_ARRAY(szTemp));
		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_STRING|MIIM_ID;
		mii.wID			= MENU_OPEN_IN_NEW_TAB;
		mii.dwTypeData	= szTemp;
		InsertMenuItem(hMenu,1,TRUE,&mii);
	}
}

BOOL Explorerplusplus::HandleShellMenuItem(LPCITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,const TCHAR *szCmd)
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

void Explorerplusplus::HandleCustomMenuItem(LPCITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,int iCmd)
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
					std::vector<LPITEMIDLIST> pidlItemVector(pidlItemList.begin(),pidlItemList.end());
					pidlComplete = ILCombine(pidlParent,pidlItemVector[0]);

					bOpenInNewTab = FALSE;
				}
				else
				{
					pidlComplete = ILClone(pidlParent);

					bOpenInNewTab = TRUE;
				}

				GetDisplayName(pidlComplete,szParsingPath,SIZEOF_ARRAY(szParsingPath),SHGDN_FORPARSING);
				BrowseFolder(szParsingPath,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);

				m_bTreeViewOpenInNewTab = TRUE;

				CoTaskMemFree(pidlComplete);
			}
			break;
	}
}

HRESULT Explorerplusplus::ShowMultipleFileProperties(LPITEMIDLIST pidlDirectory,
	LPCITEMIDLIST *ppidl, int nFiles) const
{
	return ExecuteActionFromContextMenu(pidlDirectory, ppidl, m_hContainer, nFiles, _T("properties"), 0);
}