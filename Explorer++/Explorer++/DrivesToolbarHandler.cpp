/******************************************************************
 *
 * Project: Explorer++
 * File: DrivesToolbarHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles all messages associated with the drives toolbar.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"


LRESULT CALLBACK DrivesToolbarSubclassStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->DrivesToolbarSubclass(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::DrivesToolbarSubclass(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_MBUTTONUP:
		{
			TBBUTTON tbButton;
			TCHAR *pszDrivePath = NULL;
			POINT ptCursor;
			DWORD dwPos;
			LRESULT lResult;
			int iIndex;

			dwPos = GetMessagePos();
			ptCursor.x = GET_X_LPARAM(dwPos);
			ptCursor.y = GET_Y_LPARAM(dwPos);
			MapWindowPoints(HWND_DESKTOP,m_hDrivesToolbar,&ptCursor,1);

			iIndex = (int)SendMessage(m_hDrivesToolbar,TB_HITTEST,0,(LPARAM)&ptCursor);

			if(iIndex >= 0)
			{
				lResult = SendMessage(m_hDrivesToolbar,TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

				if(lResult)
				{
					pszDrivePath = (TCHAR *)tbButton.dwData;

					BrowseFolder(pszDrivePath,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
				}
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void Explorerplusplus::InsertDrivesIntoDrivesToolbar(void)
{
	HIMAGELIST	SmallIcons;
	TCHAR		*pszDriveStrings = NULL;
	TCHAR		*ptrDrive = NULL;
	DWORD		dwSize;
	LRESULT		lResult;

	Shell_GetImageLists(NULL,&SmallIcons);
	SendMessage(m_hDrivesToolbar,TB_SETIMAGELIST,0,(LPARAM)SmallIcons);

	dwSize = GetLogicalDriveStrings(0,NULL);

	pszDriveStrings = (TCHAR *)malloc((dwSize + 1) * sizeof(TCHAR));

	if(pszDriveStrings == NULL)
		return;

	dwSize = GetLogicalDriveStrings(dwSize,pszDriveStrings);

	if(dwSize != 0)
	{
		ptrDrive = pszDriveStrings;

		while(*ptrDrive != '\0')
		{
			lResult = InsertDriveIntoDrivesToolbar(ptrDrive);

			ptrDrive += lstrlen(ptrDrive) + 1;
		}
	}

	free(pszDriveStrings);
}

LRESULT Explorerplusplus::InsertDriveIntoDrivesToolbar(TCHAR *szDrive)
{
	TBBUTTON	tbButton;
	SHFILEINFO	shfi;
	LRESULT		lResult;
	TCHAR		szDisplayName[32];
	TCHAR		*szItemData = NULL;
	TCHAR		*pszDrivePath = NULL;
	int			iPos = 0;
	int			nButtons = 0;
	int			i = 0;

	StringCchCopy(szDisplayName,SIZEOF_ARRAY(szDisplayName),szDrive);

	/* If the last character is a backslash (\), remove it, by
	replacing it with a NULL character. */
	if(szDisplayName[lstrlen(szDisplayName) - 1] == '\\')
		szDisplayName[lstrlen(szDisplayName) - 1] = '\0';

	/* Get the drives icon. */
	SHGetFileInfo(szDrive,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES);

	/* The drives path will be stored with the inserted item
	as item data. */
	szItemData = (TCHAR *)malloc((lstrlen(szDrive) + 1) * sizeof(TCHAR));
	StringCchCopy(szItemData,lstrlen(szDrive) + 1,szDrive);

	/* Find the correct position in which to insert the button.
	All drive buttons are sorted alphabetically by drive letter. */
	nButtons = (int)SendMessage(m_hDrivesToolbar,TB_BUTTONCOUNT,0,0);

	for(i = 0;i < nButtons;i++)
	{
		lResult = SendMessage(m_hDrivesToolbar,TB_GETBUTTON,i,(LPARAM)&tbButton);

		if(lResult)
		{
			pszDrivePath = (TCHAR *)tbButton.dwData;

			if(lstrcmp(szDrive,pszDrivePath) < 0)
				break;

			iPos++;
		}
	}

	tbButton.iBitmap	= shfi.iIcon;
	tbButton.idCommand	= TOOLBAR_DRIVES_ID_START + m_nDrivesInToolbar;
	tbButton.fsState	= TBSTATE_ENABLED;
	tbButton.fsStyle	= BTNS_BUTTON|BTNS_AUTOSIZE|BTNS_SHOWTEXT;
	tbButton.dwData		= (DWORD_PTR)szItemData;
	tbButton.iString	= (INT_PTR)szDisplayName;

	lResult = SendMessage(m_hDrivesToolbar,TB_INSERTBUTTON,(WPARAM)iPos,(LPARAM)&tbButton);

	if(lResult)
		m_nDrivesInToolbar++;

	return lResult;
}

/* Remove the specified drive from the toolbar.
A drive may need to be removed if it was
disconnected from the system (i.e. a removable
device may have been ejected).
Returns TRUE if the drive was found and removed;
FALSE otherwise. */
LRESULT Explorerplusplus::RemoveDriveFromDrivesToolbar(TCHAR *szDrive)
{
	TBBUTTON	tbButton;
	TCHAR		*pszDrivePath = NULL;
	LRESULT		lResult = FALSE;
	int			nButtons = 0;
	int			i = 0;

	nButtons = (int)SendMessage(m_hDrivesToolbar,TB_BUTTONCOUNT,0,0);

	/* Loop through each of the drives, and attempt to
	find the specified one. If the drive is found, remove
	it; otherwise just return FALSE without doing anything
	further. */
	for(i = 0;i < nButtons;i++)
	{
		lResult = SendMessage(m_hDrivesToolbar,TB_GETBUTTON,i,(LPARAM)&tbButton);

		if(lResult)
		{
			pszDrivePath = (TCHAR *)tbButton.dwData;

			if(lstrcmp(szDrive,pszDrivePath) == 0)
			{
				lResult = SendMessage(m_hDrivesToolbar,TB_DELETEBUTTON,i,0);
				break;
			}
		}
	}

	return lResult;
}

/* Updates an items icon. This may be necessary,
for example, if a cd/dvd is inserted/removed. */
void Explorerplusplus::UpdateDrivesToolbarIcon(TCHAR *szDrive)
{
	TBBUTTON tbButton;
	SHFILEINFO shfi;
	TCHAR *pszDrivePath = NULL;
	LRESULT lResult;
	int nButtons = 0;
	int i = 0;

	nButtons = (int)SendMessage(m_hDrivesToolbar,TB_BUTTONCOUNT,0,0);

	/* Search through the drives toolbar to find the
	correct item. */
	for(i = 0;i < nButtons;i++)
	{
		lResult = SendMessage(m_hDrivesToolbar,TB_GETBUTTON,
			i,(LPARAM)&tbButton);

		if(lResult)
		{
			pszDrivePath = (TCHAR *)tbButton.dwData;

			if(lstrcmpi(szDrive,pszDrivePath) == 0)
			{
				/* Get the drives icon. */
				SHGetFileInfo(szDrive,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);

				/* Update the drives icon. */
				SendMessage(m_hDrivesToolbar,TB_CHANGEBITMAP,
					(WPARAM)tbButton.idCommand,(LPARAM)shfi.iIcon);

				break;
			}
		}
	}
}

void Explorerplusplus::DrivesToolbarRefreshAllIcons(void)
{
	TBBUTTON tbButton;
	SHFILEINFO shfi;
	TCHAR *pszDrive = NULL;
	LRESULT lResult;
	int nButtons;
	int i = 0;

	nButtons = (int)SendMessage(m_hDrivesToolbar,TB_BUTTONCOUNT,0,0);

	/* Search through the drives toolbar to find the
	correct item. */
	for(i = 0;i < nButtons;i++)
	{
		lResult = SendMessage(m_hDrivesToolbar,TB_GETBUTTON,
			i,(LPARAM)&tbButton);

		if(lResult)
		{
			pszDrive = (TCHAR *)tbButton.dwData;

			/* Get the drives icon. */
			SHGetFileInfo(pszDrive,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);

			/* Update the drives icon. */
			SendMessage(m_hDrivesToolbar,TB_CHANGEBITMAP,
				(WPARAM)tbButton.idCommand,(LPARAM)shfi.iIcon);
		}
	}
}