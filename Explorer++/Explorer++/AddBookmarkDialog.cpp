// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddBookmarkDialog.h"
#include "BookmarkHelper.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"

const TCHAR CAddBookmarkDialogPersistentSettings::SETTINGS_KEY[] = _T("AddBookmark");

CAddBookmarkDialog::CAddBookmarkDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *expp,
	BookmarkTree *bookmarkTree, std::unique_ptr<BookmarkItem> bookmarkItem) :
	CBaseDialog(hInstance, IDD_ADD_BOOKMARK, hParent, true),
	m_expp(expp),
	m_bookmarkTree(bookmarkTree),
	m_bookmarkItem(std::move(bookmarkItem)),
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
		auto rootGuid = m_bookmarkTree->GetRoot()->GetGUID();

		m_pabdps->m_guidSelected = rootGuid;
		m_pabdps->m_setExpansion.insert(rootGuid);

		m_pabdps->m_bInitialized = true;
	}
}

INT_PTR CAddBookmarkDialog::OnInitDialog()
{
	SetDlgItemText(m_hDlg, IDC_BOOKMARK_NAME, m_bookmarkItem->GetName().c_str());
	SetDlgItemText(m_hDlg, IDC_BOOKMARK_LOCATION, m_bookmarkItem->GetLocation().c_str());

	if(m_bookmarkItem->GetName().empty() || m_bookmarkItem->GetLocation().empty())
	{
		EnableWindow(GetDlgItem(m_hDlg,IDOK),FALSE);
	}

	HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);

	m_pBookmarkTreeView = new CBookmarkTreeView(hTreeView,GetInstance(),m_expp,m_bookmarkTree,
		m_pabdps->m_guidSelected,m_pabdps->m_setExpansion);

	HWND hEditName = GetDlgItem(m_hDlg,IDC_BOOKMARK_NAME);
	SendMessage(hEditName,EM_SETSEL,0,-1);
	SetFocus(hEditName);

	m_pabdps->RestoreDialogPosition(m_hDlg,true);

	return 0;
}

wil::unique_hicon CAddBookmarkDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_expp->GetIconResourceLoader()->LoadIconFromPNGAndScale(Icon::AddBookmark, iconWidth, iconHeight);
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
			return reinterpret_cast<INT_PTR>(m_ErrorBrush.get());
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
	std::wstring name;
	GetWindowString(hName,name);

	HWND hLocation = GetDlgItem(m_hDlg,IDC_BOOKMARK_LOCATION);
	std::wstring location;
	GetWindowString(hLocation,location);

	if(!name.empty() && !location.empty())
	{
		HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);
		HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
		auto bookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected);

		m_bookmarkItem->SetName(name);
		m_bookmarkItem->SetLocation(location);

		m_bookmarkTree->AddBookmarkItem(bookmarkFolder, std::move(m_bookmarkItem), bookmarkFolder->GetChildren().size());
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
	const auto bookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected);
	m_pabdps->m_guidSelected = bookmarkFolder->GetGUID();

	m_pabdps->m_setExpansion.clear();
	SaveTreeViewExpansionState(hTreeView,TreeView_GetRoot(hTreeView));
}

void CAddBookmarkDialog::SaveTreeViewExpansionState(HWND hTreeView,HTREEITEM hItem)
{
	UINT uState = TreeView_GetItemState(hTreeView,hItem,TVIS_EXPANDED);

	if(uState & TVIS_EXPANDED)
	{
		const auto bookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hItem);
		m_pabdps->m_setExpansion.insert(bookmarkFolder->GetGUID());

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

INT_PTR CAddBookmarkDialog::OnNcDestroy()
{
	delete m_pBookmarkTreeView;

	return 0;
}

CAddBookmarkDialogPersistentSettings::CAddBookmarkDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	m_bInitialized = false;
}

CAddBookmarkDialogPersistentSettings& CAddBookmarkDialogPersistentSettings::GetInstance()
{
	static CAddBookmarkDialogPersistentSettings abdps;
	return abdps;
}