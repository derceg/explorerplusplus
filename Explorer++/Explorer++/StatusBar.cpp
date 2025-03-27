// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "StatusBar.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabEvents.h"
#include "../Helper/Controls.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <wil/common.h>
#include <wil/resource.h>
#include <format>

StatusBar *StatusBar::Create(HWND parent, const BrowserWindow *browser, const Config *config,
	TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
	NavigationEvents *navigationEvents, const ResourceLoader *resourceLoader)
{
	return new StatusBar(parent, browser, config, tabEvents, shellBrowserEvents, navigationEvents,
		resourceLoader);
}

StatusBar::StatusBar(HWND parent, const BrowserWindow *browser, const Config *config,
	TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
	NavigationEvents *navigationEvents, const ResourceLoader *resourceLoader) :
	m_hwnd(CreateStatusBar(parent, WS_CHILD | WS_CLIPSIBLINGS | SBARS_SIZEGRIP | WS_CLIPCHILDREN)),
	m_browser(browser),
	m_config(config),
	m_fontSetter(m_hwnd, config),
	m_resourceLoader(resourceLoader)
{
	SetParts();

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hwnd,
		std::bind_front(&StatusBar::StatusBarSubclass, this)));

	m_fontSetter.fontUpdatedSignal.AddObserver(std::bind_front(&StatusBar::UpdateMinHeight, this));

	m_connections.push_back(tabEvents->AddSelectedObserver(
		std::bind_front(&StatusBar::OnTabSelected, this), TabEventScope::ForBrowser(*browser)));

	m_connections.push_back(shellBrowserEvents->AddItemsChangedObserver(
		std::bind_front(&StatusBar::OnDirectoryContentsChanged, this),
		ShellBrowserEventScope::ForActiveShellBrowser(*browser)));
	m_connections.push_back(shellBrowserEvents->AddSelectionChangedObserver(
		std::bind_front(&StatusBar::OnListViewSelectionChanged, this),
		ShellBrowserEventScope::ForActiveShellBrowser(*browser)));

	m_connections.push_back(navigationEvents->AddStartedObserver(
		std::bind_front(&StatusBar::UpdateTextForNavigation, this),
		NavigationEventScope::ForActiveShellBrowser(*browser)));
	m_connections.push_back(navigationEvents->AddCommittedObserver(
		std::bind_front(&StatusBar::UpdateTextForNavigation, this),
		NavigationEventScope::ForActiveShellBrowser(*browser)));
	m_connections.push_back(navigationEvents->AddFailedObserver(
		std::bind_front(&StatusBar::UpdateTextForNavigation, this),
		NavigationEventScope::ForActiveShellBrowser(*browser)));
	m_connections.push_back(navigationEvents->AddCancelledObserver(
		std::bind_front(&StatusBar::UpdateTextForNavigation, this),
		NavigationEventScope::ForActiveShellBrowser(*browser)));
	m_connections.push_back(navigationEvents->AddStoppedObserver(
		std::bind_front(&StatusBar::OnNavigationsStopped, this),
		NavigationEventScope::ForActiveShellBrowser(*browser)));

	// Even if the status bar uses the default font, the height won't necessarily be correct. As
	// detailed below, if the text size is increased in Windows, the height of the status bar won't
	// change at 96 DPI. Therefore, setting the minimum height here ensures that the status bar is
	// sized correctly in that situation.
	UpdateMinHeight();
}

void StatusBar::SetParts()
{
	RECT clientRect;
	BOOL res = GetClientRect(m_hwnd, &clientRect);
	CHECK(res);

	int width = GetRectWidth(&clientRect);

	int parts[] = { static_cast<int>(0.50 * width), static_cast<int>(0.75 * width), -1 };
	auto setPartsRes =
		SendMessage(m_hwnd, SB_SETPARTS, std::size(parts), reinterpret_cast<LPARAM>(parts));
	DCHECK(setPartsRes);
}

void StatusBar::UpdateMinHeight()
{
	auto hdc = wil::GetDC(m_hwnd);

	auto font = reinterpret_cast<HFONT>(SendMessage(m_hwnd, WM_GETFONT, 0, 0));
	wil::unique_select_object selectFont;

	if (font)
	{
		selectFont = wil::SelectObject(hdc.get(), font);
	}

	wil::unique_htheme theme(OpenThemeData(m_hwnd, L"Status"));
	DCHECK(theme);

	TEXTMETRIC textMetrics;
	HRESULT hr = GetThemeTextMetrics(theme.get(), hdc.get(), 0, 0, &textMetrics);
	DCHECK(SUCCEEDED(hr));

	// From looking into precisely what the status bar control does when it receives a WM_SETFONT
	// message, it appears that it does recalculate its height. However, for whatever reason, the
	// font that's passed to WM_SETFONT will only be used as part of the calculation if the DPI is
	// greater than 96. Otherwise, the default DC font will be used for the calculation.
	//
	// That means that, confusingly, if the DPI is greater than 96, WM_SETFONT will work as expected
	// - the font will be set and the control size will be correctly updated. However, if the DPI is
	// 96, the font will be set, but the control size won't change.
	//
	// The same issue can also be seen when increasing the text size in Windows. Doing that will
	// cause the status bar to use the larger font, but the height of the status bar won't change,
	// leading to the text being potentially cut off.
	//
	// Therefore, to work around those sorts of issues, the minimum height is set here. Note that
	// the height set here will be ignored if it's smaller than the text height calculated by the
	// control. That means that, at 96 DPI, the height will be ignored if the font that's set is
	// smaller than the default font.
	//
	// That shouldn't really be a problem, though, since the control will simply be a bit larger
	// than necessary.
	SendMessage(m_hwnd, SB_SETMINHEIGHT, textMetrics.tmHeight, 0);

	// As per the documentation for SB_SETMINHEIGHT, this will redraw the window.
	SendMessage(m_hwnd, WM_SIZE, 0, 0);
}

LRESULT StatusBar::StatusBarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		SetParts();
		break;

	case WM_NCDESTROY:
		OnNcDestroy();
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void StatusBar::OnTabSelected(const Tab &tab)
{
	UpdateText(tab);
}

void StatusBar::OnDirectoryContentsChanged(const ShellBrowser *shellBrowser)
{
	const auto *tab = shellBrowser->GetTab();
	UpdateText(*tab);
}

void StatusBar::OnListViewSelectionChanged(const ShellBrowser *shellBrowser)
{
	const auto *tab = shellBrowser->GetTab();
	UpdateText(*tab);
}

void StatusBar::UpdateTextForNavigation(const NavigationRequest *request)
{
	const auto *tab = request->GetShellBrowser()->GetTab();
	UpdateText(*tab);
}

void StatusBar::OnNavigationsStopped(const ShellBrowser *shellBrowser)
{
	const auto *tab = shellBrowser->GetTab();
	UpdateText(*tab);
}

HWND StatusBar::GetHWND() const
{
	return m_hwnd;
}

void StatusBar::OnMenuSelect(HMENU menu, UINT itemId, UINT flags)
{
	if (flags == 0xFFFF && menu == nullptr)
	{
		OnMenuClose();
	}
	else
	{
		OnMenuItemSelected(menu, itemId, flags);
	}
}

void StatusBar::OnMenuClose()
{
	const auto *tab = m_browser->GetActiveShellBrowser()->GetTab();
	SetParts();
	UpdateText(*tab);
}

void StatusBar::OnMenuItemSelected(HMENU menu, UINT itemId, UINT flags)
{
	int parts[] = { -1 };
	auto res = SendMessage(m_hwnd, SB_SETPARTS, std::size(parts), reinterpret_cast<LPARAM>(parts));
	DCHECK(res);

	std::optional<std::wstring> helpText;

	if (WI_AreAllFlagsClear(flags, MF_POPUP | MF_SEPARATOR))
	{
		helpText = m_browser->RequestMenuHelpText(menu, itemId);
	}

	if (helpText)
	{
		SetWindowText(m_hwnd, helpText->c_str());
	}
	else
	{
		SetWindowText(m_hwnd, L"");
	}
}

void StatusBar::UpdateText(const Tab &tab)
{
	if (auto *navigation = tab.GetShellBrowser()->MaybeGetLatestActiveNavigation())
	{
		// In this case, there is at least one active navigation in progress, so the status bar
		// should reflect that.
		SetLoadingText(navigation->GetNavigateParams().pidl.Raw());
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
			numItemsText = m_resourceLoader->LoadString(IDS_GENERAL_SELECTED_ONE_ITEM);
		}
		else
		{
			auto multipleItemsText =
				m_resourceLoader->LoadString(IDS_GENERAL_SELECTED_MULTIPLE_ITEMS);
			numItemsText = std::format(L"{:L} {}", numItemsSelected, multipleItemsText);
		}
	}
	else
	{
		int numItems = tab.GetShellBrowserImpl()->GetNumItems();

		if (numItems == 1)
		{
			numItemsText = m_resourceLoader->LoadString(IDS_GENERAL_ONE_ITEM);
		}
		else
		{
			auto multipleItemsText = m_resourceLoader->LoadString(IDS_GENERAL_MULTIPLE_ITEMS);
			numItemsText = std::format(L"{:L} {}", numItems, multipleItemsText);
		}
	}

	if (tab.GetShellBrowserImpl()->IsFilterApplied())
	{
		auto filterAppliedText = m_resourceLoader->LoadString(IDS_FILTER_APPLIED);
		numItemsText += L" | " + filterAppliedText;
	}

	SendMessage(m_hwnd, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(numItemsText.c_str()));

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

	SendMessage(m_hwnd, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(sizeText.c_str()));

	std::wstring driveFreeSpaceText =
		CreateDriveFreeSpaceString(tab.GetShellBrowserImpl()->GetDirectory().c_str());
	SendMessage(m_hwnd, SB_SETTEXT, 2, reinterpret_cast<LPARAM>(driveFreeSpaceText.c_str()));
}

void StatusBar::SetLoadingText(PCIDLIST_ABSOLUTE pidl)
{
	std::wstring displayName;
	HRESULT hr = GetDisplayName(pidl, SHGDN_INFOLDER, displayName);

	if (FAILED(hr))
	{
		return;
	}

	std::wstring loadingTemplate = m_resourceLoader->LoadString(IDS_GENERAL_LOADING);
	std::wstring loadingText =
		fmt::format(fmt::runtime(loadingTemplate), fmt::arg(L"folder_name", displayName));
	SendMessage(m_hwnd, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(loadingText.c_str()));
	SendMessage(m_hwnd, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(L""));
	SendMessage(m_hwnd, SB_SETTEXT, 2, reinterpret_cast<LPARAM>(L""));
}

std::wstring StatusBar::CreateDriveFreeSpaceString(const std::wstring &path)
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
		m_resourceLoader->LoadString(IDS_GENERAL_FREE),
		totalNumberOfFreeBytes.QuadPart * 100.0 / totalNumberOfBytes.QuadPart);
}

void StatusBar::OnNcDestroy()
{
	delete this;
}
