/******************************************************************
 *
 * Project: Explorer++
 * File: ManageBookmarksDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Manage Bookmarks' dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <stack>
#include "Explorer++_internal.h"
#include "ManageBookmarksDialog.h"
#include "BookmarkHelper.h"
#include "MainResource.h"
#include "../Helper/ListViewHelper.h"


namespace NManageBookmarksDialog
{
	LRESULT CALLBACK EditSearchProcStub(HWND hwnd,UINT uMsg,
		WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
}

const TCHAR CManageBookmarksDialogPersistentSettings::SETTINGS_KEY[] = _T("ManageBookmarks");

CManageBookmarksDialog::CManageBookmarksDialog(HINSTANCE hInstance,int iResource,HWND hParent,
	CBookmarkFolder *pAllBookmarks) :
m_pAllBookmarks(pAllBookmarks),
m_bSearchFieldBlank(true),
m_bEditingSearchField(false),
m_hEditSearchFont(NULL),
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pmbdps = &CManageBookmarksDialogPersistentSettings::GetInstance();

	if(!m_pmbdps->m_bInitialized)
	{
		m_pmbdps->m_guidSelected = pAllBookmarks->GetGUID();
		m_pmbdps->m_setExpansion.insert(pAllBookmarks->GetGUID());

		m_pmbdps->m_bInitialized = true;
	}
}

CManageBookmarksDialog::~CManageBookmarksDialog()
{

}

BOOL CManageBookmarksDialog::OnInitDialog()
{
	SetupSearchField();
	SetupToolbar();
	SetupTreeView();
	SetupListView();

	SetFocus(GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW));

	return 0;
}

void CManageBookmarksDialog::SetupSearchField()
{
	HWND hEdit = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_EDITSEARCH);
	SetWindowSubclass(hEdit,NManageBookmarksDialog::EditSearchProcStub,
		0,reinterpret_cast<DWORD_PTR>(this));
	SetSearchFieldDefaultState();
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

	m_himlToolbar = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetInstance(),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himlToolbar,hBitmap,NULL);
	DeleteObject(hBitmap);
	SendMessage(m_hToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(m_himlToolbar));

	/* TODO: String into string table. */
	TBBUTTON tbb;
	tbb.iBitmap		= SHELLIMAGES_VIEWS;
	tbb.idCommand	= 1;
	tbb.fsState		= TBSTATE_ENABLED;
	tbb.fsStyle		= BTNS_BUTTON|BTNS_AUTOSIZE|BTNS_SHOWTEXT|BTNS_DROPDOWN;
	tbb.dwData		= 0;
	tbb.iString		= reinterpret_cast<INT_PTR>(_T("Views"));
	SendMessage(m_hToolbar,TB_INSERTBUTTON,0,reinterpret_cast<LPARAM>(&tbb));

	RECT rcTreeView;
	GetWindowRect(GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW),&rcTreeView);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,reinterpret_cast<LPPOINT>(&rcTreeView),2);

	RECT rcSearch;
	GetWindowRect(GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_EDITSEARCH),&rcSearch);
	MapWindowPoints(HWND_DESKTOP,m_hDlg,reinterpret_cast<LPPOINT>(&rcSearch),2);

	DWORD dwButtonSize = static_cast<DWORD>(SendMessage(m_hToolbar,TB_GETBUTTONSIZE,0,0));
	SetWindowPos(m_hToolbar,NULL,rcTreeView.left,rcSearch.top - (HIWORD(dwButtonSize) - GetRectHeight(&rcSearch)) / 2,
		rcSearch.left - rcTreeView.left - 10,HIWORD(dwButtonSize),0);
}

void CManageBookmarksDialog::SetupTreeView()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);

	m_pBookmarkTreeView = new CBookmarkTreeView(hTreeView);
	m_pBookmarkTreeView->InsertFoldersIntoTreeView(m_pAllBookmarks,
		m_pmbdps->m_guidSelected,m_pmbdps->m_setExpansion);
}

void CManageBookmarksDialog::SetupListView()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

	m_pBookmarkListView = new CBookmarkListView(hListView);

	int iColumn = 0;

	for each(auto ci in m_pmbdps->m_vectorColumnInfo)
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

	m_pBookmarkListView->InsertBookmarksIntoListView(m_pAllBookmarks);

	int iItem = 0;

	/* Update the data for each of the sub-items. */
	for(auto itr = m_pAllBookmarks->begin();itr != m_pAllBookmarks->end();++itr)
	{
		int iSubItem = 1;

		for each(auto ci in m_pmbdps->m_vectorColumnInfo)
		{
			/* The name column will always appear first in
			the set of columns and can be skipped here. */
			if(ci.bActive && ci.ColumnType != CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_NAME)
			{
				TCHAR szColumn[256];
				GetBookmarkItemColumnInfo(&(*itr),ci.ColumnType,szColumn,SIZEOF_ARRAY(szColumn));
				ListView_SetItemText(hListView,iItem,iSubItem,szColumn);

				++iSubItem;
			}
		}

		++iItem;
	}

	NListView::ListView_SelectItem(hListView,0,TRUE);
}

/* Changes the font within the search edit
control, and sets the default text. */
void CManageBookmarksDialog::SetSearchFieldDefaultState()
{
	HWND hEdit = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_EDITSEARCH);

	LOGFONT lf;
	HFONT hCurentFont = reinterpret_cast<HFONT>(SendMessage(hEdit,WM_GETFONT,0,0));
	GetObject(hCurentFont,sizeof(lf),reinterpret_cast<LPVOID>(&lf));

	HFONT hPrevEditSearchFont = m_hEditSearchFont;

	lf.lfItalic = TRUE;
	m_hEditSearchFont = CreateFontIndirect(&lf);
	SendMessage(hEdit,WM_SETFONT,reinterpret_cast<WPARAM>(m_hEditSearchFont),MAKEWORD(TRUE,0));

	if(hPrevEditSearchFont != NULL)
	{
		DeleteFont(hPrevEditSearchFont);
	}

	TCHAR szTemp[64];
	LoadString(GetInstance(),IDS_MANAGE_BOOKMARKS_DEFAULT_SEARCH_TEXT,szTemp,SIZEOF_ARRAY(szTemp));
	SetWindowText(hEdit,szTemp);
}

/* Resets the font within the search
field and removes any text. */
void CManageBookmarksDialog::RemoveSearchFieldDefaultState()
{
	HWND hEdit = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_EDITSEARCH);

	LOGFONT lf;
	HFONT hCurentFont = reinterpret_cast<HFONT>(SendMessage(hEdit,WM_GETFONT,0,0));
	GetObject(hCurentFont,sizeof(lf),reinterpret_cast<LPVOID>(&lf));

	HFONT hPrevEditSearchFont = m_hEditSearchFont;

	lf.lfItalic = FALSE;
	m_hEditSearchFont = CreateFontIndirect(&lf);
	SendMessage(hEdit,WM_SETFONT,reinterpret_cast<WPARAM>(m_hEditSearchFont),MAKEWORD(TRUE,0));

	DeleteFont(hPrevEditSearchFont);

	SetWindowText(hEdit,EMPTY_STRING);
}

LRESULT CALLBACK NManageBookmarksDialog::EditSearchProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CManageBookmarksDialog *pmbd = reinterpret_cast<CManageBookmarksDialog *>(dwRefData);

	return pmbd->EditSearchProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CManageBookmarksDialog::EditSearchProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_SETFOCUS:
		if(m_bSearchFieldBlank)
		{
			RemoveSearchFieldDefaultState();
		}

		m_bEditingSearchField = true;
		break;

	case WM_KILLFOCUS:
		if(GetWindowTextLength(hwnd) == 0)
		{
			m_bSearchFieldBlank = true;
			SetSearchFieldDefaultState();
		}
		else
		{
			m_bSearchFieldBlank = false;
		}

		m_bEditingSearchField = false;
		break;
	}

	return DefSubclassProc(hwnd,Msg,wParam,lParam);
}

INT_PTR CManageBookmarksDialog::OnCtlColorEdit(HWND hwnd,HDC hdc)
{
	if(hwnd == GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_EDITSEARCH))
	{
		if(m_bSearchFieldBlank &&
			!m_bEditingSearchField)
		{
			SetTextColor(hdc,SEARCH_TEXT_COLOR);
			SetBkMode(hdc,TRANSPARENT);
			return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
		}
		else
		{
			SetTextColor(hdc,GetSysColor(COLOR_WINDOWTEXT));
			SetBkMode(hdc,TRANSPARENT);
			return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
		}
	}

	return 0;
}

BOOL CManageBookmarksDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case EN_CHANGE:
		OnEnChange(reinterpret_cast<HWND>(lParam));
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

BOOL CManageBookmarksDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case NM_DBLCLK:
		OnDblClk(pnmhdr);
		break;

	case NM_RCLICK:
		OnRClick(pnmhdr);
		break;

	case TVN_SELCHANGED:
		OnTvnSelChanged(reinterpret_cast<NMTREEVIEW *>(pnmhdr));
		break;
	}

	return 0;
}

void CManageBookmarksDialog::OnEnChange(HWND hEdit)
{
	if(hEdit != GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_EDITSEARCH))
	{
		return;
	}

	std::wstring strSearch;
	GetWindowString(hEdit,strSearch);

	if(strSearch.size() > 0)
	{

	}
}

void CManageBookmarksDialog::OnListViewRClick()
{
	DWORD dwCursorPos = GetMessagePos();

	POINT ptCursor;
	ptCursor.x = GET_X_LPARAM(dwCursorPos);
	ptCursor.y = GET_Y_LPARAM(dwCursorPos);

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
	int iItem = 0;

	for each(auto ci in m_pmbdps->m_vectorColumnInfo)
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

		InsertMenuItem(hMenu,iItem,TRUE,&mii);

		++iItem;
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
				CBookmarkFolder *pBookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected,m_pAllBookmarks);

				int iItem = 0;

				for(auto itrBookmarks = pBookmarkFolder->begin();itrBookmarks != pBookmarkFolder->end();++itrBookmarks)
				{
					TCHAR szColumn[256];
					GetBookmarkItemColumnInfo(&(*itrBookmarks),itr->ColumnType,szColumn,SIZEOF_ARRAY(szColumn));
					ListView_SetItemText(hListView,iItem,iColumn,szColumn);

					++iItem;
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

void CManageBookmarksDialog::GetBookmarkItemColumnInfo(boost::variant<CBookmarkFolder,CBookmark> *pBookmarkVariant,
	CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType,TCHAR *szColumn,size_t cchBuf)
{
	if(CBookmarkFolder *pBookmarkFolder = boost::get<CBookmarkFolder>(pBookmarkVariant))
	{
		GetBookmarkFolderColumnInfo(pBookmarkFolder,ColumnType,szColumn,cchBuf);
	}
	else if(CBookmark *pBookmark = boost::get<CBookmark>(pBookmarkVariant))
	{
		GetBookmarkColumnInfo(pBookmark,ColumnType,szColumn,cchBuf);
	}
}

void CManageBookmarksDialog::GetBookmarkColumnInfo(CBookmark *pBookmark,
	CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType,
	TCHAR *szColumn,size_t cchBuf)
{
	switch(ColumnType)
	{
	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_NAME:
		StringCchCopy(szColumn,cchBuf,pBookmark->GetName().c_str());
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_LOCATION:
		StringCchCopy(szColumn,cchBuf,pBookmark->GetLocation().c_str());
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_VISIT_DATE:
		{
			/* TODO: Friendly dates. */
			FILETIME ftLastVisited = pBookmark->GetDateLastVisited();
			CreateFileTimeString(&ftLastVisited,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_VISIT_COUNT:
		StringCchPrintf(szColumn,cchBuf,_T("%d"),pBookmark->GetVisitCount());
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_ADDED:
		{
			/* TODO: Friendly dates. */
			FILETIME ftCreated = pBookmark->GetDateCreated();
			CreateFileTimeString(&ftCreated,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_LAST_MODIFIED:
		{
			/* TODO: Friendly dates. */
			FILETIME ftModified = pBookmark->GetDateModified();
			CreateFileTimeString(&ftModified,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	default:
		assert(FALSE);
		break;
	}
}

void CManageBookmarksDialog::GetBookmarkFolderColumnInfo(CBookmarkFolder *pBookmarkFolder,
	CManageBookmarksDialogPersistentSettings::ColumnType_t ColumnType,
	TCHAR *szColumn,size_t cchBuf)
{
	switch(ColumnType)
	{
	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_NAME:
		StringCchCopy(szColumn,cchBuf,pBookmarkFolder->GetName().c_str());
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
			FILETIME ftCreated = pBookmarkFolder->GetDateCreated();
			CreateFileTimeString(&ftCreated,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	case CManageBookmarksDialogPersistentSettings::COLUMN_TYPE_LAST_MODIFIED:
		{
			/* TODO: Friendly dates. */
			FILETIME ftModified = pBookmarkFolder->GetDateModified();
			CreateFileTimeString(&ftModified,szColumn,static_cast<int>(cchBuf),FALSE);
		}
		break;

	default:
		assert(FALSE);
		break;
	}
}

void CManageBookmarksDialog::OnTvnSelChanged(NMTREEVIEW *pnmtv)
{
	CBookmarkFolder *pBookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(pnmtv->itemNew.hItem,m_pAllBookmarks);
	assert(pBookmarkFolder != NULL);

	m_pBookmarkListView->InsertBookmarksIntoListView(pBookmarkFolder);
}

void CManageBookmarksDialog::OnDblClk(NMHDR *pnmhdr)
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

	if(pnmhdr->hwndFrom == hListView)
	{
		NMITEMACTIVATE *pnmia = reinterpret_cast<NMITEMACTIVATE *>(pnmhdr);

		std::pair<void *,NBookmarks::BookmarkType_t> BookmarkItem =
			m_pBookmarkListView->GetBookmarkItemFromListView(pnmia->iItem);

		switch(BookmarkItem.second)
		{
		case NBookmarks::TYPE_BOOKMARK:
			/* TODO: Send the bookmark back to the main
			window to open. */
			break;

		case NBookmarks::TYPE_FOLDER:
			/* TODO: Browse into the folder. */
			break;
		}
	}
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
	EndDialog(m_hDlg,1);
}

void CManageBookmarksDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

BOOL CManageBookmarksDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

BOOL CManageBookmarksDialog::OnDestroy()
{
	DeleteFont(m_hEditSearchFont);
	ImageList_Destroy(m_himlToolbar);

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
CDialogSettings(SETTINGS_KEY)
{
	m_bInitialized = false;

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