// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/WindowHelper.h"

void Explorerplusplus::CreateStatusBar()
{
	UINT style = WS_CHILD | WS_CLIPSIBLINGS | SBARS_SIZEGRIP | WS_CLIPCHILDREN;

	if (m_config->showStatusBar)
	{
		style |= WS_VISIBLE;
	}

	m_hStatusBar = ::CreateStatusBar(m_hContainer, style);
	m_pStatusBar = new StatusBar(m_hStatusBar);

	int width = 0;

	RECT rc;
	BOOL res = GetWindowRect(m_hContainer, &rc);

	if (res)
	{
		width = GetRectWidth(&rc);
	}

	SetStatusBarParts(width);

	m_statusBarFontSetter = std::make_unique<MainFontSetter>(m_hStatusBar, m_config.get());
	m_statusBarFontSetter->fontUpdatedSignal.AddObserver([this]() { UpdateStatusBarMinHeight(); });

	// Even if the status bar uses the default font, the height won't necessarily be correct. As
	// detailed below, if the text size is increased in Windows, the height of the status bar won't
	// change at 96 DPI. Therefore, setting the minimum height here ensures that the status bar is
	// sized correctly in that situation.
	UpdateStatusBarMinHeight();
}

void Explorerplusplus::SetStatusBarParts(int width)
{
	int parts[3];

	parts[0] = (int) (0.50 * width);
	parts[1] = (int) (0.75 * width);
	parts[2] = width;

	SendMessage(m_hStatusBar, SB_SETPARTS, 3, (LPARAM) parts);
}

// This function should be called whenever the font is updated for the status bar control.
void Explorerplusplus::UpdateStatusBarMinHeight()
{
	auto hdc = wil::GetDC(m_hStatusBar);

	auto font = reinterpret_cast<HFONT>(SendMessage(m_hStatusBar, WM_GETFONT, 0, 0));
	wil::unique_select_object selectFont;

	if (font)
	{
		selectFont = wil::SelectObject(hdc.get(), font);
	}

	wil::unique_htheme theme(OpenThemeData(m_hStatusBar, L"Status"));
	assert(theme);

	TEXTMETRIC textMetrics;
	[[maybe_unused]] HRESULT hr = GetThemeTextMetrics(theme.get(), hdc.get(), 0, 0, &textMetrics);
	assert(SUCCEEDED(hr));

	// From looking into precisely what the status bar control does when it receives a WM_SETFONT
	// message, it appears that it does recalculate its height. However, for whatever reason, the
	// font that's passed to WM_SETFONT will only be used as part of the calculation if the DPI is
	// greater than 96. Otherwise, the default DC font will be used for the calculation.
	// That means that, confusingly, if the DPI is greater than 96, WM_SETFONT will work as expected
	// - the font will be set and the control size will be correctly updated. However, if the DPI is
	// 96, the font will be set, but the control size won't change.
	// The same issue can also be seen when increasing the text size in Windows. Doing that will
	// cause the status bar to use the larger font, but the height of the status bar won't change,
	// leading to the text being potentially cut off.
	// Therefore, to work around those sorts of issues, the minimum height is set here.
	// Note that the height set here will be ignored if it's smaller than the text height calculated
	// by the control. That means that, at 96 DPI, the height will be ignored if the font that's set
	// is smaller than the default font.
	// That shouldn't really be a problem, though, since the control will simply be a bit larger
	// than necessary.
	SendMessage(m_hStatusBar, SB_SETMINHEIGHT, textMetrics.tmHeight, 0);

	// As per the documentation for SB_SETMINHEIGHT, this will redraw the window.
	SendMessage(m_hStatusBar, WM_SIZE, 0, 0);
}

LRESULT Explorerplusplus::StatusBarMenuSelect(WPARAM wParam, LPARAM lParam)
{
	/* Is the menu being closed? .*/
	if (HIWORD(wParam) == 0xFFFF && lParam == 0)
	{
		m_pStatusBar->HandleStatusBarMenuClose();
	}
	else
	{
		m_pStatusBar->HandleStatusBarMenuOpen();

		std::optional<std::wstring> helperText;

		if (WI_IsFlagClear(HIWORD(wParam), MF_POPUP))
		{
			HMENU menu = reinterpret_cast<HMENU>(lParam);
			int menuItemId = LOWORD(wParam);

			helperText = m_getMenuItemHelperTextSignal(menu, menuItemId);

			if (!helperText)
			{
				helperText = ResourceHelper::MaybeLoadString(m_resourceInstance, menuItemId);
			}
		}

		if (helperText)
		{
			SetWindowText(m_hStatusBar, helperText->c_str());
		}
		else
		{
			SetWindowText(m_hStatusBar, L"");
		}
	}

	return 0;
}

void Explorerplusplus::OnNavigationStartedStatusBar(const Tab &tab,
	const NavigateParams &navigateParams)
{
	if (GetActivePane()->GetTabContainer()->IsTabSelected(tab))
	{
		SetStatusBarLoadingText(navigateParams.pidl.Raw());
	}
}

void Explorerplusplus::SetStatusBarLoadingText(PCIDLIST_ABSOLUTE pidl)
{
	std::wstring displayName;
	HRESULT hr = GetDisplayName(pidl, SHGDN_INFOLDER, displayName);

	if (FAILED(hr))
	{
		return;
	}

	TCHAR szTemp[64];
	TCHAR szLoadingText[512];
	LoadString(m_resourceInstance, IDS_GENERAL_LOADING, szTemp, SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szLoadingText, SIZEOF_ARRAY(szLoadingText), szTemp, displayName.c_str());

	/* Browsing of a folder has started. Set the status bar text to indicate that
	the folder is being loaded. */
	SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM) szLoadingText);

	/* Clear the text in all other parts of the status bar. */
	SendMessage(m_hStatusBar, SB_SETTEXT, 1, (LPARAM) EMPTY_STRING);
	SendMessage(m_hStatusBar, SB_SETTEXT, 2, (LPARAM) EMPTY_STRING);
}

void Explorerplusplus::OnNavigationCompletedStatusBar(const Tab &tab,
	const NavigateParams &navigateParams)
{
	UNREFERENCED_PARAMETER(navigateParams);

	if (GetActivePane()->GetTabContainer()->IsTabSelected(tab))
	{
		UpdateStatusBarText(tab);
	}
}

void Explorerplusplus::OnNavigationFailedStatusBar(const Tab &tab,
	const NavigateParams &navigateParams)
{
	UNREFERENCED_PARAMETER(navigateParams);

	if (GetActivePane()->GetTabContainer()->IsTabSelected(tab))
	{
		UpdateStatusBarText(tab);
	}
}

HRESULT Explorerplusplus::UpdateStatusBarText(const Tab &tab)
{
	int numItemsSelected = tab.GetShellBrowser()->GetNumSelected();
	std::wstring numItemsText;

	// The item count that's shown will either be the number of items selected, or the total number
	// of items if there is no selection.
	if (numItemsSelected != 0)
	{
		if (numItemsSelected == 1)
		{
			numItemsText =
				ResourceHelper::LoadString(m_resourceInstance, IDS_GENERAL_SELECTED_ONE_ITEM);
		}
		else
		{
			auto multipleItemsText =
				ResourceHelper::LoadString(m_resourceInstance, IDS_GENERAL_SELECTED_MULTIPLE_ITEMS);
			numItemsText = std::format(L"{:L} {}", numItemsSelected, multipleItemsText);
		}
	}
	else
	{
		int numItems = tab.GetShellBrowser()->GetNumItems();

		if (numItems == 1)
		{
			numItemsText = ResourceHelper::LoadString(m_resourceInstance, IDS_GENERAL_ONE_ITEM);
		}
		else
		{
			auto multipleItemsText =
				ResourceHelper::LoadString(m_resourceInstance, IDS_GENERAL_MULTIPLE_ITEMS);
			numItemsText = std::format(L"{:L} {}", numItems, multipleItemsText);
		}
	}

	if (tab.GetShellBrowser()->IsFilterApplied())
	{
		auto filterAppliedText = ResourceHelper::LoadString(m_resourceInstance, IDS_FILTER_APPLIED);
		numItemsText += L" | " + filterAppliedText;
	}

	SendMessage(m_hStatusBar, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(numItemsText.c_str()));

	std::wstring sizeText;

	if (numItemsSelected == 0)
	{
		SizeDisplayFormat displayFormat = m_config->globalFolderSettings.forceSize
			? m_config->globalFolderSettings.sizeDisplayFormat
			: SizeDisplayFormat::None;
		sizeText = FormatSizeString(tab.GetShellBrowser()->GetTotalDirectorySize(), displayFormat);
	}
	else
	{
		// Note that no size will be shown if only folders are selected.
		if (tab.GetShellBrowser()->GetNumSelectedFiles() != 0)
		{
			SizeDisplayFormat displayFormat = m_config->globalFolderSettings.forceSize
				? m_config->globalFolderSettings.sizeDisplayFormat
				: SizeDisplayFormat::None;
			sizeText = FormatSizeString(tab.GetShellBrowser()->GetSelectionSize(), displayFormat);
		}
	}

	SendMessage(m_hStatusBar, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(sizeText.c_str()));

	std::wstring driveFreeSpaceText =
		CreateDriveFreeSpaceString(tab.GetShellBrowser()->GetDirectory().c_str());

	SendMessage(m_hStatusBar, SB_SETTEXT, 2, reinterpret_cast<LPARAM>(driveFreeSpaceText.c_str()));

	return S_OK;
}

std::wstring Explorerplusplus::CreateDriveFreeSpaceString(const std::wstring &path)
{
	ULARGE_INTEGER totalNumberOfBytes;
	ULARGE_INTEGER totalNumberOfFreeBytes;
	ULARGE_INTEGER bytesAvailableToCaller;
	if (GetDiskFreeSpaceEx(path.c_str(), &bytesAvailableToCaller, &totalNumberOfBytes,
			&totalNumberOfFreeBytes)
		== 0)
	{
		return {};
	}

	return std::format(L"{} {} ({:.0Lf}%)", FormatSizeString(totalNumberOfFreeBytes.QuadPart),
		ResourceHelper::LoadString(m_resourceInstance, IDS_GENERAL_FREE),
		totalNumberOfFreeBytes.QuadPart * 100.0 / totalNumberOfBytes.QuadPart);
}
