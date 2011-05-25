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
#include <stack>
#include "Explorer++_internal.h"
#include "AddBookmarkDialog.h"
#include "BookmarkHelper.h"
#include "MainResource.h"


namespace NAddBookmarkDialog
{
	LRESULT CALLBACK TreeViewEditProcStub(HWND hwnd,UINT uMsg,
		WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
}

const TCHAR CAddBookmarkDialogPersistentSettings::SETTINGS_KEY[] = _T("AddBookmark");

CAddBookmarkDialog::CAddBookmarkDialog(HINSTANCE hInstance,int iResource,HWND hParent,
	BookmarkFolder *pAllBookmarks,Bookmark *pBookmark) :
m_pAllBookmarks(pAllBookmarks),
m_pBookmark(pBookmark),
CBaseDialog(hInstance,iResource,hParent,true)
{
	m_pabdps = &CAddBookmarkDialogPersistentSettings::GetInstance();
}

CAddBookmarkDialog::~CAddBookmarkDialog()
{

}

BOOL CAddBookmarkDialog::OnInitDialog()
{
	SetDlgItemText(m_hDlg,IDC_BOOKMARK_NAME,m_pBookmark->GetName().c_str());
	SetDlgItemText(m_hDlg,IDC_BOOKMARK_LOCATION,m_pBookmark->GetLocation().c_str());

	if(m_pBookmark->GetName().size() == 0 ||
		m_pBookmark->GetLocation().size() == 0)
	{
		EnableWindow(GetDlgItem(m_hDlg,IDOK),FALSE);
	}

	HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);
	SetWindowTheme(hTreeView,L"Explorer",NULL);

	m_himlTreeView = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);
	HBITMAP hBitmap = LoadBitmap(GetInstance(),MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himlTreeView,hBitmap,NULL);
	TreeView_SetImageList(hTreeView,m_himlTreeView,TVSIL_NORMAL);
	DeleteObject(hBitmap);

	NBookmarkHelper::InsertFoldersIntoTreeView(hTreeView,m_pAllBookmarks);

	HWND hEditName = GetDlgItem(m_hDlg,IDC_BOOKMARK_NAME);
	SendMessage(hEditName,EM_SETSEL,0,-1);
	SetFocus(hEditName);

	m_pabdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

void CAddBookmarkDialog::GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,
	std::list<CResizableDialog::Control_t> &ControlList)
{
	dsc = CBaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	CResizableDialog::Control_t Control;

	Control.iID = IDC_BOOKMARK_NAME;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_BOOKMARK_LOCATION;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_BOOKMARK_TREEVIEW;
	Control.Type = CResizableDialog::TYPE_RESIZE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_BOOKMARK_NEWFOLDER;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDOK;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDCANCEL;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_GRIPPER;
	Control.Type = CResizableDialog::TYPE_MOVE;
	Control.Constraint = CResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);
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

BOOL CAddBookmarkDialog::OnNotify(NMHDR *pnmhdr)
{
	switch(pnmhdr->code)
	{
	case TVN_BEGINLABELEDIT:
		OnTvnBeginLabelEdit();
		break;

	case TVN_ENDLABELEDIT:
		return OnTvnEndLabelEdit(reinterpret_cast<NMTVDISPINFO *>(pnmhdr));
		break;

	case TVN_KEYDOWN:
		OnTvnKeyDown(reinterpret_cast<NMTVKEYDOWN *>(pnmhdr));
		break;
	}

	return 0;
}

void CAddBookmarkDialog::OnNewFolder()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);
	HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);

	assert(hSelectedItem != NULL);

	TCHAR szTemp[64];
	LoadString(GetInstance(),IDS_BOOKMARKS_NEWBOOKMARKFOLDER,szTemp,SIZEOF_ARRAY(szTemp));
	BookmarkFolder NewBookmarkFolder(szTemp);

	BookmarkFolder *pParentBookmarkFolder = NBookmarkHelper::GetBookmarkFolderFromTreeView(hTreeView,
		hSelectedItem,m_pAllBookmarks);
	pParentBookmarkFolder->InsertBookmarkFolder(NewBookmarkFolder);
	HTREEITEM hNewItem = NBookmarkHelper::InsertFolderIntoTreeView(hTreeView,hSelectedItem,&NewBookmarkFolder);

	TVITEM tvi;
	tvi.mask		= TVIF_CHILDREN;
	tvi.hItem		= hSelectedItem;
	tvi.cChildren	= 1;
	TreeView_SetItem(hTreeView,&tvi);
	TreeView_Expand(hTreeView,hSelectedItem,TVE_EXPAND);

	/* The item will be selected, as it is assumed that if
	the user creates a new folder, they intend to place any
	new bookmark within that folder. */
	SetFocus(hTreeView);
	TreeView_SelectItem(hTreeView,hNewItem);
	TreeView_EditLabel(hTreeView,hNewItem);
}

void CAddBookmarkDialog::OnTvnBeginLabelEdit()
{
	HWND hEdit = reinterpret_cast<HWND>(SendDlgItemMessage(m_hDlg,
		IDC_BOOKMARK_TREEVIEW,TVM_GETEDITCONTROL,0,0));
	SetWindowSubclass(hEdit,NAddBookmarkDialog::TreeViewEditProcStub,0,
		reinterpret_cast<DWORD_PTR>(this));
}

BOOL CAddBookmarkDialog::OnTvnEndLabelEdit(NMTVDISPINFO *pnmtvdi)
{
	HWND hEdit = reinterpret_cast<HWND>(SendDlgItemMessage(m_hDlg,
		IDC_BOOKMARK_TREEVIEW,TVM_GETEDITCONTROL,0,0));
	RemoveWindowSubclass(hEdit,NAddBookmarkDialog::TreeViewEditProcStub,0);

	if(pnmtvdi->item.pszText != NULL &&
		lstrlen(pnmtvdi->item.pszText) > 0)
	{
		HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);
		BookmarkFolder *pBookmarkFolder = NBookmarkHelper::GetBookmarkFolderFromTreeView(hTreeView,
			pnmtvdi->item.hItem,m_pAllBookmarks);
		pBookmarkFolder->SetName(pnmtvdi->item.pszText);

		SetWindowLongPtr(m_hDlg,DWLP_MSGRESULT,TRUE);
		return TRUE;
	}

	SetWindowLongPtr(m_hDlg,DWLP_MSGRESULT,FALSE);
	return FALSE;
}

LRESULT CALLBACK NAddBookmarkDialog::TreeViewEditProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CAddBookmarkDialog *pabd = reinterpret_cast<CAddBookmarkDialog *>(dwRefData);

	return pabd->TreeViewEditProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CAddBookmarkDialog::TreeViewEditProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_GETDLGCODE:
		switch(wParam)
		{
		case VK_RETURN:
			return DLGC_WANTALLKEYS;
			break;
		}
		break;
	}

	return DefSubclassProc(hwnd,Msg,wParam,lParam);
}

void CAddBookmarkDialog::OnTvnKeyDown(NMTVKEYDOWN *pnmtvkd)
{
	switch(pnmtvkd->wVKey)
	{
	case VK_F2:
		{
			HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);
			HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);
			TreeView_EditLabel(hTreeView,hSelectedItem);
		}
		break;
	}
}

void CAddBookmarkDialog::OnOk()
{
	EndDialog(m_hDlg,1);
}

void CAddBookmarkDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CAddBookmarkDialog::SaveState()
{
	m_pabdps->SaveDialogPosition(m_hDlg);

	m_pabdps->m_bStateSaved = TRUE;
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
	/* TODO: Save treeview expansion and selection info. */
}

CAddBookmarkDialogPersistentSettings::~CAddBookmarkDialogPersistentSettings()
{
	
}

CAddBookmarkDialogPersistentSettings& CAddBookmarkDialogPersistentSettings::GetInstance()
{
	static CAddBookmarkDialogPersistentSettings abdps;
	return abdps;
}