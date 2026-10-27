// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Controls.h"
#include "DpiCompatibility.h"
#include "ScopedRedrawDisabler.h"
#include "WindowHelper.h"
#include <glog/logging.h>
#include <VSStyle.h>

// wil/resource.h needs to be included after uxtheme.h to ensure that wil::unique_htheme is defined.
// clang-format off
#include <uxtheme.h>
#include <wil/resource.h>
#include <windowsx.h>
#include <Shlwapi.h>
// clang-format on

constexpr int DEFAULT_CHECKBOX_WIDTH = 13;
constexpr int DEFAULT_CHECKBOX_HEIGHT = 13;

constexpr int DEFAULT_RADIO_BUTTON_WIDTH = 13;
constexpr int DEFAULT_RADIO_BUTTON_HEIGHT = 13;

HWND CreateListView(HWND hParent, DWORD dwStyle)
{
	HWND hListView = CreateWindow(WC_LISTVIEW, L"", dwStyle, 0, 0, 0, 0, hParent, nullptr,
		GetModuleHandle(nullptr), nullptr);

	if (hListView != nullptr)
	{
		/* Set the extended hListView styles. These styles can't be set
		properly with CreateWindowEx(). */
		SendMessage(hListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
			LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);
	}

	return hListView;
}

HWND CreateTreeView(HWND hParent, DWORD dwStyle)
{
	HWND hTreeView = CreateWindow(WC_TREEVIEW, L"", dwStyle, 0, 0, 0, 0, hParent, nullptr,
		GetModuleHandle(nullptr), nullptr);

	if (hTreeView != nullptr)
	{
		/* Retrieve the small version of the system image list. */
		HIMAGELIST smallIcons;
		BOOL bRet = Shell_GetImageLists(nullptr, &smallIcons);

		if (bRet)
		{
			TreeView_SetImageList(hTreeView, smallIcons, TVSIL_NORMAL);
		}
	}

	return hTreeView;
}

HWND CreateStatusBar(HWND hParent, DWORD dwStyle)
{
	HWND hStatusBar = CreateWindow(STATUSCLASSNAME, L"", dwStyle, 0, 0, 0, 0, hParent, nullptr,
		GetModuleHandle(nullptr), nullptr);

	return hStatusBar;
}

HWND CreateToolbar(HWND hParent, DWORD dwStyle, DWORD dwExStyle)
{
	HWND hToolbar = CreateWindow(TOOLBARCLASSNAME, L"", dwStyle, 0, 0, 0, 0, hParent, nullptr,
		GetModuleHandle(nullptr), nullptr);

	if (hToolbar != nullptr)
	{
		/* Set the extended styles for the toolbar. */
		SendMessage(hToolbar, TB_SETEXTENDEDSTYLE, 0, dwExStyle);
	}

	return hToolbar;
}

HWND CreateTabControl(HWND hParent, DWORD dwStyle)
{
	HWND hTabControl = CreateWindowEx(0, WC_TABCONTROL, L"", dwStyle, 0, 0, 0, 0, hParent, nullptr,
		GetModuleHandle(nullptr), nullptr);

	return hTabControl;
}

HWND CreateTooltipControl(HWND parent)
{
	HWND tipWnd = CreateWindowEx(0, TOOLTIPS_CLASS, nullptr, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, parent, nullptr, GetModuleHandle(nullptr), nullptr);

	SetWindowPos(tipWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	return tipWnd;
}

BOOL AddPathsToComboBoxEx(HWND hComboBoxEx, const TCHAR *path)
{
	/* Remove all items that are currently in the list. */
	SendMessage(hComboBoxEx, CB_RESETCONTENT, 0, 0);

	TCHAR findPath[MAX_PATH];
	StringCchCopy(findPath, std::size(findPath), path);
	BOOL bRet = PathAppend(findPath, _T("*"));

	if (!bRet)
	{
		return FALSE;
	}

	WIN32_FIND_DATA wfd;
	HANDLE hFirstFile = FindFirstFile(findPath, &wfd);

	if (hFirstFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	BOOL success = TRUE;

	while (FindNextFile(hFirstFile, &wfd))
	{
		if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY
			&& StrCmp(wfd.cFileName, _T("..")) != 0)
		{
			TCHAR fullFileName[MAX_PATH];
			LPTSTR szRet = PathCombine(fullFileName, path, wfd.cFileName);

			if (szRet == nullptr)
			{
				success = FALSE;
				break;
			}

			COMBOBOXEXITEM cbItem = {};
			cbItem.mask = CBEIF_TEXT | CBEIF_INDENT;
			cbItem.iItem = -1;
			cbItem.iIndent = 1;
			cbItem.pszText = wfd.cFileName;

			SHFILEINFO shfi;
			auto res = SHGetFileInfo(fullFileName, 0, &shfi, sizeof(shfi), SHGFI_SYSICONINDEX);

			if (res != 0)
			{
				WI_SetAllFlags(cbItem.mask, CBEIF_IMAGE | CBEIF_SELECTEDIMAGE);
				cbItem.iImage = shfi.iIcon;
				cbItem.iSelectedImage = shfi.iIcon;
			}

			LRESULT lRet =
				SendMessage(hComboBoxEx, CBEM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&cbItem));

			if (lRet == -1)
			{
				success = FALSE;
				break;
			}
		}
	}

	FindClose(hFirstFile);

	return success;
}

BOOL lCheckDlgButton(HWND hDlg, int buttonId, BOOL bCheck)
{
	UINT uCheck;

	if (bCheck)
	{
		uCheck = BST_CHECKED;
	}
	else
	{
		uCheck = BST_UNCHECKED;
	}

	return CheckDlgButton(hDlg, buttonId, uCheck);
}

SIZE GetCheckboxSize(HWND hwnd)
{
	return GetButtonSize(hwnd, BP_CHECKBOX, CBS_UNCHECKEDNORMAL, DEFAULT_CHECKBOX_WIDTH,
		DEFAULT_CHECKBOX_HEIGHT);
}

SIZE GetRadioButtonSize(HWND hwnd)
{
	return GetButtonSize(hwnd, BP_RADIOBUTTON, RBS_UNCHECKEDNORMAL, DEFAULT_RADIO_BUTTON_WIDTH,
		DEFAULT_RADIO_BUTTON_HEIGHT);
}

SIZE GetButtonSize(HWND hwnd, int partId, int stateId, int defaultWidth, int defaultHeight)
{
	if (IsAppThemed())
	{
		wil::unique_htheme theme(OpenThemeData(hwnd, L"BUTTON"));

		if (theme)
		{
			wil::unique_hdc_window screenDC(GetDC(nullptr));

			SIZE size;
			HRESULT hr = GetThemePartSize(theme.get(), screenDC.get(), partId, stateId, nullptr,
				TS_DRAW, &size);

			if (SUCCEEDED(hr))
			{
				return size;
			}
		}
	}

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(hwnd);
	return { MulDiv(defaultWidth, dpi, USER_DEFAULT_SCREEN_DPI),
		MulDiv(defaultHeight, dpi, USER_DEFAULT_SCREEN_DPI) };
}

void AddTooltipForControl(HWND tipWnd, HWND control, const std::wstring &tooltip,
	TooltipType tooltipType)
{
	TOOLINFO toolInfo = {};
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.uFlags = TTF_SUBCLASS;
	toolInfo.lpszText = const_cast<wchar_t *>(tooltip.c_str());

	if (tooltipType == TooltipType::Control)
	{
		WI_SetFlag(toolInfo.uFlags, TTF_IDISHWND);
		toolInfo.uId = reinterpret_cast<UINT_PTR>(control);
	}
	else
	{
		HWND parent = GetParent(control);

		RECT controlRect;
		GetWindowRect(control, &controlRect);
		MapWindowPoints(HWND_DESKTOP, parent, reinterpret_cast<LPPOINT>(&controlRect), 2);

		toolInfo.hwnd = parent;
		toolInfo.rect = controlRect;
	}

	auto res = SendMessage(tipWnd, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));
	DCHECK(res);
}

void AddItemsToComboBox(HWND comboBox, const std::vector<ComboBoxItem> &items, int currentItemId)
{
	for (const auto &item : items)
	{
		int index = static_cast<int>(
			SendMessage(comboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.text.c_str())));

		if (index == CB_ERR)
		{
			DCHECK(false);
			continue;
		}

		auto res = SendMessage(comboBox, CB_SETITEMDATA, index, static_cast<LPARAM>(item.id));
		DCHECK(res != CB_ERR);

		if (item.id == currentItemId)
		{
			res = SendMessage(comboBox, CB_SETCURSEL, index, 0);
			DCHECK(res != CB_ERR);
		}
	}
}

bool DoesComboBoxContainText(HWND comboBox, const std::wstring &text,
	StringComparatorFunc stringComparator)
{
	int numItems = ComboBox_GetCount(comboBox);

	if (numItems == CB_ERR)
	{
		DCHECK(false);
		return false;
	}

	for (int i = 0; i < numItems; i++)
	{
		int numCharacters = ComboBox_GetLBTextLen(comboBox, i);

		if (numCharacters == CB_ERR)
		{
			DCHECK(false);
			continue;
		}

		// The character count returned by CB_GETLBTEXTLEN doesn't include the terminating NULL.
		std::wstring listBoxItemText;
		listBoxItemText.resize(numCharacters + 1);

		numCharacters = ComboBox_GetLBText(comboBox, i, listBoxItemText.data());

		if (numCharacters == CB_ERR)
		{
			DCHECK(false);
			continue;
		}

		listBoxItemText.resize(numCharacters);

		if (stringComparator(listBoxItemText, text))
		{
			return true;
		}
	}

	return false;
}
