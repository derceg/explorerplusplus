// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Controls.h"
#include "DpiCompatibility.h"
#include "Macros.h"
#include "WindowHelper.h"
#include <VSStyle.h>
#include <uxtheme.h>
#include <wil/resource.h>

constexpr int DEFAULT_CHECKBOX_WIDTH = 13;
constexpr int DEFAULT_CHECKBOX_HEIGHT = 13;

constexpr int DEFAULT_RADIO_BUTTON_WIDTH = 13;
constexpr int DEFAULT_RADIO_BUTTON_HEIGHT = 13;

HWND CreateListView(HWND hParent, DWORD dwStyle)
{
	HWND hListView = CreateWindow(WC_LISTVIEW, EMPTY_STRING, dwStyle,
		0, 0, 0, 0, hParent, nullptr, GetModuleHandle(nullptr), nullptr);

	if(hListView != nullptr)
	{
		/* Set the extended hListView styles. These styles can't be set
		properly with CreateWindowEx(). */
		SendMessage(hListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);
	}

	return hListView;
}

HWND CreateTreeView(HWND hParent, DWORD dwStyle)
{
	HWND hTreeView = CreateWindow(WC_TREEVIEW, EMPTY_STRING, dwStyle,
		0, 0, 0, 0, hParent, nullptr, GetModuleHandle(nullptr), nullptr);

	if(hTreeView != nullptr)
	{
		/* Retrieve the small version of the system image list. */
		HIMAGELIST smallIcons;
		BOOL bRet = Shell_GetImageLists(nullptr, &smallIcons);

		if(bRet)
		{
			TreeView_SetImageList(hTreeView, smallIcons, TVSIL_NORMAL);
		}
	}

	return hTreeView;
}

HWND CreateStatusBar(HWND hParent, DWORD dwStyle)
{
	HWND hStatusBar = CreateWindow(STATUSCLASSNAME, EMPTY_STRING, dwStyle,
		0, 0, 0, 0, hParent, nullptr, GetModuleHandle(nullptr), nullptr);

	return hStatusBar;
}

HWND CreateToolbar(HWND hParent, DWORD dwStyle, DWORD dwExStyle)
{
	HWND hToolbar = CreateWindow(TOOLBARCLASSNAME, EMPTY_STRING, dwStyle,
		0, 0, 0, 0, hParent, nullptr, GetModuleHandle(nullptr), nullptr);

	if(hToolbar != nullptr)
	{
		/* Set the extended styles for the toolbar. */
		SendMessage(hToolbar, TB_SETEXTENDEDSTYLE, 0, dwExStyle);
	}

	return hToolbar;
}

HWND CreateComboBox(HWND parent, DWORD dwStyle)
{
	HWND hComboBox = CreateWindowEx(WS_EX_TOOLWINDOW, WC_COMBOBOXEX,
		EMPTY_STRING, dwStyle, 0, 0, 0, 200, parent, nullptr,
		GetModuleHandle(nullptr), nullptr);

	return hComboBox;
}

HWND CreateTabControl(HWND hParent, DWORD dwStyle)
{
	HWND hTabControl = CreateWindowEx(0, WC_TABCONTROL, EMPTY_STRING,
		dwStyle, 0, 0, 0, 0, hParent, nullptr, GetModuleHandle(nullptr), nullptr);

	return hTabControl;
}

HWND CreateTooltipControl(HWND parent, HINSTANCE instance)
{
	HWND tipWnd = CreateWindowEx(0, TOOLTIPS_CLASS, nullptr, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, parent, nullptr, instance, nullptr);

	SetWindowPos(tipWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	return tipWnd;
}

BOOL PinStatusBar(HWND hStatusBar, int width, int height)
{
	RECT rc;
	BOOL bRet = GetWindowRect(hStatusBar, &rc);

	if(bRet)
	{
		/* Pin the status bar to the bottom of the window. */
		bRet = SetWindowPos(hStatusBar, nullptr, 0, height - GetRectHeight(&rc),
			width, GetRectHeight(&rc), SWP_NOZORDER);
	}

	return bRet;
}

BOOL AddPathsToComboBoxEx(HWND hComboBoxEx, const TCHAR *path)
{
	HIMAGELIST smallIcons;
	BOOL bRet = Shell_GetImageLists(nullptr, &smallIcons);

	if(!bRet)
	{
		return FALSE;
	}

	SendMessage(hComboBoxEx, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(smallIcons));

	/* Remove all items that are currently in the list. */
	SendMessage(hComboBoxEx, CB_RESETCONTENT, 0, 0);

	TCHAR findPath[MAX_PATH];
	StringCchCopy(findPath, SIZEOF_ARRAY(findPath), path);
	bRet = PathAppend(findPath, _T("*"));

	if(!bRet)
	{
		return FALSE;
	}

	WIN32_FIND_DATA wfd;
	HANDLE hFirstFile = FindFirstFile(findPath, &wfd);

	if(hFirstFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	BOOL success = TRUE;

	while(FindNextFile(hFirstFile, &wfd))
	{
		if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY &&
			StrCmp(wfd.cFileName, _T("..")) != 0)
		{
			TCHAR fullFileName[MAX_PATH];
			LPTSTR szRet = PathCombine(fullFileName, path, wfd.cFileName);

			if(szRet == nullptr)
			{
				success = FALSE;
				break;
			}

			SHFILEINFO shfi;
			SHGetFileInfo(path, NULL, &shfi, NULL, SHGFI_SYSICONINDEX);

			COMBOBOXEXITEM cbItem;
			cbItem.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_INDENT | CBEIF_SELECTEDIMAGE;
			cbItem.iItem = -1;
			cbItem.iImage = shfi.iIcon;
			cbItem.iSelectedImage = shfi.iIcon;
			cbItem.iIndent = 1;
			cbItem.iOverlay = 1;
			cbItem.pszText = wfd.cFileName;

			LRESULT lRet = SendMessage(hComboBoxEx, CBEM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&cbItem));

			if(lRet == -1)
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

	if(bCheck)
	{
		uCheck = BST_CHECKED;
	}
	else
	{
		uCheck = BST_UNCHECKED;
	}

	return CheckDlgButton(hDlg, buttonId, uCheck);
}

void AddStyleToToolbar(UINT *fStyle, UINT fStyleToAdd)
{
	if((*fStyle & fStyleToAdd) != fStyleToAdd)
	{
		*fStyle |= fStyleToAdd;
	}
}

void AddGripperStyle(UINT *fStyle, BOOL bAddGripper)
{
	if(bAddGripper)
	{
		/* Remove the no-gripper style (if present). */
		if((*fStyle & RBBS_NOGRIPPER) == RBBS_NOGRIPPER)
		{
			*fStyle &= ~RBBS_NOGRIPPER;
		}

		/* Only add the gripper style if it isn't already present. */
		if((*fStyle & RBBS_GRIPPERALWAYS) != RBBS_GRIPPERALWAYS)
		{
			*fStyle |= RBBS_GRIPPERALWAYS;
		}
	}
	else
	{
		if((*fStyle & RBBS_GRIPPERALWAYS) == RBBS_GRIPPERALWAYS)
		{
			*fStyle &= ~RBBS_GRIPPERALWAYS;
		}

		if((*fStyle & RBBS_NOGRIPPER) != RBBS_NOGRIPPER)
		{
			*fStyle |= RBBS_NOGRIPPER;
		}
	}
}

void UpdateToolbarBandSizing(HWND hRebar, HWND hToolbar)
{
	REBARBANDINFO rbbi;
	SIZE sz;
	int nBands;
	int iBand = -1;
	int i = 0;

	nBands = (int) SendMessage(hRebar, RB_GETBANDCOUNT, 0, 0);

	for(i = 0; i < nBands; i++)
	{
		rbbi.cbSize = sizeof(rbbi);
		rbbi.fMask = RBBIM_CHILD;
		SendMessage(hRebar, RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&rbbi));

		if(rbbi.hwndChild == hToolbar)
		{
			iBand = i;
			break;
		}
	}

	if(iBand != -1)
	{
		SendMessage(hToolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&sz));

		rbbi.cbSize = sizeof(rbbi);
		rbbi.fMask = RBBIM_IDEALSIZE;
		rbbi.cxIdeal = sz.cx;
		SendMessage(hRebar, RB_SETBANDINFO, iBand, reinterpret_cast<LPARAM>(&rbbi));
	}
}

SIZE GetCheckboxSize(HWND hwnd)
{
	return GetButtonSize(
		hwnd, BP_CHECKBOX, CBS_UNCHECKEDNORMAL, DEFAULT_CHECKBOX_WIDTH, DEFAULT_CHECKBOX_HEIGHT);
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
			HRESULT hr = GetThemePartSize(
				theme.get(), screenDC.get(), partId, stateId, nullptr, TS_DRAW, &size);

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