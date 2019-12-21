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
	int CALLBACK SortBookmarksStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
}

const TCHAR CManageBookmarksDialogPersistentSettings::SETTINGS_KEY[] = _T("ManageBookmarks");

CManageBookmarksDialog::CManageBookmarksDialog(HINSTANCE hInstance, HWND hParent,
	IExplorerplusplus *pexpp, Navigation *navigation, BookmarkTree *bookmarkTree) :
	CBaseDialog(hInstance, IDD_MANAGE_BOOKMARKS, hParent, true),
	m_pexpp(pexpp),
	m_navigation(navigation),
	m_bookmarkTree(bookmarkTree),
	m_guidCurrentFolder(bookmarkTree->GetBookmarksToolbarFolder()->GetGUID()),
	m_bNewFolderAdded(false),
	m_bListViewInitialized(false),
	m_bSaveHistory(true)
{
	m_pmbdps = &CManageBookmarksDialogPersistentSettings::GetInstance();

	if(!m_pmbdps->m_bInitialized)
	{
		m_pmbdps->m_guidSelected = m_bookmarkTree->GetBookmarksToolbarFolder()->GetGUID();

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
	SetupToolbar();
	SetupTreeView();
	SetupListView();

	UpdateToolbarState();

	SetFocus(GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW));

	return 0;
}

wil::unique_hicon CManageBookmarksDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_pexpp->GetIconResourceLoader()->LoadIconFromPNGAndScale(Icon::Bookmarks, iconWidth, iconHeight);
}

void CManageBookmarksDialog::SetupToolbar()
{
	m_hToolbar = CreateToolbar(m_hDlg,
		WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
		TBSTYLE_TOOLTIPS|TBSTYLE_LIST|TBSTYLE_TRANSPARENT|
		TBSTYLE_FLAT|CCS_NODIVIDER|CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS|
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	SendMessage(m_hToolbar,TB_BUTTONSTRUCTSIZE,static_cast<WPARAM>(sizeof(TBBUTTON)),0);

	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hToolbar);
	int iconWidth = m_dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = m_dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	SendMessage(m_hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));

	std::tie(m_imageListToolbar, m_imageListToolbarMappings) = ResourceHelper::CreateIconImageList(
		m_pexpp->GetIconResourceLoader(), iconWidth, iconHeight, { Icon::Back, Icon::Forward,
		Icon::Copy, Icon::Views});
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
	HWND hTreeView = GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW);

	m_pBookmarkTreeView = new CBookmarkTreeView(hTreeView, GetInstance(), m_pexpp, m_bookmarkTree,
		m_pmbdps->m_guidSelected, m_pmbdps->m_setExpansion);
}

void CManageBookmarksDialog::SetupListView()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

	m_pBookmarkListView = new CBookmarkListView(hListView, m_pexpp);

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

	m_pBookmarkListView->NavigateToBookmarkFolder(m_bookmarkTree->GetRoot());

	int iItem = 0;

	/* Update the data for each of the sub-items. */
	/* TODO: This needs to be done by CBookmarkListView. */
	for(const auto &childItem : m_bookmarkTree->GetRoot()->GetChildren())
	{
		int iSubItem = 1;

		for(const auto &ci : m_pmbdps->m_vectorColumnInfo)
		{
			/* The name column will always appear first in
			the set of columns and can be skipped here. */
			if(ci.bActive && ci.ColumnType != CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_NAME)
			{
				TCHAR szColumn[256];
				GetBookmarkItemColumnInfo(childItem.get(),ci.ColumnType,szColumn,SIZEOF_ARRAY(szColumn));
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

void CManageBookmarksDialog::SortListViewItems(BookmarkHelper::SortMode_t SortMode)
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
	auto firstItem = m_pBookmarkListView->GetBookmarkItemFromListViewlParam(lParam1);
	auto secondItem = m_pBookmarkListView->GetBookmarkItemFromListViewlParam(lParam2);

	int iRes = BookmarkHelper::Sort(m_pmbdps->m_SortMode, firstItem, secondItem);

	if (!m_pmbdps->m_bSortAscending)
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
		SortListViewItems(BookmarkHelper::SM_NAME);
		break;

	case IDM_MB_VIEW_SORTBYLOCATION:
		SortListViewItems(BookmarkHelper::SM_LOCATION);
		break;

	case IDM_MB_VIEW_SORTBYADDED:
		SortListViewItems(BookmarkHelper::SM_DATE_ADDED);
		break;

	case IDM_MB_VIEW_SORTBYLASTMODIFIED:
		SortListViewItems(BookmarkHelper::SM_DATE_MODIFIED);
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
	std::wstring newBookmarkFolderName = ResourceHelper::LoadString(GetInstance(), IDS_BOOKMARKS_NEWBOOKMARKFOLDER);
	auto newBookmarkFolder = std::make_unique<BookmarkItem>(std::nullopt, newBookmarkFolderName, std::nullopt);

	/* Save the folder GUID, so that it can be selected and
	placed into edit mode once the bookmark notification
	comes through. */
	m_bNewFolderAdded = true;
	m_guidNewFolder = newBookmarkFolder->GetGUID();

	HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);
	HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);

	assert(hSelectedItem != NULL);

	auto bookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(
		hSelectedItem);
	bookmarkFolder->AddChild(std::move(newBookmarkFolder));
}

void CManageBookmarksDialog::OnDeleteBookmark(const std::wstring &guid)
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
				const auto bookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected);

				int iBookmarkItem = 0;

				for(auto &childItem : bookmarkFolder->GetChildren())
				{
					TCHAR szColumn[256];
					GetBookmarkItemColumnInfo(childItem.get(),itr->ColumnType,szColumn,SIZEOF_ARRAY(szColumn));
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
		auto bookmarkItem = m_pBookmarkListView->GetBookmarkItemFromListView(pnmlvdi->item.iItem);
		bookmarkItem->SetName(pnmlvdi->item.pszText);

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

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_DATE_CREATED:
		uResourceID = IDS_MANAGE_BOOKMARKS_COLUMN_ADDED;
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_DATE_MODIFIED:
		uResourceID = IDS_MANAGE_BOOKMARKS_COLUMN_LAST_MODIFIED;
		break;

	default:
		assert(FALSE);
		break;
	}

	LoadString(GetInstance(),uResourceID,szColumn,cchBuf);
}

void CManageBookmarksDialog::GetBookmarkItemColumnInfo(const BookmarkItem *bookmarkItem,
	CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType, TCHAR *szColumn, size_t cchBuf)
{
	if(bookmarkItem->IsFolder())
	{
		GetBookmarkFolderColumnInfo(bookmarkItem,ColumnType,szColumn,cchBuf);
	}
	else
	{
		GetBookmarkColumnInfo(bookmarkItem,ColumnType,szColumn,cchBuf);
	}
}

void CManageBookmarksDialog::GetBookmarkColumnInfo(const BookmarkItem *bookmarkItem,
	CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType, TCHAR *szColumn, size_t cchBuf)
{
	switch(ColumnType)
	{
	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_NAME:
		StringCchCopy(szColumn,cchBuf, bookmarkItem->GetName().c_str());
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_LOCATION:
		StringCchCopy(szColumn,cchBuf, bookmarkItem->GetLocation().c_str());
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_DATE_CREATED:
		{
			/* TODO: Friendly dates. */
			FILETIME dateCreated = bookmarkItem->GetDateCreated();
			CreateFileTimeString(&dateCreated, szColumn, static_cast<int>(cchBuf), FALSE);
		}
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_DATE_MODIFIED:
		{
			/* TODO: Friendly dates. */
			FILETIME dateModified = bookmarkItem->GetDateModified();
			CreateFileTimeString(&dateModified, szColumn, static_cast<int>(cchBuf), FALSE);
		}
		break;

	default:
		assert(FALSE);
		break;
	}
}

void CManageBookmarksDialog::GetBookmarkFolderColumnInfo(const BookmarkItem *bookmarkItem,
	CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType, TCHAR *szColumn, size_t cchBuf)
{
	switch(ColumnType)
	{
	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_NAME:
		StringCchCopy(szColumn,cchBuf, bookmarkItem->GetName().c_str());
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_LOCATION:
		StringCchCopy(szColumn,cchBuf,EMPTY_STRING);
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_DATE_CREATED:
		{
			/* TODO: Friendly dates. */
			FILETIME dateCreated = bookmarkItem->GetDateCreated();
			CreateFileTimeString(&dateCreated, szColumn, static_cast<int>(cchBuf), FALSE);
		}
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_DATE_MODIFIED:
		{
			/* TODO: Friendly dates. */
			FILETIME dateModified = bookmarkItem->GetDateModified();
			CreateFileTimeString(&dateModified, szColumn, static_cast<int>(cchBuf), FALSE);
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
	case BookmarkHelper::SM_NAME:
		uCheck = IDM_MB_VIEW_SORTBYNAME;
		break;

	case BookmarkHelper::SM_LOCATION:
		uCheck = IDM_MB_VIEW_SORTBYLOCATION;
		break;

	case BookmarkHelper::SM_DATE_ADDED:
		uCheck = IDM_MB_VIEW_SORTBYADDED;
		break;

	case BookmarkHelper::SM_DATE_MODIFIED:
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

	auto bookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(pnmtv->itemNew.hItem);

	if(bookmarkFolder->GetGUID() == m_guidCurrentFolder)
	{
		return;
	}

	BrowseBookmarkFolder(bookmarkFolder);
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

		auto bookmarkItem = m_pBookmarkListView->GetBookmarkItemFromListView(pnmia->iItem);

		if(bookmarkItem->IsFolder())
		{
			BrowseBookmarkFolder(bookmarkItem);
		}
		else
		{
			m_navigation->BrowseFolderInCurrentTab(bookmarkItem->GetLocation().c_str());
		}
	}
}

void CManageBookmarksDialog::BrowseBookmarkFolder(BookmarkItem *bookmarkItem)
{
	/* Temporary flag used to indicate whether history should
	be saved. It will be reset each time a folder is browsed. */
	if(m_bSaveHistory)
	{
		m_stackBack.push(m_guidCurrentFolder);
	}

	m_bSaveHistory = true;

	m_guidCurrentFolder = bookmarkItem->GetGUID();
	m_pBookmarkTreeView->SelectFolder(bookmarkItem->GetGUID());
	m_pBookmarkListView->NavigateToBookmarkFolder(bookmarkItem);

	UpdateToolbarState();
}

void CManageBookmarksDialog::BrowseBack()
{
	if(m_stackBack.size() == 0)
	{
		return;
	}

	std::wstring guid = m_stackBack.top();
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

	std::wstring guid = m_stackForward.top();
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

// TODO: Update.
//void CManageBookmarksDialog::OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,
//	const CBookmarkFolder &BookmarkFolder,std::size_t Position)
//{
//	if(ParentBookmarkFolder.GetGUID() == m_guidCurrentFolder)
//	{
//		int iItem = m_pBookmarkListView->InsertBookmarkFolderIntoListView(BookmarkFolder,static_cast<int>(Position));
//
//		if(BookmarkFolder.GetGUID() == m_guidNewFolder)
//		{
//			HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);
//
//			SetFocus(hListView);
//			NListView::ListView_SelectAllItems(hListView,FALSE);
//			NListView::ListView_SelectItem(hListView,iItem,TRUE);
//			ListView_EditLabel(hListView,iItem);
//
//			m_bNewFolderAdded = false;
//		}
//	}
//}

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
	m_SortMode(BookmarkHelper::SM_NAME),
	m_bSortAscending(true),
	CDialogSettings(SETTINGS_KEY)
{
	SetupDefaultColumns();

	/* TODO: Save listview selection information. */
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

	ci.ColumnType	= COLUMN_TYPE_DATE_CREATED;
	ci.iWidth		= DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	ci.bActive		= FALSE;
	m_vectorColumnInfo.push_back(ci);

	ci.ColumnType	= COLUMN_TYPE_DATE_MODIFIED;
	ci.iWidth		= DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	ci.bActive		= FALSE;
	m_vectorColumnInfo.push_back(ci);
}