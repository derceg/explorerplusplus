// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarksToolbar.h"
#include "AddBookmarkDialog.h"
#include "BookmarkMenu.h"
#include "MainResource.h"
#include "TabContainer.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include <algorithm>

CBookmarksToolbar::CBookmarksToolbar(HWND hToolbar, HINSTANCE instance, IExplorerplusplus *pexpp,
	Navigation *navigation, CBookmarkFolder &AllBookmarks, const GUID &guidBookmarksToolbar,
	UINT uIDStart, UINT uIDEnd) :
	m_hToolbar(hToolbar),
	m_instance(instance),
	m_pexpp(pexpp),
	m_navigation(navigation),
	m_AllBookmarks(AllBookmarks),
	m_guidBookmarksToolbar(guidBookmarksToolbar),
	m_uIDStart(uIDStart),
	m_uIDEnd(uIDEnd),
	m_uIDCounter(0)
{
	InitializeToolbar();

	CBookmarkItemNotifier::GetInstance().AddObserver(this);
}

void CBookmarksToolbar::InitializeToolbar()
{
	SendMessage(m_hToolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);

	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hToolbar);
	int iconWidth = m_dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = m_dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	SendMessage(m_hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));

	std::tie(m_imageList, m_imageListMappings) = ResourceHelper::CreateIconImageList(
		m_pexpp->GetIconResourceLoader(), iconWidth, iconHeight, { Icon::Folder, Icon::Bookmarks});
	SendMessage(m_hToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(m_imageList.get()));

	m_pbtdh = new CBookmarksToolbarDropHandler(m_hToolbar,m_AllBookmarks,m_guidBookmarksToolbar);
	RegisterDragDrop(m_hToolbar,m_pbtdh);

	m_windowSubclasses.push_back(WindowSubclassWrapper(m_hToolbar, BookmarksToolbarProcStub, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	/* Also subclass the parent window, so that WM_COMMAND/WM_NOTIFY messages
	can be caught. */
	m_windowSubclasses.push_back(WindowSubclassWrapper(GetParent(m_hToolbar), BookmarksToolbarParentProcStub, PARENT_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this)));

	InsertBookmarkItems();

	m_connections.push_back(m_pexpp->AddToolbarContextMenuObserver(boost::bind(&CBookmarksToolbar::OnToolbarContextMenuPreShow, this, _1, _2)));
}

CBookmarksToolbar::~CBookmarksToolbar()
{
	m_pbtdh->Release();

	CBookmarkItemNotifier::GetInstance().RemoveObserver(this);
}

LRESULT CALLBACK CBookmarksToolbar::BookmarksToolbarProcStub(HWND hwnd,UINT uMsg,
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

				if (auto variantBookmarkItem = GetBookmarkItemFromToolbarIndex(iIndex))
				{
					OpenBookmarkItemInNewTab(*variantBookmarkItem);
				}
			}
		}
		break;

	case WM_DESTROY:
		RevokeDragDrop(m_hToolbar);
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

// If the specified item is a bookmark, it will be opened in a new tab.
// If it's a bookmark folder, each of its children will be opened in new
// tabs.
void CBookmarksToolbar::OpenBookmarkItemInNewTab(const VariantBookmark &variantBookmarkItem)
{
	if (variantBookmarkItem.type() == typeid(CBookmarkFolder))
	{
		const CBookmarkFolder &bookmarkFolder = boost::get<CBookmarkFolder>(variantBookmarkItem);

		for (auto variantBookmarkChild : bookmarkFolder)
		{
			if (variantBookmarkChild.type() == typeid(CBookmark))
			{
				const CBookmark &bookmark = boost::get<CBookmark>(variantBookmarkChild);
				m_pexpp->GetTabContainer()->CreateNewTab(bookmark.GetLocation().c_str());
			}
		}
	}
	else
	{
		const CBookmark &bookmark = boost::get<CBookmark>(variantBookmarkItem);
		m_pexpp->GetTabContainer()->CreateNewTab(bookmark.GetLocation().c_str());
	}
}

LRESULT CALLBACK CBookmarksToolbar::BookmarksToolbarParentProcStub(HWND hwnd,UINT uMsg,
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
		if (OnCommand(wParam, lParam))
		{
			return 0;
		}
		break;

	case WM_NOTIFY:
		if(reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hToolbar)
		{
			switch(reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_RCLICK:
				if (OnRightClick(reinterpret_cast<NMMOUSE *>(lParam)))
				{
					return TRUE;
				}
				break;

			case TBN_GETINFOTIP:
				if (OnGetInfoTip(reinterpret_cast<NMTBGETINFOTIP *>(lParam)))
				{
					return 0;
				}
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

BOOL CBookmarksToolbar::OnRightClick(const NMMOUSE *nmm)
{
	if (nmm->dwItemSpec == -1)
	{
		return FALSE;
	}

	int index = static_cast<int>(SendMessage(m_hToolbar, TB_COMMANDTOINDEX, nmm->dwItemSpec, 0));

	if (index == -1)
	{
		return FALSE;
	}

	auto variantBookmarkItem = GetBookmarkItemFromToolbarIndex(index);

	if (!variantBookmarkItem)
	{
		return FALSE;
	}

	auto parentMenu = wil::unique_hmenu(LoadMenu(m_instance, MAKEINTRESOURCE(IDR_BOOKMARKSTOOLBAR_RCLICK_MENU)));

	if (!parentMenu)
	{
		return FALSE;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	POINT pt = nmm->pt;
	BOOL res = ClientToScreen(m_hToolbar, &pt);

	if (!res)
	{
		return FALSE;
	}

	int menuItemId = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, GetParent(m_hToolbar), NULL);

	if (menuItemId != 0)
	{
		OnRightClickMenuItemSelected(menuItemId, *variantBookmarkItem);
	}

	return TRUE;
}

void CBookmarksToolbar::OnRightClickMenuItemSelected(int menuItemId, const VariantBookmark &variantBookmark)
{
	switch (menuItemId)
	{
	case IDM_BT_OPEN:
	{
		if (variantBookmark.type() == typeid(CBookmark))
		{
			const CBookmark &bookmark = boost::get<CBookmark>(variantBookmark);
			m_navigation->BrowseFolderInCurrentTab(bookmark.GetLocation().c_str());
		}
	}
		break;

	case IDM_BT_OPENINNEWTAB:
		OpenBookmarkItemInNewTab(variantBookmark);
		break;

	case IDM_BT_NEWBOOKMARK:
		OnNewBookmark();
		break;

	case IDM_BT_NEWFOLDER:
		/* TODO: Handle menu item. */
		break;

	case IDM_BT_DELETE:
		/* TODO: Handle menu item. */
		break;

	case IDM_BT_PROPERTIES:
		/* TODO: Handle menu item. */
		break;
	}
}

bool CBookmarksToolbar::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		return false;
	}

	if ((LOWORD(wParam) >= m_uIDStart &&
		LOWORD(wParam) <= m_uIDEnd))
	{
		return OnButtonClick(LOWORD(wParam));
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDM_BT_NEWBOOKMARK:
			OnNewBookmark();
			return true;

		case IDM_BT_NEWFOLDER:
			/* TODO: Show new bookmark
			folder dialog. */
			return true;
		}
	}

	return false;
}

bool CBookmarksToolbar::OnButtonClick(int command)
{
	int index = static_cast<int>(SendMessage(m_hToolbar, TB_COMMANDTOINDEX, command, 0));

	if (index == -1)
	{
		return false;
	}

	auto variantBookmarkItem = GetBookmarkItemFromToolbarIndex(index);

	if (!variantBookmarkItem)
	{
		return false;
	}

	if (variantBookmarkItem->type() == typeid(CBookmarkFolder))
	{
		const CBookmarkFolder &bookmarkFolder = boost::get<CBookmarkFolder>(*variantBookmarkItem);
		ShowBookmarkFolderMenu(bookmarkFolder, command, index);
	}
	else
	{
		CBookmark &bookmark = boost::get<CBookmark>(*variantBookmarkItem);
		m_navigation->BrowseFolderInCurrentTab(bookmark.GetLocation().c_str());
	}

	return true;
}

void CBookmarksToolbar::ShowBookmarkFolderMenu(const CBookmarkFolder &bookmarkFolder, int command, int index)
{
	RECT rc;
	BOOL res = static_cast<BOOL>(SendMessage(m_hToolbar, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rc)));

	if (!res)
	{
		return;
	}

	SetLastError(ERROR_SUCCESS);
	auto mapRes = MapWindowPoints(m_hToolbar, nullptr, reinterpret_cast<LPPOINT>(&rc), 2);

	if (mapRes == 0 && GetLastError() != ERROR_SUCCESS)
	{
		return;
	}

	auto state = SendMessage(m_hToolbar, TB_GETSTATE, command, 0);

	if (state == -1)
	{
		return;
	}

	SendMessage(m_hToolbar, TB_SETSTATE, command, MAKEWORD(state | TBSTATE_PRESSED, 0));

	BookmarkMenu bookmarkMenu(m_instance);

	POINT pt;
	pt.x = rc.left;
	pt.y = rc.bottom;
	bookmarkMenu.ShowMenu(m_hToolbar, bookmarkFolder, pt ,
		boost::bind(&CBookmarksToolbar::OnBookmarkMenuItemClicked, this, _1));

	SendMessage(m_hToolbar, TB_SETSTATE, command, MAKEWORD(state & ~TBSTATE_PRESSED, 0));
}

void CBookmarksToolbar::OnBookmarkMenuItemClicked(const CBookmark &bookmark)
{
	m_navigation->BrowseFolderInCurrentTab(bookmark.GetLocation().c_str());
}

void CBookmarksToolbar::OnNewBookmark()
{
	TCHAR displayName[MAX_PATH];
	std::wstring currentDirectory = m_pexpp->GetActiveShellBrowser()->GetDirectory();
	GetDisplayName(currentDirectory.c_str(), displayName, SIZEOF_ARRAY(displayName), SHGDN_INFOLDER);
	CBookmark Bookmark = CBookmark::Create(displayName, currentDirectory, EMPTY_STRING);

	CAddBookmarkDialog AddBookmarkDialog(m_instance, IDD_ADD_BOOKMARK, m_hToolbar, m_pexpp, m_AllBookmarks, Bookmark);
	AddBookmarkDialog.ShowModalDialog();
}

bool CBookmarksToolbar::OnGetInfoTip(NMTBGETINFOTIP *infoTip)
{
	int index = static_cast<int>(SendMessage(m_hToolbar, TB_COMMANDTOINDEX, infoTip->iItem, 0));

	if (index == -1)
	{
		return false;
	}

	auto variantBookmarkItem = GetBookmarkItemFromToolbarIndex(index);

	if (!variantBookmarkItem)
	{
		return false;
	}

	if (variantBookmarkItem->type() == typeid(CBookmark))
	{
		CBookmark &Bookmark = boost::get<CBookmark>(*variantBookmarkItem);

		StringCchPrintf(infoTip->pszText, infoTip->cchTextMax, _T("%s\n%s"),
			Bookmark.GetName().c_str(), Bookmark.GetLocation().c_str());

		return true;
	}

	return false;
}

void CBookmarksToolbar::InsertBookmarkItems()
{
	/* The bookmarks toolbar folder should always be a direct child
	of the root. */
	auto variantBookmarksToolbar = NBookmarkHelper::GetBookmarkItem(m_AllBookmarks,m_guidBookmarksToolbar);
	assert(variantBookmarksToolbar.type() == typeid(CBookmarkFolder));
	const CBookmarkFolder &BookmarksToolbarFolder = boost::get<CBookmarkFolder>(variantBookmarksToolbar);

	for(const auto &variantBookmark : BookmarksToolbarFolder)
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
		iImage = m_imageListMappings.at(Icon::Folder);
	}
	else
	{
		iImage = m_imageListMappings.at(Icon::Bookmarks);
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

void CBookmarksToolbar::OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow)
{
	if (sourceWindow != m_hToolbar)
	{
		return;
	}

	TCHAR newBookmark[64];
	LoadString(m_instance, IDS_BOOKMARKS_TOOLBAR_NEW_BOOKMARK, newBookmark, SIZEOF_ARRAY(newBookmark));

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.dwTypeData = newBookmark;
	mii.wID = IDM_BT_NEWBOOKMARK;

	InsertMenuItem(menu, IDM_TOOLBARS_CUSTOMIZE, FALSE, &mii);

	TCHAR newBookmarkFolder[64];
	LoadString(m_instance, IDS_BOOKMARKS_TOOLBAR_NEW_FOLDER, newBookmarkFolder, SIZEOF_ARRAY(newBookmarkFolder));

	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.dwTypeData = newBookmarkFolder;
	mii.wID = IDM_BT_NEWFOLDER;

	InsertMenuItem(menu, IDM_TOOLBARS_CUSTOMIZE, FALSE, &mii);
}

VariantBookmark *CBookmarksToolbar::GetBookmarkItemFromToolbarIndex(int index)
{
	TBBUTTON tbButton;
	BOOL ret = static_cast<BOOL>(SendMessage(m_hToolbar, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&tbButton)));

	if (!ret)
	{
		return nullptr;
	}

	auto itr = m_mapID.find(static_cast<UINT>(tbButton.dwData));

	if (itr == m_mapID.end())
	{
		return nullptr;
	}

	auto &variantBookmarksToolbar = NBookmarkHelper::GetBookmarkItem(m_AllBookmarks, m_guidBookmarksToolbar);
	CBookmarkFolder &BookmarksToolbarFolder = boost::get<CBookmarkFolder>(variantBookmarksToolbar);

	auto &variantBookmarkItem = NBookmarkHelper::GetBookmarkItem(BookmarksToolbarFolder, itr->second);
	return &variantBookmarkItem;
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
m_ulRefCount(1),
m_hToolbar(hToolbar),
m_AllBookmarks(AllBookmarks),
m_guidBookmarksToolbar(guidBookmarksToolbar)
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

					CBookmark Bookmark = CBookmark::Create(szDisplayName, szFullFileName, EMPTY_STRING);

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
//void Explorerplusplus::BookmarkToolbarNewFolder(int iItem)
//{
//	CNewBookmarkFolderDialog NewBookmarkFolderDialog(g_hLanguageModule,IDD_NEWBOOKMARKFOLDER,m_hContainer);
//	NewBookmarkFolderDialog.ShowModalDialog();
//}