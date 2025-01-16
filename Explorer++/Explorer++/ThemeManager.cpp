// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
// clang-format off
#include "ThemeManager.h"
// clang-format on
#include "Explorer++.h"
#include "DarkModeManager.h"
#include "SystemFontHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <glog/logging.h>
#include <wil/resource.h>
#include <vssym32.h>

ThemeManager::ThemeManager(DarkModeManager *darkModeManager) : m_darkModeManager(darkModeManager)
{
	m_connections.push_back(darkModeManager->darkModeStatusChanged.AddObserver(
		std::bind(&ThemeManager::OnDarkModeStatusChanged, this)));
}

void ThemeManager::OnDarkModeStatusChanged()
{
	m_windowSubclasses.clear();

	for (HWND hwnd : m_trackedTopLevelWindows)
	{
		ApplyThemeToWindowAndChildren(hwnd);
	}
}

void ThemeManager::TrackTopLevelWindow(HWND hwnd)
{
	ApplyThemeToWindowAndChildren(hwnd);

	[[maybe_unused]] auto insertionResult = m_trackedTopLevelWindows.insert(hwnd);
	assert(insertionResult.second);
}

void ThemeManager::UntrackTopLevelWindow(HWND hwnd)
{
	[[maybe_unused]] auto numErased = m_trackedTopLevelWindows.erase(hwnd);
	assert(numErased == 1);
}

void ThemeManager::ApplyThemeToWindowAndChildren(HWND hwnd)
{
	ApplyThemeToWindow(hwnd);
	EnumChildWindows(
		hwnd,
		[](HWND childWindow, LPARAM lParam)
		{
			auto themeManager = reinterpret_cast<ThemeManager *>(lParam);
			return themeManager->ProcessChildWindow(childWindow);
		},
		reinterpret_cast<LPARAM>(this));

	// Tooltip windows won't be enumerated by EnumChildWindows(). They will, however, be enumerated
	// by EnumThreadWindows(), which is why that's called here.
	// Note that this is explicitly called after EnumChildWindows(). That way, tooltip windows can
	// be initialized during the call to EnumChildWindows() (since they won't necessarily exist
	// initially). Those tooltip windows will then be processed as part of the call to
	// EnumThreadWindows().
	EnumThreadWindows(
		GetCurrentThreadId(),
		[](HWND threadWindow, LPARAM lParam)
		{
			auto themeManager = reinterpret_cast<ThemeManager *>(lParam);
			return themeManager->ProcessThreadWindow(threadWindow);
		},
		reinterpret_cast<LPARAM>(this));

	RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_FRAME);
}

BOOL ThemeManager::ProcessChildWindow(HWND hwnd)
{
	ApplyThemeToWindow(hwnd);
	return TRUE;
}

BOOL ThemeManager::ProcessThreadWindow(HWND hwnd)
{
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

void ThemeManager::ApplyThemeToWindow(HWND hwnd)
{
	bool enableDarkMode = m_darkModeManager->IsDarkModeEnabled();
	m_darkModeManager->AllowDarkModeForWindow(hwnd, enableDarkMode);

	// The maximum length of a class name is 256 characters (see the documentation for lpszClassName
	// in https://learn.microsoft.com/en-au/windows/win32/api/winuser/ns-winuser-wndclassw).
	WCHAR className[256];
	auto res = GetClassName(hwnd, className, static_cast<int>(std::size(className)));

	if (res == 0)
	{
		assert(false);
		return;
	}

	if (lstrcmp(className, Explorerplusplus::WINDOW_CLASS_NAME) == 0)
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
		ApplyThemeToHeader(hwnd);
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
		ApplyThemeToComboBox(hwnd);
	}
	else if (lstrcmp(className, WC_BUTTON) == 0)
	{
		ApplyThemeToButton(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, TOOLTIPS_CLASS) == 0)
	{
		ApplyThemeToTooltips(hwnd);
	}
	else if (lstrcmp(className, STATUSCLASSNAME) == 0)
	{
		ApplyThemeToStatusBar(hwnd, enableDarkMode);
	}
	else if (lstrcmp(className, WC_SCROLLBAR) == 0)
	{
		ApplyThemeToScrollBar(hwnd, enableDarkMode);
	}
}

void ThemeManager::ApplyThemeToMainWindow(HWND hwnd, bool enableDarkMode)
{
	// There's no need to owner-draw the menu bar if dark mode isn't supported (in practice, this
	// means that the menu bar will only be owner-drawn on Windows 10 and 11). Additionally,
	// owner-drawing the menu bar is problematic on Windows 7, for at least two reasons:
	//
	// 1. In the default theme, the menu bar background isn't a flat color. Rather, it's more like a
	// gradient that's specified in a bitmap. DrawThemeBackground() can be used to easily draw that
	// background, however, it appears that Windows only allows you to set the background brush for
	// the menu bar, rather than providing a DC to paint into.
	// 2. Visual styles can be turned off, so the current owner-drawing implementation wouldn't work
	// in that scenario.
	if (!m_darkModeManager->IsDarkModeSupported())
	{
		return;
	}

	BOOL dark = enableDarkMode;
	DarkModeManager::WINDOWCOMPOSITIONATTRIBDATA compositionData = {
		DarkModeManager::WCA_USEDARKMODECOLORS, &dark, sizeof(dark)
	};
	m_darkModeManager->SetWindowCompositionAttribute(hwnd, &compositionData);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(hwnd,
		std::bind_front(&ThemeManager::MainWindowSubclass, this)));

	auto mainMenu = GetMenu(hwnd);
	int numItems = GetMenuItemCount(mainMenu);

	for (int i = 0; i < numItems; i++)
	{
		MENUITEMINFO menuItemInfo = {};
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_FTYPE;
		[[maybe_unused]] auto res = GetMenuItemInfo(mainMenu, i, true, &menuItemInfo);
		assert(res);

		// Removing the MFT_OWNERDRAW style once it's been applied to the menu bar items appears to
		// be problematic. The resulting items aren't spaced correctly. Because of that, the menu
		// bar will always be owner-drawn, regardless of the theme.
		WI_SetFlag(menuItemInfo.fType, MFT_OWNERDRAW);
		WI_SetFlag(menuItemInfo.fMask, MIIM_DATA);
		menuItemInfo.dwItemData = i;
		res = SetMenuItemInfo(mainMenu, i, true, &menuItemInfo);
		assert(res);
	}

	// Turning on owner draw for at least one item in the menu bar will disable visual styles in the
	// bar. That's useful here, as (1) the items will be oner-drawn, so the presence or absence of
	// visual styles doesn't matter and (2) disabling visual styles means that the background brush
	// here will be used to paint the empty section of the bar (the section behind each item will be
	// painted when drawing the item).
	// Note that the background should really be drawn by DrawThemeBackground(), however, as noted
	// above, Windows seemingly doesn't provide any documented way of drawing directly into the menu
	// bar DC. The most you can do is provide a background brush. That's fine on Windows 10 and
	// 11, where the background colors are effectively flat and a solid brush works.
	MENUINFO menuInfo = {};
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_BACKGROUND;
	menuInfo.hbrBack = GetMenuBarBackgroundBrush(enableDarkMode);
	[[maybe_unused]] auto res = SetMenuInfo(mainMenu, &menuInfo);
	assert(res);
}

void ThemeManager::ApplyThemeToDialog(HWND hwnd, bool enableDarkMode)
{
	BOOL dark = enableDarkMode;
	DarkModeManager::WINDOWCOMPOSITIONATTRIBDATA compositionData = {
		DarkModeManager::WCA_USEDARKMODECOLORS, &dark, sizeof(dark)
	};
	m_darkModeManager->SetWindowCompositionAttribute(hwnd, &compositionData);

	if (enableDarkMode)
	{
		m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(hwnd,
			std::bind_front(&ThemeManager::DialogSubclass, this)));
	}
}

void ThemeManager::ApplyThemeToListView(HWND hwnd, bool enableDarkMode)
{
	DWORD extendedStyle = ListView_GetExtendedListViewStyle(hwnd);

	if (enableDarkMode)
	{
		SetWindowTheme(hwnd, L"ItemsView", nullptr);
	}
	else
	{
		SetWindowTheme(hwnd, L"Explorer", nullptr);
	}

	if (WI_IsFlagSet(extendedStyle, LVS_EX_TRANSPARENTBKGND))
	{
		// Setting the window theme above will clear the LVS_EX_TRANSPARENTBKGND style. So, if that
		// style was set, it will be restored here.
		ListView_SetExtendedListViewStyle(hwnd, extendedStyle);
	}

	COLORREF backgroundColor;
	COLORREF textColor;

	if (enableDarkMode)
	{
		backgroundColor = DarkModeManager::BACKGROUND_COLOR;
		textColor = DarkModeManager::TEXT_COLOR;
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
		m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(hwnd,
			std::bind_front(&ThemeManager::ListViewSubclass, this)));
	}
}

void ThemeManager::ApplyThemeToHeader(HWND hwnd)
{
	SetWindowTheme(hwnd, L"ItemsView", nullptr);
}

void ThemeManager::ApplyThemeToTreeView(HWND hwnd, bool enableDarkMode)
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
		backgroundColor = DarkModeManager::BACKGROUND_COLOR;
		textColor = DarkModeManager::TEXT_COLOR;
		insertMarkColor = DarkModeManager::FOREGROUND_COLOR;
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

void ThemeManager::ApplyThemeToRichEdit(HWND hwnd, bool enableDarkMode)
{
	COLORREF backgroundColor;
	COLORREF textColor;

	if (enableDarkMode)
	{
		backgroundColor = DarkModeManager::BACKGROUND_COLOR;
		textColor = DarkModeManager::TEXT_COLOR;
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

void ThemeManager::ApplyThemeToRebar(HWND hwnd, bool enableDarkMode)
{
	if (enableDarkMode)
	{
		m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(hwnd,
			std::bind_front(&ThemeManager::RebarSubclass, this)));
	}
}

void ThemeManager::ApplyThemeToToolbar(HWND hwnd, bool enableDarkMode)
{
	COLORREF insertMarkColor;

	if (enableDarkMode)
	{
		insertMarkColor = DarkModeManager::FOREGROUND_COLOR;
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
		// Note that the parent window may end up being subclassed multiple times. That shouldn't
		// have any correctness issues, since when receiving the relevant drawing messages, one of
		// the subclasses (it's not specified which) will perform the appropriate handling. It is
		// inefficient generally, since each subclass will be invoked for other messages as well.
		// That shouldn't be too much of an issue, since there's only a limited number of toolbars,
		// so the number of extraneous subclasses won't be very high.
		m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(parent,
			std::bind_front(&ThemeManager::ToolbarParentSubclass, this)));
	}
}

void ThemeManager::ApplyThemeToComboBoxEx(HWND hwnd, bool enableDarkMode)
{
	if (enableDarkMode)
	{
		m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(hwnd,
			std::bind_front(&ThemeManager::ComboBoxExSubclass, this)));
	}
}

void ThemeManager::ApplyThemeToComboBox(HWND hwnd)
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

void ThemeManager::ApplyThemeToButton(HWND hwnd, bool enableDarkMode)
{
	SetWindowTheme(hwnd, L"Explorer", nullptr);

	auto style = GetWindowLongPtr(hwnd, GWL_STYLE);

	if ((style & BS_TYPEMASK) == BS_GROUPBOX)
	{
		if (enableDarkMode)
		{
			m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(hwnd,
				std::bind_front(&ThemeManager::GroupBoxSubclass, this)));
		}
	}
}

void ThemeManager::ApplyThemeToTooltips(HWND hwnd)
{
	SetWindowTheme(hwnd, L"Explorer", nullptr);
}

void ThemeManager::ApplyThemeToStatusBar(HWND hwnd, bool enableDarkMode)
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

void ThemeManager::ApplyThemeToScrollBar(HWND hwnd, bool enableDarkMode)
{
	auto style = GetWindowLongPtr(hwnd, GWL_STYLE);

	if (WI_IsAnyFlagSet(style, SBS_SIZEGRIP | SBS_SIZEBOX))
	{
		if (enableDarkMode)
		{
			m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(hwnd,
				std::bind_front(&ThemeManager::ScrollBarSubclass, this)));
		}
	}
}

LRESULT ThemeManager::MainWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static wil::unique_htheme theme(OpenThemeData(hwnd, L"Menu"));
	static wil::unique_hbrush hotBrush(CreateSolidBrush(DarkModeManager::HOT_ITEM_HIGHLIGHT_COLOR));
	static constexpr DWORD drawFlagsBase = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
	static bool alwaysShowAccessKeys = ShouldAlwaysShowAccessKeys();

	switch (msg)
	{
	case WM_MEASUREITEM:
	{
		auto *measureItem = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);

		if (measureItem->CtlType != ODT_MENU)
		{
			break;
		}

		auto hdc = wil::GetWindowDC(hwnd);

		// Only items in the menu bar are owner-drawn, so the menu will always be the menu
		// associated with the window.
		auto menu = GetMenu(hwnd);
		assert(menu);

		auto text =
			MenuHelper::GetMenuItemString(menu, static_cast<UINT>(measureItem->itemData), true);

		auto logFont = GetSystemFontScaledToWindow(SystemFont::Menu, hwnd);
		wil::unique_hfont font(CreateFontIndirect(&logFont));
		wil::unique_select_object selectFont;

		if (font)
		{
			selectFont = wil::SelectObject(hdc.get(), font.get());
		}

		assert(selectFont);

		RECT textRect;
		[[maybe_unused]] HRESULT hr = GetThemeTextExtent(theme.get(), hdc.get(), MENU_BARITEM,
			MBI_NORMAL, text.c_str(), -1, drawFlagsBase, nullptr, &textRect);
		assert(SUCCEEDED(hr));

		measureItem->itemWidth = GetRectWidth(&textRect);
		measureItem->itemHeight = GetRectHeight(&textRect);

		return TRUE;
	}
	break;

	// This contains just enough functionality to owner-draw the items in the menu bar; it's not a
	// complete owner-drawn menu implementation. For example, neither checked nor bitmap items have
	// any handling. The code here implicitly assumes that the only menu items that are owner-drawn
	// are the items in the menu bar.
	case WM_DRAWITEM:
	{
		auto *drawItem = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);

		if (drawItem->CtlType != ODT_MENU)
		{
			break;
		}

		int itemState;

		if (WI_IsAnyFlagSet(drawItem->itemState, ODS_INACTIVE | ODS_GRAYED | ODS_DISABLED))
		{
			if (WI_IsFlagSet(drawItem->itemState, ODS_HOTLIGHT))
			{
				itemState = MBI_DISABLEDHOT;
			}
			else if (WI_IsFlagSet(drawItem->itemState, ODS_SELECTED))
			{
				itemState = MBI_DISABLEDPUSHED;
			}
			else
			{
				itemState = MBI_DISABLED;
			}
		}
		else
		{
			if (WI_IsFlagSet(drawItem->itemState, ODS_HOTLIGHT))
			{
				itemState = MBI_HOT;
			}
			else if (WI_IsFlagSet(drawItem->itemState, ODS_SELECTED))
			{
				itemState = MBI_PUSHED;
			}
			else
			{
				itemState = MBI_NORMAL;
			}
		}

		bool darkModeEnabled = m_darkModeManager->IsDarkModeEnabled();
		bool selected = false;
		bool selectionPartiallyTransparent = false;

		if (itemState == MBI_HOT || itemState == MBI_PUSHED || itemState == MBI_DISABLEDHOT
			|| itemState == MBI_DISABLEDPUSHED)
		{
			selected = true;

			if (!darkModeEnabled)
			{
				selectionPartiallyTransparent =
					IsThemeBackgroundPartiallyTransparent(theme.get(), MENU_BARITEM, itemState);
			}
		}

		// The hot item selection rectangle drawn by DrawThemeBackground() may be partially
		// transparent. In that case, the menu bar background will need to be drawn first. In dark
		// mode, the background color and selection color are both opaque, so there's no need to
		// draw both for a single item.
		if (!selected || selectionPartiallyTransparent)
		{
			// In non-dark mode, the background could be drawn by using:
			//
			// DrawThemeBackground(theme.get(), drawItem->hDC, MENU_BARBACKGROUND, MB_ACTIVE,
			//   &drawItem->rcItem, nullptr);
			//
			// However, if that background isn't simply a flat color, the background underneath
			// each item will differ from the background shown in the empty space. In Windows
			// 11, for example, the themed background includes a 1px border at the bottom, so
			// drawing the themed background here would result in a slight difference. It's also
			// possible the theme could change in the future. So, the safest thing to do is to
			// use the same brush to draw the bar background and the item background.
			FillRect(drawItem->hDC, &drawItem->rcItem, GetMenuBarBackgroundBrush(darkModeEnabled));
		}

		if (selected)
		{
			if (darkModeEnabled)
			{
				FillRect(drawItem->hDC, &drawItem->rcItem, hotBrush.get());
			}
			else
			{
				DrawThemeBackground(theme.get(), drawItem->hDC, MENU_BARITEM, itemState,
					&drawItem->rcItem, nullptr);
			}
		}

		// As per the documentation for DRAWITEMSTRUCT, this is the menu handle when a menu is being
		// drawn.
		auto menu = reinterpret_cast<HMENU>(drawItem->hwndItem);
		auto text =
			MenuHelper::GetMenuItemString(menu, static_cast<UINT>(drawItem->itemData), true);

		DWORD drawFlags = drawFlagsBase;

		// It appears that Windows passes in the ODS_NOACCEL flag even when the access keys option
		// is turned on in settings. Therefore, if that setting is on, the ODS_NOACCEL flag will be
		// ignored.
		if (!alwaysShowAccessKeys && WI_IsFlagSet(drawItem->itemState, ODS_NOACCEL))
		{
			WI_SetFlag(drawFlags, DT_HIDEPREFIX);
		}

		DTTOPTS options = {};
		options.dwSize = sizeof(options);

		if (darkModeEnabled)
		{
			WI_SetFlag(options.dwFlags, DTT_TEXTCOLOR);
			COLORREF textColor;

			if (itemState == MBI_DISABLED || itemState == MBI_DISABLEDHOT
				|| itemState == MBI_DISABLEDPUSHED)
			{
				textColor = DarkModeManager::TEXT_COLOR_DISABLED;
			}
			else
			{
				textColor = DarkModeManager::TEXT_COLOR;
			}

			options.crText = textColor;
		}

		[[maybe_unused]] HRESULT hr = DrawThemeTextEx(theme.get(), drawItem->hDC, MENU_BARITEM,
			itemState, text.c_str(), -1, drawFlags, &drawItem->rcItem, &options);
		assert(SUCCEEDED(hr));

		return TRUE;
	}
	break;

	case WM_THEMECHANGED:
		theme.reset(OpenThemeData(hwnd, L"Menu"));
		break;

	case WM_SETTINGCHANGE:
		if (wParam == SPI_SETKEYBOARDCUES)
		{
			alwaysShowAccessKeys = ShouldAlwaysShowAccessKeys();

			RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
		}
		break;

	// A 1px border will be drawn under the menu bar, in a color specified by the current theme. In
	// dark mode, the application will draw with custom colors, that aren't derived from the theme.
	// That then means that the	border can look out of place (e.g. it might be drawn in a light
	// color when the rest of the application is dark). To fix that, the border will be painted over
	// here.
	// This is done in both WM_NCPAINT and WM_NCACTIVATE, since painting over the border in
	// WM_NCPAINT only will cause it to reappear whenever the window loses focus.
	// Also see https://github.com/notepad-plus-plus/notepad-plus-plus/pull/9985.
	case WM_NCPAINT:
	case WM_NCACTIVATE:
	{
		if (!m_darkModeManager->IsDarkModeEnabled())
		{
			break;
		}

		auto defWindowProcResult = DefWindowProc(hwnd, msg, wParam, lParam);

		RECT windowRect;
		[[maybe_unused]] auto res = GetWindowRect(hwnd, &windowRect);
		assert(res);

		MENUBARINFO barInfo = {};
		barInfo.cbSize = sizeof(barInfo);
		res = GetMenuBarInfo(hwnd, OBJID_MENU, 0, &barInfo);
		assert(res);

		// The border is drawn directly underneath the menu bar.
		RECT menuBarBorderRect = { barInfo.rcBar.left, barInfo.rcBar.bottom, barInfo.rcBar.right,
			barInfo.rcBar.bottom + 1 };
		OffsetRect(&menuBarBorderRect, -windowRect.left, -windowRect.top);

		auto hdc = wil::GetWindowDC(hwnd);
		FillRect(hdc.get(), &menuBarBorderRect, m_darkModeManager->GetBackgroundBrush());

		return defWindowProcResult;
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

HBRUSH ThemeManager::GetMenuBarBackgroundBrush(bool enableDarkMode)
{
	if (enableDarkMode)
	{
		return m_darkModeManager->GetBackgroundBrush();
	}
	else
	{
		int systemColorIndex;

		if (DarkModeManager::IsHighContrast())
		{
			systemColorIndex = COLOR_BTNFACE;
		}
		else
		{
			systemColorIndex = COLOR_WINDOW;
		}

		return GetSysColorBrush(systemColorIndex);
	}
}

bool ThemeManager::ShouldAlwaysShowAccessKeys()
{
	BOOL alwaysShow;
	BOOL res = SystemParametersInfo(SPI_GETKEYBOARDCUES, 0, &alwaysShow, 0);

	if (!res)
	{
		assert(false);
		return false;
	}

	return alwaysShow;
}

LRESULT ThemeManager::DialogSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORBTN:
	{
		auto hdc = reinterpret_cast<HDC>(wParam);
		SetBkColor(hdc, DarkModeManager::BACKGROUND_COLOR);
		SetTextColor(hdc, DarkModeManager::TEXT_COLOR);
		return reinterpret_cast<LRESULT>(m_darkModeManager->GetBackgroundBrush());
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
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT ThemeManager::ToolbarParentSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT ThemeManager::OnCustomDraw(NMCUSTOMDRAW *customDraw)
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

LRESULT ThemeManager::OnButtonCustomDraw(NMCUSTOMDRAW *customDraw)
{
	switch (customDraw->dwDrawStage)
	{
	case CDDS_PREPAINT:
	{
		auto style = GetWindowLongPtr(customDraw->hdr.hwndFrom, GWL_STYLE);

		// The size of the interactive element of the control (i.e. the check box or radio button
		// part).
		SIZE elementSize;

		// Although the documentation
		// (https://learn.microsoft.com/en-au/windows/win32/controls/button-styles#constants)
		// states that BS_TYPEMASK is out of date and shouldn't be used, it's necessary
		// here, for the reasons discussed in https://stackoverflow.com/a/7293345. That is,
		// the type flags from BS_PUSHBUTTON to BS_OWNERDRAW are mutually exclusive and the
		// values in that range can have multiple bits set. So, checking that a single bit
		// is set isn't going to work.
		if ((style & BS_TYPEMASK) == BS_AUTOCHECKBOX)
		{
			elementSize = GetCheckboxSize(customDraw->hdr.hwndFrom);
		}
		else if ((style & BS_TYPEMASK) == BS_AUTORADIOBUTTON)
		{
			elementSize = GetRadioButtonSize(customDraw->hdr.hwndFrom);
		}
		else
		{
			break;
		}

		// Also applies to radio buttons.
		constexpr int CHECKBOX_TEXT_SPACING_96DPI = 3;

		UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(customDraw->hdr.hwndFrom);

		RECT textRect = customDraw->rc;
		textRect.left +=
			elementSize.cx + MulDiv(CHECKBOX_TEXT_SPACING_96DPI, dpi, USER_DEFAULT_SCREEN_DPI);

		std::wstring text = GetWindowString(customDraw->hdr.hwndFrom);
		assert(!text.empty());

		COLORREF textColor;

		if (IsWindowEnabled(customDraw->hdr.hwndFrom))
		{
			textColor = DarkModeManager::TEXT_COLOR;
		}
		else
		{
			textColor = DarkModeManager::TEXT_COLOR_DISABLED;
		}

		SetTextColor(customDraw->hdc, textColor);

		UINT textFormat = DT_LEFT;

		if (!WI_IsFlagSet(customDraw->uItemState, CDIS_SHOWKEYBOARDCUES))
		{
			WI_SetFlag(textFormat, DT_HIDEPREFIX);
		}

		RECT finalTextRect = textRect;
		DrawText(customDraw->hdc, text.c_str(), static_cast<int>(text.size()), &finalTextRect,
			textFormat | DT_CALCRECT);

		if (GetRectHeight(&finalTextRect) < GetRectHeight(&textRect))
		{
			textRect.top += (GetRectHeight(&textRect) - GetRectHeight(&finalTextRect)) / 2;
		}

		DrawText(customDraw->hdc, text.c_str(), static_cast<int>(text.size()), &textRect,
			textFormat);

		if (WI_IsFlagSet(customDraw->uItemState, CDIS_FOCUS))
		{
			DrawFocusRect(customDraw->hdc, &textRect);
		}

		// TODO: May also need to handle CDIS_DISABLED and CDIS_GRAYED.
	}
		return CDRF_SKIPDEFAULT;
	}

	return CDRF_DODEFAULT;
}

LRESULT ThemeManager::OnToolbarCustomDraw(NMTBCUSTOMDRAW *customDraw)
{
	switch (customDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		customDraw->clrText = DarkModeManager::TEXT_COLOR;
		customDraw->clrHighlightHotTrack = DarkModeManager::HOT_ITEM_HIGHLIGHT_COLOR;
		return TBCDRF_USECDCOLORS | TBCDRF_HILITEHOTTRACK;
	}

	return CDRF_DODEFAULT;
}

LRESULT ThemeManager::ComboBoxExSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CTLCOLOREDIT:
	{
		auto hdc = reinterpret_cast<HDC>(wParam);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, DarkModeManager::TEXT_COLOR);
		return reinterpret_cast<LRESULT>(GetComboBoxExBackgroundBrush());
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

HBRUSH ThemeManager::GetComboBoxExBackgroundBrush()
{
	static wil::unique_hbrush backgroundBrush;

	if (!backgroundBrush)
	{
		backgroundBrush.reset(CreateSolidBrush(COMBO_BOX_EX_DARK_MODE_BACKGROUND_COLOR));
	}

	return backgroundBrush.get();
}

LRESULT ThemeManager::ListViewSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
				SetTextColor(customDraw->hdc, DarkModeManager::TEXT_COLOR);
				return CDRF_NEWFONT;
			}
		}
		break;
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT ThemeManager::RebarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
	{
		auto hdc = reinterpret_cast<HDC>(wParam);

		RECT rc;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, m_darkModeManager->GetBackgroundBrush());

		return 1;
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT ThemeManager::GroupBoxSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
		SetTextColor(hdc, DarkModeManager::TEXT_COLOR);

		auto font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
		wil::unique_select_object selectFont;

		if (font)
		{
			selectFont = wil::SelectObject(hdc, font);
		}

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

		// When the system draws the group box, it appears appears that the constant below (8) is
		// embedded within the code, rather than being retrieved dynamically.
		int xBorder = GetSystemMetrics(SM_CXBORDER);
		int xEdge = GetSystemMetrics(SM_CXEDGE);
		OffsetRect(&textRect, 8 - xBorder + xEdge, 0);

		RECT textBackgroundRect = textRect;
		InflateRect(&textBackgroundRect, xEdge, 0);
		DrawThemeParentBackground(hwnd, hdc, &textBackgroundRect);

		DrawText(hdc, text.c_str(), static_cast<int>(text.size()), &textRect, DT_LEFT);

		EndPaint(hwnd, &ps);
	}
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT ThemeManager::ScrollBarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		int partId = SBP_SIZEBOX;
		int stateId;

		auto style = GetWindowLongPtr(hwnd, GWL_STYLE);

		if (WI_IsFlagSet(style, SBS_SIZEBOXTOPLEFTALIGN))
		{
			stateId = SZB_TOPLEFTALIGN;
		}
		else
		{
			stateId = SZB_RIGHTALIGN;
		}

		RECT rect;
		GetClientRect(hwnd, &rect);

		wil::unique_htheme theme(OpenThemeData(hwnd, L"SCROLLBAR"));

		if (IsThemeBackgroundPartiallyTransparent(theme.get(), partId, stateId))
		{
			DrawThemeParentBackground(hwnd, hdc, &ps.rcPaint);
		}

		DrawThemeBackground(theme.get(), hdc, partId, stateId, &rect, &ps.rcPaint);

		EndPaint(hwnd, &ps);
	}
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}
