// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/NavigationRequest.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainerImpl.h"
#include "../Helper/Controls.h"
#include "../Helper/WindowHelper.h"
#include <fmt/format.h>
#include <fmt/xchar.h>

void Explorerplusplus::CreateStatusBar()
{
	UINT style = WS_CHILD | WS_CLIPSIBLINGS | SBARS_SIZEGRIP | WS_CLIPCHILDREN;

	if (m_config->showStatusBar)
	{
		style |= WS_VISIBLE;
	}

	m_hStatusBar = ::CreateStatusBar(m_hContainer, style);
	m_pStatusBar = new StatusBar(m_hStatusBar);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hStatusBar,
		std::bind_front(&Explorerplusplus::StatusBarSubclass, this)));

	SetStatusBarParts();

	m_statusBarFontSetter = std::make_unique<MainFontSetter>(m_hStatusBar, m_config);
	m_statusBarFontSetter->fontUpdatedSignal.AddObserver([this]() { UpdateStatusBarMinHeight(); });

	// Even if the status bar uses the default font, the height won't necessarily be correct. As
	// detailed below, if the text size is increased in Windows, the height of the status bar won't
	// change at 96 DPI. Therefore, setting the minimum height here ensures that the status bar is
	// sized correctly in that situation.
	UpdateStatusBarMinHeight();
}

LRESULT Explorerplusplus::StatusBarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		SetStatusBarParts();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void Explorerplusplus::SetStatusBarParts()
{
	RECT clientRect;
	BOOL res = GetClientRect(m_hStatusBar, &clientRect);
	CHECK(res);

	int width = GetRectWidth(&clientRect);

	int parts[] = { static_cast<int>(0.50 * width), static_cast<int>(0.75 * width), -1 };
	auto setPartsRes =
		SendMessage(m_hStatusBar, SB_SETPARTS, std::size(parts), reinterpret_cast<LPARAM>(parts));
	DCHECK(setPartsRes);
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

		if (WI_AreAllFlagsClear(HIWORD(wParam), MF_POPUP | MF_SEPARATOR))
		{
			HMENU menu = reinterpret_cast<HMENU>(lParam);
			UINT menuItemId = LOWORD(wParam);

			helperText = m_getMenuItemHelperTextSignal(menu, menuItemId);

			if (!helperText)
			{
				helperText =
					ResourceHelper::MaybeLoadString(m_app->GetResourceInstance(), menuItemId);
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

void Explorerplusplus::OnNavigationStartedStatusBar(const NavigationRequest *request)
{
	const auto *tab = request->GetShellBrowser()->GetTab();
	UpdateStatusBarText(*tab);
}

void Explorerplusplus::SetStatusBarLoadingText(PCIDLIST_ABSOLUTE pidl)
{
	std::wstring displayName;
	HRESULT hr = GetDisplayName(pidl, SHGDN_INFOLDER, displayName);

	if (FAILED(hr))
	{
		return;
	}

	std::wstring loadingTemplate =
		ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_GENERAL_LOADING);
	std::wstring loadingText =
		fmt::format(fmt::runtime(loadingTemplate), fmt::arg(L"folder_name", displayName));

	/* Browsing of a folder has started. Set the status bar text to indicate that
	the folder is being loaded. */
	SendMessage(m_hStatusBar, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(loadingText.c_str()));

	/* Clear the text in all other parts of the status bar. */
	SendMessage(m_hStatusBar, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(L""));
	SendMessage(m_hStatusBar, SB_SETTEXT, 2, reinterpret_cast<LPARAM>(L""));
}

void Explorerplusplus::OnNavigationFailedStatusBar(const NavigationRequest *request)
{
	const auto *tab = request->GetShellBrowser()->GetTab();
	UpdateStatusBarText(*tab);
}

void Explorerplusplus::OnNavigationCancelledStatusBar(const NavigationRequest *request)
{
	const auto *tab = request->GetShellBrowser()->GetTab();
	UpdateStatusBarText(*tab);
}

void Explorerplusplus::OnNavigationsStoppedStatusBar(const ShellBrowser *shellBrowser)
{
	// All pending navigations have been stopped, so it's possible there are no longer any active
	// navigations, in which case, the status bar text will need to be updated.
	const auto *tab = shellBrowser->GetTab();
	UpdateStatusBarText(*tab);
}

void Explorerplusplus::UpdateStatusBarText(const Tab &tab)
{
	if (auto *navigation = tab.GetShellBrowser()->MaybeGetLatestActiveNavigation())
	{
		// In this case, there is at least one active navigation in progress, so the status bar
		// should reflect that.
		SetStatusBarLoadingText(navigation->GetNavigateParams().pidl.Raw());
		return;
	}

	int numItemsSelected = tab.GetShellBrowserImpl()->GetNumSelected();
	std::wstring numItemsText;

	// The item count that's shown will either be the number of items selected, or the total number
	// of items if there is no selection.
	if (numItemsSelected != 0)
	{
		if (numItemsSelected == 1)
		{
			numItemsText = ResourceHelper::LoadString(m_app->GetResourceInstance(),
				IDS_GENERAL_SELECTED_ONE_ITEM);
		}
		else
		{
			auto multipleItemsText = ResourceHelper::LoadString(m_app->GetResourceInstance(),
				IDS_GENERAL_SELECTED_MULTIPLE_ITEMS);
			numItemsText = std::format(L"{:L} {}", numItemsSelected, multipleItemsText);
		}
	}
	else
	{
		int numItems = tab.GetShellBrowserImpl()->GetNumItems();

		if (numItems == 1)
		{
			numItemsText =
				ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_GENERAL_ONE_ITEM);
		}
		else
		{
			auto multipleItemsText = ResourceHelper::LoadString(m_app->GetResourceInstance(),
				IDS_GENERAL_MULTIPLE_ITEMS);
			numItemsText = std::format(L"{:L} {}", numItems, multipleItemsText);
		}
	}

	if (tab.GetShellBrowserImpl()->IsFilterApplied())
	{
		auto filterAppliedText =
			ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_FILTER_APPLIED);
		numItemsText += L" | " + filterAppliedText;
	}

	SendMessage(m_hStatusBar, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(numItemsText.c_str()));

	std::wstring sizeText;

	if (numItemsSelected == 0)
	{
		auto displayFormat = m_config->globalFolderSettings.forceSize
			? m_config->globalFolderSettings.sizeDisplayFormat
			: +SizeDisplayFormat::None;
		sizeText =
			FormatSizeString(tab.GetShellBrowserImpl()->GetTotalDirectorySize(), displayFormat);
	}
	else
	{
		// Note that no size will be shown if only folders are selected.
		if (tab.GetShellBrowserImpl()->GetNumSelectedFiles() != 0)
		{
			auto displayFormat = m_config->globalFolderSettings.forceSize
				? m_config->globalFolderSettings.sizeDisplayFormat
				: +SizeDisplayFormat::None;
			sizeText =
				FormatSizeString(tab.GetShellBrowserImpl()->GetSelectionSize(), displayFormat);
		}
	}

	SendMessage(m_hStatusBar, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(sizeText.c_str()));

	std::wstring driveFreeSpaceText =
		CreateDriveFreeSpaceString(tab.GetShellBrowserImpl()->GetDirectory().c_str());

	SendMessage(m_hStatusBar, SB_SETTEXT, 2, reinterpret_cast<LPARAM>(driveFreeSpaceText.c_str()));
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
		ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_GENERAL_FREE),
		totalNumberOfFreeBytes.QuadPart * 100.0 / totalNumberOfBytes.QuadPart);
}
