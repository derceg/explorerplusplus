/******************************************************************
 *
 * Project: Explorer++
 * File: AddBookmarkDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Add Bookmark' dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++_internal.h"
#include "AddBookmarkDialog.h"
#include "MainResource.h"


const TCHAR CAddBookmarkDialogPersistentSettings::SETTINGS_KEY[] = _T("AddBookmark");

CAddBookmarkDialog::CAddBookmarkDialog(HINSTANCE hInstance,int iResource,HWND hParent,
	BookmarkFolder *pAllBookmarks,Bookmark *pBookmark) :
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pAllBookmarks = pAllBookmarks;
	m_pBookmark = pBookmark;

	m_pabdps = &CAddBookmarkDialogPersistentSettings::GetInstance();
}

CAddBookmarkDialog::~CAddBookmarkDialog()
{

}

BOOL CAddBookmarkDialog::OnInitDialog()
{
	SetDlgItemText(m_hDlg,IDC_BOOKMARK_NAME,m_pBookmark->GetName().c_str());
	SetDlgItemText(m_hDlg,IDC_BOOKMARK_LOCATION,m_pBookmark->GetLocation().c_str());

	HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);
	SetWindowTheme(hTreeView,L"Explorer",NULL);

	HIMAGELIST m_himlTreeView = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetInstance(),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himlTreeView,hBitmap,NULL);
	TreeView_SetImageList(hTreeView,m_himlTreeView,TVSIL_NORMAL);
	DeleteObject(hBitmap);

	InsertFoldersIntoTreeView();

	SetFocus(GetDlgItem(m_hDlg,IDC_BOOKMARK_NAME));

	return 0;
}

void CAddBookmarkDialog::InsertFoldersIntoTreeView()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);

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

HTREEITEM CAddBookmarkDialog::InsertFolderIntoTreeView(HWND hTreeView,HTREEITEM hParent,
	BookmarkFolder *pBookmarkFolder)
{
	TCHAR szText[256];
	StringCchCopy(szText,SIZEOF_ARRAY(szText),pBookmarkFolder->GetName().c_str());

	TVITEMEX tviex;
	tviex.mask				= TVIF_TEXT|TVIF_IMAGE|TVIF_CHILDREN|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tviex.pszText			= szText;
	tviex.iImage			= SHELLIMAGES_NEWTAB;
	tviex.iSelectedImage	= SHELLIMAGES_NEWTAB;
	tviex.cChildren			= 1;
	tviex.lParam			= pBookmarkFolder->GetID();

	TVINSERTSTRUCT tvis;
	tvis.hParent			= hParent;
	tvis.hInsertAfter		= TVI_LAST;
	tvis.itemex				= tviex;
	HTREEITEM hItem = TreeView_InsertItem(hTreeView,&tvis);

	return hItem;
}

BOOL CAddBookmarkDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	if(HIWORD(wParam) != 0)
	{
		switch(HIWORD(wParam))
		{
		case EN_CHANGE:
			/* If either the name or location fields are empty,
			disable the ok button. */
			BOOL bEnable = (GetWindowTextLength(GetDlgItem(m_hDlg,IDC_BOOKMARK_NAME)) != 0 &&
				GetWindowTextLength(GetDlgItem(m_hDlg,IDC_BOOKMARK_LOCATION)) != 0);

			EnableWindow(GetDlgItem(m_hDlg,IDOK),bEnable);
			break;
		}
	}
	else
	{
		switch(LOWORD(wParam))
		{
		case IDC_BOOKMARK_NEWFOLDER:
			OnNewFolder();
			break;

		case IDOK:
			OnOk();
			break;

		case IDCANCEL:
			OnCancel();
			break;
		}
	}

	return 0;
}

void CAddBookmarkDialog::OnNewFolder()
{
	
}

void CAddBookmarkDialog::OnOk()
{
	EndDialog(m_hDlg,1);
}

void CAddBookmarkDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

BOOL CAddBookmarkDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

BOOL CAddBookmarkDialog::OnDestroy()
{
	ImageList_Destroy(m_himlTreeView);
	return 0;
}

CAddBookmarkDialogPersistentSettings::CAddBookmarkDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{

}

CAddBookmarkDialogPersistentSettings::~CAddBookmarkDialogPersistentSettings()
{
	
}

CAddBookmarkDialogPersistentSettings& CAddBookmarkDialogPersistentSettings::GetInstance()
{
	static CAddBookmarkDialogPersistentSettings abdps;
	return abdps;
}