// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DarkModeThemeManager.h"
#include "DarkModeHelper.h"
#include <vssym32.h>

static const WCHAR DIALOG_CLASS_NAME[] = L"#32770";

DarkModeThemeManager &DarkModeThemeManager::GetInstance()
{
	static DarkModeThemeManager themeManager;
	return themeManager;
}

void DarkModeThemeManager::ApplyThemeToTopLevelWindow(HWND topLevelWindow)
{
	ApplyThemeToWindow(topLevelWindow);
	EnumChildWindows(topLevelWindow, ProcessChildWindow, 0);
	RedrawWindow(topLevelWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
}

BOOL CALLBACK DarkModeThemeManager::ProcessChildWindow(HWND hwnd, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	ApplyThemeToWindow(hwnd);
	return TRUE;
}

void DarkModeThemeManager::ApplyThemeToWindow(HWND hwnd)
{
	auto &darkModeHelper = DarkModeHelper::GetInstance();
	darkModeHelper.AllowDarkModeForWindow(hwnd, true);

	// The maximum length of a class name is 256 characters (see the documentation for lpszClassName
	// in https://learn.microsoft.com/en-au/windows/win32/api/winuser/ns-winuser-wndclassw).
	WCHAR className[256];
	auto res = GetClassName(hwnd, className, static_cast<int>(std::size(className)));

	if (res == 0)
	{
		assert(false);
		return;
	}

	if (darkModeHelper.IsDarkModeEnabled())
	{
		if (lstrcmp(className, DIALOG_CLASS_NAME) == 0)
		{
			BOOL dark = TRUE;
			DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA compositionData = {
				DarkModeHelper::WCA_USEDARKMODECOLORS, &dark, sizeof(dark)
			};
			darkModeHelper.SetWindowCompositionAttribute(hwnd, &compositionData);

			SetWindowSubclass(hwnd, DialogSubclass, SUBCLASS_ID, 0);
		}
		else if (lstrcmp(className, WC_LISTVIEW) == 0)
		{
			darkModeHelper.AllowDarkModeForWindow(hwnd, true);
			SetWindowTheme(hwnd, L"ItemsView", nullptr);

			HWND header = ListView_GetHeader(hwnd);
			darkModeHelper.AllowDarkModeForWindow(header, true);
			SetWindowTheme(header, L"ItemsView", nullptr);

			HWND tooltips = ListView_GetToolTips(hwnd);
			darkModeHelper.AllowDarkModeForWindow(tooltips, true);
			SetWindowTheme(tooltips, L"Explorer", nullptr);

			ListView_SetBkColor(hwnd, DarkModeHelper::BACKGROUND_COLOR);
			ListView_SetTextBkColor(hwnd, DarkModeHelper::BACKGROUND_COLOR);
			ListView_SetTextColor(hwnd, DarkModeHelper::TEXT_COLOR);

			SetWindowSubclass(hwnd, ListViewSubclass, SUBCLASS_ID, 0);
		}
		else if (lstrcmp(className, WC_TREEVIEW) == 0)
		{
			// When in dark mode, this theme sets the following colors correctly:
			//
			// - the item selection color,
			// - the colors of the arrows that appear to the left of the items,
			// - the color of the scrollbars.
			//
			// It doesn't, however, change the background color, or the text color.
			SetWindowTheme(hwnd, L"Explorer", nullptr);

			TreeView_SetBkColor(hwnd, DarkModeHelper::BACKGROUND_COLOR);
			TreeView_SetTextColor(hwnd, DarkModeHelper::TEXT_COLOR);
			TreeView_SetInsertMarkColor(hwnd, DarkModeHelper::FOREGROUND_COLOR);

			HWND tooltips = TreeView_GetToolTips(hwnd);
			darkModeHelper.AllowDarkModeForWindow(tooltips, true);
			SetWindowTheme(tooltips, L"Explorer", nullptr);
		}
		else if (lstrcmp(className, MSFTEDIT_CLASS) == 0)
		{
			SendMessage(hwnd, EM_SETBKGNDCOLOR, 0, DarkModeHelper::BACKGROUND_COLOR);

			CHARFORMAT charFormat = {};
			charFormat.cbSize = sizeof(charFormat);
			charFormat.dwMask = CFM_COLOR;
			charFormat.crTextColor = DarkModeHelper::TEXT_COLOR;
			charFormat.dwEffects = 0;
			SendMessage(hwnd, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&charFormat));
		}
		else if (lstrcmp(className, WC_COMBOBOX) == 0)
		{
			SetWindowTheme(hwnd, L"CFD", nullptr);
		}
		else if (lstrcmp(className, WC_BUTTON) == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
		}
	}
	else
	{
		if (lstrcmp(className, DIALOG_CLASS_NAME) == 0)
		{
			BOOL dark = FALSE;
			DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA compositionData = {
				DarkModeHelper::WCA_USEDARKMODECOLORS, &dark, sizeof(dark)
			};
			darkModeHelper.SetWindowCompositionAttribute(hwnd, &compositionData);

			RemoveWindowSubclass(hwnd, DialogSubclass, SUBCLASS_ID);
		}
		else if (lstrcmp(className, WC_LISTVIEW) == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);

			HWND header = ListView_GetHeader(hwnd);
			darkModeHelper.AllowDarkModeForWindow(header, false);
			SetWindowTheme(header, L"ItemsView", nullptr);

			HWND tooltips = ListView_GetToolTips(hwnd);
			darkModeHelper.AllowDarkModeForWindow(tooltips, false);
			SetWindowTheme(tooltips, L"Explorer", nullptr);

			COLORREF backgroundColor = GetSysColor(COLOR_WINDOW);
			ListView_SetBkColor(hwnd, backgroundColor);
			ListView_SetTextBkColor(hwnd, backgroundColor);

			wil::unique_htheme theme(OpenThemeData(nullptr, L"ItemsView"));

			if (theme)
			{
				COLORREF textColor;
				HRESULT hr = GetThemeColor(theme.get(), 0, 0, TMT_TEXTCOLOR, &textColor);

				if (SUCCEEDED(hr))
				{
					ListView_SetTextColor(hwnd, textColor);
				}
			}

			RemoveWindowSubclass(hwnd, ListViewSubclass, SUBCLASS_ID);
		}
		else if (lstrcmp(className, WC_TREEVIEW) == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);

			wil::unique_htheme theme(OpenThemeData(nullptr, L"ItemsView"));

			if (theme)
			{
				COLORREF textColor;
				HRESULT hr = GetThemeColor(theme.get(), 0, 0, TMT_TEXTCOLOR, &textColor);

				if (SUCCEEDED(hr))
				{
					TreeView_SetTextColor(hwnd, textColor);
				}

				COLORREF fillColor;
				hr = GetThemeColor(theme.get(), 0, 0, TMT_FILLCOLOR, &fillColor);

				if (SUCCEEDED(hr))
				{
					TreeView_SetBkColor(hwnd, fillColor);
				}
			}

			TreeView_SetInsertMarkColor(hwnd, CLR_DEFAULT);

			HWND tooltips = TreeView_GetToolTips(hwnd);
			darkModeHelper.AllowDarkModeForWindow(tooltips, false);
			SetWindowTheme(tooltips, L"Explorer", nullptr);
		}
		else if (lstrcmp(className, MSFTEDIT_CLASS) == 0)
		{
			SendMessage(hwnd, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_WINDOW));

			CHARFORMAT charFormat = {};
			charFormat.cbSize = sizeof(charFormat);
			charFormat.dwMask = CFM_COLOR;
			charFormat.crTextColor = GetSysColor(COLOR_WINDOWTEXT);
			charFormat.dwEffects = 0;
			SendMessage(hwnd, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&charFormat));
		}
		else if (lstrcmp(className, WC_COMBOBOX) == 0)
		{
			SetWindowTheme(hwnd, L"CFD", nullptr);
		}
		else if (lstrcmp(className, WC_BUTTON) == 0)
		{
			SetWindowTheme(hwnd, L"Explorer", nullptr);
		}
	}
}

LRESULT CALLBACK DarkModeThemeManager::DialogSubclass(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam, UINT_PTR subclassId, DWORD_PTR data)
{
	UNREFERENCED_PARAMETER(data);

	switch (msg)
	{
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORBTN:
	{
		auto hdc = reinterpret_cast<HDC>(wParam);
		SetBkColor(hdc, DarkModeHelper::BACKGROUND_COLOR);
		SetTextColor(hdc, DarkModeHelper::TEXT_COLOR);
		return reinterpret_cast<INT_PTR>(DarkModeHelper::GetInstance().GetBackgroundBrush());
	}
	break;

	case WM_NCDESTROY:
	{
		[[maybe_unused]] auto res = RemoveWindowSubclass(hwnd, DialogSubclass, subclassId);
		assert(res);
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK DarkModeThemeManager::ListViewSubclass(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam, UINT_PTR subclassId, DWORD_PTR data)
{
	UNREFERENCED_PARAMETER(data);

	switch (msg)
	{
	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code)
		{
		case NM_CUSTOMDRAW:
		{
			auto *customDraw = reinterpret_cast<NMCUSTOMDRAW *>(lParam);

			switch (customDraw->dwDrawStage)
			{
			case CDDS_PREPAINT:
				return CDRF_NOTIFYITEMDRAW;

			case CDDS_ITEMPREPAINT:
				SetTextColor(customDraw->hdc, DarkModeHelper::TEXT_COLOR);
				return CDRF_NEWFONT;
			}
		}
		break;
		}
		break;

	case WM_NCDESTROY:
	{
		[[maybe_unused]] auto res = RemoveWindowSubclass(hwnd, ListViewSubclass, subclassId);
		assert(res);
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}
