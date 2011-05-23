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
#include "MainResource.h"


const TCHAR CManageBookmarksDialogPersistentSettings::SETTINGS_KEY[] = _T("ManageBookmarks");

CManageBookmarksDialog::CManageBookmarksDialog(HINSTANCE hInstance,int iResource,HWND hParent,
	BookmarkFolder *pAllBookmarks) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pAllBookmarks = pAllBookmarks;

	m_pmbdps = &CManageBookmarksDialogPersistentSettings::GetInstance();
}

CManageBookmarksDialog::~CManageBookmarksDialog()
{

}

BOOL CManageBookmarksDialog::OnInitDialog()
{
	/* TODO: Set text color to gray. */
	SetDlgItemText(m_hDlg,IDC_MANAGEBOOKMARKS_EDITSEARCH,_T("Search Bookmarks"));

	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);
	SetWindowTheme(hTreeView,L"Explorer",NULL);

	m_himlTreeView = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetInstance(),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himlTreeView,hBitmap,NULL);
	TreeView_SetImageList(hTreeView,m_himlTreeView,TVSIL_NORMAL);
	DeleteObject(hBitmap);

	InsertFoldersIntoTreeView();

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

	InsertBookmarksIntoListView(m_pAllBookmarks);

	return 0;
}

/* TODO: The three methods below are shared with CAddBookmarkDialog.
Extract into helper. */
void CManageBookmarksDialog::InsertFoldersIntoTreeView()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);

	HTREEITEM hRoot = InsertFolderIntoTreeView(hTreeView,NULL,m_pAllBookmarks);

	for(auto itr = m_pAllBookmarks->begin();itr != m_pAllBookmarks->end();itr++)
	{
		if(BookmarkFolder *pBookmarkFolder = boost::get<BookmarkFolder>(&(*itr)))
		{
			InsertFolderIntoTreeView(hTreeView,hRoot,pBookmarkFolder);
		}
	}

	TreeView_Expand(hTreeView,hRoot,TVE_EXPAND);
	TreeView_SelectItem(hTreeView,hRoot);
}

HTREEITEM CManageBookmarksDialog::InsertFolderIntoTreeView(HWND hTreeView,HTREEITEM hParent,
	BookmarkFolder *pBookmarkFolder)
{
	TCHAR szText[256];
	StringCchCopy(szText,SIZEOF_ARRAY(szText),pBookmarkFolder->GetName().c_str());

	int nChildren = 0;

	if(pBookmarkFolder->HasChildFolder())
	{
		nChildren = 1;
	}

	TVITEMEX tviex;
	tviex.mask				= TVIF_TEXT|TVIF_IMAGE|TVIF_CHILDREN|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tviex.pszText			= szText;
	tviex.iImage			= SHELLIMAGES_NEWTAB;
	tviex.iSelectedImage	= SHELLIMAGES_NEWTAB;
	tviex.cChildren			= nChildren;
	tviex.lParam			= pBookmarkFolder->GetID();

	TVINSERTSTRUCT tvis;
	tvis.hParent			= hParent;
	tvis.hInsertAfter		= TVI_LAST;
	tvis.itemex				= tviex;
	HTREEITEM hItem = TreeView_InsertItem(hTreeView,&tvis);

	return hItem;
}

BookmarkFolder *CManageBookmarksDialog::GetBookmarkFolderFromTreeView(HTREEITEM hItem)
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_TREEVIEW);

	std::stack<UINT> stackIDs;
	HTREEITEM hParent;
	HTREEITEM hCurrentItem = hItem;

	while((hParent = TreeView_GetParent(hTreeView,hCurrentItem)) != NULL)
	{
		TVITEM tvi;
		tvi.mask	= TVIF_HANDLE|TVIF_PARAM;
		tvi.hItem	= hCurrentItem;
		TreeView_GetItem(hTreeView,&tvi);

		stackIDs.push(static_cast<UINT>(tvi.lParam));

		hCurrentItem = hParent;
	}

	BookmarkFolder *pBookmarkFolder = m_pAllBookmarks;

	while(!stackIDs.empty())
	{
		UINT uID = stackIDs.top();
		pBookmarkFolder = pBookmarkFolder->GetBookmarkFolder(uID);
		assert(pBookmarkFolder != NULL);

		stackIDs.pop();
	}

	return pBookmarkFolder;
}

void CManageBookmarksDialog::InsertBookmarksIntoListView(BookmarkFolder *pBookmarkFolder)
{
	HWND hListView = GetDlgItem(m_hDlg,IDC_MANAGEBOOKMARKS_LISTVIEW);

	int iItem = 0;

	for(auto itr = pBookmarkFolder->begin();itr != pBookmarkFolder->end();itr++)
	{
		if(BookmarkFolder *pBookmarkFolder = boost::get<BookmarkFolder>(&(*itr)))
		{
			InsertBookmarkFolderIntoListView(hListView,pBookmarkFolder,iItem);
		}
		else if(Bookmark *pBookmark = boost::get<Bookmark>(&(*itr)))
		{
			InsertBookmarkIntoListView(hListView,pBookmark);
		}

		iItem++;
	}
}

void CManageBookmarksDialog::InsertBookmarkFolderIntoListView(HWND hListView,
	BookmarkFolder *pBookmarkFolder,int iPosition)
{
	TCHAR szName[256];
	StringCchCopy(szName,SIZEOF_ARRAY(szName),pBookmarkFolder->GetName().c_str());

	LVITEM lvi;
	lvi.mask		= LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
	lvi.iItem		= iPosition;
	lvi.iSubItem	= 0;
	lvi.iImage		= 0;
	lvi.pszText		= szName;
	lvi.lParam		= pBookmarkFolder->GetID();
	ListView_InsertItem(hListView,&lvi);
}

void CManageBookmarksDialog::InsertBookmarkIntoListView(HWND hListView,Bookmark *pBookmark)
{

}

/* TODO: No cancel button, so won't respond to escape. */
BOOL CManageBookmarksDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case IDOK:
		OnOk();
		break;
	}

	return 0;
}

void CManageBookmarksDialog::OnOk()
{
	EndDialog(m_hDlg,1);
}

BOOL CManageBookmarksDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

BOOL CManageBookmarksDialog::OnDestroy()
{
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