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
#include "StatusBarView.h"
#include "TabEvents.h"
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <wil/common.h>
#include <wil/resource.h>
#include <format>

StatusBar *StatusBar::Create(StatusBarView *view, const BrowserWindow *browser,
	const Config *config, TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
	NavigationEvents *navigationEvents, const ResourceLoader *resourceLoader)
{
	return new StatusBar(view, browser, config, tabEvents, shellBrowserEvents, navigationEvents,
		resourceLoader);
}

StatusBar::StatusBar(StatusBarView *view, const BrowserWindow *browser, const Config *config,
	TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
	NavigationEvents *navigationEvents, const ResourceLoader *resourceLoader) :
	m_view(view),
	m_browser(browser),
	m_config(config),
	m_resourceLoader(resourceLoader)
{
	SetStandardParts();

	m_view->windowDestroyedSignal.AddObserver(std::bind_front(&StatusBar::OnWindowDestroyed, this));

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
}

void StatusBar::SetStandardParts()
{
	m_view->SetParts({ 50, 75, 100 });
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

StatusBarView *StatusBar::GetView()
{
	return m_view;
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
	SetStandardParts();
	UpdateText(*tab);
}

void StatusBar::OnMenuItemSelected(HMENU menu, UINT itemId, UINT flags)
{
	m_view->SetParts({ 100 });

	std::optional<std::wstring> helpText;

	if (WI_AreAllFlagsClear(flags, MF_POPUP | MF_SEPARATOR))
	{
		helpText = m_browser->RequestMenuHelpText(menu, itemId);
	}

	if (helpText)
	{
		m_view->SetPartText(0, *helpText);
	}
	else
	{
		m_view->SetPartText(0, L"");
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

	m_view->SetPartText(0, numItemsText);

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

	m_view->SetPartText(1, sizeText);

	std::wstring driveFreeSpaceText =
		CreateDriveFreeSpaceString(tab.GetShellBrowserImpl()->GetDirectory().c_str());
	m_view->SetPartText(2, driveFreeSpaceText);
}

void StatusBar::SetLoadingText(PCIDLIST_ABSOLUTE pidl)
{
	std::wstring loadingTemplate = m_resourceLoader->LoadString(IDS_GENERAL_LOADING);
	std::wstring loadingText = fmt::format(fmt::runtime(loadingTemplate),
		fmt::arg(L"folder_name", GetDisplayNameWithFallback(pidl, SHGDN_INFOLDER)));
	m_view->SetPartText(0, loadingText);
	m_view->SetPartText(1, L"");
	m_view->SetPartText(2, L"");
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

void StatusBar::OnWindowDestroyed()
{
	delete this;
}
