// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "StartupOptionsPage.h"
#include "Config.h"
#include "DarkModeManager.h"
#include "MainResource.h"
#include "ThemeManager.h"
#include "../Helper/Controls.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ResizableDialogHelper.h"
#include "../Helper/ScopedRedrawDisabler.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"

StartupOptionsPage::StartupOptionsPage(HWND parent, HINSTANCE resourceInstance, Config *config,
	CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback, HWND tooltipWindow,
	DarkModeManager *darkModeManager, ThemeManager *themeManager) :
	OptionsPage(IDD_OPTIONS_STARTUP, IDS_OPTIONS_STARTUP_TITLE, parent, resourceInstance, config,
		coreInterface, settingChangedCallback, tooltipWindow),
	m_darkModeManager(darkModeManager),
	m_themeManager(themeManager),
	m_listViewDisabledBackgroundBrush(
		CreateSolidBrush(LISTVIEW_DARK_MODE_DISABLED_BACKGROUND_COLOR))
{
	CHECK(m_listViewDisabledBackgroundBrush);
}

std::unique_ptr<ResizableDialogHelper> StartupOptionsPage::InitializeResizeDialogHelper()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STARTUP_DEFAULT_FOLDER), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STARTUP_PREVIOUS_TABS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST),
		MovingType::None, SizingType::Both);
	return std::make_unique<ResizableDialogHelper>(GetDialog(), controls);
}

void StartupOptionsPage::InitializeControls()
{
	CheckDlgButton(GetDialog(), MapStartupModeToControlId(m_config->startupMode), BST_CHECKED);

	AddTooltipForControl(m_tooltipWindow, GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS),
		m_resourceInstance, IDS_OPTIONS_CUSTOM_FOLDERS_TOOLTIP, TooltipType::Control);

	SetUpListView();

	AddStartupFolders();

	// This allows the user to easily add an item at the end of the list by double-clicking the last
	// item and editing it. When the settings are saved, empty entries like this will be ignored.
	AddStartupFolder(static_cast<int>(m_config->startupFolders.size()), L"");

	m_connections.push_back(m_darkModeManager->darkModeStatusChanged.AddObserver(
		std::bind(&StartupOptionsPage::OnDarkModeStatusChanged, this)));
}

void StartupOptionsPage::SetUpListView()
{
	HWND listView = GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST);
	ListViewHelper::AddRemoveExtendedStyles(listView,
		LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, true);

	// At least one column needs to be present, though its properties doesn't matter, since it won't
	// be shown.
	LV_COLUMN lvColumn = {};
	int index = ListView_InsertColumn(listView, 0, &lvColumn);
	DCHECK_EQ(index, 0);

	RECT rect;
	auto res = GetClientRect(listView, &rect);
	DCHECK(res);
	ListView_SetColumnWidth(listView, 0, GetRectWidth(&rect));

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(listView,
		std::bind_front(&StartupOptionsPage::ListViewWndProc, this)));

	EnableWindow(listView, m_config->startupMode == +StartupMode::CustomFolders);
}

int StartupOptionsPage::MapStartupModeToControlId(StartupMode startupMode)
{
	switch (startupMode)
	{
	case StartupMode::DefaultFolder:
		return IDC_STARTUP_DEFAULT_FOLDER;

	case StartupMode::CustomFolders:
		return IDC_STARTUP_CUSTOM_FOLDERS;

	case StartupMode::PreviousTabs:
	default:
		return IDC_STARTUP_PREVIOUS_TABS;
	}
}

void StartupOptionsPage::AddStartupFolders()
{
	int index = 0;

	for (const auto &startupFolder : m_config->startupFolders)
	{
		AddStartupFolder(index++, startupFolder);
	}
}

void StartupOptionsPage::AddStartupFolder(int index, const std::wstring &startupFolder)
{
	LVITEM lvItem = {};
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = index;
	lvItem.iSubItem = 0;
	lvItem.pszText = const_cast<wchar_t *>(startupFolder.c_str());
	int finalIndex =
		ListView_InsertItem(GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST), &lvItem);
	DCHECK_EQ(finalIndex, index);
}

void StartupOptionsPage::OnDarkModeStatusChanged()
{
	UpdateListViewTransparentStyle();

	// The above function will add/remove the LVS_EX_TRANSPARENTBKGND style. As that's done after
	// changing the dark mode theme, it's necessary to re-apply the window theme, otherwise the
	// colors used to draw the control may not be updated properly. For example, if dark mode is
	// disabled and the LVS_EX_TRANSPARENTBKGND style is then removed, the control will continue to
	// use the original background color (sourced from the parent) until the theme is re-applied.
	m_themeManager->ApplyThemeToWindowAndChildren(
		GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST));
}

void StartupOptionsPage::UpdateListViewTransparentStyle()
{
	HWND listView = GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST);
	bool addStyle = m_darkModeManager->IsDarkModeEnabled() && !IsWindowEnabled(listView);

	// As noted in
	// https://www.geoffchappell.com/studies/windows/shell/comctl32/controls/listview/background.htm,
	// there is no documented way to set the background color for a listview control when it's
	// disabled. While ListView_SetBkColor() works when the control is enabled, it has no effect
	// on a disabled control.
	//
	// However, the page above also notes that LVS_EX_TRANSPARENTBKGND can be used to set the
	// background color when the control is disabled. That's because the control will then
	// source its background from the parent and there's full control over the background used by
	// the parent.
	//
	// Therefore, in addition to setting the LVS_EX_TRANSPARENTBKGND style when necessary, the
	// WM_ERASEBKGND message also needs to be handled in the parent dialog, to draw the correct
	// color behind the listview.
	//
	// Finally, the page above also mentions that since the behavior of ListView_SetBkColor()
	// and LVS_EX_TRANSPARENTBKGND differ in whether they allow the disabled background color to
	// be set, the behavior of LVS_EX_TRANSPARENTBKGND might eventually be seen as a bug by
	// Microsoft and fixed. However, using that style is still seen as safe and useful for a few
	// different reasons:
	//
	// 1. The page linked above was last updated in 2010. In Windows 11, as of 2025, the
	// behavior of LVS_EX_TRANSPARENTBKGND remains the same.
	//
	// 2. For years, Microsoft have made minimal changes to the Windows API, particularly the
	// common controls.
	//
	// 3. Using LVS_EX_TRANSPARENTBKGND is seemingly the only way to set a specific background
	// color on a disabled listview control.
	//
	// 4. If the behavior of LVS_EX_TRANSPARENTBKGND ever changes, the most that should happen
	// is that the background color set by the parent will be ignored.
	ListViewHelper::AddRemoveExtendedStyles(listView, LVS_EX_TRANSPARENTBKGND, addStyle);
}

INT_PTR StartupOptionsPage::DialogProcExtra(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(dlg);
	UNREFERENCED_PARAMETER(lParam);

	switch (msg)
	{
	case WM_ERASEBKGND:
		OnEraseDialogBackground(reinterpret_cast<HDC>(wParam));
		return TRUE;
	}

	return FALSE;
}

void StartupOptionsPage::OnEraseDialogBackground(HDC hdc)
{
	auto brush = reinterpret_cast<HBRUSH>(SendMessage(GetDialog(), WM_CTLCOLORDLG,
		reinterpret_cast<WPARAM>(hdc), reinterpret_cast<LPARAM>(GetDialog())));

	RECT rect;
	auto res = GetClientRect(GetDialog(), &rect);
	DCHECK(res);
	auto fillRes = FillRect(hdc, &rect, brush);
	DCHECK(fillRes);

	HWND listView = GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST);

	// As noted above, if a listview control has the LVS_EX_TRANSPARENTBKGND style set, its
	// background will be sourced from the parent control. Therefore, when the startup folders
	// listview is disabled, the rectangle drawn below is what will be used as the listview
	// background.
	if (m_darkModeManager->IsDarkModeEnabled() && !IsWindowEnabled(listView))
	{
		RECT listViewRect;
		res = GetClientRect(listView, &listViewRect);
		DCHECK(res);
		MapWindowRect(listView, GetDialog(), &listViewRect);

		fillRes = FillRect(hdc, &listViewRect, m_listViewDisabledBackgroundBrush.get());
		DCHECK(fillRes);
	}
}

LRESULT StartupOptionsPage::ListViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ENABLE:
	{
		auto res = DefSubclassProc(hwnd, msg, wParam, lParam);

		// The default handler will clear the LVS_EX_TRANSPARENTBKGND style, so it's important that
		// the style be set after the default handler has been invoked.
		UpdateListViewTransparentStyle();

		return res;
	}
	break;

	case WM_SIZE:
		ListView_SetColumnWidth(hwnd, 0, LOWORD(lParam));
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void StartupOptionsPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) == 0)
	{
		switch (LOWORD(wParam))
		{
		case IDC_STARTUP_DEFAULT_FOLDER:
		case IDC_STARTUP_PREVIOUS_TABS:
		case IDC_STARTUP_CUSTOM_FOLDERS:
			EnableWindow(GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST),
				IsDlgButtonChecked(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS) == BST_CHECKED);
			m_settingChangedCallback();
			break;
		}
	}
}

INT_PTR StartupOptionsPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);

	if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom
		== GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST))
	{
		switch (reinterpret_cast<NMHDR *>(lParam)->code)
		{
		case NM_DBLCLK:
			OnListViewDoubleClick(reinterpret_cast<NMITEMACTIVATE *>(lParam));
			return TRUE;

		case LVN_ENDLABELEDIT:
			OnListViewEndLabelEdit(reinterpret_cast<NMLVDISPINFO *>(lParam));
			return TRUE;

		case LVN_KEYDOWN:
			return OnListViewKeyDown(reinterpret_cast<NMLVKEYDOWN *>(lParam));
		}
	}

	return FALSE;
}

void StartupOptionsPage::OnListViewDoubleClick(const NMITEMACTIVATE *itemActivate)
{
	if (itemActivate->iItem == -1)
	{
		return;
	}

	ListView_EditLabel(itemActivate->hdr.hwndFrom, itemActivate->iItem);
}

void StartupOptionsPage::OnListViewEndLabelEdit(const NMLVDISPINFO *dispInfo)
{
	if (dispInfo->item.pszText == nullptr)
	{
		// The item text wasn't changed.
		return;
	}

	m_settingChangedCallback();

	int numItems = ListView_GetItemCount(dispInfo->hdr.hwndFrom);
	bool isLastItem = (dispInfo->item.iItem == (numItems - 1));

	if (lstrlen(dispInfo->item.pszText) == 0 && !isLastItem)
	{
		ListView_DeleteItem(dispInfo->hdr.hwndFrom, dispInfo->item.iItem);
		SetWindowLongPtr(GetDialog(), DWLP_MSGRESULT, FALSE);
		return;
	}

	if (lstrlen(dispInfo->item.pszText) != 0 && isLastItem)
	{
		// The text for the last item was updated, so insert a new blank item at the end.
		AddStartupFolder(numItems, L"");
	}

	SetWindowLongPtr(GetDialog(), DWLP_MSGRESULT, TRUE);
}

INT_PTR StartupOptionsPage::OnListViewKeyDown(const NMLVKEYDOWN *keyDown)
{
	switch (keyDown->wVKey)
	{
	case 'A':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			ListViewHelper::SelectAllItems(GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST),
				TRUE);
			return TRUE;
		}
		break;

	case VK_UP:
		if (!IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && IsKeyDown(VK_MENU))
		{
			MoveStartupFolder(MoveDirection::Up);
			return TRUE;
		}
		break;

	case VK_DOWN:
		if (!IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && IsKeyDown(VK_MENU))
		{
			MoveStartupFolder(MoveDirection::Down);
			return TRUE;
		}
		break;

	case VK_DELETE:
		OnListViewDeletePressed();
		return TRUE;
	}

	return FALSE;
}

void StartupOptionsPage::MoveStartupFolder(MoveDirection direction)
{
	HWND listView = GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST);
	auto selectedItems = ListViewHelper::GetSelectedItems(listView);

	if (selectedItems.size() != 1)
	{
		return;
	}

	auto index = *selectedItems.begin();
	int numItems = ListView_GetItemCount(listView);

	if (index == (numItems - 1))
	{
		// This item is the dummy entry that appears at the end. It can't be moved.
		return;
	}

	int newIndex = (direction == MoveDirection::Up) ? index - 1 : index + 1;

	if (newIndex < 0 || newIndex >= (numItems - 1))
	{
		return;
	}

	ListViewHelper::SwapItems(listView, index, newIndex);
	ListView_EnsureVisible(listView, newIndex, false);

	m_settingChangedCallback();
}

void StartupOptionsPage::OnListViewDeletePressed()
{
	HWND listView = GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST);
	auto selectedItems = ListViewHelper::GetSelectedItems(listView);

	if (selectedItems.empty())
	{
		return;
	}

	if (selectedItems.size() == 1
		&& *selectedItems.begin() == (ListView_GetItemCount(listView) - 1))
	{
		// In this case, only the last dummy entry is selected, so there's nothing that needs to be
		// done.
		return;
	}

	ScopedRedrawDisabler redrawDisabler(listView);

	// It's important to go through items in reverse order here (from largest to smallest),
	// otherwise the index of later items would change during the iteration.
	for (int index : selectedItems | std::views::reverse)
	{
		auto res = ListView_DeleteItem(listView, index);
		DCHECK(res);
	}

	int numItems = ListView_GetItemCount(listView);

	if (numItems == 0 || !ListViewHelper::GetItemText(listView, numItems - 1).empty())
	{
		AddStartupFolder(numItems, L"");
	}

	if (selectedItems.size() == 1)
	{
		// The original index should always be valid, since the last (dummy) item can't be deleted.
		ListViewHelper::FocusItem(listView, *selectedItems.begin(), true);
		ListViewHelper::SelectItem(listView, *selectedItems.begin(), true);
	}

	m_settingChangedCallback();
}

void StartupOptionsPage::SaveSettings()
{
	if (IsDlgButtonChecked(GetDialog(), IDC_STARTUP_DEFAULT_FOLDER) == BST_CHECKED)
	{
		m_config->startupMode = StartupMode::DefaultFolder;
	}
	else if (IsDlgButtonChecked(GetDialog(), IDC_STARTUP_PREVIOUS_TABS) == BST_CHECKED)
	{
		m_config->startupMode = StartupMode::PreviousTabs;
	}
	else if (IsDlgButtonChecked(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS) == BST_CHECKED)
	{
		m_config->startupMode = StartupMode::CustomFolders;
	}

	HWND listView = GetDlgItem(GetDialog(), IDC_STARTUP_CUSTOM_FOLDERS_LIST);
	int numItems = ListView_GetItemCount(listView);
	std::vector<std::wstring> startupFolders;

	for (int i = 0; i < numItems; i++)
	{
		auto startupFolder = ListViewHelper::GetItemText(listView, i);

		if (startupFolder.empty())
		{
			continue;
		}

		startupFolders.push_back(startupFolder);
	}

	m_config->startupFolders = startupFolders;
}
