/******************************************************************
 *
 * Project: Helper
 * File: Controls.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Generic implementation of control creation.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Controls.h"
#include "WindowHelper.h"
#include "Macros.h"


HWND CreateListView(HWND hParent, DWORD dwStyle)
{
	HWND hListView = CreateWindow(WC_LISTVIEW, EMPTY_STRING, dwStyle,
		0, 0, 0, 0, hParent, NULL, GetModuleHandle(0), NULL);

	if(hListView != NULL)
	{
		/* Set the extended hListView styles. These styles can't be set
		properly with CreateWindowEx(). */
		SendMessage(hListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);
	}

	return hListView;
}

HWND CreateTreeView(HWND hParent, DWORD dwStyle)
{
	HWND hTreeView = CreateWindow(WC_TREEVIEW, EMPTY_STRING, dwStyle,
		0, 0, 0, 0, hParent, NULL, GetModuleHandle(0), NULL);

	if(hTreeView != NULL)
	{
		/* Retrieve the small version of the system image list. */
		HIMAGELIST SmallIcons;
		BOOL bRet = Shell_GetImageLists(NULL, &SmallIcons);

		if(bRet)
		{
			TreeView_SetImageList(hTreeView, SmallIcons, TVSIL_NORMAL);
		}
	}

	return hTreeView;
}

HWND CreateStatusBar(HWND hParent, DWORD dwStyle)
{
	HWND hStatusBar = CreateWindow(STATUSCLASSNAME, EMPTY_STRING, dwStyle,
		0, 0, 0, 0, hParent, NULL, GetModuleHandle(0), NULL);

	return hStatusBar;
}

HWND CreateToolbar(HWND hParent, DWORD dwStyle, DWORD dwExStyle)
{
	HWND hToolbar = CreateWindow(TOOLBARCLASSNAME, EMPTY_STRING, dwStyle,
		0, 0, 0, 0, hParent, NULL, GetModuleHandle(NULL), NULL);

	if(hToolbar != NULL)
	{
		/* Set the extended styles for the toolbar. */
		SendMessage(hToolbar, TB_SETEXTENDEDSTYLE, 0, dwExStyle);
	}

	return hToolbar;
}

HWND CreateComboBox(HWND Parent, DWORD dwStyle)
{
	HWND hComboBox = CreateWindowEx(WS_EX_TOOLWINDOW, WC_COMBOBOXEX,
		EMPTY_STRING, dwStyle, 0, 0, 0, 200, Parent, NULL,
		GetModuleHandle(0), NULL);

	return hComboBox;
}

HWND CreateTabControl(HWND hParent, DWORD dwStyle)
{
	HWND hTabControl = CreateWindowEx(0, WC_TABCONTROL, EMPTY_STRING,
		dwStyle, 0, 0, 0, 0, hParent, NULL, GetModuleHandle(0), NULL);

	return hTabControl;
}

BOOL PinStatusBar(HWND hStatusBar, int Width, int Height)
{
	RECT rc;
	BOOL bRet = GetWindowRect(hStatusBar, &rc);

	if(bRet)
	{
		/* Pin the status bar to the bottom of the window. */
		bRet = SetWindowPos(hStatusBar, NULL, 0, Height - GetRectHeight(&rc),
			Width, GetRectHeight(&rc), SWP_NOZORDER);
	}

	return bRet;
}

BOOL AddPathsToComboBoxEx(HWND hComboBoxEx, const TCHAR *Path)
{
	HIMAGELIST SmallIcons;
	BOOL bRet = Shell_GetImageLists(NULL, &SmallIcons);

	if(!bRet)
	{
		return FALSE;
	}

	SendMessage(hComboBoxEx, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(SmallIcons));

	/* Remove all items that are currently in the list. */
	SendMessage(hComboBoxEx, CB_RESETCONTENT, 0, 0);

	TCHAR FindPath[MAX_PATH];
	StringCchCopy(FindPath, SIZEOF_ARRAY(FindPath), Path);
	bRet = PathAppend(FindPath, _T("*"));

	if(!bRet)
	{
		return FALSE;
	}

	WIN32_FIND_DATA wfd;
	HANDLE hFirstFile = FindFirstFile(FindPath, &wfd);

	if(hFirstFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	BOOL success = TRUE;

	while(FindNextFile(hFirstFile, &wfd))
	{
		if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY &&
			StrCmp(wfd.cFileName, _T("..")) != 0)
		{
			TCHAR FullFileName[MAX_PATH];
			LPTSTR szRet = PathCombine(FullFileName, Path, wfd.cFileName);

			if(szRet == NULL)
			{
				success = FALSE;
				break;
			}

			SHFILEINFO shfi;
			SHGetFileInfo(Path, NULL, &shfi, NULL, SHGFI_SYSICONINDEX);

			COMBOBOXEXITEM cbItem;
			cbItem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_INDENT | CBEIF_SELECTEDIMAGE;
			cbItem.iItem = -1;
			cbItem.iImage = shfi.iIcon;
			cbItem.iSelectedImage = shfi.iIcon;
			cbItem.iIndent = 1;
			cbItem.iOverlay = 1;
			cbItem.pszText = wfd.cFileName;

			LRESULT lRet = SendMessage(hComboBoxEx, CBEM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&cbItem));

			if(lRet == -1)
			{
				success = FALSE;
				break;
			}
		}
	}

	FindClose(hFirstFile);

	return success;
}

BOOL lCheckDlgButton(HWND hDlg, int ButtonId, BOOL bCheck)
{
	UINT uCheck;

	if(bCheck)
	{
		uCheck = BST_CHECKED;
	}
	else
	{
		uCheck = BST_UNCHECKED;
	}

	return CheckDlgButton(hDlg, ButtonId, uCheck);
}

void AddStyleToToolbar(UINT *fStyle, UINT fStyleToAdd)
{
	if((*fStyle & fStyleToAdd) != fStyleToAdd)
	{
		*fStyle |= fStyleToAdd;
	}
}

void AddGripperStyle(UINT *fStyle, BOOL bAddGripper)
{
	if(bAddGripper)
	{
		/* Remove the no-gripper style (if present). */
		if((*fStyle & RBBS_NOGRIPPER) == RBBS_NOGRIPPER)
		{
			*fStyle &= ~RBBS_NOGRIPPER;
		}

		/* Only add the gripper style if it isn't already present. */
		if((*fStyle & RBBS_GRIPPERALWAYS) != RBBS_GRIPPERALWAYS)
		{
			*fStyle |= RBBS_GRIPPERALWAYS;
		}
	}
	else
	{
		if((*fStyle & RBBS_GRIPPERALWAYS) == RBBS_GRIPPERALWAYS)
		{
			*fStyle &= ~RBBS_GRIPPERALWAYS;
		}

		if((*fStyle & RBBS_NOGRIPPER) != RBBS_NOGRIPPER)
		{
			*fStyle |= RBBS_NOGRIPPER;
		}
	}
}

void UpdateToolbarBandSizing(HWND hRebar, HWND hToolbar)
{
	REBARBANDINFO rbbi;
	SIZE sz;
	int nBands;
	int iBand = -1;
	int i = 0;

	nBands = (int) SendMessage(hRebar, RB_GETBANDCOUNT, 0, 0);

	for(i = 0; i < nBands; i++)
	{
		rbbi.cbSize = sizeof(rbbi);
		rbbi.fMask = RBBIM_CHILD;
		SendMessage(hRebar, RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&rbbi));

		if(rbbi.hwndChild == hToolbar)
		{
			iBand = i;
			break;
		}
	}

	if(iBand != -1)
	{
		SendMessage(hToolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&sz));

		rbbi.cbSize = sizeof(rbbi);
		rbbi.fMask = RBBIM_IDEALSIZE;
		rbbi.cxIdeal = sz.cx;
		SendMessage(hRebar, RB_SETBANDINFO, iBand, reinterpret_cast<LPARAM>(&rbbi));
	}
}