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


#define BOOKMARK_SUBMENU_POSITION_START	2

DWORD BookmarksTreeViewStyles	= WS_CHILD|WS_VISIBLE|TVS_SHOWSELALWAYS|TVS_HASBUTTONS|
								  TVS_EDITLABELS|TVS_HASLINES;

int g_iStartId = MENU_BOOKMARK_STARTID;

LRESULT CALLBACK BookmarksToolbarSubclassStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->BookmarksToolbarSubclass(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::BookmarksToolbarSubclass(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_MBUTTONUP:
		{
			Bookmark_t Bookmark;
			TBBUTTON tbButton;
			POINT ptCursor;
			DWORD dwPos;
			int iIndex;

			dwPos = GetMessagePos();
			ptCursor.x = GET_X_LPARAM(dwPos);
			ptCursor.y = GET_Y_LPARAM(dwPos);
			MapWindowPoints(HWND_DESKTOP,m_hBookmarksToolbar,&ptCursor,1);

			iIndex = (int)SendMessage(m_hBookmarksToolbar,TB_HITTEST,0,(LPARAM)&ptCursor);

			if(iIndex >= 0)
			{
				SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

				m_Bookmark.RetrieveBookmark((void *)tbButton.dwData,&Bookmark);

				/* If this is a bookmark, open it in a new tab. */
				if(Bookmark.Type == BOOKMARK_TYPE_BOOKMARK)
				{
					ExpandAndBrowsePath(Bookmark.szLocation,TRUE,TRUE);
				}
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void Explorerplusplus::InsertBookmarksIntoMenu(void)
{
	Bookmark_t		RootBookmark;
	Bookmark_t		FirstChild;
	MENUITEMINFO	mii;
	HRESULT			hr;
	int				nBookmarksAddedToMenu;
	int				i = 0;

	nBookmarksAddedToMenu = GetMenuItemCount(m_hBookmarksMenu) - 3;

	if(nBookmarksAddedToMenu > 0)
	{
		/* First, delete any previous bookmarks from the
		menu. */
		for(i = nBookmarksAddedToMenu - 1;i >= 0;i--)
		{
			mii.cbSize	= sizeof(mii);
			mii.fMask	= MIIM_DATA;
			GetMenuItemInfo(m_hBookmarksMenu,MENU_BOOKMARK_STARTPOS + i,TRUE,&mii);

			free((void *)mii.dwItemData);

			DeleteMenu(m_hBookmarksMenu,MENU_BOOKMARK_STARTPOS + i,MF_BYPOSITION);
		}

		g_iStartId = MENU_BOOKMARK_STARTID;
	}

	m_Bookmark.GetRoot(&RootBookmark);

	hr = m_Bookmark.GetChild(&RootBookmark,&FirstChild);

	if(SUCCEEDED(hr))
	{
		InsertBookmarksIntoMenuInternal(m_hBookmarksMenu,&FirstChild,MENU_BOOKMARK_STARTPOS);
	}
}

void Explorerplusplus::InsertBookmarksIntoMenuInternal(HMENU hMenu,
Bookmark_t *pBookmark,int iStartPos,int iStartId)
{
	g_iStartId = iStartId;

	InsertBookmarksIntoMenuInternal(hMenu,pBookmark,iStartPos);
}

void Explorerplusplus::InsertBookmarksIntoMenuInternal(HMENU hMenu,
Bookmark_t *pBookmark,int iStartPos)
{
	MENUITEMINFO		mi;
	Bookmark_t			ChildBookmark;
	Bookmark_t			SiblingBookmark;
	CustomMenuInfo_t	*pcmi = NULL;
	HRESULT				hr;
	BOOL				res;

	/* pBookmark may be NULL when creating a menu
	for a folder on the bookmarks toolbar, when that
	folder has no children. */
	if(pBookmark == NULL)
	{
		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_STRING|MIIM_ID;
		mi.wID			= IDM_BOOKMARKS_BOOKMARKTHISTAB;
		mi.dwTypeData	= _T("Bookmark This Tab...");
		res = InsertMenuItem(hMenu,0,TRUE,&mi);

		SetMenuItemOwnerDrawn(hMenu,0);

		return;
	}

	if(pBookmark->Type == BOOKMARK_TYPE_BOOKMARK)
	{
		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_STRING|MIIM_ID;
		mi.wID			= g_iStartId++;
		mi.dwTypeData	= pBookmark->szItemName;
		res = InsertMenuItem(hMenu,iStartPos,TRUE,&mi);

		SetMenuItemOwnerDrawn(hMenu,iStartPos);

		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_DATA;
		res = GetMenuItemInfo(hMenu,iStartPos,TRUE,&mi);

		pcmi = (CustomMenuInfo_t *)mi.dwItemData;

		pcmi->dwItemData = (ULONG_PTR)pBookmark->pHandle;
	}
	else
	{
		HMENU hSubMenu;

		hSubMenu = CreateMenu();

		InsertMenu(hMenu,iStartPos,MF_BYPOSITION|MF_POPUP,1010,pBookmark->szItemName);

		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_SUBMENU;
		mi.hSubMenu		= hSubMenu;
		SetMenuItemInfo(hMenu,iStartPos,TRUE,&mi);
		SetMenuItemOwnerDrawn(hMenu,iStartPos);

		mi.cbSize		= sizeof(mi);
		mi.fMask		= MIIM_STRING|MIIM_ID;
		mi.wID			= IDM_BOOKMARKS_BOOKMARKTHISTAB;
		mi.dwTypeData	= _T("Bookmark This Tab...");
		res = InsertMenuItem(hSubMenu,0,TRUE,&mi);
		SetMenuItemOwnerDrawn(hSubMenu,0);

		hr = m_Bookmark.GetChild(pBookmark,&ChildBookmark);

		if(SUCCEEDED(hr))
		{
			/* Only insert a separator if this item actually
			has children. */
			mi.cbSize		= sizeof(mi);
			mi.fMask		= MIIM_FTYPE;
			mi.fType		= MFT_SEPARATOR;
			res = InsertMenuItem(hSubMenu,1,TRUE,&mi);
			SetMenuItemOwnerDrawn(hSubMenu,1);

			InsertBookmarksIntoMenuInternal(hSubMenu,&ChildBookmark,
				BOOKMARK_SUBMENU_POSITION_START);
		}
	}

	hr = m_Bookmark.GetNextBookmarkSibling(pBookmark,&SiblingBookmark);

	if(SUCCEEDED(hr))
	{
		InsertBookmarksIntoMenuInternal(hMenu,&SiblingBookmark,iStartPos + 1);
	}
}

void Explorerplusplus::InsertBookmarkToolbarButtons(void)
{
	Bookmark_t		RootBookmark;
	HIMAGELIST		himl;
	HBITMAP			hb;

	himl = ImageList_Create(16,16,ILC_COLOR32 | ILC_MASK,0,1);
	hb = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(himl,hb,NULL);
	DeleteObject(hb);

	/* Add the custom buttons to the toolbars image list. */
	SendMessage(m_hBookmarksToolbar,TB_SETIMAGELIST,0,(LPARAM)himl);

	m_Bookmark.GetRoot(&RootBookmark);

	InsertToolbarButtonsInternal(&RootBookmark);
}

void Explorerplusplus::InsertToolbarButtonsInternal(Bookmark_t *pBookmark)
{
	Bookmark_t	CurrentBookmark;
	Bookmark_t	ChildBookmark;
	HRESULT		hr;

	hr = m_Bookmark.GetChild(pBookmark,&CurrentBookmark);

	while(SUCCEEDED(hr))
	{
		if(CurrentBookmark.bShowOnToolbar)
		{
			InsertBookmarkIntoToolbar(&CurrentBookmark,GenerateUniqueBookmarkToolbarId());
		}

		if(CurrentBookmark.Type == BOOKMARK_TYPE_FOLDER)
		{
			hr = m_Bookmark.GetChild(&CurrentBookmark,&ChildBookmark);

			if(SUCCEEDED(hr))
			{
				InsertToolbarButtonsInternal(&ChildBookmark);
			}
		}

		hr = m_Bookmark.GetNextBookmarkSibling(&CurrentBookmark,&CurrentBookmark);
	}
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
			nResult = DialogBoxParam(g_hLanguageModule,
				MAKEINTRESOURCE(IDD_BOOKMARKFOLDER_PROPERTIES),
				m_hContainer,BookmarkFolderPropertiesProcStub,
				(LPARAM)&bpi);
		}
		else
		{
			nResult = DialogBoxParam(g_hLanguageModule,
				MAKEINTRESOURCE(IDD_BOOKMARK_PROPERTIES),
				m_hContainer,BookmarkPropertiesProcStub,
				(LPARAM)&bpi);
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

		CAddBookmarkDialog AddBookmarkDialog(g_hLanguageModule,IDD_ADD_BOOKMARK,m_hContainer);
		AddBookmarkDialog.ShowModalDialog();
	}
}

void Explorerplusplus::BookmarkToolbarNewFolder(int iItem)
{
	CNewBookmarkFolderDialog NewBookmarkFolderDialog(g_hLanguageModule,IDD_NEWBOOKMARKFOLDER,m_hContainer);
	NewBookmarkFolderDialog.ShowModalDialog();
}

void Explorerplusplus::RemoveItemFromBookmarksToolbar(void *pBookmarkHandle)
{
	TBBUTTON	tbButton;
	LRESULT		lResult;
	int			nButtons;
	int			i = 0;

	nButtons = (int)SendMessage(m_hBookmarksToolbar,TB_BUTTONCOUNT,0,0);

	for(i = 0;i < nButtons;i++)
	{
		lResult = SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,
			i,(LPARAM)&tbButton);

		if(lResult)
		{
			if((void *)tbButton.dwData == pBookmarkHandle)
			{
				SendMessage(m_hBookmarksToolbar,TB_DELETEBUTTON,i,0);

				UpdateToolbarBandSizing(m_hMainRebar,m_hBookmarksToolbar);
				break;
			}
		}
	}
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