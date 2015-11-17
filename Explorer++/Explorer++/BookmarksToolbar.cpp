/******************************************************************
 *
 * Project: Explorer++
 * File: BookmarksToolbar.cpp
 * License: GPL - See LICENSE in the top level directory
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
#include "MainImages.h"
#include "BookmarksToolbar.h"
#include "MainResource.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/Macros.h"


CBookmarksToolbar::CBookmarksToolbar(HWND hToolbar,CBookmarkFolder &AllBookmarks,
	const GUID &guidBookmarksToolbar,UINT uIDStart,UINT uIDEnd) :
m_hToolbar(hToolbar),
m_AllBookmarks(AllBookmarks),
m_guidBookmarksToolbar(guidBookmarksToolbar),
m_uIDStart(uIDStart),
m_uIDEnd(uIDEnd),
m_uIDCounter(0)
{
	InitializeToolbar();

	CBookmarkItemNotifier::GetInstance().AddObserver(this);
}

CBookmarksToolbar::~CBookmarksToolbar()
{
	ImageList_Destroy(m_himl);

	RevokeDragDrop(m_hToolbar);
	m_pbtdh->Release();

	RemoveWindowSubclass(m_hToolbar,BookmarksToolbarProcStub,SUBCLASS_ID);
	RemoveWindowSubclass(GetParent(m_hToolbar),BookmarksToolbarParentProcStub,PARENT_SUBCLASS_ID);

	CBookmarkItemNotifier::GetInstance().RemoveObserver(this);
}

void CBookmarksToolbar::InitializeToolbar()
{
	SendMessage(m_hToolbar,TB_SETBITMAPSIZE,0,MAKELONG(16,16));
	SendMessage(m_hToolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);

	m_himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himl,hBitmap,NULL);
	SendMessage(m_hToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(m_himl));
	DeleteObject(hBitmap);

	m_pbtdh = new CBookmarksToolbarDropHandler(m_hToolbar,m_AllBookmarks,m_guidBookmarksToolbar);
	RegisterDragDrop(m_hToolbar,m_pbtdh);

	SetWindowSubclass(m_hToolbar,BookmarksToolbarProcStub,SUBCLASS_ID,reinterpret_cast<DWORD_PTR>(this));

	/* Also subclass the parent window, so that WM_COMMAND/WM_NOTIFY messages
	can be caught. */
	SetWindowSubclass(GetParent(m_hToolbar),BookmarksToolbarParentProcStub,PARENT_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this));

	InsertBookmarkItems();
}

LRESULT CALLBACK BookmarksToolbarProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

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

LRESULT CALLBACK BookmarksToolbarParentProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CBookmarksToolbar *pbt = reinterpret_cast<CBookmarksToolbar *>(dwRefData);

	return pbt->BookmarksToolbarParentProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CBookmarksToolbar::BookmarksToolbarParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		if(LOWORD(wParam) >= m_uIDStart &&
			LOWORD(wParam) <= m_uIDEnd)
		{
			/* TODO: Map the id back to a GUID, and
			then open the bookmark/show a dropdown
			list. */
			return 0;
		}
		break;

	case WM_NOTIFY:
		if(reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hToolbar)
		{
			switch(reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case TBN_GETINFOTIP:
				{
					//NMTBGETINFOTIP *pnmtbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

					/* TODO: Build an infotip for the bookmark. */
					//return 0;
				}
				break;
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
	InsertBookmark(Bookmark,nButtons);
}

void CBookmarksToolbar::InsertBookmark(const CBookmark &Bookmark,std::size_t Position)
{
	InsertBookmarkItem(Bookmark.GetName(),Bookmark.GetGUID(),false,Position);
}

void CBookmarksToolbar::InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder)
{
	int nButtons = static_cast<int>(SendMessage(m_hToolbar,TB_BUTTONCOUNT,0,0));
	InsertBookmarkFolder(BookmarkFolder,nButtons);
}

void CBookmarksToolbar::InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder,std::size_t Position)
{
	InsertBookmarkItem(BookmarkFolder.GetName(),BookmarkFolder.GetGUID(),true,Position);
}

void CBookmarksToolbar::InsertBookmarkItem(const std::wstring &strName,
	const GUID &guid,bool bFolder,std::size_t Position)
{
	assert(Position <= static_cast<std::size_t>(SendMessage(m_hToolbar,TB_BUTTONCOUNT,0,0)));

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
	SendMessage(m_hToolbar,TB_INSERTBUTTON,Position,reinterpret_cast<LPARAM>(&tbb));

	m_mapID.insert(std::make_pair(m_uIDCounter,guid));
	++m_uIDCounter;
}

void CBookmarksToolbar::OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmark &Bookmark,std::size_t Position)
{
	if(IsEqualGUID(ParentBookmarkFolder.GetGUID(),m_guidBookmarksToolbar))
	{
		InsertBookmark(Bookmark,Position);
	}
}

void CBookmarksToolbar::OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmarkFolder &BookmarkFolder,std::size_t Position)
{
	if(IsEqualGUID(ParentBookmarkFolder.GetGUID(),m_guidBookmarksToolbar))
	{
		InsertBookmarkFolder(BookmarkFolder,Position);
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

		/* TODO: */
		//UpdateToolbarBandSizing(m_hMainRebar,m_hBookmarksToolbar);

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

CBookmarksToolbarDropHandler::CBookmarksToolbarDropHandler(HWND hToolbar,
	CBookmarkFolder &AllBookmarks,const GUID &guidBookmarksToolbar) :
m_hToolbar(hToolbar),
m_AllBookmarks(AllBookmarks),
m_guidBookmarksToolbar(guidBookmarksToolbar),
m_ulRefCount(1)
{
	CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&m_pDragSourceHelper));

	/* Note that the above call is assumed to always succeed. */
	m_pDragSourceHelper->QueryInterface(IID_PPV_ARGS(&m_pDropTargetHelper));
}

CBookmarksToolbarDropHandler::~CBookmarksToolbarDropHandler()
{
	m_pDragSourceHelper->Release();
	m_pDropTargetHelper->Release();
}

HRESULT __stdcall CBookmarksToolbarDropHandler::QueryInterface(REFIID iid,void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IDropTarget ||
		iid == IID_IUnknown)
	{
		*ppvObject = this;
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CBookmarksToolbarDropHandler::AddRef(void)
{
	return ++m_ulRefCount;
}

ULONG __stdcall CBookmarksToolbarDropHandler::Release(void)
{
	m_ulRefCount--;
	
	if(m_ulRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_ulRefCount;
}

HRESULT __stdcall CBookmarksToolbarDropHandler::DragEnter(IDataObject *pDataObject,
	DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	UNREFERENCED_PARAMETER(grfKeyState);

	bool m_bValid = false;
	bool m_bAllFolders = true;

	FORMATETC ftc = {CF_HDROP,0,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	STGMEDIUM stg;

	HRESULT hr = pDataObject->GetData(&ftc,&stg);

	if(hr == S_OK)
	{
		DROPFILES *pdf = reinterpret_cast<DROPFILES *>(GlobalLock(stg.hGlobal));

		if(pdf != NULL)
		{
			m_bValid = true;

			UINT nDroppedFiles = DragQueryFile(reinterpret_cast<HDROP>(pdf),0xFFFFFFFF,NULL,NULL);

			for(UINT i = 0;i < nDroppedFiles;i++)
			{
				TCHAR szFullFileName[MAX_PATH];
				DragQueryFile(reinterpret_cast<HDROP>(pdf),i,szFullFileName,
					SIZEOF_ARRAY(szFullFileName));

				if(!PathIsDirectory(szFullFileName))
				{
					m_bAllFolders = false;
					break;
				}
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	if(m_bValid &&
		m_bAllFolders)
	{
		*pdwEffect = DROPEFFECT_COPY;

		m_bAcceptData = true;
	}
	else
	{
		*pdwEffect = DROPEFFECT_NONE;

		m_bAcceptData = false;
	}

	m_pDropTargetHelper->DragEnter(m_hToolbar,pDataObject,reinterpret_cast<POINT *>(&pt),*pdwEffect);

	return S_OK;
}

HRESULT __stdcall CBookmarksToolbarDropHandler::DragOver(DWORD grfKeyState,
	POINTL pt,DWORD *pdwEffect)
{
	UNREFERENCED_PARAMETER(grfKeyState);

	if(m_bAcceptData)
	{
		*pdwEffect = DROPEFFECT_COPY;
	}
	else
	{
		*pdwEffect = DROPEFFECT_NONE;
	}

	bool bAfter;
	int iButton = GetToolbarPositionIndex(pt,bAfter);

	if(iButton < 0)
	{
		int nButtons = static_cast<int>(SendMessage(m_hToolbar,TB_BUTTONCOUNT,0,0));

		TBINSERTMARK tbim;
		tbim.iButton = nButtons - 1;
		tbim.dwFlags = TBIMHT_AFTER;
		SendMessage(m_hToolbar,TB_SETINSERTMARK,0,reinterpret_cast<LPARAM>(&tbim));
	}
	else
	{
		TBINSERTMARK tbim;

		if(bAfter)
		{
			tbim.dwFlags = TBIMHT_AFTER;
		}
		else
		{
			tbim.dwFlags = 0;
		}

		tbim.iButton = iButton;
		SendMessage(m_hToolbar,TB_SETINSERTMARK,0,reinterpret_cast<LPARAM>(&tbim));
	}

	m_pDropTargetHelper->DragOver(reinterpret_cast<POINT *>(&pt),*pdwEffect);

	return S_OK;
}

HRESULT __stdcall CBookmarksToolbarDropHandler::DragLeave(void)
{
	RemoveInsertionMark();

	m_pDropTargetHelper->DragLeave();

	return S_OK;
}

HRESULT __stdcall CBookmarksToolbarDropHandler::Drop(IDataObject *pDataObject,
	DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	UNREFERENCED_PARAMETER(grfKeyState);

	FORMATETC ftc = {CF_HDROP,0,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	STGMEDIUM stg;

	HRESULT hr = pDataObject->GetData(&ftc,&stg);

	if(hr == S_OK)
	{
		DROPFILES *pdf = reinterpret_cast<DROPFILES *>(GlobalLock(stg.hGlobal));

		if(pdf != NULL)
		{
			bool bAfter;
			int iPosition = GetToolbarPositionIndex(pt,bAfter);

			if(iPosition < 0)
			{
				iPosition = static_cast<int>(SendMessage(m_hToolbar,TB_BUTTONCOUNT,0,0));
			}
			else
			{
				if(bAfter)
				{
					iPosition++;
				}
			}

			UINT nDroppedFiles = DragQueryFile(reinterpret_cast<HDROP>(pdf),0xFFFFFFFF,NULL,NULL);

			for(UINT i = 0;i < nDroppedFiles;i++)
			{
				TCHAR szFullFileName[MAX_PATH];
				DragQueryFile(reinterpret_cast<HDROP>(pdf),i,szFullFileName,
					SIZEOF_ARRAY(szFullFileName));

				if(PathIsDirectory(szFullFileName))
				{
					TCHAR szDisplayName[MAX_PATH];
					GetDisplayName(szFullFileName,szDisplayName,SIZEOF_ARRAY(szDisplayName),SHGDN_INFOLDER);

					CBookmark Bookmark(szDisplayName,szFullFileName,EMPTY_STRING);

					auto variantBookmarksToolbar = NBookmarkHelper::GetBookmarkItem(m_AllBookmarks,m_guidBookmarksToolbar);
					assert(variantBookmarksToolbar.type() == typeid(CBookmarkFolder));
					CBookmarkFolder &BookmarksToolbarFolder = boost::get<CBookmarkFolder>(variantBookmarksToolbar);

					BookmarksToolbarFolder.InsertBookmark(Bookmark,iPosition + i);
				}
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	RemoveInsertionMark();
	m_pDropTargetHelper->Drop(pDataObject,reinterpret_cast<POINT *>(&pt),*pdwEffect);

	return S_OK;
}

int CBookmarksToolbarDropHandler::GetToolbarPositionIndex(const POINTL &pt,bool &bAfter)
{
	POINT ptClient;
	ptClient.x = pt.x;
	ptClient.y = pt.y;
	ScreenToClient(m_hToolbar,&ptClient);
	int iButton = static_cast<int>(SendMessage(m_hToolbar,TB_HITTEST,
		0,reinterpret_cast<LPARAM>(&ptClient)));

	if(iButton >= 0)
	{
		RECT rc;
		SendMessage(m_hToolbar,TB_GETITEMRECT,iButton,reinterpret_cast<LPARAM>(&rc));

		bAfter = (ptClient.x > (rc.left + GetRectWidth(&rc) / 2)) ? true : false;
	}

	return iButton;
}

void CBookmarksToolbarDropHandler::RemoveInsertionMark()
{
	TBINSERTMARK tbim;
	tbim.iButton = -1;
	SendMessage(m_hToolbar,TB_SETINSERTMARK,0,reinterpret_cast<LPARAM>(&tbim));
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