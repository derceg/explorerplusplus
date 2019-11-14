// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddBookmarkDialog.h"
#include "BookmarkHelper.h"
#include "Explorer++_internal.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"
#include <stack>

const TCHAR CAddBookmarkDialogPersistentSettings::SETTINGS_KEY[] = _T("AddBookmark");

CAddBookmarkDialog::CAddBookmarkDialog(HINSTANCE hInstance,int iResource,HWND hParent,
	CBookmarkFolder &AllBookmarks,CBookmark &Bookmark) :
	CBaseDialog(hInstance, iResource, hParent, true),
	m_AllBookmarks(AllBookmarks),
	m_Bookmark(Bookmark),
	m_ErrorBrush(CreateSolidBrush(ERROR_BACKGROUND_COLOR))
{
	m_pabdps = &CAddBookmarkDialogPersistentSettings::GetInstance();

	/* If the singleton settings class has not been initialized
	yet, mark the root bookmark as selected and expanded. This
	is only needed the first time this dialog is shown, as
	selection and expansion info will be saved each time after
	that. */
	if(!m_pabdps->m_bInitialized)
	{
		m_pabdps->m_guidSelected = AllBookmarks.GetGUID();
		m_pabdps->m_setExpansion.insert(AllBookmarks.GetGUID());

		m_pabdps->m_bInitialized = true;
	}
}

CAddBookmarkDialog::~CAddBookmarkDialog()
{
	DeleteObject(m_ErrorBrush);
}

INT_PTR CAddBookmarkDialog::OnInitDialog()
{
	SetDlgItemText(m_hDlg,IDC_BOOKMARK_NAME,m_Bookmark.GetName().c_str());
	SetDlgItemText(m_hDlg,IDC_BOOKMARK_LOCATION,m_Bookmark.GetLocation().c_str());

	if(m_Bookmark.GetName().size() == 0 ||
		m_Bookmark.GetLocation().size() == 0)
	{
		EnableWindow(GetDlgItem(m_hDlg,IDOK),FALSE);
	}

	HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);

	m_pBookmarkTreeView = new CBookmarkTreeView(hTreeView,GetInstance(),&m_AllBookmarks,
		m_pabdps->m_guidSelected,m_pabdps->m_setExpansion);

	HWND hEditName = GetDlgItem(m_hDlg,IDC_BOOKMARK_NAME);
	SendMessage(hEditName,EM_SETSEL,0,-1);
	SetFocus(hEditName);

	CBookmarkItemNotifier::GetInstance().AddObserver(this);

	m_pabdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

wil::unique_hicon CAddBookmarkDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return IconResourceLoader::LoadIconFromPNGAndScale(Icon::AddBookmark, iconWidth, iconHeight);
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

INT_PTR CAddBookmarkDialog::OnCtlColorEdit(HWND hwnd,HDC hdc)
{
	if(hwnd == GetDlgItem(m_hDlg,IDC_BOOKMARK_NAME) ||
		hwnd == GetDlgItem(m_hDlg,IDC_BOOKMARK_LOCATION))
	{
		if(GetWindowTextLength(hwnd) == 0)
		{
			SetBkMode(hdc,TRANSPARENT);
			return reinterpret_cast<INT_PTR>(m_ErrorBrush);
		}
	}

	return FALSE;
}

INT_PTR CAddBookmarkDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

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

			if(LOWORD(wParam) == IDC_BOOKMARK_NAME ||
				LOWORD(wParam) == IDC_BOOKMARK_LOCATION)
			{
				/* Used to ensure the edit controls are redrawn properly when
				changing the background color. */
				InvalidateRect(GetDlgItem(m_hDlg,LOWORD(wParam)),NULL,TRUE);
			}
			break;
		}
	}
	else
	{
		switch(LOWORD(wParam))
		{
		case IDC_BOOKMARK_NEWFOLDER:
			m_pBookmarkTreeView->CreateNewFolder();
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

void CAddBookmarkDialog::OnOk()
{
	HWND hName = GetDlgItem(m_hDlg,IDC_BOOKMARK_NAME);
	std::wstring strName;
	GetWindowString(hName,strName);

	HWND hLocation = GetDlgItem(m_hDlg,IDC_BOOKMARK_LOCATION);
	std::wstring strLocation;
	GetWindowString(hLocation,strLocation);

	if(strName.size() > 0 &&
		strLocation.size() > 0)
	{
		HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);
		HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
		CBookmarkFolder &BookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected);

		CBookmark Bookmark = CBookmark::Create(strName,strLocation,_T(""));
		BookmarkFolder.InsertBookmark(Bookmark);
	}

	EndDialog(m_hDlg,1);
}

void CAddBookmarkDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

void CAddBookmarkDialog::SaveState()
{
	m_pabdps->SaveDialogPosition(m_hDlg);

	SaveTreeViewState();

	m_pabdps->m_bStateSaved = TRUE;
}

void CAddBookmarkDialog::SaveTreeViewState()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);

	HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
	CBookmarkFolder &BookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected);
	m_pabdps->m_guidSelected = BookmarkFolder.GetGUID();

	m_pabdps->m_setExpansion.clear();
	SaveTreeViewExpansionState(hTreeView,TreeView_GetRoot(hTreeView));
}

void CAddBookmarkDialog::SaveTreeViewExpansionState(HWND hTreeView,HTREEITEM hItem)
{
	UINT uState = TreeView_GetItemState(hTreeView,hItem,TVIS_EXPANDED);

	if(uState & TVIS_EXPANDED)
	{
		CBookmarkFolder &BookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hItem);
		m_pabdps->m_setExpansion.insert(BookmarkFolder.GetGUID());

		HTREEITEM hChild = TreeView_GetChild(hTreeView,hItem);
		SaveTreeViewExpansionState(hTreeView,hChild);

		while((hChild = TreeView_GetNextSibling(hTreeView,hChild)) != NULL)
		{
			SaveTreeViewExpansionState(hTreeView,hChild);
		}
	}
}

INT_PTR CAddBookmarkDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

INT_PTR CAddBookmarkDialog::OnDestroy()
{
	CBookmarkItemNotifier::GetInstance().RemoveObserver(this);

	return 0;
}

INT_PTR CAddBookmarkDialog::OnNcDestroy()
{
	delete m_pBookmarkTreeView;

	return 0;
}

void CAddBookmarkDialog::OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmark &Bookmark,std::size_t Position)
{
	UNREFERENCED_PARAMETER(ParentBookmarkFolder);
	UNREFERENCED_PARAMETER(Bookmark);
	UNREFERENCED_PARAMETER(Position);
}

void CAddBookmarkDialog::OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmarkFolder &BookmarkFolder,std::size_t Position)
{
	m_pBookmarkTreeView->BookmarkFolderAdded(ParentBookmarkFolder,BookmarkFolder,Position);
}

void CAddBookmarkDialog::OnBookmarkModified(const GUID &guid)
{
	UNREFERENCED_PARAMETER(guid);
}

void CAddBookmarkDialog::OnBookmarkFolderModified(const GUID &guid)
{
	m_pBookmarkTreeView->BookmarkFolderModified(guid);
}

void CAddBookmarkDialog::OnBookmarkRemoved(const GUID &guid)
{
	UNREFERENCED_PARAMETER(guid);
}

void CAddBookmarkDialog::OnBookmarkFolderRemoved(const GUID &guid)
{
	m_pBookmarkTreeView->BookmarkFolderRemoved(guid);
}

CAddBookmarkDialogPersistentSettings::CAddBookmarkDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	m_bInitialized = false;
}

CAddBookmarkDialogPersistentSettings::~CAddBookmarkDialogPersistentSettings()
{
	
}

CAddBookmarkDialogPersistentSettings& CAddBookmarkDialogPersistentSettings::GetInstance()
{
	static CAddBookmarkDialogPersistentSettings abdps;
	return abdps;
}