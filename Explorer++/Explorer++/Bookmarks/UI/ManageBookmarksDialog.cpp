// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/ManageBookmarksDialog.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkNavigationController.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarkTreeView.h"
#include "CoreInterface.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "Navigation.h"
#include "ResourceHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"

const TCHAR ManageBookmarksDialogPersistentSettings::SETTINGS_KEY[] = _T("ManageBookmarks");

ManageBookmarksDialog::ManageBookmarksDialog(HINSTANCE hInstance, HWND hParent,
	IExplorerplusplus *pexpp, Navigation *navigation, BookmarkTree *bookmarkTree) :
	BaseDialog(hInstance, IDD_MANAGE_BOOKMARKS, hParent, true),
	m_pexpp(pexpp),
	m_navigation(navigation),
	m_bookmarkTree(bookmarkTree)
{
	m_persistentSettings = &ManageBookmarksDialogPersistentSettings::GetInstance();

	if (!m_persistentSettings->m_bInitialized)
	{
		m_persistentSettings->m_bInitialized = true;
	}
}

ManageBookmarksDialog::~ManageBookmarksDialog()
{
	delete m_bookmarkTreeView;
	delete m_bookmarkListView;
}

INT_PTR ManageBookmarksDialog::OnInitDialog()
{
	SetupToolbar();
	SetupTreeView();
	SetupListView();

	m_navigationController =
		std::make_unique<BookmarkNavigationController>(m_bookmarkTree, m_bookmarkListView);
	m_navigationController->BrowseFolder(m_bookmarkTree->GetBookmarksToolbarFolder());

	SetFocus(GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW));

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

void ManageBookmarksDialog::GetResizableControlInformation(
	BaseDialog::DialogSizeConstraint &dsc, std::list<ResizableDialog::Control_t> &controlList)
{
	dsc = BaseDialog::DIALOG_SIZE_CONSTRAINT_NONE;

	ResizableDialog::Control_t control;

	control.iID = IDC_MANAGEBOOKMARKS_TREEVIEW;
	control.Type = ResizableDialog::TYPE_RESIZE;
	control.Constraint = ResizableDialog::CONSTRAINT_Y;
	controlList.push_back(control);

	control.iID = IDC_MANAGEBOOKMARKS_LISTVIEW;
	control.Type = ResizableDialog::TYPE_RESIZE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	controlList.push_back(control);

	control.iID = IDOK;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	controlList.push_back(control);

	control.iID = IDC_GRIPPER;
	control.Type = ResizableDialog::TYPE_MOVE;
	control.Constraint = ResizableDialog::CONSTRAINT_NONE;
	controlList.push_back(control);
}

wil::unique_hicon ManageBookmarksDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_pexpp->GetIconResourceLoader()->LoadIconFromPNGAndScale(
		Icon::Bookmarks, iconWidth, iconHeight);
}

void ManageBookmarksDialog::SetupToolbar()
{
	m_hToolbar = CreateToolbar(m_hDlg,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_DOUBLEBUFFER
			| TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	SendMessage(m_hToolbar, TB_BUTTONSTRUCTSIZE, static_cast<WPARAM>(sizeof(TBBUTTON)), 0);

	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hToolbar);
	int iconWidth = m_dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = m_dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	SendMessage(m_hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));

	std::tie(m_imageListToolbar, m_imageListToolbarMappings) =
		ResourceHelper::CreateIconImageList(m_pexpp->GetIconResourceLoader(), iconWidth, iconHeight,
			{ Icon::Back, Icon::Forward, Icon::Copy, Icon::Views });
	SendMessage(m_hToolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(m_imageListToolbar.get()));

	TBBUTTON tbb;

	std::wstring text = ResourceHelper::LoadString(GetInstance(), IDS_MANAGE_BOOKMARKS_TOOLBAR_BACK);

	tbb.iBitmap = m_imageListToolbarMappings.at(Icon::Back);
	tbb.idCommand = TOOLBAR_ID_BACK;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
	tbb.dwData = 0;
	tbb.iString = reinterpret_cast<INT_PTR>(text.c_str());
	SendMessage(m_hToolbar, TB_INSERTBUTTON, 0, reinterpret_cast<LPARAM>(&tbb));

	text = ResourceHelper::LoadString(GetInstance(), IDS_MANAGE_BOOKMARKS_TOOLBAR_FORWARD);

	tbb.iBitmap = m_imageListToolbarMappings.at(Icon::Forward);
	tbb.idCommand = TOOLBAR_ID_FORWARD;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
	tbb.dwData = 0;
	tbb.iString = reinterpret_cast<INT_PTR>(text.c_str());
	SendMessage(m_hToolbar, TB_INSERTBUTTON, 1, reinterpret_cast<LPARAM>(&tbb));

	text = ResourceHelper::LoadString(GetInstance(), IDS_MANAGE_BOOKMARKS_TOOLBAR_ORGANIZE);

	tbb.iBitmap = m_imageListToolbarMappings.at(Icon::Copy);
	tbb.idCommand = TOOLBAR_ID_ORGANIZE;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_DROPDOWN;
	tbb.dwData = 0;
	tbb.iString = reinterpret_cast<INT_PTR>(text.c_str());
	SendMessage(m_hToolbar, TB_INSERTBUTTON, 2, reinterpret_cast<LPARAM>(&tbb));

	text = ResourceHelper::LoadString(GetInstance(), IDS_MANAGE_BOOKMARKS_TOOLBAR_VIEWS);

	tbb.iBitmap = m_imageListToolbarMappings.at(Icon::Views);
	tbb.idCommand = TOOLBAR_ID_VIEWS;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_DROPDOWN;
	tbb.dwData = 0;
	tbb.iString = reinterpret_cast<INT_PTR>(text.c_str());
	SendMessage(m_hToolbar, TB_INSERTBUTTON, 3, reinterpret_cast<LPARAM>(&tbb));

	RECT rcTreeView;
	GetWindowRect(GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW), &rcTreeView);
	MapWindowPoints(HWND_DESKTOP, m_hDlg, reinterpret_cast<LPPOINT>(&rcTreeView), 2);

	RECT rcListView;
	GetWindowRect(GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW), &rcListView);
	MapWindowPoints(HWND_DESKTOP, m_hDlg, reinterpret_cast<LPPOINT>(&rcListView), 2);

	auto dwButtonSize = static_cast<DWORD>(SendMessage(m_hToolbar, TB_GETBUTTONSIZE, 0, 0));
	SetWindowPos(m_hToolbar, nullptr, rcTreeView.left, (rcTreeView.top - HIWORD(dwButtonSize)) / 2,
		rcListView.right - rcTreeView.left, HIWORD(dwButtonSize), 0);
}

void ManageBookmarksDialog::SetupTreeView()
{
	HWND hTreeView = GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW);

	m_bookmarkTreeView = new BookmarkTreeView(
		hTreeView, GetInstance(), m_pexpp, m_bookmarkTree, m_persistentSettings->m_setExpansion);

	m_connections.push_back(m_bookmarkTreeView->selectionChangedSignal.AddObserver(std::bind(
		&ManageBookmarksDialog::OnTreeViewSelectionChanged, this, std::placeholders::_1)));
}

void ManageBookmarksDialog::SetupListView()
{
	HWND hListView = GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW);

	m_bookmarkListView = new BookmarkListView(
		hListView, GetInstance(), m_bookmarkTree, m_pexpp, m_persistentSettings->m_listViewColumns);

	m_connections.push_back(m_bookmarkListView->AddNavigationCompletedObserver(
		std::bind(&ManageBookmarksDialog::OnListViewNavigation, this, std::placeholders::_1,
			std::placeholders::_2)));
}

INT_PTR ManageBookmarksDialog::OnAppCommand(HWND hwnd, UINT uCmd, UINT uDevice, DWORD dwKeys)
{
	UNREFERENCED_PARAMETER(dwKeys);
	UNREFERENCED_PARAMETER(uDevice);
	UNREFERENCED_PARAMETER(hwnd);

	switch (uCmd)
	{
	case APPCOMMAND_BROWSER_BACKWARD:
		m_navigationController->GoBack();
		break;

	case APPCOMMAND_BROWSER_FORWARD:
		m_navigationController->GoForward();
		break;
	}

	return 0;
}

INT_PTR ManageBookmarksDialog::OnCommand(WPARAM wParam, LPARAM lParam)
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
		m_navigationController->GoBack();
		break;

	case TOOLBAR_ID_FORWARD:
		m_navigationController->GoForward();
		break;

	case TOOLBAR_ID_ORGANIZE:
		ShowOrganizeMenu();
		break;

	case TOOLBAR_ID_VIEWS:
		ShowViewMenu();
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
	switch (pnmhdr->code)
	{
	case TBN_DROPDOWN:
		OnTbnDropDown(reinterpret_cast<NMTOOLBAR *>(pnmhdr));
		break;
	}

	return 0;
}

void ManageBookmarksDialog::OnTbnDropDown(NMTOOLBAR *nmtb)
{
	switch (nmtb->iItem)
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
	wil::unique_hmenu parentMenu(
		LoadMenu(GetInstance(), MAKEINTRESOURCE(IDR_MANAGEBOOKMARKS_VIEW_MENU)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	auto columnsMenu = m_bookmarkListView->BuildColumnsMenu();

	if (!columnsMenu)
	{
		return;
	}

	MenuHelper::EnableItem(
		columnsMenu.get(), static_cast<UINT>(BookmarkListView::ColumnType::Name), FALSE);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU;
	mii.hSubMenu = columnsMenu.get();
	SetMenuItemInfo(menu, IDM_POPUP_SHOW_COLUMNS, FALSE, &mii);

	// As the columns menu is now part of the parent views menu, it will be destroyed when the
	// parent menu is destroyed.
	columnsMenu.release();

	SetViewMenuItemStates(menu);

	RECT rc;
	BOOL res = static_cast<BOOL>(
		SendMessage(m_hToolbar, TB_GETRECT, TOOLBAR_ID_VIEWS, reinterpret_cast<LPARAM>(&rc)));

	if (!res)
	{
		return;
	}

	POINT pt;
	pt.x = rc.left;
	pt.y = rc.bottom;
	res = ClientToScreen(m_hToolbar, &pt);

	if (!res)
	{
		return;
	}

	SendMessage(m_hToolbar, TB_PRESSBUTTON, TOOLBAR_ID_VIEWS, MAKEWORD(TRUE, 0));

	int menuItemId =
		TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hDlg, nullptr);

	if (menuItemId != 0)
	{
		OnViewMenuItemSelected(menuItemId);
	}

	SendMessage(m_hToolbar, TB_PRESSBUTTON, TOOLBAR_ID_VIEWS, MAKEWORD(FALSE, 0));
}

void ManageBookmarksDialog::SetViewMenuItemStates(HMENU menu)
{
	UINT itemToCheck;

	switch (m_bookmarkListView->GetSortMode())
	{
	case BookmarkHelper::SortMode::Default:
		itemToCheck = IDM_MB_VIEW_SORT_BY_DEFAULT;
		break;

	case BookmarkHelper::SortMode::Name:
		itemToCheck = IDM_MB_VIEW_SORTBYNAME;
		break;

	case BookmarkHelper::SortMode::Location:
		itemToCheck = IDM_MB_VIEW_SORTBYLOCATION;
		break;

	case BookmarkHelper::SortMode::DateCreated:
		itemToCheck = IDM_MB_VIEW_SORTBYADDED;
		break;

	case BookmarkHelper::SortMode::DateModified:
		itemToCheck = IDM_MB_VIEW_SORTBYLASTMODIFIED;
		break;

	default:
		itemToCheck = IDM_MB_VIEW_SORT_BY_DEFAULT;
		break;
	}

	CheckMenuRadioItem(
		menu, IDM_MB_VIEW_SORTBYNAME, IDM_MB_VIEW_SORT_BY_DEFAULT, itemToCheck, MF_BYCOMMAND);

	if (m_bookmarkListView->GetSortAscending())
	{
		itemToCheck = IDM_MB_VIEW_SORTASCENDING;
	}
	else
	{
		itemToCheck = IDM_MB_VIEW_SORTDESCENDING;
	}

	CheckMenuRadioItem(
		menu, IDM_MB_VIEW_SORTASCENDING, IDM_MB_VIEW_SORTDESCENDING, itemToCheck, MF_BYCOMMAND);
}

void ManageBookmarksDialog::OnViewMenuItemSelected(int menuItemId)
{
	switch (menuItemId)
	{
	case static_cast<int>(BookmarkListView::ColumnType::Name):
		m_bookmarkListView->ToggleColumn(BookmarkListView::ColumnType::Name);
		break;

	case static_cast<int>(BookmarkListView::ColumnType::Location):
		m_bookmarkListView->ToggleColumn(BookmarkListView::ColumnType::Location);
		break;

	case static_cast<int>(BookmarkListView::ColumnType::DateCreated):
		m_bookmarkListView->ToggleColumn(BookmarkListView::ColumnType::DateCreated);
		break;

	case static_cast<int>(BookmarkListView::ColumnType::DateModified):
		m_bookmarkListView->ToggleColumn(BookmarkListView::ColumnType::DateModified);
		break;

	case IDM_MB_VIEW_SORT_BY_DEFAULT:
		m_bookmarkListView->SetSortMode(BookmarkHelper::SortMode::Default);
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

	default:
		assert(false);
		break;
	}
}

void ManageBookmarksDialog::ShowOrganizeMenu()
{
	wil::unique_hmenu parentMenu(
		LoadMenu(GetInstance(), MAKEINTRESOURCE(IDR_MANAGEBOOKMARKS_ORGANIZE_MENU)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	SetOrganizeMenuItemStates(menu);

	RECT rc;
	BOOL res = static_cast<BOOL>(
		SendMessage(m_hToolbar, TB_GETRECT, TOOLBAR_ID_ORGANIZE, reinterpret_cast<LPARAM>(&rc)));

	if (!res)
	{
		return;
	}

	POINT pt;
	pt.x = rc.left;
	pt.y = rc.bottom;
	res = ClientToScreen(m_hToolbar, &pt);

	if (!res)
	{
		return;
	}

	SendMessage(m_hToolbar, TB_PRESSBUTTON, TOOLBAR_ID_ORGANIZE, MAKEWORD(TRUE, 0));

	int menuItemId =
		TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hDlg, nullptr);

	if (menuItemId != 0)
	{
		OnOrganizeMenuItemSelected(menuItemId);
	}

	SendMessage(m_hToolbar, TB_PRESSBUTTON, TOOLBAR_ID_ORGANIZE, MAKEWORD(FALSE, 0));
}

void ManageBookmarksDialog::SetOrganizeMenuItemStates(HMENU menu)
{
	HWND focus = GetFocus();
	HWND listView = GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW);
	HWND treeView = GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW);

	MenuHelper::EnableItem(
		menu, IDM_MB_ORGANIZE_NEWBOOKMARK, focus == listView || focus == treeView);
	MenuHelper::EnableItem(
		menu, IDM_MB_ORGANIZE_NEWFOLDER, focus == listView || focus == treeView);
	MenuHelper::EnableItem(menu, IDM_MB_ORGANIZE_SELECTALL, focus == listView);

	bool canDelete = false;

	if (focus == listView)
	{
		canDelete = m_bookmarkListView->CanDelete();
	}
	else if (focus == treeView)
	{
		canDelete = m_bookmarkTreeView->CanDelete();
	}

	MenuHelper::EnableItem(menu, IDM_MB_ORGANIZE_DELETE, canDelete);
}

void ManageBookmarksDialog::OnOrganizeMenuItemSelected(int menuItemId)
{
	switch (menuItemId)
	{
	case IDM_MB_ORGANIZE_NEWBOOKMARK:
		OnNewBookmark();
		break;

	case IDM_MB_ORGANIZE_NEWFOLDER:
		OnNewFolder();
		break;

	case IDM_MB_ORGANIZE_CUT:
		OnCopy(true);
		break;

	case IDM_MB_ORGANIZE_COPY:
		OnCopy(false);
		break;

	case IDM_MB_ORGANIZE_PASTE:
		OnPaste();
		break;

	case IDM_MB_ORGANIZE_DELETE:
		OnDelete();
		break;

	case IDM_MB_ORGANIZE_SELECTALL:
		OnSelectAll();
		break;

	default:
		assert(false);
		break;
	}
}

void ManageBookmarksDialog::OnNewBookmark()
{
	HWND focus = GetFocus();
	HWND listView = GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW);
	HWND treeView = GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW);

	if (focus != listView && focus != treeView)
	{
		return;
	}

	std::optional<size_t> targetIndex;

	if (focus == listView)
	{
		targetIndex = m_bookmarkListView->GetLastSelectedItemIndex() + 1;
	}

	auto bookmark = BookmarkHelper::AddBookmarkItem(m_bookmarkTree, BookmarkItem::Type::Bookmark,
		m_currentBookmarkFolder, targetIndex, focus, m_pexpp);

	if (!bookmark || focus != listView || bookmark->GetParent() != m_currentBookmarkFolder)
	{
		return;
	}

	m_bookmarkListView->SelectItem(bookmark);
}

void ManageBookmarksDialog::OnNewFolder()
{
	HWND focus = GetFocus();

	if (focus == GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW))
	{
		m_bookmarkListView->CreateNewFolder();
	}
	else if (focus == GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW))
	{
		m_bookmarkTreeView->CreateNewFolder();
	}
}

void ManageBookmarksDialog::OnCopy(bool cut)
{
	HWND focus = GetFocus();
	RawBookmarkItems selectedItems;

	if (focus == GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW))
	{
		selectedItems = m_bookmarkListView->GetSelectedBookmarkItems();
	}
	else if (focus == GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW))
	{
		// The treeview selection and current folder are always the same, as
		// they're kept in sync with each other.
		selectedItems.push_back(m_currentBookmarkFolder);
	}

	if (selectedItems.empty())
	{
		return;
	}

	BookmarkHelper::CopyBookmarkItems(m_bookmarkTree, selectedItems, cut);
}

void ManageBookmarksDialog::OnPaste()
{
	HWND focus = GetFocus();
	size_t targetIndex;

	if (focus == GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW))
	{
		targetIndex = m_bookmarkListView->GetLastSelectedItemIndex() + 1;
	}
	else
	{
		targetIndex = m_currentBookmarkFolder->GetChildren().size();
	}

	BookmarkHelper::PasteBookmarkItems(m_bookmarkTree, m_currentBookmarkFolder, targetIndex);
}

void ManageBookmarksDialog::OnDelete()
{
	HWND focus = GetFocus();

	if (focus == GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW))
	{
		m_bookmarkListView->DeleteSelection();
	}
	else if (focus == GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW))
	{
		m_bookmarkTreeView->DeleteSelection();
	}
}

void ManageBookmarksDialog::OnSelectAll()
{
	HWND focus = GetFocus();
	HWND listView = GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW);

	if (focus == listView)
	{
		ListViewHelper::SelectAllItems(listView, TRUE);
	}
}

void ManageBookmarksDialog::OnTreeViewSelectionChanged(BookmarkItem *bookmarkFolder)
{
	if (bookmarkFolder == m_currentBookmarkFolder)
	{
		return;
	}

	m_navigationController->BrowseFolder(bookmarkFolder);
}

void ManageBookmarksDialog::OnListViewNavigation(BookmarkItem *bookmarkFolder, bool addHistoryEntry)
{
	UNREFERENCED_PARAMETER(addHistoryEntry);

	m_currentBookmarkFolder = bookmarkFolder;
	m_bookmarkTreeView->SelectFolder(bookmarkFolder->GetGUID());

	UpdateToolbarState();
}

void ManageBookmarksDialog::UpdateToolbarState()
{
	SendMessage(m_hToolbar, TB_ENABLEBUTTON, TOOLBAR_ID_BACK, m_navigationController->CanGoBack());
	SendMessage(
		m_hToolbar, TB_ENABLEBUTTON, TOOLBAR_ID_FORWARD, m_navigationController->CanGoForward());
}

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

INT_PTR ManageBookmarksDialog::OnDestroy()
{
	m_persistentSettings->m_listViewColumns = m_bookmarkListView->GetColumns();
	return 0;
}

INT_PTR ManageBookmarksDialog::OnNcDestroy()
{
	delete this;

	return 0;
}

void ManageBookmarksDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);

	m_persistentSettings->m_bStateSaved = TRUE;
}

ManageBookmarksDialogPersistentSettings::ManageBookmarksDialogPersistentSettings() :
	m_bInitialized(false),
	DialogSettings(SETTINGS_KEY)
{
	SetupDefaultColumns();
}

ManageBookmarksDialogPersistentSettings &ManageBookmarksDialogPersistentSettings::GetInstance()
{
	static ManageBookmarksDialogPersistentSettings mbdps;
	return mbdps;
}

void ManageBookmarksDialogPersistentSettings::SetupDefaultColumns()
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