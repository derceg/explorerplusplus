// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarksToolbar.h"
#include "AddBookmarkDialog.h"
#include "MainResource.h"
#include "TabContainer.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include <algorithm>

CBookmarksToolbar::CBookmarksToolbar(HWND hToolbar, HINSTANCE instance, IExplorerplusplus *pexpp,
	Navigation *navigation, BookmarkTree *bookmarkTree, UINT uIDStart, UINT uIDEnd) :
	m_hToolbar(hToolbar),
	m_instance(instance),
	m_pexpp(pexpp),
	m_navigation(navigation),
	m_bookmarkTree(bookmarkTree),
	m_uIDStart(uIDStart),
	m_uIDEnd(uIDEnd),
	m_bookmarkContextMenu(bookmarkTree, instance, pexpp),
	m_bookmarkMenu(bookmarkTree, instance, pexpp, hToolbar),
	m_uIDCounter(0)
{
	InitializeToolbar();
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

	m_pbtdh = new CBookmarksToolbarDropHandler(m_hToolbar, m_bookmarkTree);
	RegisterDragDrop(m_hToolbar, m_pbtdh);

	m_windowSubclasses.push_back(WindowSubclassWrapper(m_hToolbar, BookmarksToolbarProcStub, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	/* Also subclass the parent window, so that WM_COMMAND/WM_NOTIFY messages
	can be caught. */
	m_windowSubclasses.push_back(WindowSubclassWrapper(GetParent(m_hToolbar), BookmarksToolbarParentProcStub, PARENT_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this)));

	InsertBookmarkItems();

	m_connections.push_back(m_bookmarkTree->bookmarkItemAddedSignal.AddObserver(
		std::bind(&CBookmarksToolbar::OnBookmarkItemAdded, this, std::placeholders::_1, std::placeholders::_2)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemUpdatedSignal.AddObserver(
		std::bind(&CBookmarksToolbar::OnBookmarkItemUpdated, this, std::placeholders::_1, std::placeholders::_2)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemMovedSignal.AddObserver(
		std::bind(&CBookmarksToolbar::OnBookmarkItemMoved, this, std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemPreRemovalSignal.AddObserver(
		std::bind(&CBookmarksToolbar::OnBookmarkItemPreRemoval, this, std::placeholders::_1)));
	m_connections.push_back(m_pexpp->AddToolbarContextMenuObserver(
		std::bind(&CBookmarksToolbar::OnToolbarContextMenuPreShow, this, std::placeholders::_1, std::placeholders::_2)));
}

CBookmarksToolbar::~CBookmarksToolbar()
{
	m_pbtdh->Release();
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

				auto bookmarkItem = GetBookmarkItemFromToolbarIndex(iIndex);

				if (bookmarkItem)
				{
					BookmarkHelper::OpenBookmarkItemInNewTab(bookmarkItem, m_pexpp);
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

	auto bookmarkItem = GetBookmarkItemFromToolbarIndex(index);

	if (!bookmarkItem)
	{
		return FALSE;
	}

	POINT pt = nmm->pt;
	BOOL res = ClientToScreen(m_hToolbar, &pt);

	if (!res)
	{
		return FALSE;
	}

	m_bookmarkContextMenu.ShowMenu(m_hToolbar, bookmarkItem, pt);

	return TRUE;
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
			OnNewBookmarkItem(BookmarkItem::Type::Bookmark);
			return true;

		case IDM_BT_NEWFOLDER:
			OnNewBookmarkItem(BookmarkItem::Type::Folder);
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

	auto bookmarkItem = GetBookmarkItemFromToolbarIndex(index);

	if (!bookmarkItem)
	{
		return false;
	}

	if (bookmarkItem->IsFolder())
	{
		ShowBookmarkFolderMenu(bookmarkItem, command, index);
	}
	else
	{
		m_navigation->BrowseFolderInCurrentTab(bookmarkItem->GetLocation().c_str());
	}

	return true;
}

void CBookmarksToolbar::ShowBookmarkFolderMenu(BookmarkItem *bookmarkItem, int command, int index)
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

	POINT pt;
	pt.x = rc.left;
	pt.y = rc.bottom;
	m_bookmarkMenu.ShowMenu(bookmarkItem, pt, std::bind(&CBookmarksToolbar::OnBookmarkMenuItemClicked,
		this, std::placeholders::_1));

	SendMessage(m_hToolbar, TB_SETSTATE, command, MAKEWORD(state & ~TBSTATE_PRESSED, 0));
}

void CBookmarksToolbar::OnBookmarkMenuItemClicked(const BookmarkItem *bookmarkItem)
{
	assert(bookmarkItem->IsBookmark());

	m_navigation->BrowseFolderInCurrentTab(bookmarkItem->GetLocation().c_str());
}

void CBookmarksToolbar::OnNewBookmarkItem(BookmarkItem::Type type)
{
	BookmarkHelper::AddBookmarkItem(m_bookmarkTree, type, m_instance, m_hToolbar,
		m_pexpp->GetTabContainer(), m_pexpp);
}

void CBookmarksToolbar::OnEditBookmarkItem(BookmarkItem *bookmarkItem)
{
	BookmarkHelper::EditBookmarkItem(bookmarkItem, m_bookmarkTree, m_instance,
		m_hToolbar, m_pexpp);
}

bool CBookmarksToolbar::OnGetInfoTip(NMTBGETINFOTIP *infoTip)
{
	int index = static_cast<int>(SendMessage(m_hToolbar, TB_COMMANDTOINDEX, infoTip->iItem, 0));

	if (index == -1)
	{
		return false;
	}

	auto bookmarkItem = GetBookmarkItemFromToolbarIndex(index);

	if (!bookmarkItem)
	{
		return false;
	}

	if (bookmarkItem->IsBookmark())
	{
		StringCchPrintf(infoTip->pszText, infoTip->cchTextMax, _T("%s\n%s"),
			bookmarkItem->GetName().c_str(), bookmarkItem->GetLocation().c_str());

		return true;
	}

	return false;
}

void CBookmarksToolbar::InsertBookmarkItems()
{
	int position = 0;

	for (const auto &bookmarkItem : m_bookmarkTree->GetBookmarksToolbarFolder()->GetChildren())
	{
		InsertBookmarkItem(bookmarkItem.get(), position);

		position++;
	}
}

void CBookmarksToolbar::InsertBookmarkItem(BookmarkItem *bookmarkItem, int position)
{
	assert(position <= SendMessage(m_hToolbar, TB_BUTTONCOUNT, 0, 0));

	TCHAR szName[256];
	StringCchCopy(szName, SIZEOF_ARRAY(szName), bookmarkItem->GetName().c_str());

	int iImage;

	if(bookmarkItem->IsFolder())
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
	tbb.dwData		= reinterpret_cast<DWORD_PTR>(bookmarkItem);
	tbb.iString		= reinterpret_cast<INT_PTR>(szName);
	SendMessage(m_hToolbar, TB_INSERTBUTTON, position, reinterpret_cast<LPARAM>(&tbb));

	++m_uIDCounter;
}

void CBookmarksToolbar::OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index)
{
	if(bookmarkItem.GetParent() == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		InsertBookmarkItem(&bookmarkItem, static_cast<int>(index));
	}
}

void CBookmarksToolbar::OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType)
{
	if (propertyType != BookmarkItem::PropertyType::Name)
	{
		return;
	}

	auto index = GetBookmarkItemIndex(&bookmarkItem);

	if (!index)
	{
		return;
	}

	TCHAR name[128];
	StringCchCopy(name, std::size(name), bookmarkItem.GetName().c_str());

	TBBUTTONINFO tbbi;
	tbbi.cbSize = sizeof(tbbi);
	tbbi.dwMask = TBIF_BYINDEX | TBIF_TEXT;
	tbbi.pszText = name;
	SendMessage(m_hToolbar, TB_SETBUTTONINFO, *index, reinterpret_cast<LPARAM>(&tbbi));
}

void CBookmarksToolbar::OnBookmarkItemMoved(BookmarkItem *bookmarkItem, const BookmarkItem *oldParent,
	size_t oldIndex, const BookmarkItem *newParent, size_t newIndex)
{
	UNREFERENCED_PARAMETER(oldIndex);

	if (oldParent == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		RemoveBookmarkItem(bookmarkItem);
	}

	if (newParent == m_bookmarkTree->GetBookmarksToolbarFolder())
	{
		InsertBookmarkItem(bookmarkItem, static_cast<int>(newIndex));
	}
}

void CBookmarksToolbar::OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem)
{
	RemoveBookmarkItem(&bookmarkItem);
}

void CBookmarksToolbar::RemoveBookmarkItem(const BookmarkItem *bookmarkItem)
{
	auto index = GetBookmarkItemIndex(bookmarkItem);
	assert(index);

	SendMessage(m_hToolbar, TB_DELETEBUTTON, *index, 0);

	/* TODO: */
	//UpdateToolbarBandSizing(m_hMainRebar,m_hBookmarksToolbar);
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

BookmarkItem *CBookmarksToolbar::GetBookmarkItemFromToolbarIndex(int index)
{
	TBBUTTON tbButton;
	BOOL ret = static_cast<BOOL>(SendMessage(m_hToolbar, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&tbButton)));

	if (!ret)
	{
		return nullptr;
	}

	return reinterpret_cast<BookmarkItem *>(tbButton.dwData);
}

std::optional<int> CBookmarksToolbar::GetBookmarkItemIndex(const BookmarkItem *bookmarkItem) const
{
	int nButtons = static_cast<int>(SendMessage(m_hToolbar, TB_BUTTONCOUNT, 0, 0));

	for (int i = 0; i < nButtons; i++)
	{
		TBBUTTON tb;
		SendMessage(m_hToolbar, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tb));

		BookmarkItem *currentBookmarkItem = reinterpret_cast<BookmarkItem *>(tb.dwData);

		if (currentBookmarkItem == bookmarkItem)
		{
			return i;
		}
	}

	return std::nullopt;
}

CBookmarksToolbarDropHandler::CBookmarksToolbarDropHandler(HWND hToolbar, BookmarkTree *bookmarkTree) :
	m_ulRefCount(1),
	m_hToolbar(hToolbar),
	m_bookmarkTree(bookmarkTree)
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

					auto bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt, szDisplayName, szFullFileName);
					m_bookmarkTree->AddBookmarkItem(m_bookmarkTree->GetBookmarksToolbarFolder(), std::move(bookmarkItem), iPosition + i);
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