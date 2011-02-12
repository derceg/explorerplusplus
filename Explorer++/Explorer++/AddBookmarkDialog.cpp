/******************************************************************
 *
 * Project: Explorer++
 * File: AddBookmarkDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Add Bookmark' dialog.
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
#include "../Helper/ShellHelper.h"
#include "MainResource.h"


#define INITIAL_HEIGHT_DELTA	-71
#define HEIGHT_DELTA			-66
#define BUTTON_DELTA			-132
#define EXTRA_DELTA				-45

/* TRUE when expanded; FALSE otherwise. */
BOOL g_bExpanded;

int g_iFolderSelected;

INT_PTR CALLBACK BookmarkTabDlgProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				AddBookmarkInfo_t *pabi;

				pabi = (AddBookmarkInfo_t *)lParam;

				pContainer = (Explorerplusplus *)pabi->pContainer;
			}
			break;
	}

	return pContainer->BookmarkTabDlgProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::BookmarkTabDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnAddBookmarkInit(hDlg,lParam);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_BOOKMARK_NEWFOLDER:
					OnAddBookmarkNewFolder(hDlg);
					break;

				case IDC_BOOKMARK_DETAILS:
					OnBookmarkDetails(hDlg);
					break;

				case IDOK:
					OnAddBookmarkOk(hDlg);
					break;

				case IDCANCEL:
					AddBookmarkSaveState(hDlg);
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_CLOSE:
			AddBookmarkSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return FALSE;
}

void Explorerplusplus::OnAddBookmarkInit(HWND hDlg,LPARAM lParam)
{
	AddBookmarkInfo_t	*pabi = NULL;
	Bookmark_t			ParentBookmark;
	TCHAR				szTabTitle[MAX_PATH];
	TCHAR				szDirectory[MAX_PATH];
	HWND				hName;
	HWND				hLocation;
	HWND				hCreateIn;
	DWORD				dwParsingFlags;
	int					iParentBookmark;

	pabi = (AddBookmarkInfo_t *)lParam;

	hName = GetDlgItem(hDlg,IDC_BOOKMARK_NAME);
	hLocation = GetDlgItem(hDlg,IDC_BOOKMARK_LOCATION);
	hCreateIn = GetDlgItem(hDlg,IDC_BOOKMARK_CREATEIN);

	/* No text will be set if a directory to bookmark
	has not been specified. */
	if(pabi->pidlDirectory != NULL)
	{
		/* Use the in-folder name of the directory as
		the bookmarks name. */
		GetDisplayName(pabi->pidlDirectory,szTabTitle,SHGDN_INFOLDER);

		SetWindowText(hName,szTabTitle);
		SendMessage(hName,EM_SETSEL,0,-1);

		/* Don't show full paths for virtual folders (as only the folders
		GUID will be shown). */
		if(!m_pActiveShellBrowser->InVirtualFolder())
			dwParsingFlags = SHGDN_FORPARSING;
		else
			dwParsingFlags = SHGDN_NORMAL;

		GetDisplayName(pabi->pidlDirectory,szDirectory,dwParsingFlags);

		SetWindowText(hLocation,szDirectory);
	}


	/* Initialize the 'Create In' control. This is where the
	bookmark will be created. */
	HIMAGELIST		himl;
	HBITMAP			hb;

	himl = ImageList_Create(16,16,ILC_COLOR32 | ILC_MASK,0,1);
	hb = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(himl,hb,NULL);
	DeleteObject(hb);
	SendMessage(hCreateIn,CBEM_SETIMAGELIST,0,(LPARAM)himl);


	m_Bookmark.GetRoot(&ParentBookmark);

	InsertFolderItemsIntoComboBox(hCreateIn,&ParentBookmark);

	if(pabi->pParentBookmark != NULL)
		iParentBookmark = LocateBookmarkInComboBox(hCreateIn,pabi->pParentBookmark);
	else
		iParentBookmark = 0;

	/* Now, select the parent bookmark from the
	list. */
	SendMessage(hCreateIn,CB_SETCURSEL,iParentBookmark,0);


	g_bExpanded = !pabi->bExpandInitial;
	OnBookmarkDetails(hDlg);

	/* The initial state of the dialog (contracted or expanded). */
	g_bExpanded = pabi->bExpandInitial;

	SetFocus(hName);

	if(m_bAddBookmarkDlgStateSaved)
	{
		SetWindowPos(hDlg,NULL,m_ptAddBookmark.x,
			m_ptAddBookmark.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(m_hContainer,hDlg);
	}
}

void Explorerplusplus::OnAddBookmarkOk(HWND hDlg)
{
	HWND			hEditName;
	HWND			hEditDescription;
	HWND			hLocation;
	HWND			hCreateIn;
	COMBOBOXEXITEM	cbexItem;
	TCHAR			*pszBookmarkName = NULL;
	TCHAR			*pszBookmarkDescription = NULL;
	TCHAR			*pszLocation = NULL;
	UINT			ToolbarCheckState;
	int				iBufSize;
	int				iSel;

	hEditName = GetDlgItem(hDlg,IDC_BOOKMARK_NAME);
	hEditDescription = GetDlgItem(hDlg,IDC_BOOKMARK_DESCRIPTION);
	hLocation = GetDlgItem(hDlg,IDC_BOOKMARK_LOCATION);
	hCreateIn = GetDlgItem(hDlg,IDC_BOOKMARK_CREATEIN);

	iBufSize = GetWindowTextLength(hEditName);

	pszBookmarkName = (TCHAR *)malloc((iBufSize + 1) * sizeof(TCHAR));

	if(pszBookmarkName != NULL)
	{
		SendMessage(hEditName,WM_GETTEXT,iBufSize + 1,(LPARAM)pszBookmarkName);
	}

	iBufSize = GetWindowTextLength(hEditDescription);

	pszBookmarkDescription = (TCHAR *)malloc((iBufSize + 1) * sizeof(TCHAR));

	if(pszBookmarkDescription != NULL)
	{
		SendMessage(hEditDescription,WM_GETTEXT,iBufSize + 1,(LPARAM)pszBookmarkDescription);
	}

	iBufSize = GetWindowTextLength(hLocation);

	pszLocation = (TCHAR *)malloc((iBufSize + 1) * sizeof(TCHAR));

	if(pszLocation != NULL)
	{
		SendMessage(hLocation,WM_GETTEXT,iBufSize + 1,(LPARAM)pszLocation);
	}

	iSel = (int)SendMessage(hCreateIn,CB_GETCURSEL,0,0);

	cbexItem.iItem	= iSel;
	cbexItem.mask	= CBEIF_LPARAM;
	SendMessage(hCreateIn,CBEM_GETITEM,0,(LPARAM)&cbexItem);

	ToolbarCheckState = IsDlgButtonChecked(hDlg,IDC_BOOKMARK_TOOLBAR);

	Bookmark_t NewBookmark;

	StringCchCopy(NewBookmark.szItemName,SIZEOF_ARRAY(NewBookmark.szItemName),pszBookmarkName);
	StringCchCopy(NewBookmark.szItemDescription,SIZEOF_ARRAY(NewBookmark.szItemDescription),pszBookmarkDescription);

	TCHAR szParsingPath[MAX_PATH];
	HRESULT hr;

	/* The item may be virtual. */
	hr = DecodeFriendlyPath(pszLocation,szParsingPath);

	if(SUCCEEDED(hr))
		StringCchCopy(NewBookmark.szLocation,SIZEOF_ARRAY(NewBookmark.szLocation),szParsingPath);
	else
		StringCchCopy(NewBookmark.szLocation,SIZEOF_ARRAY(NewBookmark.szLocation),pszLocation);

	NewBookmark.Type = BOOKMARK_TYPE_BOOKMARK;
	NewBookmark.bShowOnToolbar = (ToolbarCheckState == BST_CHECKED);
	m_Bookmark.CreateNewBookmark((void *)cbexItem.lParam,&NewBookmark);

	free(pszBookmarkDescription);
	free(pszBookmarkName);
	free(pszLocation);

	/* If the 'Show On Bookmarks Toolbar' control
	is checked, add the bookmark to the bookmarks
	toolbar. */
	if(ToolbarCheckState == BST_CHECKED)
		InsertBookmarkIntoToolbar(&NewBookmark,GenerateUniqueBookmarkToolbarId());

	AddBookmarkSaveState(hDlg);

	EndDialog(hDlg,1);
}

void Explorerplusplus::OnAddBookmarkNewFolder(HWND hDlg)
{
	HWND	hCreateIn;

	hCreateIn = GetDlgItem(hDlg,IDC_BOOKMARK_CREATEIN);

	g_iFolderSelected = (int)SendMessage(hCreateIn,CB_GETCURSEL,0,0);

	/* Show the 'New Folder' dialog box. If the OK button was pressed
	(i.e. a new folder was created), refresh the folder list in this dialog. */
	if(DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_NEWBOOKMARKFOLDER),
		hDlg,NewBookmarkFolderProcStub,(LPARAM)this) == 1)
	{
		Bookmark_t RootBookmark;

		/* Remove all items that are currently in the list. */
		SendMessage(hCreateIn,CB_RESETCONTENT,0,0);

		m_Bookmark.GetRoot(&RootBookmark);

		/* Now, insert the items back into the list. */
		InsertFolderItemsIntoComboBox(hCreateIn,&RootBookmark);

		/* Select the newly created folder. */
		SendMessage(hCreateIn,CB_SETCURSEL,g_iFolderSelected + 1,0);
	}
}

void Explorerplusplus::OnBookmarkDetails(HWND hDlg)
{
	HWND	hSeparator;
	HWND	hOk;
	HWND	hCancel;
	HWND	hLocation;
	HWND	hDescription;
	HWND	hStaticLocation;
	HWND	hStaticDescription;
	HWND	hDetails;
	HWND	hBookmarksToolbar;
	RECT	rc;

	hSeparator = GetDlgItem(hDlg,IDC_BOOKMARK_SEPARATOR);
	hOk = GetDlgItem(hDlg,IDOK);
	hCancel = GetDlgItem(hDlg,IDCANCEL);
	hLocation = GetDlgItem(hDlg,IDC_BOOKMARK_LOCATION);
	hDescription = GetDlgItem(hDlg,IDC_BOOKMARK_DESCRIPTION);
	hStaticLocation = GetDlgItem(hDlg,IDC_STATIC_LOCATION);
	hStaticDescription = GetDlgItem(hDlg,IDC_STATIC_DESCRIPTION);
	hDetails = GetDlgItem(hDlg,IDC_BOOKMARK_DETAILS);
	hBookmarksToolbar = GetDlgItem(hDlg,IDC_BOOKMARK_TOOLBAR);

	if(g_bExpanded)
	{
		RECT rcDetails;
		GetWindowRect(hDetails,&rcDetails);
		GetWindowRect(hSeparator,&rc);
		OffsetRect(&rcDetails,0,6);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rcDetails,2);
		SetWindowPos(hSeparator,NULL,rc.left,rcDetails.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);


		/* Ok and cancel buttons. */
		RECT rcSeparator;
		GetWindowRect(hOk,&rc);
		GetWindowRect(hSeparator,&rcSeparator);
		OffsetRect(&rcSeparator,0,6);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rcSeparator,2);
		SetWindowPos(hOk,NULL,rc.left,rcSeparator.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);

		GetWindowRect(hCancel,&rc);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hCancel,NULL,rc.left,rcSeparator.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);


		/* Contract the dialog, so that only the 'Name'
		and 'Create In' controls are visible. */
		RECT rcOk;
		GetWindowRect(hDlg,&rc);
		GetWindowRect(hOk,&rcOk);
		InflateRect(&rcOk,0,10);
		SetWindowPos(hDlg,NULL,0,0,rc.right - rc.left,rcOk.bottom - rc.top,SWP_NOMOVE|SWP_NOZORDER);


		GetWindowRect(hStaticLocation,&rc);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hStaticLocation,NULL,rc.left,rc.top - EXTRA_DELTA,0,0,SWP_NOSIZE|SWP_NOZORDER);

		GetWindowRect(hLocation,&rc);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hLocation,NULL,rc.left,rc.top - EXTRA_DELTA,0,0,SWP_NOSIZE|SWP_NOZORDER);


		GetWindowRect(hStaticDescription,&rc);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hStaticDescription,NULL,rc.left,rc.top - EXTRA_DELTA,0,0,SWP_NOSIZE|SWP_NOZORDER);

		GetWindowRect(hDescription,&rc);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hDescription,NULL,rc.left,rc.top - EXTRA_DELTA,0,0,SWP_NOSIZE|SWP_NOZORDER);


		GetWindowRect(hBookmarksToolbar,&rc);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hBookmarksToolbar,NULL,rc.left,rc.top - EXTRA_DELTA,0,0,SWP_NOSIZE|SWP_NOZORDER);

		SetWindowText(hDetails,_T("Details >>"));
	}
	else
	{
		/* Move the location edit box and static control 5 pixels below the details button. */
		RECT rcDetails;
		GetWindowRect(hDetails,&rcDetails);
		GetWindowRect(hStaticLocation,&rc);
		OffsetRect(&rcDetails,0,5);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rcDetails,2);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hStaticLocation,NULL,rc.left,rcDetails.bottom + 2,0,0,SWP_NOSIZE|SWP_NOZORDER);

		GetWindowRect(hLocation,&rc);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hLocation,NULL,rc.left,rcDetails.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);


		/* These two controls will be located 5 pixels below the location controls. */
		RECT rcLocation;
		GetWindowRect(hLocation,&rcLocation);
		GetWindowRect(hDescription,&rc);
		OffsetRect(&rcLocation,0,5);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rcLocation,2);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hDescription,NULL,rc.left,rcLocation.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);

		GetWindowRect(hStaticDescription,&rc);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hStaticDescription,NULL,rc.left,rcLocation.bottom + 2,0,0,SWP_NOSIZE|SWP_NOZORDER);


		/* 'Show On Bookmarks Toolbar' checkbox. */
		RECT rcDescription;
		GetWindowRect(hDescription,&rcDescription);
		GetWindowRect(hBookmarksToolbar,&rc);
		OffsetRect(&rcDescription,0,7);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rcDescription,2);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hBookmarksToolbar,NULL,rc.left,rcDescription.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);


		/* Separator. */
		RECT rcBookmarksToolbar;
		GetWindowRect(hSeparator,&rc);
		GetWindowRect(hBookmarksToolbar,&rcBookmarksToolbar);
		OffsetRect(&rcBookmarksToolbar,0,6);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rcBookmarksToolbar,2);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hSeparator,NULL,rc.left,rcBookmarksToolbar.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);


		/* Ok and cancel buttons. */
		RECT rcSeparator;
		GetWindowRect(hOk,&rc);
		GetWindowRect(hSeparator,&rcSeparator);
		OffsetRect(&rcSeparator,0,6);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rcSeparator,2);
		SetWindowPos(hOk,NULL,rc.left,rcSeparator.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);

		GetWindowRect(hCancel,&rc);
		MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);
		SetWindowPos(hCancel,NULL,rc.left,rcSeparator.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);


		/* Expand the dialog, so that all the
		controls are visible. */
		RECT rcOk;
		GetWindowRect(hDlg,&rc);
		GetWindowRect(hOk,&rcOk);
		InflateRect(&rcOk,0,10);
		SetWindowPos(hDlg,NULL,0,0,rc.right - rc.left,rcOk.bottom - rc.top,SWP_NOMOVE|SWP_NOZORDER);


		SetWindowText(hDetails,_T("Details <<"));
	}

	g_bExpanded = !g_bExpanded;
}

void Explorerplusplus::AddBookmarkSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptAddBookmark.x = rcTemp.left;
	m_ptAddBookmark.y = rcTemp.top;

	m_bAddBookmarkDlgStateSaved = TRUE;
}