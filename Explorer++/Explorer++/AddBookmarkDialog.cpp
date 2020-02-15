// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddBookmarkDialog.h"
#include "BookmarkHelper.h"
#include "BookmarkItem.h"
#include "BookmarkTree.h"
#include "BookmarkTreeView.h"
#include "CoreInterface.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/WindowHelper.h"

const TCHAR AddBookmarkDialogPersistentSettings::SETTINGS_KEY[] = _T("AddBookmark");

AddBookmarkDialog::AddBookmarkDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *expp,
	BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem, BookmarkItem *defaultParentSelection,
	BookmarkItem **selectedParentFolder, std::optional<std::wstring> customDialogTitle) :
	BaseDialog(hInstance, IDD_ADD_BOOKMARK, hParent, true),
	m_expp(expp),
	m_bookmarkTree(bookmarkTree),
	m_bookmarkItem(bookmarkItem),
	m_selectedParentFolder(selectedParentFolder),
	m_customDialogTitle(customDialogTitle),
	m_ErrorBrush(CreateSolidBrush(ERROR_BACKGROUND_COLOR))
{
	m_persistentSettings = &AddBookmarkDialogPersistentSettings::GetInstance();

	/* If the singleton settings class has not been initialized
	yet, mark the root bookmark as selected and expanded. This
	is only needed the first time this dialog is shown, as
	selection and expansion info will be saved each time after
	that. */
	if(!m_persistentSettings->m_bInitialized)
	{
		m_persistentSettings->m_guidSelected = m_bookmarkTree->GetBookmarksToolbarFolder()->GetGUID();

		m_persistentSettings->m_bInitialized = true;
	}

	BookmarkItem *parent = bookmarkItem->GetParent();

	if (parent)
	{
		m_persistentSettings->m_guidSelected = parent->GetGUID();
	}
	else if (defaultParentSelection)
	{
		m_persistentSettings->m_guidSelected = defaultParentSelection->GetGUID();
	}
}

INT_PTR AddBookmarkDialog::OnInitDialog()
{
	if (m_bookmarkItem->IsFolder())
	{
		UpdateDialogForBookmarkFolder();
	}

	SetDialogTitle();

	SetDlgItemText(m_hDlg, IDC_BOOKMARK_NAME, m_bookmarkItem->GetName().c_str());

	if (m_bookmarkItem->IsBookmark())
	{
		SetDlgItemText(m_hDlg, IDC_BOOKMARK_LOCATION, m_bookmarkItem->GetLocation().c_str());
	}

	if (m_bookmarkItem->GetName().empty() || (m_bookmarkItem->IsBookmark() && m_bookmarkItem->GetLocation().empty()))
	{
		EnableWindow(GetDlgItem(m_hDlg,IDOK),FALSE);
	}

	HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);

	m_pBookmarkTreeView = new BookmarkTreeView(hTreeView, GetInstance(), m_expp, m_bookmarkTree,
		m_persistentSettings->m_setExpansion, m_persistentSettings->m_guidSelected);

	HWND hEditName = GetDlgItem(m_hDlg,IDC_BOOKMARK_NAME);
	SendMessage(hEditName,EM_SETSEL,0,-1);
	SetFocus(hEditName);

	m_persistentSettings->RestoreDialogPosition(m_hDlg,false);

	return 0;
}

// A bookmark folder has no location field. Therefore, if the bookmark item
// being added or updated is a folder, the location control will be removed, the
// controls below it will be moved up and the dialog will be resized.
void AddBookmarkDialog::UpdateDialogForBookmarkFolder()
{
	RECT locationLabelRect;
	HWND locationLabel = GetDlgItem(m_hDlg, IDC_STATIC_LOCATION);
	GetWindowRect(locationLabel, &locationLabelRect);
	ShowWindow(locationLabel, SW_HIDE);

	HWND location = GetDlgItem(m_hDlg, IDC_BOOKMARK_LOCATION);
	ShowWindow(location, SW_HIDE);

	RECT treeViewRect;
	HWND treeView = GetDlgItem(m_hDlg, IDC_BOOKMARK_TREEVIEW);
	GetWindowRect(treeView, &treeViewRect);

	int yOffset = treeViewRect.top - locationLabelRect.top;

	const UINT controlsToMove[] = { IDC_BOOKMARK_TREEVIEW, IDC_BOOKMARK_NEWFOLDER,
		IDOK, IDCANCEL, IDC_GRIPPER };

	for (auto control : controlsToMove)
	{
		HWND controlWindow = GetDlgItem(m_hDlg, control);

		RECT controlRect;
		GetWindowRect(controlWindow, &controlRect);

		MapWindowPoints(HWND_DESKTOP, m_hDlg, reinterpret_cast<LPPOINT>(&controlRect), 2);
		SetWindowPos(controlWindow, nullptr, controlRect.left, controlRect.top - yOffset,
			0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

	RECT dialogRect;
	GetWindowRect(m_hDlg, &dialogRect);
	InflateRect(&dialogRect, 0, -yOffset);

	m_iMinWidth = GetRectWidth(&dialogRect);
	m_iMinHeight = GetRectHeight(&dialogRect);

	SetWindowPos(m_hDlg, nullptr, 0, 0, GetRectWidth(&dialogRect), GetRectHeight(&dialogRect),
		SWP_NOMOVE | SWP_NOZORDER);
}

void AddBookmarkDialog::SetDialogTitle()
{
	std::wstring dialogTitle = LoadDialogTitle();
	SetWindowText(m_hDlg, dialogTitle.c_str());
}

std::wstring AddBookmarkDialog::LoadDialogTitle()
{
	if (m_customDialogTitle)
	{
		return *m_customDialogTitle;
	}

	auto existingBookmarkItem = BookmarkHelper::GetBookmarkItemById(m_bookmarkTree, m_bookmarkItem->GetGUID());
	UINT stringId;

	if (existingBookmarkItem)
	{
		if (m_bookmarkItem->IsBookmark())
		{
			stringId = IDS_ADD_BOOKMARK_TITLE_EDIT_BOOKMARK;
		}
		else
		{
			stringId = IDS_ADD_BOOKMARK_TITLE_EDIT_FOLDER;
		}
	}
	else
	{
		if (m_bookmarkItem->IsBookmark())
		{
			stringId = IDS_ADD_BOOKMARK_TITLE_ADD_BOOKMARK;
		}
		else
		{
			stringId = IDS_ADD_BOOKMARK_TITLE_ADD_FOLDER;
		}
	}

	return ResourceHelper::LoadString(GetInstance(), stringId);
}

wil::unique_hicon AddBookmarkDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_expp->GetIconResourceLoader()->LoadIconFromPNGAndScale(Icon::AddBookmark, iconWidth, iconHeight);
}

void AddBookmarkDialog::GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc,
	std::list<ResizableDialog::Control_t> &ControlList)
{
	dsc = BaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	ResizableDialog::Control_t Control;

	Control.iID = IDC_BOOKMARK_NAME;
	Control.Type = ResizableDialog::TYPE_RESIZE;
	Control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_BOOKMARK_LOCATION;
	Control.Type = ResizableDialog::TYPE_RESIZE;
	Control.Constraint = ResizableDialog::CONSTRAINT_X;
	ControlList.push_back(Control);

	Control.iID = IDC_BOOKMARK_TREEVIEW;
	Control.Type = ResizableDialog::TYPE_RESIZE;
	Control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_BOOKMARK_NEWFOLDER;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_Y;
	ControlList.push_back(Control);

	Control.iID = IDOK;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDCANCEL;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);

	Control.iID = IDC_GRIPPER;
	Control.Type = ResizableDialog::TYPE_MOVE;
	Control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	ControlList.push_back(Control);
}

INT_PTR AddBookmarkDialog::OnCtlColorEdit(HWND hwnd,HDC hdc)
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

INT_PTR AddBookmarkDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if(HIWORD(wParam) != 0)
	{
		switch(HIWORD(wParam))
		{
		case EN_CHANGE:
			BOOL bEnable = (GetWindowTextLength(GetDlgItem(m_hDlg, IDC_BOOKMARK_NAME)) != 0);

			if (m_bookmarkItem->IsBookmark())
			{
				bEnable &= (GetWindowTextLength(GetDlgItem(m_hDlg, IDC_BOOKMARK_LOCATION)) != 0);
			}

			EnableWindow(GetDlgItem(m_hDlg, IDOK), bEnable);

			if (LOWORD(wParam) == IDC_BOOKMARK_NAME ||
				LOWORD(wParam) == IDC_BOOKMARK_LOCATION)
			{
				/* Used to ensure the edit controls are redrawn properly when
				changing the background color. */
				InvalidateRect(GetDlgItem(m_hDlg, LOWORD(wParam)), nullptr, TRUE);
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

void AddBookmarkDialog::OnOk()
{
	HWND hName = GetDlgItem(m_hDlg,IDC_BOOKMARK_NAME);
	std::wstring name;
	GetWindowString(hName,name);

	HWND hLocation = GetDlgItem(m_hDlg,IDC_BOOKMARK_LOCATION);
	std::wstring location;
	GetWindowString(hLocation,location);

	if (name.empty() || (m_bookmarkItem->IsBookmark() && location.empty()))
	{
		EndDialog(m_hDlg, RETURN_CANCEL);
		return;
	}

	HWND hTreeView = GetDlgItem(m_hDlg, IDC_BOOKMARK_TREEVIEW);
	HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
	*m_selectedParentFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected);

	m_bookmarkItem->SetName(name);

	if (m_bookmarkItem->IsBookmark())
	{
		m_bookmarkItem->SetLocation(location);
	}

	EndDialog(m_hDlg, RETURN_OK);
}

void AddBookmarkDialog::OnCancel()
{
	EndDialog(m_hDlg, RETURN_CANCEL);
}

void AddBookmarkDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);

	SaveTreeViewState();

	m_persistentSettings->m_bStateSaved = TRUE;
}

void AddBookmarkDialog::SaveTreeViewState()
{
	HWND hTreeView = GetDlgItem(m_hDlg,IDC_BOOKMARK_TREEVIEW);

	HTREEITEM hSelected = TreeView_GetSelection(hTreeView);
	const auto bookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected);
	m_persistentSettings->m_guidSelected = bookmarkFolder->GetGUID();

	m_persistentSettings->m_setExpansion.clear();
	SaveTreeViewExpansionState(hTreeView,TreeView_GetRoot(hTreeView));
}

void AddBookmarkDialog::SaveTreeViewExpansionState(HWND hTreeView,HTREEITEM hItem)
{
	UINT uState = TreeView_GetItemState(hTreeView,hItem,TVIS_EXPANDED);

	if(uState & TVIS_EXPANDED)
	{
		const auto bookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hItem);
		m_persistentSettings->m_setExpansion.insert(bookmarkFolder->GetGUID());

		HTREEITEM hChild = TreeView_GetChild(hTreeView,hItem);
		SaveTreeViewExpansionState(hTreeView,hChild);

		while((hChild = TreeView_GetNextSibling(hTreeView,hChild)) != nullptr)
		{
			SaveTreeViewExpansionState(hTreeView,hChild);
		}
	}
}

INT_PTR AddBookmarkDialog::OnClose()
{
	EndDialog(m_hDlg, RETURN_CANCEL);
	return 0;
}

INT_PTR AddBookmarkDialog::OnNcDestroy()
{
	delete m_pBookmarkTreeView;

	return 0;
}

AddBookmarkDialogPersistentSettings::AddBookmarkDialogPersistentSettings() :
DialogSettings(SETTINGS_KEY)
{
	m_bInitialized = false;
}

AddBookmarkDialogPersistentSettings& AddBookmarkDialogPersistentSettings::GetInstance()
{
	static AddBookmarkDialogPersistentSettings abdps;
	return abdps;
}