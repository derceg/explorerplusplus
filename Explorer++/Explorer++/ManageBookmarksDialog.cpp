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

const TCHAR CManageBookmarksDialogPersistentSettings::SETTINGS_KEY[] = _T("ManageBookmarks");

ManageBookmarksDialog::ManageBookmarksDialog(HINSTANCE hInstance, HWND hParent,
	IExplorerplusplus *pexpp, Navigation *navigation, BookmarkTree *bookmarkTree) :
	CBaseDialog(hInstance, IDD_MANAGE_BOOKMARKS, hParent, true),
	m_pexpp(pexpp),
	m_navigation(navigation),
	m_bookmarkTree(bookmarkTree),
	m_bNewFolderAdded(false),
	m_bSaveHistory(true)
{
	m_pmbdps = &CManageBookmarksDialogPersistentSettings::GetInstance();

	if(!m_pmbdps->m_bInitialized)
	{
		m_pmbdps->m_bInitialized = true;
	}
}

ManageBookmarksDialog::~ManageBookmarksDialog()
{
	delete m_bookmarkTreeView;
	delete m_bookmarkListView;
}

INT_PTR ManageBookmarksDialog::OnInitDialog()
{
	/* TODO: Enable drag and drop for listview and treeview. */
	SetupToolbar();
	SetupTreeView();
	SetupListView();

	m_bookmarkListView->NavigateToBookmarkFolder(m_bookmarkTree->GetBookmarksToolbarFolder());

	SetFocus(GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW));

	return 0;
}

wil::unique_hicon ManageBookmarksDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_pexpp->GetIconResourceLoader()->LoadIconFromPNGAndScale(Icon::Bookmarks, iconWidth, iconHeight);
}

void ManageBookmarksDialog::SetupToolbar()
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

void ManageBookmarksDialog::SetupTreeView()
{
	HWND hTreeView = GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW);

	m_bookmarkTreeView = new BookmarkTreeView(hTreeView, GetInstance(), m_pexpp,
		m_bookmarkTree, m_pmbdps->m_setExpansion);

	m_connections.push_back(m_bookmarkTreeView->selectionChangedSignal.AddObserver(
		std::bind(&ManageBookmarksDialog::OnTreeViewSelectionChanged, this, std::placeholders::_1)));
}

void ManageBookmarksDialog::SetupListView()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

	m_bookmarkListView = new BookmarkListView(hListView, GetInstance(), m_bookmarkTree,
		m_pexpp, m_pmbdps->m_listViewColumns);

	m_connections.push_back(m_bookmarkListView->navigationSignal.AddObserver(
		std::bind(&ManageBookmarksDialog::OnListViewNavigation, this, std::placeholders::_1)));
}

INT_PTR ManageBookmarksDialog::OnAppCommand(HWND hwnd,UINT uCmd,UINT uDevice,DWORD dwKeys)
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

INT_PTR ManageBookmarksDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)
	{
		return HandleMenuOrAccelerator(wParam);
	}

	return 1;
}

LRESULT ManageBookmarksDialog::HandleMenuOrAccelerator(WPARAM wParam)
{
	switch (LOWORD(wParam))
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

	case IDM_MB_VIEW_SORT_BY_DEFAULT:
		m_bookmarkListView->SetSortMode(BookmarkHelper::SortMode::Name);
		break;

	case IDM_MB_VIEW_SORTBYNAME:
		m_bookmarkListView->SetSortMode(BookmarkHelper::SortMode::Name);
		break;

	case IDM_MB_VIEW_SORTBYLOCATION:
		m_bookmarkListView->SetSortMode(BookmarkHelper::SortMode::Location);
		break;

	case IDM_MB_VIEW_SORTBYADDED:
		m_bookmarkListView->SetSortMode(BookmarkHelper::SortMode::DateCreated);
		break;

	case IDM_MB_VIEW_SORTBYLASTMODIFIED:
		m_bookmarkListView->SetSortMode(BookmarkHelper::SortMode::DateModified);
		break;

	case IDM_MB_VIEW_SORTASCENDING:
		m_bookmarkListView->SetSortAscending(true);
		break;

	case IDM_MB_VIEW_SORTDESCENDING:
		m_bookmarkListView->SetSortAscending(false);
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

INT_PTR ManageBookmarksDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case TBN_DROPDOWN:
		OnTbnDropDown(reinterpret_cast<NMTOOLBAR *>(pnmhdr));
		break;
	}

	return 0;
}

void ManageBookmarksDialog::OnNewFolder()
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

	auto bookmarkFolder = m_bookmarkTreeView->GetBookmarkFolderFromTreeView(
		hSelectedItem);
	m_bookmarkTree->AddBookmarkItem(bookmarkFolder, std::move(newBookmarkFolder), bookmarkFolder->GetChildren().size());
}

void ManageBookmarksDialog::OnDeleteBookmark(const std::wstring &guid)
{
	UNREFERENCED_PARAMETER(guid);

	/* TODO: Move the bookmark/bookmark folder to the trash folder. */
}

void ManageBookmarksDialog::OnTbnDropDown(NMTOOLBAR *nmtb)
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

void ManageBookmarksDialog::ShowViewMenu()
{
	DWORD dwButtonState = static_cast<DWORD>(SendMessage(m_hToolbar,TB_GETSTATE,TOOLBAR_ID_VIEWS,MAKEWORD(TBSTATE_PRESSED,0)));
	SendMessage(m_hToolbar,TB_SETSTATE,TOOLBAR_ID_VIEWS,MAKEWORD(dwButtonState|TBSTATE_PRESSED,0));

	HMENU hMenu = LoadMenu(GetInstance(),MAKEINTRESOURCE(IDR_MANAGEBOOKMARKS_VIEW_MENU));

	UINT uCheck;

	switch(m_bookmarkListView->GetSortMode())
	{
	case BookmarkHelper::SortMode::Default:
		uCheck = IDM_MB_VIEW_SORT_BY_DEFAULT;
		break;

	case BookmarkHelper::SortMode::Name:
		uCheck = IDM_MB_VIEW_SORTBYNAME;
		break;

	case BookmarkHelper::SortMode::Location:
		uCheck = IDM_MB_VIEW_SORTBYLOCATION;
		break;

	case BookmarkHelper::SortMode::DateCreated:
		uCheck = IDM_MB_VIEW_SORTBYADDED;
		break;

	case BookmarkHelper::SortMode::DateModified:
		uCheck = IDM_MB_VIEW_SORTBYLASTMODIFIED;
		break;

	default:
		uCheck = IDM_MB_VIEW_SORT_BY_DEFAULT;
		break;
	}

	CheckMenuRadioItem(hMenu, IDM_MB_VIEW_SORTBYNAME, IDM_MB_VIEW_SORT_BY_DEFAULT, uCheck, MF_BYCOMMAND);

	if (m_bookmarkListView->GetSortAscending())
	{
		uCheck = IDM_MB_VIEW_SORTASCENDING;
	}
	else
	{
		uCheck = IDM_MB_VIEW_SORTDESCENDING;
	}

	CheckMenuRadioItem(hMenu, IDM_MB_VIEW_SORTASCENDING, IDM_MB_VIEW_SORTDESCENDING, uCheck, MF_BYCOMMAND);

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

void ManageBookmarksDialog::ShowOrganizeMenu()
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

void ManageBookmarksDialog::OnTreeViewSelectionChanged(BookmarkItem *bookmarkFolder)
{
	if (bookmarkFolder->GetGUID() == m_guidCurrentFolder)
	{
		return;
	}

	m_bookmarkListView->NavigateToBookmarkFolder(bookmarkFolder);
}

void ManageBookmarksDialog::OnListViewNavigation(BookmarkItem *bookmarkFolder)
{
	/* Temporary flag used to indicate whether history should
	be saved. It will be reset each time a folder is browsed. */
	if (m_bSaveHistory)
	{
		m_stackBack.push(m_guidCurrentFolder);
	}

	m_bSaveHistory = true;

	m_guidCurrentFolder = bookmarkFolder->GetGUID();
	m_bookmarkTreeView->SelectFolder(bookmarkFolder->GetGUID());

	UpdateToolbarState();
}

void ManageBookmarksDialog::BrowseBack()
{
	if(m_stackBack.size() == 0)
	{
		return;
	}

	std::wstring guid = m_stackBack.top();
	m_stackBack.pop();
	m_stackForward.push(m_guidCurrentFolder);

	m_bSaveHistory = false;
	m_bookmarkTreeView->SelectFolder(guid);
}

void ManageBookmarksDialog::BrowseForward()
{
	if(m_stackForward.size() == 0)
	{
		return;
	}

	std::wstring guid = m_stackForward.top();
	m_stackForward.pop();
	m_stackBack.push(m_guidCurrentFolder);

	m_bSaveHistory = false;
	m_bookmarkTreeView->SelectFolder(guid);
}

void ManageBookmarksDialog::UpdateToolbarState()
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

void ManageBookmarksDialog::OnOk()
{
	DestroyWindow(m_hDlg);
}

void ManageBookmarksDialog::OnCancel()
{
	DestroyWindow(m_hDlg);
}

INT_PTR ManageBookmarksDialog::OnClose()
{
	DestroyWindow(m_hDlg);
	return 0;
}

INT_PTR	ManageBookmarksDialog::OnDestroy()
{
	m_pmbdps->m_listViewColumns = m_bookmarkListView->GetColumns();
	return 0;
}

INT_PTR ManageBookmarksDialog::OnNcDestroy()
{
	delete this;

	return 0;
}

void ManageBookmarksDialog::SaveState()
{
	m_pmbdps->SaveDialogPosition(m_hDlg);

	m_pmbdps->m_bStateSaved = TRUE;
}

CManageBookmarksDialogPersistentSettings::CManageBookmarksDialogPersistentSettings() :
	m_bInitialized(false),
	CDialogSettings(SETTINGS_KEY)
{
	SetupDefaultColumns();
}

CManageBookmarksDialogPersistentSettings& CManageBookmarksDialogPersistentSettings::GetInstance()
{
	static CManageBookmarksDialogPersistentSettings mbdps;
	return mbdps;
}

void CManageBookmarksDialogPersistentSettings::SetupDefaultColumns()
{
	BookmarkListView::Column column;

	column.columnType = BookmarkListView::ColumnType::Name;
	column.width = DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	column.active = true;
	m_listViewColumns.push_back(column);

	column.columnType = BookmarkListView::ColumnType::Location;
	column.width = DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	column.active = true;
	m_listViewColumns.push_back(column);

	column.columnType = BookmarkListView::ColumnType::DateCreated;
	column.width = DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	column.active = false;
	m_listViewColumns.push_back(column);

	column.columnType = BookmarkListView::ColumnType::DateModified;
	column.width = DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	column.active = false;
	m_listViewColumns.push_back(column);
}