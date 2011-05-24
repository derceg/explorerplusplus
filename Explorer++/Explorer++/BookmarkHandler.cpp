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

void Explorerplusplus::InsertBookmarkIntoToolbar(Bookmark_t *pBookmark,int id)
{
	TBBUTTON	tbButton;
	int			iImage;

	if(pBookmark->Type == BOOKMARK_TYPE_FOLDER)
		iImage = SHELLIMAGES_NEWTAB;
	else
		iImage = SHELLIMAGES_FAV;

	tbButton.iBitmap	= iImage;
	tbButton.idCommand	= id;
	tbButton.fsState	= TBSTATE_ENABLED;
	tbButton.fsStyle	= BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_NOPREFIX;
	tbButton.dwData		= (DWORD)pBookmark->pHandle;
	tbButton.iString	= (INT_PTR)pBookmark->szItemName;

	SendMessage(m_hBookmarksToolbar,TB_ADDBUTTONS,(WPARAM)1,(LPARAM)&tbButton);

	UpdateToolbarBandSizing(m_hMainRebar,m_hBookmarksToolbar);
}

void Explorerplusplus::UpdateToolbarButton(Bookmark_t *pBookmark)
{
	TBBUTTONINFO tbbi;
	TBBUTTON tbButton;
	int iImage;
	LRESULT lResult;
	int nButtons;
	int i = 0;

	nButtons = (int)SendMessage(m_hBookmarksToolbar,TB_BUTTONCOUNT,0,0);

	for(i = 0;i < nButtons;i++)
	{
		lResult = SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,
			i,(LPARAM)&tbButton);

		if(lResult)
		{
			if((void *)tbButton.dwData == pBookmark->pHandle)
			{
				if(pBookmark->Type == BOOKMARK_TYPE_FOLDER)
					iImage = SHELLIMAGES_NEWTAB;
				else
					iImage = SHELLIMAGES_FAV;

				tbbi.cbSize		= sizeof(tbbi);
				tbbi.dwMask		= TBIF_TEXT;
				tbbi.iImage		= iImage;
				tbbi.pszText	= pBookmark->szItemName;
				tbbi.lParam		= (DWORD_PTR)pBookmark->pHandle;
				SendMessage(m_hBookmarksToolbar,TB_SETBUTTONINFO,tbButton.idCommand,(LPARAM)&tbbi);

				break;
			}
		}
	}
}

void Explorerplusplus::InitializeBookmarkToolbarMap(void)
{
	int i = 0;

	for(i = 0;i < MAX_BOOKMARKTOOLBAR_ITEMS;i++)
		m_uBookmarkToolbarMap[i] = 0;
}

int Explorerplusplus::GenerateUniqueBookmarkToolbarId(void)
{
	BOOL	bFound = FALSE;
	int		i = 0;

	for(i = 0;i < MAX_BOOKMARKTOOLBAR_ITEMS;i++)
	{
		if(m_uBookmarkToolbarMap[i] == 0)
		{
			m_uBookmarkToolbarMap[i] = 1;
			bFound = TRUE;
			break;
		}
	}

	if(bFound)
		return TOOLBAR_BOOKMARK_START + i;
	else
		return -1;
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

void Explorerplusplus::BookmarkToolbarOpenItem(int iItem,BOOL bOpenInNewTab)
{
	Bookmark_t	Bookmark;
	TBBUTTON	tbButton;

	if(iItem != -1)
	{
		SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,iItem,(LPARAM)&tbButton);

		m_Bookmark.RetrieveBookmark((void *)tbButton.dwData,&Bookmark);

		/* If the toolbar item is a bookmark, simply navigate
		to its directory. If it's a folder, open a menu with
		its sub-items on. */
		if(Bookmark.Type == BOOKMARK_TYPE_BOOKMARK)
		{
			if(bOpenInNewTab)
				BrowseFolder(Bookmark.szLocation,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
			else
				BrowseFolder(Bookmark.szLocation,SBSP_ABSOLUTE);
		}
	}
}

void Explorerplusplus::BookmarkToolbarDeleteItem(int iItem)
{
	TBBUTTON	tbButton;
	BOOL		bDeleted;

	if(iItem != -1)
	{
		SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,iItem,(LPARAM)&tbButton);

		/* Delete the bookmark. */
		bDeleted = DeleteBookmarkSafe(m_hContainer,(void *)tbButton.dwData);

		if(bDeleted)
		{
			/* Now, remove it from the toolbar. */
			SendMessage(m_hBookmarksToolbar,TB_DELETEBUTTON,iItem,0);

			/* ...and re-insert any bookmarks into the bookmarks menu. */
			InsertBookmarksIntoMenu();
		}
	}
}

void Explorerplusplus::BookmarkToolbarShowItemProperties(int iItem)
{
	Bookmark_t					Bookmark;
	BookmarkPropertiesInfo_t	bpi;
	TBBUTTON					tbButton;
	INT_PTR						nResult = 0;

	if(iItem != -1)
	{
		SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,iItem,(LPARAM)&tbButton);

		m_Bookmark.RetrieveBookmark((void *)tbButton.dwData,&Bookmark);

		bpi.pContainer		= this;
		bpi.pBookmarkHandle	= (void *)tbButton.dwData;

		/* Which dialog is shown depends on whether
		this item is a (bookmark) folder or a bookmark. */
		if(Bookmark.Type == BOOKMARK_TYPE_FOLDER)
		{
			/* TODO: Show new bookmark folder dialog. */
		}
		else
		{
			/* TODO: Show add bookmark dialog. */
		}

		/* Ok was pressed. Need to update the information
		for this bookmark. */
		if(nResult == 1)
		{
		}
	}
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

BOOL Explorerplusplus::DeleteBookmarkSafe(HWND hwnd,void *pBookmarkHandle)
{
	TCHAR szInfoMsg[128];
	int	iMessageBoxReturn;

	LoadString(g_hLanguageModule,IDS_BOOKMARK_DELETE,
		szInfoMsg,SIZEOF_ARRAY(szInfoMsg));

	iMessageBoxReturn = MessageBox(hwnd,szInfoMsg,
		NExplorerplusplus::WINDOW_NAME,MB_YESNO|MB_ICONINFORMATION|MB_DEFBUTTON2);

	if(iMessageBoxReturn == IDYES)
	{
		m_Bookmark.DeleteBookmark(pBookmarkHandle);
		return TRUE;
	}

	return FALSE;
}