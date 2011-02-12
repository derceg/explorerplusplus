/******************************************************************
 *
 * Project: Explorer++
 * File: BookmarkPropertiesDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Bookmark Properties' dialog.
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


/* A handle to the bookmark been updated. */
void *g_pBookmarkHandle;

/* Remembers whether the item been updated
was originally shown on the bookmarks
toolbar. */
BOOL g_bShownOnToolbar;

INT_PTR CALLBACK BookmarkPropertiesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				BookmarkPropertiesInfo_t	*pbpi = NULL;

				pbpi = (BookmarkPropertiesInfo_t *)lParam;

				pContainer = (Explorerplusplus *)pbpi->pContainer;
			}
			break;
	}

	return pContainer->BookmarkPropertiesProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::BookmarkPropertiesProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnBookmarkPropertiesInit(hDlg,lParam);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					OnBookmarkPropertiesOk(hDlg);
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

	return 0;
}

void Explorerplusplus::OnBookmarkPropertiesInit(HWND hDlg,LPARAM lParam)
{
	HWND	hEditName;
	HWND	hEditLocation;
	HWND	hEditDescription;
	Bookmark_t	Bookmark;
	BookmarkPropertiesInfo_t	*pbpi = NULL;

	pbpi = (BookmarkPropertiesInfo_t *)lParam;

	m_Bookmark.RetrieveBookmark(pbpi->pBookmarkHandle,&Bookmark);

	g_pBookmarkHandle = Bookmark.pHandle;
	g_bShownOnToolbar = Bookmark.bShowOnToolbar;

	hEditName = GetDlgItem(hDlg,IDC_BP_EDIT_NAME);
	hEditLocation = GetDlgItem(hDlg,IDC_BP_EDIT_LOCATION);
	hEditDescription = GetDlgItem(hDlg,IDC_BP_EDIT_DESCRIPTION);

	SetWindowText(hEditName,Bookmark.szItemName);

	SetWindowText(hEditLocation,Bookmark.szLocation);

	SetWindowText(hEditDescription,Bookmark.szItemDescription);

	if(Bookmark.bShowOnToolbar)
		CheckDlgButton(hDlg,IDC_BP_SHOWONTOOLBAR,BST_CHECKED);

	SetFocus(hEditName);

	/* Select all the text in the name edit control. */
	SendMessage(hEditName,EM_SETSEL,0,-1);
}

void Explorerplusplus::OnBookmarkPropertiesOk(HWND hDlg)
{
	HWND	hEditName;
	HWND	hEditLocation;
	HWND	hEditDescription;
	Bookmark_t	Bookmark;
	UINT	uChecked;

	hEditName = GetDlgItem(hDlg,IDC_BP_EDIT_NAME);
	hEditLocation = GetDlgItem(hDlg,IDC_BP_EDIT_LOCATION);
	hEditDescription = GetDlgItem(hDlg,IDC_BP_EDIT_DESCRIPTION);

	SendMessage(hEditName,WM_GETTEXT,SIZEOF_ARRAY(Bookmark.szItemName),
		(LPARAM)Bookmark.szItemName);

	SendMessage(hEditLocation,WM_GETTEXT,SIZEOF_ARRAY(Bookmark.szLocation),
		(LPARAM)Bookmark.szLocation);

	SendMessage(hEditDescription,WM_GETTEXT,SIZEOF_ARRAY(Bookmark.szItemDescription),
		(LPARAM)Bookmark.szItemDescription);

	uChecked = IsDlgButtonChecked(hDlg,IDC_BP_SHOWONTOOLBAR);

	Bookmark.bShowOnToolbar = (uChecked == BST_CHECKED);

	m_Bookmark.UpdateBookmark(g_pBookmarkHandle,&Bookmark);

	if(Bookmark.bShowOnToolbar != g_bShownOnToolbar)
	{
		/* Toolbar status has changed. Either remove
		the item from the toolbar, or add it. */

		if(Bookmark.bShowOnToolbar)
		{
			/* Insert the bookmark onto the toolbar. */
			InsertBookmarkIntoToolbar(&Bookmark,GenerateUniqueBookmarkToolbarId());
		}
		else
		{
			/* Find the item and remove it. */
			RemoveItemFromBookmarksToolbar(Bookmark.pHandle);
		}
	}
	else if(Bookmark.bShowOnToolbar)
	{
		UpdateToolbarButton(&Bookmark);
	}

	EndDialog(hDlg,1);
}

INT_PTR CALLBACK BookmarkFolderPropertiesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				BookmarkPropertiesInfo_t	*pbpi = NULL;

				pbpi = (BookmarkPropertiesInfo_t *)lParam;

				pContainer = (Explorerplusplus *)pbpi->pContainer;
			}
			break;
	}

	return pContainer->BookmarkFolderPropertiesProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::BookmarkFolderPropertiesProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnBookmarkFolderPropertiesInit(hDlg,lParam);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					OnBookmarkFolderPropertiesOk(hDlg);
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

	return 0;
}

void Explorerplusplus::OnBookmarkFolderPropertiesInit(HWND hDlg,LPARAM lParam)
{
	HWND		hEditName;
	HWND		hEditDescription;
	Bookmark_t	Bookmark;

	BookmarkPropertiesInfo_t	*pbpi = NULL;

	pbpi = (BookmarkPropertiesInfo_t *)lParam;

	m_Bookmark.RetrieveBookmark(pbpi->pBookmarkHandle,&Bookmark);

	g_pBookmarkHandle = Bookmark.pHandle;
	g_bShownOnToolbar = Bookmark.bShowOnToolbar;

	hEditName = GetDlgItem(hDlg,IDC_BFP_NAME);
	hEditDescription = GetDlgItem(hDlg,IDC_BFP_DESCRIPTION);

	SetWindowText(hEditName,Bookmark.szItemName);

	SetWindowText(hEditDescription,Bookmark.szItemDescription);

	if(Bookmark.bShowOnToolbar)
		CheckDlgButton(hDlg,IDC_BFP_SHOWONTOOLBAR,BST_CHECKED);

	SetFocus(hEditName);

	/* Select all the text in the name edit control. */
	SendMessage(hEditName,EM_SETSEL,0,-1);
}

void Explorerplusplus::OnBookmarkFolderPropertiesOk(HWND hDlg)
{
	HWND		hEditName;
	HWND		hEditDescription;
	Bookmark_t	Bookmark;
	UINT		uChecked;

	hEditName = GetDlgItem(hDlg,IDC_BFP_NAME);
	hEditDescription = GetDlgItem(hDlg,IDC_BFP_DESCRIPTION);

	SendMessage(hEditName,WM_GETTEXT,SIZEOF_ARRAY(Bookmark.szItemName),
		(LPARAM)Bookmark.szItemName);

	SendMessage(hEditDescription,WM_GETTEXT,SIZEOF_ARRAY(Bookmark.szItemDescription),
		(LPARAM)Bookmark.szItemDescription);

	uChecked = IsDlgButtonChecked(hDlg,IDC_BFP_SHOWONTOOLBAR);

	Bookmark.bShowOnToolbar = (uChecked == BST_CHECKED);

	m_Bookmark.UpdateBookmark(g_pBookmarkHandle,&Bookmark);

	if(Bookmark.bShowOnToolbar != g_bShownOnToolbar)
	{
		/* Toolbar status has changed. Either remove
		the item from the toolbar, or add it. */

		if(Bookmark.bShowOnToolbar)
		{
			/* Insert the bookmark onto the toolbar. */
			InsertBookmarkIntoToolbar(&Bookmark,GenerateUniqueBookmarkToolbarId());
		}
		else
		{
			/* Find the item and remove it. */
			RemoveItemFromBookmarksToolbar(Bookmark.pHandle);
		}
	}

	EndDialog(hDlg,1);
}