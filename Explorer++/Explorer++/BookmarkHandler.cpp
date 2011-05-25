/******************************************************************
 *
 * Project: Explorer++
 * File: BookmarkHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles tasks associated with bookmarks,
 * such as creating a bookmarks menu, and
 * adding bookmarks to a toolbar.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "AddBookmarkDialog.h"
#include "NewBookmarkFolderDialog.h"
#include "../Helper/ShellHelper.h"


LRESULT CALLBACK BookmarksToolbarSubclassStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	Explorerplusplus *pContainer = reinterpret_cast<Explorerplusplus *>(dwRefData);

	return pContainer->BookmarksToolbarSubclass(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::BookmarksToolbarSubclass(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_MBUTTONUP:
		{
			DWORD dwPos = GetMessagePos();

			POINT ptCursor;
			ptCursor.x = GET_X_LPARAM(dwPos);
			ptCursor.y = GET_Y_LPARAM(dwPos);
			MapWindowPoints(HWND_DESKTOP,m_hBookmarksToolbar,&ptCursor,1);

			int iIndex = static_cast<int>(SendMessage(m_hBookmarksToolbar,TB_HITTEST,0,
				reinterpret_cast<LPARAM>(&ptCursor)));

			if(iIndex >= 0)
			{
				TBBUTTON tbButton;
				SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,iIndex,reinterpret_cast<LPARAM>(&tbButton));

				/* TODO: [Bookmarks] If this is a bookmark, open it in a new tab. */
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void Explorerplusplus::InsertBookmarksIntoMenu(void)
{
	/* TODO: [Bookmarks] Rewrite. */
}

void Explorerplusplus::InsertBookmarkToolbarButtons(void)
{
	HIMAGELIST himl = ImageList_Create(16,16,ILC_COLOR32 | ILC_MASK,0,1);
	HBITMAP hb = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(himl,hb,NULL);
	SendMessage(m_hBookmarksToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(himl));
	DeleteObject(hb);

	/* TODO: [Bookmarks] Rewrite. */
}

void Explorerplusplus::GetBookmarkMenuItemDirectory(HMENU hMenu,
int iBookmarkId,TCHAR *szDirectory,UINT uBufSize)
{
	MENUITEMINFO		mii;
	Bookmark_t			Bookmark;
	CustomMenuInfo_t	*pcmi = NULL;

	mii.cbSize	= sizeof(mii);
	mii.fMask	= MIIM_DATA;
	GetMenuItemInfo(hMenu,iBookmarkId,FALSE,&mii);

	pcmi = (CustomMenuInfo_t *)mii.dwItemData;

	m_Bookmark.RetrieveBookmark((void *)pcmi->dwItemData,&Bookmark);

	StringCchCopy(szDirectory,uBufSize,Bookmark.szLocation);
}

void Explorerplusplus::BookmarkToolbarNewBookmark(int iItem)
{
	if(iItem != -1)
	{
		/* TODO: Need to retrieve bookmark details. */
		/*TBBUTTON tbButton;
		SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,iItem,(LPARAM)&tbButton);*/

		Bookmark bm(EMPTY_STRING,EMPTY_STRING,EMPTY_STRING);

		CAddBookmarkDialog AddBookmarkDialog(g_hLanguageModule,IDD_ADD_BOOKMARK,m_hContainer,m_bfAllBookmarks,&bm);
		AddBookmarkDialog.ShowModalDialog();
	}
}

void Explorerplusplus::BookmarkToolbarNewFolder(int iItem)
{
	CNewBookmarkFolderDialog NewBookmarkFolderDialog(g_hLanguageModule,IDD_NEWBOOKMARKFOLDER,m_hContainer);
	NewBookmarkFolderDialog.ShowModalDialog();
}

HRESULT Explorerplusplus::ExpandAndBrowsePath(TCHAR *szPath)
{
	return ExpandAndBrowsePath(szPath,FALSE,FALSE);
}

/* Browses to the specified path. The path may
have any environment variables expanded (if
necessary). */
HRESULT Explorerplusplus::ExpandAndBrowsePath(TCHAR *szPath,BOOL bOpenInNewTab,BOOL bSwitchToNewTab)
{
	TCHAR szExpandedPath[MAX_PATH];

	MyExpandEnvironmentStrings(szPath,
		szExpandedPath,SIZEOF_ARRAY(szExpandedPath));

	return BrowseFolder(szExpandedPath,SBSP_ABSOLUTE,bOpenInNewTab,bSwitchToNewTab,FALSE);
}