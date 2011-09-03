/******************************************************************
 *
 * Project: Explorer++
 * File: BookmarksToolbar.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Initializes the bookmarks toolbar and handles update
 * notifications, window messages, etc.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <algorithm>
#include "Explorer++.h"
#include "BookmarksToolbar.h"
#include "../Helper/Macros.h"


CBookmarksToolbar::CBookmarksToolbar(HWND hToolbar,CBookmarkFolder &AllBookmarks,
	const GUID &guidBookmarksToolbar,UINT uIDStart) :
m_hToolbar(hToolbar),
m_AllBookmarks(AllBookmarks),
m_guidBookmarksToolbar(guidBookmarksToolbar),
m_uIDStart(uIDStart),
m_uIDCounter(0)
{
	InitializeToolbar();

	CBookmarkItemNotifier::GetInstance().AddObserver(this);
}

CBookmarksToolbar::~CBookmarksToolbar()
{
	CBookmarkItemNotifier::GetInstance().RemoveObserver(this);
}

void CBookmarksToolbar::InitializeToolbar()
{
	SendMessage(m_hToolbar,TB_SETBITMAPSIZE,0,MAKELONG(16,16));
	SendMessage(m_hToolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);

	HIMAGELIST m_himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himl,hBitmap,NULL);
	SendMessage(m_hToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(m_himl));
	DeleteObject(hBitmap);

	/* TODO: Register toolbar for drag and drop. */

	SetWindowSubclass(m_hToolbar,BookmarksToolbarProcStub,0,reinterpret_cast<DWORD_PTR>(this));

	InsertBookmarkItems();
}

LRESULT CALLBACK BookmarksToolbarProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CBookmarksToolbar *pbt = reinterpret_cast<CBookmarksToolbar *>(dwRefData);

	return pbt->BookmarksToolbarProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CBookmarksToolbar::BookmarksToolbarProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_MBUTTONUP:
		{
			DWORD dwPos = GetMessagePos();

			POINT ptCursor;
			ptCursor.x = GET_X_LPARAM(dwPos);
			ptCursor.y = GET_Y_LPARAM(dwPos);
			MapWindowPoints(HWND_DESKTOP,m_hToolbar,&ptCursor,1);

			int iIndex = static_cast<int>(SendMessage(m_hToolbar,TB_HITTEST,0,
				reinterpret_cast<LPARAM>(&ptCursor)));

			if(iIndex >= 0)
			{
				TBBUTTON tbButton;
				SendMessage(m_hToolbar,TB_GETBUTTON,iIndex,reinterpret_cast<LPARAM>(&tbButton));

				/* TODO: If this is a bookmark, open it in a new tab. */
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void CBookmarksToolbar::InsertBookmarkItems()
{
	/* The bookmarks toolbar folder should always be a direct child
	of the root. */
	auto variantBookmarksToolbar = NBookmarkHelper::GetBookmarkItem(m_AllBookmarks,m_guidBookmarksToolbar);

	assert(variantBookmarksToolbar.type() == typeid(CBookmarkFolder));

	const CBookmarkFolder &BookmarksToolbarFolder = boost::get<CBookmarkFolder>(variantBookmarksToolbar);

	for each(auto variantBookmark in BookmarksToolbarFolder)
	{
		if(variantBookmark.type() == typeid(CBookmarkFolder))
		{
			const CBookmarkFolder &BookmarkFolder = boost::get<CBookmarkFolder>(variantBookmark);
			InsertBookmarkFolder(BookmarkFolder);
		}
		else
		{
			const CBookmark &Bookmark = boost::get<CBookmark>(variantBookmark);
			InsertBookmark(Bookmark);
		}
	}
}

void CBookmarksToolbar::InsertBookmark(const CBookmark &Bookmark)
{
	int nButtons = static_cast<int>(SendMessage(m_hToolbar,TB_BUTTONCOUNT,0,0));
	InsertBookmarkItem(Bookmark.GetName(),Bookmark.GetGUID(),false,nButtons);
}

void CBookmarksToolbar::InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder)
{
	int nButtons = static_cast<int>(SendMessage(m_hToolbar,TB_BUTTONCOUNT,0,0));
	InsertBookmarkItem(BookmarkFolder.GetName(),BookmarkFolder.GetGUID(),true,nButtons);
}

void CBookmarksToolbar::InsertBookmarkItem(const std::wstring &strName,
	const GUID &guid,bool bFolder,int iPosition)
{
	TCHAR szName[256];
	StringCchCopy(szName,SIZEOF_ARRAY(szName),strName.c_str());

	int iImage;

	if(bFolder)
	{
		iImage = SHELLIMAGES_NEWTAB;
	}
	else
	{
		iImage = SHELLIMAGES_FAV;
	}

	TBBUTTON tbb;
	tbb.iBitmap		= iImage;
	tbb.idCommand	= m_uIDStart + m_uIDCounter;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE|BTNS_SHOWTEXT|BTNS_NOPREFIX;
	tbb.dwData		= m_uIDCounter;
	tbb.iString		= reinterpret_cast<INT_PTR>(szName);
	SendMessage(m_hToolbar,TB_INSERTBUTTON,iPosition,reinterpret_cast<LPARAM>(&tbb));

	m_mapID.insert(std::make_pair<UINT,GUID>(m_uIDCounter,guid));
	++m_uIDCounter;
}

void CBookmarksToolbar::OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark)
{
	if(IsEqualGUID(ParentBookmarkFolder.GetGUID(),m_guidBookmarksToolbar))
	{
		InsertBookmark(Bookmark);
	}
}

void CBookmarksToolbar::OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder)
{
	if(IsEqualGUID(ParentBookmarkFolder.GetGUID(),m_guidBookmarksToolbar))
	{
		InsertBookmarkFolder(BookmarkFolder);
	}
}

void CBookmarksToolbar::OnBookmarkModified(const GUID &guid)
{
	ModifyBookmarkItem(guid,false);
}

void CBookmarksToolbar::OnBookmarkFolderModified(const GUID &guid)
{
	ModifyBookmarkItem(guid,true);
}

void CBookmarksToolbar::OnBookmarkRemoved(const GUID &guid)
{
	RemoveBookmarkItem(guid);	
}

void CBookmarksToolbar::OnBookmarkFolderRemoved(const GUID &guid)
{
	RemoveBookmarkItem(guid);
}

void CBookmarksToolbar::ModifyBookmarkItem(const GUID &guid,bool bFolder)
{
	int iIndex = GetBookmarkItemIndex(guid);

	if(iIndex != -1)
	{
		auto variantBookmarksToolbar = NBookmarkHelper::GetBookmarkItem(m_AllBookmarks,m_guidBookmarksToolbar);
		CBookmarkFolder &BookmarksToolbarFolder = boost::get<CBookmarkFolder>(variantBookmarksToolbar);

		auto variantBookmarkItem = NBookmarkHelper::GetBookmarkItem(BookmarksToolbarFolder,guid);

		TCHAR szText[128];

		if(bFolder)
		{
			assert(variantBookmarkItem.type() == typeid(CBookmarkFolder));
			CBookmarkFolder &BookmarkFolder = boost::get<CBookmarkFolder>(variantBookmarkItem);

			StringCchCopy(szText,SIZEOF_ARRAY(szText),BookmarkFolder.GetName().c_str());
		}
		else
		{
			assert(variantBookmarkItem.type() == typeid(CBookmark));
			CBookmark &Bookmark = boost::get<CBookmark>(variantBookmarkItem);

			StringCchCopy(szText,SIZEOF_ARRAY(szText),Bookmark.GetName().c_str());
		}

		TBBUTTONINFO tbbi;
		tbbi.cbSize		= sizeof(tbbi);
		tbbi.dwMask		= TBIF_BYINDEX|TBIF_TEXT;
		tbbi.pszText	= szText;

		SendMessage(m_hToolbar,TB_SETBUTTONINFO,iIndex,reinterpret_cast<LPARAM>(&tbbi));
	}
}

void CBookmarksToolbar::RemoveBookmarkItem(const GUID &guid)
{
	int iIndex = GetBookmarkItemIndex(guid);

	if(iIndex != -1)
	{
		SendMessage(m_hToolbar,TB_DELETEBUTTON,iIndex,0);

		m_mapID.erase(iIndex);
	}
}

int CBookmarksToolbar::GetBookmarkItemIndex(const GUID &guid)
{
	int iIndex = -1;
	int nButtons = static_cast<int>(SendMessage(m_hToolbar,TB_BUTTONCOUNT,0,0));

	for(int i = 0;i < nButtons;i++)
	{
		TBBUTTON tb;
		SendMessage(m_hToolbar,TB_GETBUTTON,i,reinterpret_cast<LPARAM>(&tb));

		auto itr = m_mapID.find(static_cast<UINT>(tb.dwData));

		if(itr != m_mapID.end() &&
			IsEqualGUID(itr->second,guid))
		{
			iIndex = i;
			break;
		}
	}

	return iIndex;
}

/* TODO: */
//void Explorerplusplus::BookmarkToolbarNewBookmark(int iItem)
//{
//	if(iItem != -1)
//	{
//		/* TODO: Need to retrieve bookmark details. */
//		/*TBBUTTON tbButton;
//		SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,iItem,(LPARAM)&tbButton);*/
//
//		CBookmark Bookmark(EMPTY_STRING,EMPTY_STRING,EMPTY_STRING);
//
//		CAddBookmarkDialog AddBookmarkDialog(g_hLanguageModule,IDD_ADD_BOOKMARK,m_hContainer,*m_bfAllBookmarks,Bookmark);
//		AddBookmarkDialog.ShowModalDialog();
//	}
//}

//void Explorerplusplus::BookmarkToolbarNewFolder(int iItem)
//{
//	CNewBookmarkFolderDialog NewBookmarkFolderDialog(g_hLanguageModule,IDD_NEWBOOKMARKFOLDER,m_hContainer);
//	NewBookmarkFolderDialog.ShowModalDialog();
//}