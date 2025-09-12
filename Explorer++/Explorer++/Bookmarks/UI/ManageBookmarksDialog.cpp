// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/ManageBookmarksDialog.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkNavigationController.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarkTreePresenter.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "NoOpMenuHelpTextHost.h"
#include "OrganizeBookmarksContextMenu.h"
#include "PopupMenuView.h"
#include "ResourceHelper.h"
#include "ResourceLoader.h"
#include "TreeView.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowSubclass.h"
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>
#include <glog/logging.h>

const TCHAR ManageBookmarksDialogPersistentSettings::SETTINGS_KEY[] = _T("ManageBookmarks");

ManageBookmarksDialog *ManageBookmarksDialog::Create(const ResourceLoader *resourceLoader,
	HINSTANCE resourceInstance, HWND hParent, BrowserWindow *browserWindow, const Config *config,
	const AcceleratorManager *acceleratorManager, IconFetcher *iconFetcher,
	BookmarkTree *bookmarkTree, ClipboardStore *clipboardStore)
{
	return new ManageBookmarksDialog(resourceLoader, resourceInstance, hParent, browserWindow,
		config, acceleratorManager, iconFetcher, bookmarkTree, clipboardStore);
}

ManageBookmarksDialog::ManageBookmarksDialog(const ResourceLoader *resourceLoader,
	HINSTANCE resourceInstance, HWND hParent, BrowserWindow *browserWindow, const Config *config,
	const AcceleratorManager *acceleratorManager, IconFetcher *iconFetcher,
	BookmarkTree *bookmarkTree, ClipboardStore *clipboardStore) :
	BaseDialog(resourceLoader, IDD_MANAGE_BOOKMARKS, hParent, DialogSizingType::Both),
	m_resourceInstance(resourceInstance),
	m_browserWindow(browserWindow),
	m_config(config),
	m_acceleratorManager(acceleratorManager),
	m_iconFetcher(iconFetcher),
	m_bookmarkTree(bookmarkTree),
	m_clipboardStore(clipboardStore)
{
	m_persistentSettings = &ManageBookmarksDialogPersistentSettings::GetInstance();

	if (!m_persistentSettings->m_initialized)
	{
		m_persistentSettings->m_initialized = true;
	}
}

INT_PTR ManageBookmarksDialog::OnInitDialog()
{
	SetupToolbar();
	SetupTreeView();
	SetupListView();

	m_navigationController =
		std::make_unique<BookmarkNavigationController>(m_bookmarkTree, m_bookmarkListView.get());
	m_navigationController->Navigate(m_bookmarkTree->GetBookmarksToolbarFolder());

	SetFocus(GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW));

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

void ManageBookmarksDialog::AddDynamicControls()
{
	CreateToolbar();
}

std::vector<ResizableDialogControl> ManageBookmarksDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW), MovingType::None,
		SizingType::Vertical);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW), MovingType::None,
		SizingType::Both);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Both, SizingType::None);
	return controls;
}

wil::unique_hicon ManageBookmarksDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_resourceLoader->LoadIconFromPNGAndScale(Icon::Bookmarks, iconWidth, iconHeight);
}

void ManageBookmarksDialog::CreateToolbar()
{
	m_toolbarParent = CreateWindow(WC_STATIC, L"", WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0,
		0, m_hDlg, nullptr, GetModuleHandle(nullptr), nullptr);

	m_hToolbar = ::CreateToolbar(m_toolbarParent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_DOUBLEBUFFER
			| TBSTYLE_EX_HIDECLIPPEDBUTTONS);
}

void ManageBookmarksDialog::SetupToolbar()
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_toolbarParent,
		std::bind_front(&ManageBookmarksDialog::ToolbarParentWndProc, this)));

	SendMessage(m_hToolbar, TB_BUTTONSTRUCTSIZE, static_cast<WPARAM>(sizeof(TBBUTTON)), 0);

	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(m_hToolbar);
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	SendMessage(m_hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));

	std::tie(m_imageListToolbar, m_imageListToolbarMappings) =
		ResourceHelper::CreateIconImageList(m_resourceLoader, iconWidth, iconHeight,
			{ Icon::Back, Icon::Forward, Icon::Copy, Icon::Views });
	SendMessage(m_hToolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(m_imageListToolbar.get()));

	TBBUTTON tbb;

	std::wstring text = m_resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_TOOLBAR_BACK);

	tbb.iBitmap = m_imageListToolbarMappings.at(Icon::Back);
	tbb.idCommand = TOOLBAR_ID_BACK;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_AUTOSIZE;
	tbb.dwData = 0;
	tbb.iString = reinterpret_cast<INT_PTR>(text.c_str());
	SendMessage(m_hToolbar, TB_INSERTBUTTON, 0, reinterpret_cast<LPARAM>(&tbb));

	text = m_resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_TOOLBAR_FORWARD);

	tbb.iBitmap = m_imageListToolbarMappings.at(Icon::Forward);
	tbb.idCommand = TOOLBAR_ID_FORWARD;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_AUTOSIZE;
	tbb.dwData = 0;
	tbb.iString = reinterpret_cast<INT_PTR>(text.c_str());
	SendMessage(m_hToolbar, TB_INSERTBUTTON, 1, reinterpret_cast<LPARAM>(&tbb));

	text = m_resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_TOOLBAR_ORGANIZE);

	tbb.iBitmap = m_imageListToolbarMappings.at(Icon::Copy);
	tbb.idCommand = TOOLBAR_ID_ORGANIZE;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_DROPDOWN;
	tbb.dwData = 0;
	tbb.iString = reinterpret_cast<INT_PTR>(text.c_str());
	SendMessage(m_hToolbar, TB_INSERTBUTTON, 2, reinterpret_cast<LPARAM>(&tbb));

	text = m_resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_TOOLBAR_VIEWS);

	tbb.iBitmap = m_imageListToolbarMappings.at(Icon::Views);
	tbb.idCommand = TOOLBAR_ID_VIEWS;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_DROPDOWN;
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

	SetWindowPos(m_toolbarParent, nullptr, rcTreeView.left,
		(rcTreeView.top - HIWORD(dwButtonSize)) / 2, rcListView.right - rcTreeView.left,
		HIWORD(dwButtonSize), 0);
	SetWindowPos(m_hToolbar, nullptr, 0, 0, rcListView.right - rcTreeView.left,
		HIWORD(dwButtonSize), 0);
}

void ManageBookmarksDialog::SetupTreeView()
{
	m_bookmarkTreePresenter = std::make_unique<BookmarkTreePresenter>(
		std::make_unique<TreeView>(GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW)),
		m_acceleratorManager, m_resourceLoader, m_bookmarkTree, m_clipboardStore,
		m_persistentSettings->m_expandedBookmarkIds);

	m_connections.push_back(m_bookmarkTreePresenter->selectionChangedSignal.AddObserver(
		std::bind_front(&ManageBookmarksDialog::OnTreeViewSelectionChanged, this)));
}

void ManageBookmarksDialog::SetupListView()
{
	m_bookmarkListView = std::make_unique<BookmarkListView>(
		GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_LISTVIEW), m_resourceInstance, m_bookmarkTree,
		m_browserWindow, m_config, m_acceleratorManager, m_resourceLoader, m_iconFetcher,
		m_persistentSettings->m_listViewColumns, m_clipboardStore);

	m_connections.push_back(m_bookmarkListView->AddNavigationCompletedObserver(
		std::bind_front(&ManageBookmarksDialog::OnListViewNavigation, this)));
}

LRESULT CALLBACK ManageBookmarksDialog::ToolbarParentWndProc(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)
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
			}
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hToolbar)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case TBN_DROPDOWN:
				OnTbnDropDown(reinterpret_cast<NMTOOLBAR *>(lParam));
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
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
	case IDOK:
		OnOk();
		break;

	case IDCANCEL:
		OnCancel();
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
		LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_MANAGEBOOKMARKS_VIEW_MENU)));

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

	MenuHelper::EnableItem(columnsMenu.get(), static_cast<UINT>(BookmarkHelper::ColumnType::Name),
		FALSE);

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

	SendMessage(m_hToolbar, TB_PRESSBUTTON, TOOLBAR_ID_VIEWS, MAKELPARAM(TRUE, 0));

	int menuItemId =
		TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hDlg, nullptr);

	if (menuItemId != 0)
	{
		OnViewMenuItemSelected(menuItemId);
	}

	SendMessage(m_hToolbar, TB_PRESSBUTTON, TOOLBAR_ID_VIEWS, MAKELPARAM(FALSE, 0));
}

void ManageBookmarksDialog::SetViewMenuItemStates(HMENU menu)
{
	UINT itemToCheck;

	switch (m_bookmarkListView->GetSortColumn())
	{
	case BookmarkHelper::ColumnType::Default:
		itemToCheck = IDM_MB_VIEW_SORT_BY_DEFAULT;
		break;

	case BookmarkHelper::ColumnType::Name:
		itemToCheck = IDM_MB_VIEW_SORTBYNAME;
		break;

	case BookmarkHelper::ColumnType::Location:
		itemToCheck = IDM_MB_VIEW_SORTBYLOCATION;
		break;

	case BookmarkHelper::ColumnType::DateCreated:
		itemToCheck = IDM_MB_VIEW_SORTBYADDED;
		break;

	case BookmarkHelper::ColumnType::DateModified:
		itemToCheck = IDM_MB_VIEW_SORTBYLASTMODIFIED;
		break;

	default:
		itemToCheck = IDM_MB_VIEW_SORT_BY_DEFAULT;
		break;
	}

	CheckMenuRadioItem(menu, IDM_MB_VIEW_SORTBYNAME, IDM_MB_VIEW_SORT_BY_DEFAULT, itemToCheck,
		MF_BYCOMMAND);

	if (m_bookmarkListView->GetSortColumn() == BookmarkHelper::ColumnType::Default)
	{
		MenuHelper::EnableItem(menu, IDM_MB_VIEW_SORTASCENDING, FALSE);
		MenuHelper::EnableItem(menu, IDM_MB_VIEW_SORTDESCENDING, FALSE);
	}
	else
	{
		if (m_bookmarkListView->GetSortAscending())
		{
			itemToCheck = IDM_MB_VIEW_SORTASCENDING;
		}
		else
		{
			itemToCheck = IDM_MB_VIEW_SORTDESCENDING;
		}

		CheckMenuRadioItem(menu, IDM_MB_VIEW_SORTASCENDING, IDM_MB_VIEW_SORTDESCENDING, itemToCheck,
			MF_BYCOMMAND);
	}
}

void ManageBookmarksDialog::OnViewMenuItemSelected(int menuItemId)
{
	switch (menuItemId)
	{
	case static_cast<int>(BookmarkHelper::ColumnType::Name):
		m_bookmarkListView->ToggleColumn(BookmarkHelper::ColumnType::Name);
		break;

	case static_cast<int>(BookmarkHelper::ColumnType::Location):
		m_bookmarkListView->ToggleColumn(BookmarkHelper::ColumnType::Location);
		break;

	case static_cast<int>(BookmarkHelper::ColumnType::DateCreated):
		m_bookmarkListView->ToggleColumn(BookmarkHelper::ColumnType::DateCreated);
		break;

	case static_cast<int>(BookmarkHelper::ColumnType::DateModified):
		m_bookmarkListView->ToggleColumn(BookmarkHelper::ColumnType::DateModified);
		break;

	case IDM_MB_VIEW_SORT_BY_DEFAULT:
		m_bookmarkListView->SetSortColumn(BookmarkHelper::ColumnType::Default);
		break;

	case IDM_MB_VIEW_SORTBYNAME:
		m_bookmarkListView->SetSortColumn(BookmarkHelper::ColumnType::Name);
		break;

	case IDM_MB_VIEW_SORTBYLOCATION:
		m_bookmarkListView->SetSortColumn(BookmarkHelper::ColumnType::Location);
		break;

	case IDM_MB_VIEW_SORTBYADDED:
		m_bookmarkListView->SetSortColumn(BookmarkHelper::ColumnType::DateCreated);
		break;

	case IDM_MB_VIEW_SORTBYLASTMODIFIED:
		m_bookmarkListView->SetSortColumn(BookmarkHelper::ColumnType::DateModified);
		break;

	case IDM_MB_VIEW_SORTASCENDING:
		m_bookmarkListView->SetSortAscending(true);
		break;

	case IDM_MB_VIEW_SORTDESCENDING:
		m_bookmarkListView->SetSortAscending(false);
		break;

	default:
		DCHECK(false);
		break;
	}
}

void ManageBookmarksDialog::ShowOrganizeMenu()
{
	HWND focus = GetFocus();
	HWND treeView = GetDlgItem(m_hDlg, IDC_MANAGEBOOKMARKS_TREEVIEW);
	OrganizeBookmarksContextMenuDelegate *delegate = nullptr;

	if (focus == treeView || IsChild(treeView, focus))
	{
		delegate = m_bookmarkTreePresenter.get();
	}
	else
	{
		delegate = m_bookmarkListView.get();
	}

	RECT rc;
	auto res =
		SendMessage(m_hToolbar, TB_GETRECT, TOOLBAR_ID_ORGANIZE, reinterpret_cast<LPARAM>(&rc));
	CHECK(res);

	POINT pt;
	pt.x = rc.left;
	pt.y = rc.bottom;
	res = ClientToScreen(m_hToolbar, &pt);
	CHECK(res);

	res = SendMessage(m_hToolbar, TB_PRESSBUTTON, TOOLBAR_ID_ORGANIZE, MAKELPARAM(TRUE, 0));
	DCHECK(res);

	PopupMenuView popupMenu(NoOpMenuHelpTextHost::GetInstance());
	OrganizeBookmarksContextMenu menu(&popupMenu, m_acceleratorManager, m_hDlg, m_bookmarkTree,
		m_currentBookmarkFolder, delegate, m_clipboardStore, m_resourceLoader);
	popupMenu.Show(m_hDlg, pt);

	res = SendMessage(m_hToolbar, TB_PRESSBUTTON, TOOLBAR_ID_ORGANIZE, MAKELPARAM(FALSE, 0));
	DCHECK(res);
}

void ManageBookmarksDialog::OnTreeViewSelectionChanged(BookmarkItem *bookmarkFolder)
{
	if (bookmarkFolder == m_currentBookmarkFolder)
	{
		return;
	}

	m_navigationController->Navigate(bookmarkFolder);
}

void ManageBookmarksDialog::OnListViewNavigation(BookmarkItem *bookmarkFolder,
	const BookmarkHistoryEntry *entry)
{
	UNREFERENCED_PARAMETER(entry);

	m_currentBookmarkFolder = bookmarkFolder;
	m_bookmarkTreePresenter->SelectItem(bookmarkFolder);

	UpdateToolbarState();
}

void ManageBookmarksDialog::UpdateToolbarState()
{
	SendMessage(m_hToolbar, TB_ENABLEBUTTON, TOOLBAR_ID_BACK, m_navigationController->CanGoBack());
	SendMessage(m_hToolbar, TB_ENABLEBUTTON, TOOLBAR_ID_FORWARD,
		m_navigationController->CanGoForward());
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

void ManageBookmarksDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);

	m_persistentSettings->m_expandedBookmarkIds =
		boost::copy_range<std::unordered_set<std::wstring>>(
			m_bookmarkTreePresenter->GetExpandedBookmarks()
			| boost::adaptors::transformed(std::mem_fn(&BookmarkItem::GetGUID)));

	m_persistentSettings->m_bStateSaved = TRUE;
}

ManageBookmarksDialogPersistentSettings::ManageBookmarksDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY),
	m_initialized(false)
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

	column.columnType = BookmarkHelper::ColumnType::Name;
	column.width = DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	column.active = true;
	m_listViewColumns.push_back(column);

	column.columnType = BookmarkHelper::ColumnType::Location;
	column.width = DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	column.active = true;
	m_listViewColumns.push_back(column);

	column.columnType = BookmarkHelper::ColumnType::DateCreated;
	column.width = DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	column.active = false;
	m_listViewColumns.push_back(column);

	column.columnType = BookmarkHelper::ColumnType::DateModified;
	column.width = DEFAULT_MANAGE_BOOKMARKS_COLUMN_WIDTH;
	column.active = false;
	m_listViewColumns.push_back(column);
}
