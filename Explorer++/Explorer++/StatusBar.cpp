// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "DarkModeHelper.h"
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

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (darkModeHelper.IsDarkModeEnabled())
	{
		darkModeHelper.AllowDarkModeForWindow(m_hStatusBar, true);
		SetWindowTheme(m_hStatusBar, nullptr, L"ExplorerStatusBar");
	}
}

void Explorerplusplus::SetStatusBarParts(int width)
{
	int parts[3];

	parts[0] = (int) (0.50 * width);
	parts[1] = (int) (0.75 * width);
	parts[2] = width;

	SendMessage(m_hStatusBar, SB_SETPARTS, 3, (LPARAM) parts);
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

		TCHAR szBuffer[512];
		LoadString(m_resourceInstance, LOWORD(wParam), szBuffer, SIZEOF_ARRAY(szBuffer));
		SetWindowText(m_hStatusBar, szBuffer);
	}

	return 0;
}

void Explorerplusplus::OnNavigationStartedStatusBar(const Tab &tab,
	const NavigateParams &navigateParams)
{
	if (m_tabContainer->IsTabSelected(tab))
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

	if (m_tabContainer->IsTabSelected(tab))
	{
		UpdateStatusBarText(tab);
	}
}

void Explorerplusplus::OnNavigationFailedStatusBar(const Tab &tab,
	const NavigateParams &navigateParams)
{
	UNREFERENCED_PARAMETER(navigateParams);

	if (m_tabContainer->IsTabSelected(tab))
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
