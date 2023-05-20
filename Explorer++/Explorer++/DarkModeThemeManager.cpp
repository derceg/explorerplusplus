// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DarkModeThemeManager.h"
#include "DarkModeButton.h"
#include "DarkModeHelper.h"
#include "Explorer++_internal.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowHelper.h"
#include <wil/resource.h>
#include <vssym32.h>

static const WCHAR DIALOG_CLASS_NAME[] = L"#32770";

DarkModeThemeManager &DarkModeThemeManager::GetInstance()
{
	static DarkModeThemeManager themeManager;
	return themeManager;
}

void DarkModeThemeManager::ApplyThemeToWindowAndChildren(HWND topLevelWindow)
{
	ApplyThemeToWindow(topLevelWindow);
	EnumChildWindows(topLevelWindow, ProcessChildWindow, 0);

	// Tooltip windows won't be enumerated by EnumChildWindows(). They will, however, be enumerated
	// by EnumThreadWindows(), which is why that's called here.
	// Note that this is explicitly called after EnumChildWindows(). That way, tooltip windows can
	// be initialized during the call to EnumChildWindows() (since they won't necessarily exist
	// initially). Those tooltip windows will then be processed as part of the call to
	// EnumThreadWindows().
	EnumThreadWindows(GetCurrentThreadId(), ProcessThreadWindow, 0);

	RedrawWindow(topLevelWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
}

BOOL CALLBACK DarkModeThemeManager::ProcessChildWindow(HWND hwnd, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	ApplyThemeToWindow(hwnd);
	return TRUE;
}

BOOL CALLBACK DarkModeThemeManager::ProcessThreadWindow(HWND hwnd, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	WCHAR className[256];
	auto res = GetClassName(hwnd, className, static_cast<int>(std::size(className)));

	if (res == 0)
	{
		assert(false);
		return TRUE;
	}

	if (lstrcmp(className, TOOLTIPS_CLASS) != 0)
	{
		return TRUE;
	}

	ApplyThemeToWindow(hwnd);

	return TRUE;
}

void DarkModeThemeManager::ApplyThemeToWindow(HWND hwnd)
{
	auto &darkModeHelper = DarkModeHelper::GetInstance();
	bool enableDarkMode = darkModeHelper.IsDarkModeEnabled();
	darkModeHelper.AllowDarkModeForWindow(hwnd, enableDarkMode);

	// The maximum length of a class name is 256 characters (see the documentation for lpszClassName
	// in https://learn.microsoft.com/en-au/windows/win32/api/winuser/ns-winuser-wndclassw).
	WCHAR className[256];
	auto res = GetClassName(hwnd, className, static_cast<int>(std::size(className)));

	if (res == 0)
	{
		assert(false);
		return;
	}

	if (lstrcmp(className, NExplorerplusplus::CLASS_NAME) == 0)
	{
		ApplyThemeToMainWindow(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, DIALOG_CLASS_NAME) == 0)
	{
		ApplyThemeToDialog(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, WC_LISTVIEW) == 0)
	{
		ApplyThemeToListView(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, WC_HEADER) == 0)
	{
		ApplyThemeToHeader(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, WC_TREEVIEW) == 0)
	{
		ApplyThemeToTreeView(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, MSFTEDIT_CLASS) == 0)
	{
		ApplyThemeToRichEdit(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, REBARCLASSNAME) == 0)
	{
		ApplyThemeToRebar(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, TOOLBARCLASSNAME) == 0)
	{
		ApplyThemeToToolbar(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, WC_COMBOBOXEX) == 0)
	{
		ApplyThemeToComboBoxEx(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, WC_COMBOBOX) == 0)
	{
		ApplyThemeToComboBox(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, WC_BUTTON) == 0)
	{
		ApplyThemeToButton(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, TOOLTIPS_CLASS) == 0)
	{
		ApplyThemeToTooltips(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, STATUSCLASSNAME) == 0)
	{
		ApplyThemeToStatusBar(hwnd, enableDarkMode);
	}
}

void DarkModeThemeManager::ApplyThemeToMainWindow(HWND hwnd, bool enableDarkMode)
{
	BOOL dark = enableDarkMode;
	DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA compositionData = {
		DarkModeHelper::WCA_USEDARKMODECOLORS, &dark, sizeof(dark)
	};
	DarkModeHelper::GetInstance().SetWindowCompositionAttribute(hwnd, &compositionData);
}

void DarkModeThemeManager::ApplyThemeToDialog(HWND hwnd, bool enableDarkMode)
{
	BOOL dark = enableDarkMode;
	DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA compositionData = {
		DarkModeHelper::WCA_USEDARKMODECOLORS, &dark, sizeof(dark)
	};
	DarkModeHelper::GetInstance().SetWindowCompositionAttribute(hwnd, &compositionData);

	if (enableDarkMode)
	{
		SetWindowSubclass(hwnd, DialogSubclass, SUBCLASS_ID, 0);
	}
	else
	{
		RemoveWindowSubclass(hwnd, DialogSubclass, SUBCLASS_ID);
	}
}

void DarkModeThemeManager::ApplyThemeToListView(HWND hwnd, bool enableDarkMode)
{
	if (enableDarkMode)
	{
		SetWindowTheme(hwnd, L"ItemsView", nullptr);
	}
	else
	{
		SetWindowTheme(hwnd, L"Explorer", nullptr);
	}

	COLORREF backgroundColor;
	COLORREF textColor;

	if (enableDarkMode)
	{
		backgroundColor = DarkModeHelper::BACKGROUND_COLOR;
		textColor = DarkModeHelper::TEXT_COLOR;
	}
	else
	{
		backgroundColor = GetSysColor(COLOR_WINDOW);
		textColor = GetSysColor(COLOR_WINDOWTEXT);
	}

	ListView_SetBkColor(hwnd, backgroundColor);
	ListView_SetTextBkColor(hwnd, backgroundColor);
	ListView_SetTextColor(hwnd, textColor);

	if (enableDarkMode)
	{
		SetWindowSubclass(hwnd, ListViewSubclass, SUBCLASS_ID, 0);
	}
	else
	{
		RemoveWindowSubclass(hwnd, ListViewSubclass, SUBCLASS_ID);
	}
}

void DarkModeThemeManager::ApplyThemeToHeader(HWND hwnd, bool enableDarkMode)
{
	UNREFERENCED_PARAMETER(enableDarkMode);

	SetWindowTheme(hwnd, L"ItemsView", nullptr);
}

void DarkModeThemeManager::ApplyThemeToTreeView(HWND hwnd, bool enableDarkMode)
{
	// When in dark mode, this theme sets the following colors correctly:
	//
	// - the item selection color,
	// - the colors of the arrows that appear to the left of the items,
	// - the color of the scrollbars.
	//
	// It doesn't, however, change the background color, or the text color.
	SetWindowTheme(hwnd, L"Explorer", nullptr);

	COLORREF backgroundColor;
	COLORREF textColor;
	COLORREF insertMarkColor;

	if (enableDarkMode)
	{
		backgroundColor = DarkModeHelper::BACKGROUND_COLOR;
		textColor = DarkModeHelper::TEXT_COLOR;
		insertMarkColor = DarkModeHelper::FOREGROUND_COLOR;
	}
	else
	{
		backgroundColor = GetSysColor(COLOR_WINDOW);
		textColor = GetSysColor(COLOR_WINDOWTEXT);
		insertMarkColor = CLR_DEFAULT;
	}

	TreeView_SetBkColor(hwnd, backgroundColor);
	TreeView_SetTextColor(hwnd, textColor);
	TreeView_SetInsertMarkColor(hwnd, insertMarkColor);
}

void DarkModeThemeManager::ApplyThemeToRichEdit(HWND hwnd, bool enableDarkMode)
{
	COLORREF backgroundColor;
	COLORREF textColor;

	if (enableDarkMode)
	{
		backgroundColor = DarkModeHelper::BACKGROUND_COLOR;
		textColor = DarkModeHelper::TEXT_COLOR;
	}
	else
	{
		backgroundColor = GetSysColor(COLOR_WINDOW);
		textColor = GetSysColor(COLOR_WINDOWTEXT);
	}

	SendMessage(hwnd, EM_SETBKGNDCOLOR, 0, backgroundColor);

	CHARFORMAT charFormat = {};
	charFormat.cbSize = sizeof(charFormat);
	charFormat.dwMask = CFM_COLOR;
	charFormat.crTextColor = textColor;
	charFormat.dwEffects = 0;
	SendMessage(hwnd, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&charFormat));
}

void DarkModeThemeManager::ApplyThemeToRebar(HWND hwnd, bool enableDarkMode)
{
	if (enableDarkMode)
	{
		SetWindowSubclass(hwnd, RebarSubclass, SUBCLASS_ID, 0);
	}
	else
	{
		RemoveWindowSubclass(hwnd, RebarSubclass, SUBCLASS_ID);
	}
}

void DarkModeThemeManager::ApplyThemeToToolbar(HWND hwnd, bool enableDarkMode)
{
	COLORREF insertMarkColor;

	if (enableDarkMode)
	{
		insertMarkColor = DarkModeHelper::FOREGROUND_COLOR;
	}
	else
	{
		insertMarkColor = CLR_DEFAULT;
	}

	SendMessage(hwnd, TB_SETINSERTMARKCOLOR, 0, insertMarkColor);

	// The tooltips window won't exist until either it's requested using TB_GETTOOLTIPS, or
	// the tooltip needs to be shown. Therefore, calling TB_GETTOOLTIPS will create the
	// tooltip control, if appropriate (the toolbar may not have the TBSTYLE_TOOLTIPS style
	// set, in which case, no tooltip control will be created).
	// The tooltip window will then be themed by the call to EnumThreadWindows() above.
	SendMessage(hwnd, TB_GETTOOLTIPS, 0, 0);

	HWND parent = GetParent(hwnd);
	assert(parent);

	if (enableDarkMode)
	{
		// This may be called multiple times (if there's more than one toolbar in a particular
		// window), but that's not an issue, as the subclass will only be installed once.
		SetWindowSubclass(parent, ToolbarParentSubclass, SUBCLASS_ID, 0);
	}
	else
	{
		RemoveWindowSubclass(parent, ToolbarParentSubclass, SUBCLASS_ID);
	}
}

void DarkModeThemeManager::ApplyThemeToComboBoxEx(HWND hwnd, bool enableDarkMode)
{
	if (enableDarkMode)
	{
		SetWindowSubclass(hwnd, ComboBoxExSubclass, SUBCLASS_ID, 0);
	}
	else
	{
		RemoveWindowSubclass(hwnd, ComboBoxExSubclass, SUBCLASS_ID);
	}
}

void DarkModeThemeManager::ApplyThemeToComboBox(HWND hwnd, bool enableDarkMode)
{
	if (enableDarkMode)
	{
		HWND parent = GetParent(hwnd);
		assert(parent);

		WCHAR parentClassName[256];
		auto parentClassNameResult =
			GetClassName(parent, parentClassName, static_cast<int>(std::size(parentClassName)));

		if (parentClassNameResult != 0 && lstrcmp(parentClassName, WC_COMBOBOXEX) == 0)
		{
			SetWindowTheme(hwnd, L"AddressComposited", nullptr);
		}
		else
		{
			SetWindowTheme(hwnd, L"CFD", nullptr);
		}
	}
	else
	{
		SetWindowTheme(hwnd, L"CFD", nullptr);
	}
}

void DarkModeThemeManager::ApplyThemeToButton(HWND hwnd, bool enableDarkMode)
{
	SetWindowTheme(hwnd, L"Explorer", nullptr);

	auto style = GetWindowLongPtr(hwnd, GWL_STYLE);

	if ((style & BS_TYPEMASK) == BS_GROUPBOX)
	{
		if (enableDarkMode)
		{
			SetWindowSubclass(hwnd, GroupBoxSubclass, SUBCLASS_ID, 0);
		}
		else
		{
			RemoveWindowSubclass(hwnd, GroupBoxSubclass, SUBCLASS_ID);
		}
	}
}

void DarkModeThemeManager::ApplyThemeToTooltips(HWND hwnd, bool enableDarkMode)
{
	UNREFERENCED_PARAMETER(enableDarkMode);

	SetWindowTheme(hwnd, L"Explorer", nullptr);
}

void DarkModeThemeManager::ApplyThemeToStatusBar(HWND hwnd, bool enableDarkMode)
{
	if (enableDarkMode)
	{
		SetWindowTheme(hwnd, nullptr, L"ExplorerStatusBar");
	}
	else
	{
		// Revert the control back to its default theme (see
		// https://devblogs.microsoft.com/oldnewthing/20181115-00/?p=100225).
		SetWindowTheme(hwnd, nullptr, nullptr);
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
		return reinterpret_cast<LRESULT>(DarkModeHelper::GetInstance().GetBackgroundBrush());
	}
	break;

	case WM_NOTIFY:
	{
		auto *nmhdr = reinterpret_cast<NMHDR *>(lParam);

		switch (nmhdr->code)
		{
		case NM_CUSTOMDRAW:
			return OnCustomDraw(reinterpret_cast<NMCUSTOMDRAW *>(lParam));
		}
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

LRESULT CALLBACK DarkModeThemeManager::ToolbarParentSubclass(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam, UINT_PTR subclassId, DWORD_PTR data)
{
	UNREFERENCED_PARAMETER(data);

	switch (msg)
	{
	case WM_NOTIFY:
	{
		auto *nmhdr = reinterpret_cast<NMHDR *>(lParam);

		switch (nmhdr->code)
		{
		case NM_CUSTOMDRAW:
			return OnCustomDraw(reinterpret_cast<NMCUSTOMDRAW *>(lParam));
		}
	}
	break;

	case WM_NCDESTROY:
	{
		[[maybe_unused]] auto res = RemoveWindowSubclass(hwnd, ToolbarParentSubclass, subclassId);
		assert(res);
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT DarkModeThemeManager::OnCustomDraw(NMCUSTOMDRAW *customDraw)
{
	WCHAR className[256];
	auto res =
		GetClassName(customDraw->hdr.hwndFrom, className, static_cast<int>(std::size(className)));

	if (res == 0)
	{
		assert(false);
		return CDRF_DODEFAULT;
	}

	if (lstrcmp(className, WC_BUTTON) == 0)
	{
		return OnButtonCustomDraw(customDraw);
	}
	else if (lstrcmp(className, TOOLBARCLASSNAME) == 0)
	{
		return OnToolbarCustomDraw(reinterpret_cast<NMTBCUSTOMDRAW *>(customDraw));
	}

	return CDRF_DODEFAULT;
}

LRESULT DarkModeThemeManager::OnButtonCustomDraw(NMCUSTOMDRAW *customDraw)
{
	switch (customDraw->dwDrawStage)
	{
	case CDDS_PREPAINT:
	{
		auto style = GetWindowLongPtr(customDraw->hdr.hwndFrom, GWL_STYLE);

		DarkModeButton::ButtonType buttonType;

		// Although the documentation
		// (https://learn.microsoft.com/en-au/windows/win32/controls/button-styles#constants)
		// states that BS_TYPEMASK is out of date and shouldn't be used, it's necessary
		// here, for the reasons discussed in https://stackoverflow.com/a/7293345. That is,
		// the type flags from BS_PUSHBUTTON to BS_OWNERDRAW are mutually exclusive and the
		// values in that range can have multiple bits set. So, checking that a single bit
		// is set isn't going to work.
		if ((style & BS_TYPEMASK) == BS_AUTOCHECKBOX)
		{
			buttonType = DarkModeButton::ButtonType::Checkbox;
		}
		else if ((style & BS_TYPEMASK) == BS_AUTORADIOBUTTON)
		{
			buttonType = DarkModeButton::ButtonType::Radio;
		}
		else
		{
			break;
		}

		DarkModeButton::DrawButtonText(customDraw, buttonType);
	}
		return CDRF_SKIPDEFAULT;
	}

	return CDRF_DODEFAULT;
}

LRESULT DarkModeThemeManager::OnToolbarCustomDraw(NMTBCUSTOMDRAW *customDraw)
{
	switch (customDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		customDraw->clrText = DarkModeHelper::TEXT_COLOR;
		customDraw->clrHighlightHotTrack = DarkModeHelper::BUTTON_HIGHLIGHT_COLOR;
		return TBCDRF_USECDCOLORS | TBCDRF_HILITEHOTTRACK;
	}

	return CDRF_DODEFAULT;
}

LRESULT CALLBACK DarkModeThemeManager::ComboBoxExSubclass(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam, UINT_PTR subclassId, DWORD_PTR data)
{
	UNREFERENCED_PARAMETER(data);

	switch (msg)
	{
	case WM_CTLCOLOREDIT:
	{
		auto hdc = reinterpret_cast<HDC>(wParam);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, DarkModeHelper::TEXT_COLOR);
		return reinterpret_cast<LRESULT>(GetComboBoxExBackgroundBrush());
	}
	break;

	case WM_NCDESTROY:
	{
		[[maybe_unused]] auto res = RemoveWindowSubclass(hwnd, ComboBoxExSubclass, subclassId);
		assert(res);
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

HBRUSH DarkModeThemeManager::GetComboBoxExBackgroundBrush()
{
	static wil::unique_hbrush backgroundBrush;

	if (!backgroundBrush)
	{
		backgroundBrush.reset(CreateSolidBrush(COMBO_BOX_EX_DARK_MODE_BACKGROUND_COLOR));
	}

	return backgroundBrush.get();
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

LRESULT CALLBACK DarkModeThemeManager::RebarSubclass(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam, UINT_PTR subclassId, DWORD_PTR data)
{
	UNREFERENCED_PARAMETER(data);

	switch (msg)
	{
	case WM_ERASEBKGND:
	{
		auto hdc = reinterpret_cast<HDC>(wParam);

		RECT rc;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, DarkModeHelper::GetInstance().GetBackgroundBrush());

		return 1;
	}
	break;

	case WM_NCDESTROY:
	{
		[[maybe_unused]] auto res = RemoveWindowSubclass(hwnd, RebarSubclass, subclassId);
		assert(res);
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK DarkModeThemeManager::GroupBoxSubclass(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam, UINT_PTR subclassId, DWORD_PTR data)
{
	UNREFERENCED_PARAMETER(data);

	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT rect;
		GetClientRect(hwnd, &rect);

		std::wstring text = GetWindowString(hwnd);
		assert(!text.empty());

		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, DarkModeHelper::TEXT_COLOR);

		auto &dpiCompat = DpiCompatibility::GetInstance();

		NONCLIENTMETRICS metrics;
		metrics.cbSize = sizeof(metrics);
		dpiCompat.SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0,
			dpiCompat.GetDpiForWindow(hwnd));

		wil::unique_hfont captionFont(CreateFontIndirect(&metrics.lfCaptionFont));
		wil::unique_select_object object(SelectObject(hdc, captionFont.get()));

		RECT textRect = rect;
		DrawText(hdc, text.c_str(), static_cast<int>(text.size()), &textRect, DT_CALCRECT);

		// The top border of the group box isn't anchored to the top of the control. Rather, it cuts
		// midway through the caption text. That is, the text is anchored to the top of the control
		// and the border starts at the vertical mid-point of the text.
		RECT groupBoxRect = rect;
		groupBoxRect.top += GetRectHeight(&textRect) / 2;

		wil::unique_htheme theme(OpenThemeData(nullptr, L"BUTTON"));

		// The group box border isn't shown behind the caption; instead, the text appears as if its
		// drawn directly on top of the parent.
		DrawThemeBackground(theme.get(), hdc, BP_GROUPBOX, GBS_NORMAL, &groupBoxRect, &ps.rcPaint);

		// It appears this offset isn't DPI-adjusted.
		OffsetRect(&textRect, 9, 0);

		// As with the above, it appears this offset isn't DPI-adjusted.
		RECT textBackgroundRect = textRect;
		textBackgroundRect.left -= 2;
		DrawThemeParentBackground(hwnd, hdc, &textBackgroundRect);

		DrawText(hdc, text.c_str(), static_cast<int>(text.size()), &textRect, DT_LEFT);

		EndPaint(hwnd, &ps);
	}
		return 0;

	case WM_NCDESTROY:
	{
		[[maybe_unused]] auto res = RemoveWindowSubclass(hwnd, GroupBoxSubclass, subclassId);
		assert(res);
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}
