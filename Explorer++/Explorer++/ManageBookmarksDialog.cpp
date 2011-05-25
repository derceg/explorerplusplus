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


namespace NManageBookmarksDialog
{
	LRESULT CALLBACK EditSearchProcStub(HWND hwnd,UINT uMsg,
		WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
}

const TCHAR CManageBookmarksDialogPersistentSettings::SETTINGS_KEY[] = _T("ManageBookmarks");

CManageBookmarksDialog::CManageBookmarksDialog(HINSTANCE hInstance,int iResource,HWND hParent,
	BookmarkFolder *pAllBookmarks) :
m_pAllBookmarks(pAllBookmarks),
m_bSearchFieldBlank(true),
m_bEditingSearchField(false),
m_hEditSearchFont(NULL),
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pmbdps = &CManageBookmarksDialogPersistentSettings::GetInstance();
}

CManageBookmarksDialog::~CManageBookmarksDialog()
{

}

BOOL CManageBookmarksDialog::OnInitDialog()
{
	HWND hEdit = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_EDITSEARCH);
	SetWindowSubclass(hEdit,NManageBookmarksDialog::EditSearchProcStub,
		0,reinterpret_cast<DWORD_PTR>(this));
	SetSearchFieldDefaultState();

	SetupTreeView();
	SetupListView();

	SetFocus(GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW));

	return 0;
}

void CManageBookmarksDialog::SetupTreeView()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);
	SetWindowTheme(hTreeView,L"Explorer",NULL);

	m_himlTreeView = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetInstance(),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himlTreeView,hBitmap,NULL);
	TreeView_SetImageList(hTreeView,m_himlTreeView,TVSIL_NORMAL);
	DeleteObject(hBitmap);

	NBookmarkHelper::InsertFoldersIntoTreeView(hTreeView,m_pAllBookmarks);
}

void CManageBookmarksDialog::SetupListView()
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

	SetWindowTheme(hListView,L"Explorer",NULL);
	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT,
		LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT);

	LVCOLUMN lvCol;
	TCHAR szTemp[64];

	LoadString(GetInstance(),IDS_MANAGE_BOOKMARKS_NAME,
		szTemp,SIZEOF_ARRAY(szTemp));
	lvCol.mask		= LVCF_TEXT;
	lvCol.pszText	= szTemp;
	ListView_InsertColumn(hListView,1,&lvCol);

	LoadString(GetInstance(),IDS_MANAGE_BOOKMARKS_LOCATION,
		szTemp,SIZEOF_ARRAY(szTemp));
	lvCol.mask		= LVCF_TEXT;
	lvCol.pszText	= szTemp;
	ListView_InsertColumn(hListView,2,&lvCol);

	SendMessage(hListView,LVM_SETCOLUMNWIDTH,0,m_pmbdps->m_iColumnWidth1);
	SendMessage(hListView,LVM_SETCOLUMNWIDTH,1,m_pmbdps->m_iColumnWidth2);

	NBookmarkHelper::InsertBookmarksIntoListView(hListView,m_pAllBookmarks);

	ListView_SelectItem(hListView,0,TRUE);
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

void CManageBookmarksDialog::GetBookmarkItemFromListView(int iItem)
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);
	HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);
	BookmarkFolder *pBookmarkFolder = NBookmarkHelper::GetBookmarkFolderFromTreeView(hTreeView,
		hSelectedItem,m_pAllBookmarks);

	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

	LVITEM lvi;
	lvi.mask		= LVIF_PARAM;
	lvi.iItem		= iItem;
	lvi.iSubItem	= 0;
	ListView_GetItem(hListView,&lvi);

	std::pair<void *,NBookmarks::BookmarkType_t> BookmarkItem = pBookmarkFolder->GetBookmarkItem(
		static_cast<UINT>(lvi.lParam));

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
	case TVN_SELCHANGED:
		OnTvnSelChanged(reinterpret_cast<NMTREEVIEW *>(pnmhdr));
		break;

	case NM_DBLCLK:
		OnDblClk(pnmhdr);
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

void CManageBookmarksDialog::OnTvnSelChanged(NMTREEVIEW *pnmtv)
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);
	BookmarkFolder *pBookmarkFolder = NBookmarkHelper::GetBookmarkFolderFromTreeView(hTreeView,
		pnmtv->itemNew.hItem,m_pAllBookmarks);
	assert(pBookmarkFolder != NULL);

	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);
	NBookmarkHelper::InsertBookmarksIntoListView(hListView,pBookmarkFolder);
}

void CManageBookmarksDialog::OnDblClk(NMHDR *pnmhdr)
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

	if(pnmhdr->hwndFrom == hListView)
	{
		/* TODO: Open bookmark folder/bookmark. */
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
	ImageList_Destroy(m_himlTreeView);
	return 0;
}

void CManageBookmarksDialog::SaveState()
{
	m_pmbdps->SaveDialogPosition(m_hDlg);

	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);
	m_pmbdps->m_iColumnWidth1 = ListView_GetColumnWidth(hListView,0);
	m_pmbdps->m_iColumnWidth2 = ListView_GetColumnWidth(hListView,1);

	m_pmbdps->m_bStateSaved = TRUE;
}

CManageBookmarksDialogPersistentSettings::CManageBookmarksDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	m_iColumnWidth1 = DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	m_iColumnWidth2 = DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
}

CManageBookmarksDialogPersistentSettings::~CManageBookmarksDialogPersistentSettings()
{
	
}

CManageBookmarksDialogPersistentSettings& CManageBookmarksDialogPersistentSettings::GetInstance()
{
	static CManageBookmarksDialogPersistentSettings mbdps;
	return mbdps;
}