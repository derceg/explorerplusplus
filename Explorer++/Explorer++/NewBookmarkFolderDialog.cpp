/******************************************************************
 *
 * Project: Explorer++
 * File: NewBookmarkFolderDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'New Folder' dialog (for bookmarks).
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Explorer++.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Bookmark.h"
#include "MainResource.h"


#define DEFAULT_NEWFOLDER_NAME	_T("New Folder")

extern int g_iFolderSelected;

INT_PTR CALLBACK NewBookmarkFolderProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				pContainer = (Explorerplusplus *)lParam;
			}
			break;
	}

	return pContainer->NewBookmarkFolderProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::NewBookmarkFolderProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnNewBookmarkFolderInit(hDlg);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					OnNewBookmarkFolderOk(hDlg);
					break;

				case IDCANCEL:
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;
	}

	return FALSE;
}

void Explorerplusplus::OnNewBookmarkFolderInit(HWND hDlg)
{
	HWND	hName;
	HWND	hCreateIn;

	hName = GetDlgItem(hDlg,IDC_NEWFOLDER_NAME);
	hCreateIn = GetDlgItem(hDlg,IDC_NEWFOLDER_CREATEIN);

	SetWindowText(hName,DEFAULT_NEWFOLDER_NAME);


	/* Initialize the 'Create In' control. This is where the
	bookmark will be created. */
	HIMAGELIST		himl;
	HBITMAP			hb;

	himl = ImageList_Create(16,16,ILC_COLOR32 | ILC_MASK,0,1);
	hb = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(himl,hb,NULL);
	DeleteObject(hb);
	SendMessage(hCreateIn,CBEM_SETIMAGELIST,0,(LPARAM)himl);

	Bookmark_t RootBookmark;

	m_Bookmark.GetRoot(&RootBookmark);

	InsertFolderItemsIntoComboBox(hCreateIn,&RootBookmark);

	/* Select the first item in the list. */
	SendMessage(hCreateIn,CB_SETCURSEL,g_iFolderSelected,0);

	/* Select the text in the 'Name' control. */
	SendMessage(hName,EM_SETSEL,0,-1);
	SetFocus(hName);
}

/* Determine which item in the 'Create In' combo box
is selected. Use the selection to decide where to
create the new bookmark folder, by passing selection
information back to the bookmarks module. */
void Explorerplusplus::OnNewBookmarkFolderOk(HWND hDlg)
{
	HWND			hName;
	HWND			hCreateIn;
	COMBOBOXEXITEM	cbexItem;
	TCHAR			szName[256];
	int				iSel;

	hName = GetDlgItem(hDlg,IDC_NEWFOLDER_NAME);
	hCreateIn = GetDlgItem(hDlg,IDC_NEWFOLDER_CREATEIN);

	SendMessage(hName,WM_GETTEXT,256,(LPARAM)szName);

	iSel = (int)SendMessage(hCreateIn,CB_GETCURSEL,0,0);

	cbexItem.iItem	= iSel;
	cbexItem.mask	= CBEIF_LPARAM;
	SendMessage(hCreateIn,CBEM_GETITEM,0,(LPARAM)&cbexItem);

	Bookmark_t Bookmark;

	StringCchCopy(Bookmark.szItemName,SIZEOF_ARRAY(Bookmark.szItemName),szName);
	StringCchCopy(Bookmark.szItemDescription,SIZEOF_ARRAY(Bookmark.szItemDescription),EMPTY_STRING);
	Bookmark.Type = BOOKMARK_TYPE_FOLDER;
	Bookmark.bShowOnToolbar = FALSE;
	m_Bookmark.CreateNewBookmark((void *)cbexItem.lParam,&Bookmark);

	EndDialog(hDlg,1);
}