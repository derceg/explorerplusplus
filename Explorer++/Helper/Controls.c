/******************************************************************
 *
 * Project: Helper
 * File: Controls.c
 * License: GPL - See COPYING in the top level directory
 *
 * Generic implementation of control creation.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Helper.h"
#include "Controls.h"


BOOL InitControlClasses(DWORD Classes)
{
	INITCOMMONCONTROLSEX	ccEx;

	ccEx.dwSize	= sizeof(INITCOMMONCONTROLSEX);
	ccEx.dwICC	= Classes;

	return InitCommonControlsEx(&ccEx);

}

HWND CreateListView(HWND Parent,DWORD Style)
{
	HWND hListView;

	hListView = CreateWindow(WC_LISTVIEW,EMPTY_STRING,Style,
	0,0,0,0,Parent,NULL,GetModuleHandle(0),NULL);

	/* Set the extended hListView styles. These styles can't be set
	properly with CreateWindowEx(). */
	SendMessage(hListView,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_INFOTIP|LVS_EX_DOUBLEBUFFER);
	
	return hListView;
}

HWND CreateTreeView(HWND Parent,DWORD Style)
{
	HWND TreeView;
	HIMAGELIST SmallIcons;

	TreeView = CreateWindow(WC_TREEVIEW,EMPTY_STRING,Style,
	0,0,0,0,Parent,NULL,GetModuleHandle(0),NULL);

	/* Retrieve the small version of the system image list. */
	Shell_GetImageLists(NULL,&SmallIcons);
	
	/* Assign the small system image list to the treeview control. */
	TreeView_SetImageList(TreeView,SmallIcons,TVSIL_NORMAL);

	return TreeView;
}

HWND CreateStatusBar(HWND Parent,DWORD Style)
{
	HWND StatusBar;

	StatusBar = CreateWindow(STATUSCLASSNAME,EMPTY_STRING,Style,
	0,0,0,0,Parent,NULL,GetModuleHandle(0),NULL);

	return StatusBar;
}

HWND CreateToolbar(HWND hParent,DWORD dwStyle,DWORD dwExStyle)
{
	HWND hToolbar;

	hToolbar = CreateWindow(TOOLBARCLASSNAME,EMPTY_STRING,dwStyle,
	0,0,0,0,hParent,NULL,GetModuleHandle(NULL),NULL);

	if(hToolbar != NULL)
	{
		/* Set the extended styles for the toolbar. */
		SendMessage(hToolbar,TB_SETEXTENDEDSTYLE,0,dwExStyle);
	}

	return hToolbar;
}

HWND CreateComboBox(HWND Parent,DWORD Style)
{
	HWND ComboBox;

	ComboBox = CreateWindowEx(WS_EX_TOOLWINDOW,WC_COMBOBOXEX,EMPTY_STRING,Style,0,0,0,200,
	Parent,NULL,GetModuleHandle(0),NULL);

	return ComboBox;
}

HWND CreateTabControl(HWND hParent,UINT Style)
{
	HWND TabControl;
	HFONT hFont;

	TabControl = CreateWindowEx(0,WC_TABCONTROL,EMPTY_STRING,Style,0,0,0,0,
	hParent,NULL,GetModuleHandle(0),NULL);

	/* Set a smaller (more readable) font. */
	hFont = CreateFont(15,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,ANSI_CHARSET,
	OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY,FF_DONTCARE,NULL);

	SendMessage(TabControl,WM_SETFONT,(WPARAM)hFont,(LPARAM)MAKELPARAM(TRUE,0));

	return TabControl;
}

int ResizeStatusBar(HWND StatusBar,int Width,int Height)
{
	RECT rc_sb;

	GetWindowRect(StatusBar,&rc_sb);

	/* Assumes that the status bar is pinned to the bottom of the window. */
	SetWindowPos(StatusBar,NULL,0,Height - (rc_sb.bottom - rc_sb.top),
	Width,24,SWP_NOZORDER);

	return 0;
}

void AddPathsToComboBoxEx(HWND CbEx,TCHAR *Path)
{
	COMBOBOXEXITEM cbItem;
	HIMAGELIST SmallIcons;
	int iImage;
	SHFILEINFO shfi;
	int i = 0;
	HANDLE hFirstFile;
	WIN32_FIND_DATA wfd;
	TCHAR FindPath[MAX_PATH];

	/* Retrieve the small and large versions of the system image list. */
	Shell_GetImageLists(NULL,&SmallIcons);
	SendMessage(CbEx,CBEM_SETIMAGELIST,0,(LPARAM)SmallIcons);

	/* Remove all items that are currently in the list. */
	SendMessage(CbEx,CB_RESETCONTENT,0,0);

	StringCchCopy(FindPath,SIZEOF_ARRAY(FindPath),Path);
	PathAppend(FindPath,_T("*"));

	hFirstFile = FindFirstFile(FindPath,&wfd);

	if(hFirstFile != INVALID_HANDLE_VALUE)
	{
		while(FindNextFile(hFirstFile,&wfd) != NULL)
		{
			/* If the specified item is a folder, add it to the combo box list box. */
			if((wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY &&
				StrCmp(wfd.cFileName,_T("..")) != 0)
			{
				/* Build the full path to the folder, then find its icon index (within the
				system image list). */
				PathAppend(Path,wfd.cFileName);
				SHGetFileInfo(Path,NULL,&shfi,NULL,SHGFI_SYSICONINDEX);

				PathRemoveFileSpec(Path);
				iImage = shfi.iIcon;

				cbItem.mask				= CBEIF_TEXT|CBEIF_IMAGE|CBEIF_INDENT|CBEIF_SELECTEDIMAGE;
				cbItem.iItem			= -1;
				cbItem.iImage			= iImage;
				cbItem.iSelectedImage	= iImage;
				cbItem.iIndent			= 1;
				cbItem.iOverlay			= 1;
				cbItem.pszText			= wfd.cFileName;
				cbItem.cchTextMax		= lstrlen(wfd.cFileName);

				SendMessage(CbEx,CBEM_INSERTITEM,0,(LPARAM)&cbItem);
				i++;
			}
		}

		FindClose(hFirstFile);
	}
}

HWND CreateButton(HWND hParent,DWORD Styles)
{
	HWND hButton;
	Styles = WS_VISIBLE|WS_CHILD;

	hButton = CreateWindow(WC_BUTTON,EMPTY_STRING,Styles,0,0,20,20,
	hParent,NULL,GetModuleHandle(0),NULL);

	return hButton;
}

void ListView_AddRemoveExtendedStyle(HWND hListView,DWORD dwStyle,BOOL bAdd)
{
	DWORD	dwExtendedStyle;

	dwExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	if(bAdd)
		dwExtendedStyle |= dwStyle;
	else
		dwExtendedStyle &= ~dwStyle;

	ListView_SetExtendedListViewStyle(hListView,
		dwExtendedStyle);
}