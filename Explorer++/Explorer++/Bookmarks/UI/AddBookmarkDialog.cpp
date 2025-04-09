// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/AddBookmarkDialog.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarkTreeView.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/WindowHelper.h"

const TCHAR AddBookmarkDialogPersistentSettings::SETTINGS_KEY[] = _T("AddBookmark");

AddBookmarkDialog::AddBookmarkDialog(const ResourceLoader *resourceLoader,
	HINSTANCE resourceInstance, HWND hParent, ThemeManager *themeManager,
	BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem, BookmarkItem *defaultParentSelection,
	BookmarkItem **selectedParentFolder, const IconResourceLoader *iconResourceLoader,
	std::optional<std::wstring> customDialogTitle) :
	ThemedDialog(resourceLoader, resourceInstance, IDD_ADD_BOOKMARK, hParent,
		DialogSizingType::Both, themeManager),
	m_bookmarkTree(bookmarkTree),
	m_bookmarkItem(bookmarkItem),
	m_selectedParentFolder(selectedParentFolder),
	m_iconResourceLoader(iconResourceLoader),
	m_customDialogTitle(customDialogTitle)
{
	m_persistentSettings = &AddBookmarkDialogPersistentSettings::GetInstance();

	/* If the singleton settings class has not been initialized
	yet, mark the root bookmark as selected and expanded. This
	is only needed the first time this dialog is shown, as
	selection and expansion info will be saved each time after
	that. */
	if (!m_persistentSettings->m_bInitialized)
	{
		m_persistentSettings->m_guidSelected =
			m_bookmarkTree->GetBookmarksToolbarFolder()->GetGUID();

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

	if (m_bookmarkItem->GetName().empty()
		|| (m_bookmarkItem->IsBookmark() && m_bookmarkItem->GetLocation().empty()))
	{
		EnableWindow(GetDlgItem(m_hDlg, IDOK), FALSE);
	}

	HWND hTreeView = GetDlgItem(m_hDlg, IDC_BOOKMARK_TREEVIEW);

	m_pBookmarkTreeView =
		new BookmarkTreeView(hTreeView, GetResourceInstance(), m_iconResourceLoader, m_bookmarkTree,
			m_persistentSettings->m_setExpansion, m_persistentSettings->m_guidSelected);

	HWND hEditName = GetDlgItem(m_hDlg, IDC_BOOKMARK_NAME);
	SendMessage(hEditName, EM_SETSEL, 0, -1);
	SetFocus(hEditName);

	m_persistentSettings->RestoreDialogPosition(m_hDlg, false);

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

	const UINT controlsToMove[] = { IDC_BOOKMARK_TREEVIEW, IDC_BOOKMARK_NEWFOLDER, IDOK, IDCANCEL };

	for (auto control : controlsToMove)
	{
		HWND controlWindow = GetDlgItem(m_hDlg, control);

		RECT controlRect;
		GetWindowRect(controlWindow, &controlRect);

		MapWindowPoints(HWND_DESKTOP, m_hDlg, reinterpret_cast<LPPOINT>(&controlRect), 2);
		SetWindowPos(controlWindow, nullptr, controlRect.left, controlRect.top - yOffset, 0, 0,
			SWP_NOSIZE | SWP_NOZORDER);
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

	auto existingBookmarkItem =
		BookmarkHelper::GetBookmarkItemById(m_bookmarkTree, m_bookmarkItem->GetGUID());
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

	return m_resourceLoader->LoadString(stringId);
}

wil::unique_hicon AddBookmarkDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_iconResourceLoader->LoadIconFromPNGAndScale(Icon::AddBookmark, iconWidth, iconHeight);
}

std::vector<ResizableDialogControl> AddBookmarkDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_BOOKMARK_NAME), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_BOOKMARK_LOCATION), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_BOOKMARK_TREEVIEW), MovingType::None,
		SizingType::Both);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_BOOKMARK_NEWFOLDER), MovingType::Vertical,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Both, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDCANCEL), MovingType::Both, SizingType::None);
	return controls;
}

INT_PTR AddBookmarkDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
			BOOL bEnable = (GetWindowTextLength(GetDlgItem(m_hDlg, IDC_BOOKMARK_NAME)) != 0);

			if (m_bookmarkItem->IsBookmark())
			{
				bEnable &= (GetWindowTextLength(GetDlgItem(m_hDlg, IDC_BOOKMARK_LOCATION)) != 0);
			}

			EnableWindow(GetDlgItem(m_hDlg, IDOK), bEnable);

			if (LOWORD(wParam) == IDC_BOOKMARK_NAME || LOWORD(wParam) == IDC_BOOKMARK_LOCATION)
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
		switch (LOWORD(wParam))
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
	HWND hName = GetDlgItem(m_hDlg, IDC_BOOKMARK_NAME);
	std::wstring name = GetWindowString(hName);

	HWND hLocation = GetDlgItem(m_hDlg, IDC_BOOKMARK_LOCATION);
	std::wstring location = GetWindowString(hLocation);

	if (name.empty() || (m_bookmarkItem->IsBookmark() && location.empty()))
	{
		EndDialog(m_hDlg, RETURN_CANCEL);
		return;
	}

	HWND hTreeView = GetDlgItem(m_hDlg, IDC_BOOKMARK_TREEVIEW);
	auto hSelected = TreeView_GetSelection(hTreeView);
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
	HWND hTreeView = GetDlgItem(m_hDlg, IDC_BOOKMARK_TREEVIEW);

	auto hSelected = TreeView_GetSelection(hTreeView);
	const auto bookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hSelected);
	m_persistentSettings->m_guidSelected = bookmarkFolder->GetGUID();

	m_persistentSettings->m_setExpansion.clear();
	SaveTreeViewExpansionState(hTreeView, TreeView_GetRoot(hTreeView));
}

void AddBookmarkDialog::SaveTreeViewExpansionState(HWND hTreeView, HTREEITEM hItem)
{
	UINT uState = TreeView_GetItemState(hTreeView, hItem, TVIS_EXPANDED);

	if (uState & TVIS_EXPANDED)
	{
		const auto bookmarkFolder = m_pBookmarkTreeView->GetBookmarkFolderFromTreeView(hItem);
		m_persistentSettings->m_setExpansion.insert(bookmarkFolder->GetGUID());

		auto hChild = TreeView_GetChild(hTreeView, hItem);
		SaveTreeViewExpansionState(hTreeView, hChild);

		while ((hChild = TreeView_GetNextSibling(hTreeView, hChild)) != nullptr)
		{
			SaveTreeViewExpansionState(hTreeView, hChild);
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

AddBookmarkDialogPersistentSettings &AddBookmarkDialogPersistentSettings::GetInstance()
{
	static AddBookmarkDialogPersistentSettings abdps;
	return abdps;
}
