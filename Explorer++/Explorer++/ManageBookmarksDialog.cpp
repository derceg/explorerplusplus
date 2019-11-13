// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ManageBookmarksDialog.h"
#include "BookmarkHelper.h"
#include "Explorer++_internal.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "../Helper/Controls.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"

namespace NManageBookmarksDialog
{
	int CALLBACK		SortBookmarksStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
}

const TCHAR CManageBookmarksDialogPersistentSettings::SETTINGS_KEY[] = _T("ManageBookmarks");

CManageBookmarksDialog::CManageBookmarksDialog(HINSTANCE hInstance, int iResource, HWND hParent,
	IExplorerplusplus *pexpp, Navigation *navigation, CBookmarkFolder &AllBookmarks) :
	CBaseDialog(hInstance, iResource, hParent, true),
	m_pexpp(pexpp),
	m_navigation(navigation),
	m_AllBookmarks(AllBookmarks),
	m_guidCurrentFolder(AllBookmarks.GetGUID()),
	m_bNewFolderAdded(false),
	m_bListViewInitialized(false),
	m_bSaveHistory(true)
{
	m_pmbdps = &CManageBookmarksDialogPersistentSettings::GetInstance();

	if(!m_pmbdps->m_bInitialized)
	{
		m_pmbdps->m_guidSelected = AllBookmarks.GetGUID();
		m_pmbdps->m_setExpansion.insert(AllBookmarks.GetGUID());

		m_pmbdps->m_bInitialized = true;
	}
}

CManageBookmarksDialog::~CManageBookmarksDialog()
{
	delete m_pBookmarkTreeView;
	delete m_pBookmarkListView;
}

INT_PTR CManageBookmarksDialog::OnInitDialog()
{
	/* TODO: Enable drag and drop for listview and treeview. */
	SetDialogIcon();
	SetupToolbar();
	SetupTreeView();
	SetupListView();

	CBookmarkItemNotifier::GetInstance().AddObserver(this);

	UpdateToolbarState();

	SetFocus(GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW));

	return 0;
}

void CManageBookmarksDialog::SetDialogIcon()
{
	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hDlg);
	m_icon = IconResourceLoader::LoadIconFromPNGForDpi(Icon::Bookmarks, DIALOG_ICON_SIZE_96DPI, dpi);
	SetClassLongPtr(m_hDlg,GCLP_HICONSM,reinterpret_cast<LONG_PTR>(m_icon.get()));
}

void CManageBookmarksDialog::SetupToolbar()
{
	m_hToolbar = CreateToolbar(m_hDlg,
		WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
		TBSTYLE_TOOLTIPS|TBSTYLE_LIST|TBSTYLE_TRANSPARENT|
		TBSTYLE_FLAT|CCS_NODIVIDER|CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS|
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	SendMessage(m_hToolbar,TB_SETBITMAPSIZE,0,MAKELONG(16,16));
	SendMessage(m_hToolbar,TB_BUTTONSTRUCTSIZE,static_cast<WPARAM>(sizeof(TBBUTTON)),0);

	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hToolbar);
	std::tie(m_imageListToolbar, m_imageListToolbarMappings) = CreateIconImageList(16, dpi, { Icon::Back, Icon::Forward, Icon::Copy, Icon::Views});
	SendMessage(m_hToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(m_imageListToolbar.get()));

	TBBUTTON tbb;
	TCHAR szTemp[64];

	LoadString(GetInstance(), IDS_MANAGE_BOOKMARKS_TOOLBAR_BACK, szTemp, SIZEOF_ARRAY(szTemp));

	tbb.iBitmap		= m_imageListToolbarMappings.at(Icon::Back);
	tbb.idCommand	= TOOLBAR_ID_BACK;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE;
	tbb.dwData		= 0;
	tbb.iString		= reinterpret_cast<INT_PTR>(szTemp);
	SendMessage(m_hToolbar,TB_INSERTBUTTON,0,reinterpret_cast<LPARAM>(&tbb));

	LoadString(GetInstance(), IDS_MANAGE_BOOKMARKS_TOOLBAR_FORWARD, szTemp, SIZEOF_ARRAY(szTemp));

	tbb.iBitmap		= m_imageListToolbarMappings.at(Icon::Forward);
	tbb.idCommand	= TOOLBAR_ID_FORWARD;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE;
	tbb.dwData		= 0;
	tbb.iString		= reinterpret_cast<INT_PTR>(szTemp);
	SendMessage(m_hToolbar,TB_INSERTBUTTON,1,reinterpret_cast<LPARAM>(&tbb));

	LoadString(GetInstance(),IDS_MANAGE_BOOKMARKS_TOOLBAR_ORGANIZE,szTemp,SIZEOF_ARRAY(szTemp));

	tbb.iBitmap		= m_imageListToolbarMappings.at(Icon::Copy);
	tbb.idCommand	= TOOLBAR_ID_ORGANIZE;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE|BTNS_SHOWTEXT|BTNS_DROPDOWN;
	tbb.dwData		= 0;
	tbb.iString		= reinterpret_cast<INT_PTR>(szTemp);
	SendMessage(m_hToolbar,TB_INSERTBUTTON,2,reinterpret_cast<LPARAM>(&tbb));

	LoadString(GetInstance(),IDS_MANAGE_BOOKMARKS_TOOLBAR_VIEWS,szTemp,SIZEOF_ARRAY(szTemp));

	tbb.iBitmap		= m_imageListToolbarMappings.at(Icon::Views);
	tbb.idCommand	= TOOLBAR_ID_VIEWS;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE|BTNS_SHOWTEXT|BTNS_DROPDOWN;
	tbb.dwData		= 0;
	tbb.iString		= reinterpret_cast<INT_PTR>(szTemp);
	SendMessage(m_hToolbar,TB_INSERTBUTTON,3,reinterpret_cast<LPARAM>(&tbb));

	RECT rcTreeView;
	GetWindowRect(GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW),&rcTreeView);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,reinterpret_cast<LPPOINT>(&rcTreeView),2);

	RECT rcListView;
	GetWindowRect(GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW), &rcListView);
	MapWindowPoints(HWND_DESKTOP, m_hDlg, reinterpret_cast<LPPOINT>(&rcListView), 2);

	DWORD dwButtonSize = static_cast<DWORD>(SendMessage(m_hToolbar,TB_GETBUTTONSIZE,0,0));
	SetWindowPos(m_hToolbar,NULL,rcTreeView.left,(rcTreeView.top - HIWORD(dwButtonSize)) / 2,
		rcListView.right - rcTreeView.left,HIWORD(dwButtonSize),0);
}

void CManageBookmarksDialog::SetupTreeView()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);

	m_pBookmarkTreeView = new CBookmarkTreeView(hTreeView,GetInstance(),&m_AllBookmarks,
		m_pmbdps->m_guidSelected,m_pmbdps->m_setExpansion);
}

void CManageBookmarksDialog::SetupListView()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

	m_pBookmarkListView = new CBookmarkListView(hListView);

	int iColumn = 0;

	for(const auto &ci : m_pmbdps->m_vectorColumnInfo)
	{
		if(ci.bActive)
		{
			LVCOLUMN lvCol;
			TCHAR szTemp[128];

			GetColumnString(ci.ColumnType,szTemp,SIZEOF_ARRAY(szTemp));
			lvCol.mask		= LVCF_TEXT|LVCF_WIDTH;
			lvCol.pszText	= szTemp;
			lvCol.cx		= ci.iWidth;
			ListView_InsertColumn(hListView,iColumn,&lvCol);

			++iColumn;
		}
	}

	m_pBookmarkListView->InsertBookmarksIntoListView(m_AllBookmarks);

	int iItem = 0;

	/* Update the data for each of the sub-items. */
	/* TODO: This needs to be done by CBookmarkListView. */
	for(auto itr = m_AllBookmarks.begin();itr != m_AllBookmarks.end();++itr)
	{
		int iSubItem = 1;

		for(const auto &ci : m_pmbdps->m_vectorColumnInfo)
		{
			/* The name column will always appear first in
			the set of columns and can be skipped here. */
			if(ci.bActive && ci.ColumnType != CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_NAME)
			{
				TCHAR szColumn[256];
				GetBookmarkItemColumnInfo(*itr,ci.ColumnType,szColumn,SIZEOF_ARRAY(szColumn));
				ListView_SetItemText(hListView,iItem,iSubItem,szColumn);

				++iSubItem;
			}
		}

		++iItem;
	}

	ListView_SortItems(hListView,NManageBookmarksDialog::SortBookmarksStub,reinterpret_cast<LPARAM>(this));

	NListView::ListView_SelectItem(hListView,0,TRUE);

	m_bListViewInitialized = true;
}

void CManageBookmarksDialog::SortListViewItems(NBookmarkHelper::SortMode_t SortMode)
{
	m_pmbdps->m_SortMode = SortMode;

	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);
	ListView_SortItems(hListView,NManageBookmarksDialog::SortBookmarksStub,reinterpret_cast<LPARAM>(this));
}

int CALLBACK NManageBookmarksDialog::SortBookmarksStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	assert(lParamSort != NULL);

	CManageBookmarksDialog *pmbd = reinterpret_cast<CManageBookmarksDialog *>(lParamSort);

	return pmbd->SortBookmarks(lParam1,lParam2);
}

int CALLBACK CManageBookmarksDialog::SortBookmarks(LPARAM lParam1,LPARAM lParam2)
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);
	HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
	CBookmarkFolder &BookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected);

	const VariantBookmark &variantBookmark1 = m_pBookmarkListView->GetBookmarkItemFromListViewlParam(BookmarkFolder,lParam1);
	const VariantBookmark &variantBookmark2 = m_pBookmarkListView->GetBookmarkItemFromListViewlParam(BookmarkFolder,lParam2);

	int iRes = NBookmarkHelper::Sort(m_pmbdps->m_SortMode,variantBookmark1,variantBookmark2);

	if(!m_pmbdps->m_bSortAscending)
	{
		iRes = -iRes;
	}

	return iRes;
}

INT_PTR CManageBookmarksDialog::OnAppCommand(HWND hwnd,UINT uCmd,UINT uDevice,DWORD dwKeys)
{
	UNREFERENCED_PARAMETER(dwKeys);
	UNREFERENCED_PARAMETER(uDevice);
	UNREFERENCED_PARAMETER(hwnd);

	switch(uCmd)
	{
	case APPCOMMAND_BROWSER_BACKWARD:
		BrowseBack();
		break;

	case APPCOMMAND_BROWSER_FORWARD:
		BrowseForward();
		break;
	}

	return 0;
}

INT_PTR CManageBookmarksDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch(LOWORD(wParam))
	{
	case TOOLBAR_ID_BACK:
		BrowseBack();
		break;

	case TOOLBAR_ID_FORWARD:
		BrowseForward();
		break;

	case TOOLBAR_ID_ORGANIZE:
		ShowOrganizeMenu();
		break;

	case TOOLBAR_ID_VIEWS:
		ShowViewMenu();
		break;

	case IDM_MB_ORGANIZE_NEWFOLDER:
		OnNewFolder();
		break;

	case IDM_MB_VIEW_SORTBYNAME:
		SortListViewItems(NBookmarkHelper::SM_NAME);
		break;

	case IDM_MB_VIEW_SORTBYLOCATION:
		SortListViewItems(NBookmarkHelper::SM_LOCATION);
		break;

	case IDM_MB_VIEW_SORTBYVISITDATE:
		SortListViewItems(NBookmarkHelper::SM_VISIT_DATE);
		break;

	case IDM_MB_VIEW_SORTBYVISITCOUNT:
		SortListViewItems(NBookmarkHelper::SM_VISIT_COUNT);
		break;

	case IDM_MB_VIEW_SORTBYADDED:
		SortListViewItems(NBookmarkHelper::SM_ADDED);
		break;

	case IDM_MB_VIEW_SORTBYLASTMODIFIED:
		SortListViewItems(NBookmarkHelper::SM_LAST_MODIFIED);
		break;

	case IDM_MB_VIEW_SORTASCENDING:
		m_pmbdps->m_bSortAscending = true;
		SortListViewItems(m_pmbdps->m_SortMode);
		break;

	case IDM_MB_VIEW_SORTDESCENDING:
		m_pmbdps->m_bSortAscending = false;
		SortListViewItems(m_pmbdps->m_SortMode);
		break;

		/* TODO: */
	case IDM_MB_BOOKMARK_OPEN:
		break;

	case IDM_MB_BOOKMARK_OPENINNEWTAB:
		break;

	case IDM_MB_BOOKMARK_DELETE:
		//OnDeleteBookmark();
		break;

	case IDOK:
		OnOk();
		break;

	case IDCANCEL:
		OnCancel();
		break;
	}

	return 0;
}

INT_PTR CManageBookmarksDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_DBLCLK:
		OnDblClk(pnmhdr);
		break;

	case NM_RCLICK:
		OnRClick(pnmhdr);
		break;

	case TBN_DROPDOWN:
		OnTbnDropDown(reinterpret_cast<NMTOOLBAR *>(pnmhdr));
		break;

	case TVN_SELCHANGED:
		OnTvnSelChanged(reinterpret_cast<NMTREEVIEW *>(pnmhdr));
		break;

	case LVN_ENDLABELEDIT:
		return OnLvnEndLabelEdit(reinterpret_cast<NMLVDISPINFO *>(pnmhdr));
		break;

	case LVN_KEYDOWN:
		OnLvnKeyDown(reinterpret_cast<NMLVKEYDOWN *>(pnmhdr));
		break;
	}

	return 0;
}

void CManageBookmarksDialog::OnNewFolder()
{
	TCHAR szTemp[64];
	LoadString(GetInstance(),IDS_BOOKMARKS_NEWBOOKMARKFOLDER,szTemp,SIZEOF_ARRAY(szTemp));
	CBookmarkFolder NewBookmarkFolder = CBookmarkFolder::Create(szTemp);

	/* Save the folder GUID, so that it can be selected and
	placed into edit mode once the bookmark notification
	comes through. */
	m_bNewFolderAdded = true;
	m_guidNewFolder = NewBookmarkFolder.GetGUID();

	HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);
	HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);

	assert(hSelectedItem != NULL);

	CBookmarkFolder &ParentBookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(
		hSelectedItem);
	ParentBookmarkFolder.InsertBookmarkFolder(NewBookmarkFolder);
}

void CManageBookmarksDialog::OnDeleteBookmark(const GUID &guid)
{
	UNREFERENCED_PARAMETER(guid);

	/* TODO: Move the bookmark/bookmark folder to the trash folder. */
}

void CManageBookmarksDialog::OnListViewRClick()
{
	DWORD dwCursorPos = GetMessagePos();

	POINT ptCursor;
	ptCursor.x = GET_X_LPARAM(dwCursorPos);
	ptCursor.y = GET_Y_LPARAM(dwCursorPos);

	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

	LVHITTESTINFO lvhti;
	lvhti.pt = ptCursor;
	ScreenToClient(hListView,&lvhti.pt);
	int iItem = ListView_HitTest(hListView,&lvhti);

	if(iItem == -1)
	{
		return;
	}

	HMENU hMenu = LoadMenu(GetInstance(),MAKEINTRESOURCE(IDR_MANAGEBOOKMARKS_BOOKMARK_RCLICK_MENU));
	SetMenuDefaultItem(GetSubMenu(hMenu,0),IDM_MB_BOOKMARK_OPEN,FALSE);

	TrackPopupMenu(GetSubMenu(hMenu,0),TPM_LEFTALIGN,ptCursor.x,ptCursor.y,0,m_hDlg,NULL);
	DestroyMenu(hMenu);
}

void CManageBookmarksDialog::OnListViewHeaderRClick()
{
	DWORD dwCursorPos = GetMessagePos();

	POINT ptCursor;
	ptCursor.x = GET_X_LPARAM(dwCursorPos);
	ptCursor.y = GET_Y_LPARAM(dwCursorPos);

	HMENU hMenu = CreatePopupMenu();
	int iMenuItem = 0;

	for(const auto &ci : m_pmbdps->m_vectorColumnInfo)
	{
		TCHAR szColumn[128];
		GetColumnString(ci.ColumnType,szColumn,SIZEOF_ARRAY(szColumn));

		MENUITEMINFO mii;
		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_ID|MIIM_STRING|MIIM_STATE;
		mii.wID			= ci.ColumnType;
		mii.dwTypeData	= szColumn;
		mii.fState		= 0;

		if(ci.bActive)
		{
			mii.fState |= MFS_CHECKED;
		}

		/* The name column cannot be removed. */
		if(ci.ColumnType == CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_NAME)
		{
			mii.fState |= MFS_DISABLED;
		}

		InsertMenuItem(hMenu,iMenuItem,TRUE,&mii);

		++iMenuItem;
	}

	int iCmd = TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_RETURNCMD,ptCursor.x,ptCursor.y,0,m_hDlg,NULL);
	DestroyMenu(hMenu);

	int iColumn = 0;

	for(auto itr = m_pmbdps->m_vectorColumnInfo.begin();itr != m_pmbdps->m_vectorColumnInfo.end();++itr)
	{
		if(itr->ColumnType == iCmd)
		{
			HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

			if(itr->bActive)
			{
				itr->iWidth = ListView_GetColumnWidth(hListView,iColumn);
				ListView_DeleteColumn(hListView,iColumn);
			}
			else
			{
				LVCOLUMN lvCol;
				TCHAR szTemp[128];

				GetColumnString(itr->ColumnType,szTemp,SIZEOF_ARRAY(szTemp));
				lvCol.mask		= LVCF_TEXT|LVCF_WIDTH;
				lvCol.pszText	= szTemp;
				lvCol.cx		= itr->iWidth;
				ListView_InsertColumn(hListView,iColumn,&lvCol);

				HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);
				HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
				CBookmarkFolder &BookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected);

				int iBookmarkItem = 0;

				for(auto itrBookmarks = BookmarkFolder.begin();itrBookmarks != BookmarkFolder.end();++itrBookmarks)
				{
					TCHAR szColumn[256];
					GetBookmarkItemColumnInfo(*itrBookmarks,itr->ColumnType,szColumn,SIZEOF_ARRAY(szColumn));
					ListView_SetItemText(hListView,iBookmarkItem,iColumn,szColumn);

					++iBookmarkItem;
				}
			}

			itr->bActive = !itr->bActive;

			break;
		}
		else
		{
			if(itr->bActive)
			{
				++iColumn;
			}
		}
	}
}

BOOL CManageBookmarksDialog::OnLvnEndLabelEdit(NMLVDISPINFO *pnmlvdi)
{
	if(pnmlvdi->item.pszText != NULL &&
		lstrlen(pnmlvdi->item.pszText) > 0)
	{
		HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);
		HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);

		assert(hSelectedItem != NULL);

		CBookmarkFolder &ParentBookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelectedItem);
		VariantBookmark &variantBookmark = m_pBookmarkListView->GetBookmarkItemFromListView(
			ParentBookmarkFolder,pnmlvdi->item.iItem);

		if(variantBookmark.type() == typeid(CBookmarkFolder))
		{
			CBookmarkFolder &BookmarkFolder = boost::get<CBookmarkFolder>(variantBookmark);
			BookmarkFolder.SetName(pnmlvdi->item.pszText);
		}
		else if(variantBookmark.type() == typeid(CBookmark))
		{
			CBookmark &Bookmark = boost::get<CBookmark>(variantBookmark);
			Bookmark.SetName(pnmlvdi->item.pszText);
		}

		SetWindowLongPtr(m_hDlg,DWLP_MSGRESULT,TRUE);
		return TRUE;
	}

	SetWindowLongPtr(m_hDlg,DWLP_MSGRESULT,FALSE);
	return FALSE;
}

void CManageBookmarksDialog::OnLvnKeyDown(NMLVKEYDOWN *pnmlvkd)
{
	switch(pnmlvkd->wVKey)
	{
	case VK_F2:
		OnListViewRename();
		break;

	case 'A':
		if(IsKeyDown(VK_CONTROL) &&
			!IsKeyDown(VK_SHIFT) &&
			!IsKeyDown(VK_MENU))
		{
			HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);
			NListView::ListView_SelectAllItems(hListView,TRUE);
			SetFocus(hListView);
		}
		break;

	/* TODO: */
	case VK_RETURN:
		break;

	case VK_DELETE:
		break;
	}
}

void CManageBookmarksDialog::OnListViewRename()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);
	int iItem = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iItem != -1)
	{
		ListView_EditLabel(hListView,iItem);
	}
}

void CManageBookmarksDialog::GetColumnString(CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType,
	TCHAR *szColumn,UINT cchBuf)
{
	UINT uResourceID = 0;

	switch(ColumnType)
	{
	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_NAME:
		uResourceID = IDS_MANAGE_BOOKMARKS_COLUMN_NAME;
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_LOCATION:
		uResourceID = IDS_MANAGE_BOOKMARKS_COLUMN_LOCATION;
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_VISIT_DATE:
		uResourceID = IDS_MANAGE_BOOKMARKS_COLUMN_VISIT_DATE;
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_VISIT_COUNT:
		uResourceID = IDS_MANAGE_BOOKMARKS_COLUMN_VISIT_COUNT;
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_ADDED:
		uResourceID = IDS_MANAGE_BOOKMARKS_COLUMN_ADDED;
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_LAST_MODIFIED:
		uResourceID = IDS_MANAGE_BOOKMARKS_COLUMN_LAST_MODIFIED;
		break;

	default:
		assert(FALSE);
		break;
	}

	LoadString(GetInstance(),uResourceID,szColumn,cchBuf);
}

void CManageBookmarksDialog::GetBookmarkItemColumnInfo(const VariantBookmark &variantBookmark,
	CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,size_t cchBuf)
{
	if(variantBookmark.type() == typeid(CBookmarkFolder))
	{
		const CBookmarkFolder &BookmarkFolder = boost::get<CBookmarkFolder>(variantBookmark);
		GetBookmarkFolderColumnInfo(BookmarkFolder,ColumnType,szColumn,cchBuf);
	}
	else
	{
		const CBookmark &Bookmark = boost::get<CBookmark>(variantBookmark);
		GetBookmarkColumnInfo(Bookmark,ColumnType,szColumn,cchBuf);
	}
}

void CManageBookmarksDialog::GetBookmarkColumnInfo(const CBookmark &Bookmark,
	CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType,
	TCHAR *szColumn,size_t cchBuf)
{
	switch(ColumnType)
	{
	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_NAME:
		StringCchCopy(szColumn,cchBuf,Bookmark.GetName().c_str());
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_LOCATION:
		StringCchCopy(szColumn,cchBuf,Bookmark.GetLocation().c_str());
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_VISIT_DATE:
		{
			/* TODO: Friendly dates. */
			FILETIME ftLastVisited = Bookmark.GetDateLastVisited();
			CreateFileTimeString(&ftLastVisited,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_VISIT_COUNT:
		StringCchPrintf(szColumn,cchBuf,_T("%d"),Bookmark.GetVisitCount());
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_ADDED:
		{
			/* TODO: Friendly dates. */
			FILETIME ftCreated = Bookmark.GetDateCreated();
			CreateFileTimeString(&ftCreated,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_LAST_MODIFIED:
		{
			/* TODO: Friendly dates. */
			FILETIME ftModified = Bookmark.GetDateModified();
			CreateFileTimeString(&ftModified,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	default:
		assert(FALSE);
		break;
	}
}

void CManageBookmarksDialog::GetBookmarkFolderColumnInfo(const CBookmarkFolder &BookmarkFolder,
	CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,size_t cchBuf)
{
	switch(ColumnType)
	{
	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_NAME:
		StringCchCopy(szColumn,cchBuf,BookmarkFolder.GetName().c_str());
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_LOCATION:
		StringCchCopy(szColumn,cchBuf,EMPTY_STRING);
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_VISIT_DATE:
		StringCchCopy(szColumn,cchBuf,EMPTY_STRING);
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_VISIT_COUNT:
		StringCchCopy(szColumn,cchBuf,EMPTY_STRING);
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_ADDED:
		{
			/* TODO: Friendly dates. */
			FILETIME ftCreated = BookmarkFolder.GetDateCreated();
			CreateFileTimeString(&ftCreated,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_LAST_MODIFIED:
		{
			/* TODO: Friendly dates. */
			FILETIME ftModified = BookmarkFolder.GetDateModified();
			CreateFileTimeString(&ftModified,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	default:
		assert(FALSE);
		break;
	}
}

void CManageBookmarksDialog::OnTbnDropDown(NMTOOLBAR *nmtb)
{
	switch(nmtb->iItem)
	{
	case TOOLBAR_ID_VIEWS:
		ShowViewMenu();
		break;

	case TOOLBAR_ID_ORGANIZE:
		ShowOrganizeMenu();
		break;
	}
}

void CManageBookmarksDialog::ShowViewMenu()
{
	DWORD dwButtonState = static_cast<DWORD>(SendMessage(m_hToolbar,TB_GETSTATE,TOOLBAR_ID_VIEWS,MAKEWORD(TBSTATE_PRESSED,0)));
	SendMessage(m_hToolbar,TB_SETSTATE,TOOLBAR_ID_VIEWS,MAKEWORD(dwButtonState|TBSTATE_PRESSED,0));

	HMENU hMenu = LoadMenu(GetInstance(),MAKEINTRESOURCE(IDR_MANAGEBOOKMARKS_VIEW_MENU));

	UINT uCheck;

	if(m_pmbdps->m_bSortAscending)
	{
		uCheck = IDM_MB_VIEW_SORTASCENDING;
	}
	else
	{
		uCheck = IDM_MB_VIEW_SORTDESCENDING;
	}

	CheckMenuRadioItem(hMenu,IDM_MB_VIEW_SORTASCENDING,IDM_MB_VIEW_SORTDESCENDING,uCheck,MF_BYCOMMAND);

	switch(m_pmbdps->m_SortMode)
	{
	case NBookmarkHelper::SM_NAME:
		uCheck = IDM_MB_VIEW_SORTBYNAME;
		break;

	case NBookmarkHelper::SM_LOCATION:
		uCheck = IDM_MB_VIEW_SORTBYLOCATION;
		break;

	case NBookmarkHelper::SM_VISIT_DATE:
		uCheck = IDM_MB_VIEW_SORTBYVISITDATE;
		break;

	case NBookmarkHelper::SM_VISIT_COUNT:
		uCheck = IDM_MB_VIEW_SORTBYVISITCOUNT;
		break;

	case NBookmarkHelper::SM_ADDED:
		uCheck = IDM_MB_VIEW_SORTBYADDED;
		break;

	case NBookmarkHelper::SM_LAST_MODIFIED:
		uCheck = IDM_MB_VIEW_SORTBYLASTMODIFIED;
		break;
	}

	CheckMenuRadioItem(hMenu,IDM_MB_VIEW_SORTBYNAME,IDM_MB_VIEW_SORTBYLASTMODIFIED,uCheck,MF_BYCOMMAND);

	RECT rcButton;
	SendMessage(m_hToolbar,TB_GETRECT,TOOLBAR_ID_VIEWS,reinterpret_cast<LPARAM>(&rcButton));

	POINT pt;
	pt.x = rcButton.left;
	pt.y = rcButton.bottom;
	ClientToScreen(m_hToolbar,&pt);

	TrackPopupMenu(GetSubMenu(hMenu,0),TPM_LEFTALIGN,pt.x,pt.y,0,m_hDlg,NULL);
	DestroyMenu(hMenu);

	SendMessage(m_hToolbar,TB_SETSTATE,TOOLBAR_ID_VIEWS,MAKEWORD(dwButtonState,0));
}

void CManageBookmarksDialog::ShowOrganizeMenu()
{
	DWORD dwButtonState = static_cast<DWORD>(SendMessage(m_hToolbar,TB_GETSTATE,TOOLBAR_ID_ORGANIZE,MAKEWORD(TBSTATE_PRESSED,0)));
	SendMessage(m_hToolbar,TB_SETSTATE,TOOLBAR_ID_ORGANIZE,MAKEWORD(dwButtonState|TBSTATE_PRESSED,0));

	HMENU hMenu = LoadMenu(GetInstance(),MAKEINTRESOURCE(IDR_MANAGEBOOKMARKS_ORGANIZE_MENU));

	RECT rcButton;
	SendMessage(m_hToolbar,TB_GETRECT,TOOLBAR_ID_ORGANIZE,reinterpret_cast<LPARAM>(&rcButton));

	POINT pt;
	pt.x = rcButton.left;
	pt.y = rcButton.bottom;
	ClientToScreen(m_hToolbar,&pt);

	TrackPopupMenu(GetSubMenu(hMenu,0),TPM_LEFTALIGN,pt.x,pt.y,0,m_hDlg,NULL);
	DestroyMenu(hMenu);

	SendMessage(m_hToolbar,TB_SETSTATE,TOOLBAR_ID_ORGANIZE,MAKEWORD(dwButtonState,0));
}

void CManageBookmarksDialog::OnTvnSelChanged(NMTREEVIEW *pnmtv)
{
	/* This message will come in once before the listview has been
	properly initialized (due to the selection been set in
	the treeview), and can be ignored. */
	if(!m_bListViewInitialized)
	{
		return;
	}

	CBookmarkFolder &BookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(pnmtv->itemNew.hItem);

	if(IsEqualGUID(BookmarkFolder.GetGUID(),m_guidCurrentFolder))
	{
		return;
	}

	BrowseBookmarkFolder(BookmarkFolder);
}

void CManageBookmarksDialog::OnDblClk(NMHDR *pnmhdr)
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

	if(pnmhdr->hwndFrom == hListView)
	{
		NMITEMACTIVATE *pnmia = reinterpret_cast<NMITEMACTIVATE *>(pnmhdr);

		if(pnmia->iItem == -1)
		{
			return;
		}

		HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);
		HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
		CBookmarkFolder &ParentBookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected);
		const VariantBookmark &variantBookmark = m_pBookmarkListView->GetBookmarkItemFromListView(
			ParentBookmarkFolder,pnmia->iItem);

		if(variantBookmark.type() == typeid(CBookmarkFolder))
		{
			const CBookmarkFolder &BookmarkFolder = boost::get<CBookmarkFolder>(variantBookmark);
			BrowseBookmarkFolder(BookmarkFolder);
		}
		else if(variantBookmark.type() == typeid(CBookmark))
		{
			const CBookmark &Bookmark = boost::get<CBookmark>(variantBookmark);
			m_navigation->BrowseFolderInCurrentTab(Bookmark.GetLocation().c_str(), SBSP_ABSOLUTE);
		}
	}
}

void CManageBookmarksDialog::BrowseBookmarkFolder(const CBookmarkFolder &BookmarkFolder)
{
	/* Temporary flag used to indicate whether history should
	be saved. It will be reset each time a folder is browsed. */
	if(m_bSaveHistory)
	{
		m_stackBack.push(m_guidCurrentFolder);
	}

	m_bSaveHistory = true;

	m_guidCurrentFolder = BookmarkFolder.GetGUID();
	m_pBookmarkTreeView->SelectFolder(BookmarkFolder.GetGUID());
	m_pBookmarkListView->InsertBookmarksIntoListView(BookmarkFolder);

	UpdateToolbarState();
}

void CManageBookmarksDialog::BrowseBack()
{
	if(m_stackBack.size() == 0)
	{
		return;
	}

	GUID guid = m_stackBack.top();
	m_stackBack.pop();
	m_stackForward.push(m_guidCurrentFolder);

	m_bSaveHistory = false;
	m_pBookmarkTreeView->SelectFolder(guid);
}

void CManageBookmarksDialog::BrowseForward()
{
	if(m_stackForward.size() == 0)
	{
		return;
	}

	GUID guid = m_stackForward.top();
	m_stackForward.pop();
	m_stackBack.push(m_guidCurrentFolder);

	m_bSaveHistory = false;
	m_pBookmarkTreeView->SelectFolder(guid);
}

void CManageBookmarksDialog::UpdateToolbarState()
{
	SendMessage(m_hToolbar,TB_ENABLEBUTTON,TOOLBAR_ID_BACK,m_stackBack.size() != 0);
	SendMessage(m_hToolbar,TB_ENABLEBUTTON,TOOLBAR_ID_FORWARD,m_stackForward.size() != 0);
}

void CManageBookmarksDialog::OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmark &Bookmark,std::size_t Position)
{
	if(IsEqualGUID(ParentBookmarkFolder.GetGUID(),m_guidCurrentFolder))
	{
		m_pBookmarkListView->InsertBookmarkIntoListView(Bookmark,static_cast<int>(Position));
	}
}

void CManageBookmarksDialog::OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmarkFolder &BookmarkFolder,std::size_t Position)
{
	m_pBookmarkTreeView->BookmarkFolderAdded(ParentBookmarkFolder,BookmarkFolder,Position);

	if(IsEqualGUID(ParentBookmarkFolder.GetGUID(),m_guidCurrentFolder))
	{
		int iItem = m_pBookmarkListView->InsertBookmarkFolderIntoListView(BookmarkFolder,static_cast<int>(Position));

		if(IsEqualGUID(BookmarkFolder.GetGUID(),m_guidNewFolder))
		{
			HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

			SetFocus(hListView);
			NListView::ListView_SelectAllItems(hListView,FALSE);
			NListView::ListView_SelectItem(hListView,iItem,TRUE);
			ListView_EditLabel(hListView,iItem);

			m_bNewFolderAdded = false;
		}
	}
}

void CManageBookmarksDialog::OnBookmarkModified(const GUID &guid)
{
	UNREFERENCED_PARAMETER(guid);

	/* TODO: Notify listview if necessary. */
}

void CManageBookmarksDialog::OnBookmarkFolderModified(const GUID &guid)
{
	m_pBookmarkTreeView->BookmarkFolderModified(guid);
}

void CManageBookmarksDialog::OnBookmarkRemoved(const GUID &guid)
{
	UNREFERENCED_PARAMETER(guid);

	/* TODO: Notify listview if necessary. */
}

void CManageBookmarksDialog::OnBookmarkFolderRemoved(const GUID &guid)
{
	m_pBookmarkTreeView->BookmarkFolderRemoved(guid);

	/* TODO: Remove the deleted folder from the history list. */
}

void CManageBookmarksDialog::OnRClick(NMHDR *pnmhdr)
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);

	if(pnmhdr->hwndFrom == hListView)
	{
		OnListViewRClick();
	}
	else if(pnmhdr->hwndFrom == ListView_GetHeader(hListView))
	{
		OnListViewHeaderRClick();
	}
	else if(pnmhdr->hwndFrom == hTreeView)
	{

	}
}

void CManageBookmarksDialog::OnOk()
{
	DestroyWindow(m_hDlg);
}

void CManageBookmarksDialog::OnCancel()
{
	DestroyWindow(m_hDlg);
}

INT_PTR CManageBookmarksDialog::OnClose()
{
	DestroyWindow(m_hDlg);
	return 0;
}

INT_PTR CManageBookmarksDialog::OnDestroy()
{
	CBookmarkItemNotifier::GetInstance().RemoveObserver(this);

	return 0;
}

INT_PTR CManageBookmarksDialog::OnNcDestroy()
{
	delete this;

	return 0;
}

void CManageBookmarksDialog::SaveState()
{
	m_pmbdps->SaveDialogPosition(m_hDlg);

	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);
	int iColumn = 0;

	for(auto itr = m_pmbdps->m_vectorColumnInfo.begin();itr != m_pmbdps->m_vectorColumnInfo.end();++itr)
	{
		if(itr->bActive)
		{
			itr->iWidth = ListView_GetColumnWidth(hListView,iColumn);
			++iColumn;
		}
	}

	m_pmbdps->m_bStateSaved = TRUE;
}

CManageBookmarksDialogPersistentSettings::CManageBookmarksDialogPersistentSettings() :
m_bInitialized(false),
m_SortMode(NBookmarkHelper::SM_NAME),
m_bSortAscending(true),
CDialogSettings(SETTINGS_KEY)
{
	SetupDefaultColumns();

	/* TODO: Save listview selection information. */
}

CManageBookmarksDialogPersistentSettings::~CManageBookmarksDialogPersistentSettings()
{
	
}

CManageBookmarksDialogPersistentSettings& CManageBookmarksDialogPersistentSettings::GetInstance()
{
	static CManageBookmarksDialogPersistentSettings mbdps;
	return mbdps;
}

void CManageBookmarksDialogPersistentSettings::SetupDefaultColumns()
{
	ColumnInfo_t ci;

	ci.ColumnType	= COLUMN_TYPE_NAME;
	ci.iWidth		= DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	ci.bActive		= TRUE;
	m_vectorColumnInfo.push_back(ci);

	ci.ColumnType	= COLUMN_TYPE_LOCATION;
	ci.iWidth		= DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	ci.bActive		= TRUE;
	m_vectorColumnInfo.push_back(ci);

	ci.ColumnType	= COLUMN_TYPE_VISIT_DATE;
	ci.iWidth		= DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	ci.bActive		= FALSE;
	m_vectorColumnInfo.push_back(ci);

	ci.ColumnType	= COLUMN_TYPE_VISIT_COUNT;
	ci.iWidth		= DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	ci.bActive		= FALSE;
	m_vectorColumnInfo.push_back(ci);

	ci.ColumnType	= COLUMN_TYPE_ADDED;
	ci.iWidth		= DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	ci.bActive		= FALSE;
	m_vectorColumnInfo.push_back(ci);

	ci.ColumnType	= COLUMN_TYPE_LAST_MODIFIED;
	ci.iWidth		= DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	ci.bActive		= FALSE;
	m_vectorColumnInfo.push_back(ci);
}