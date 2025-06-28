// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellNavigationController.h"
#include "BrowserWindow.h"
#include "NavigationEvents.h"
#include "NavigationManager.h"
#include "NavigationRequest.h"
#include "PreservedHistoryEntry.h"
#include "../Helper/ShellHelper.h"

ShellNavigationController::ShellNavigationController(const ShellBrowser *shellBrowser,
	BrowserWindow *browser, NavigationManager *navigationManager,
	NavigationEvents *navigationEvents, const PidlAbsolute &initialPidl) :
	m_browser(browser),
	m_navigationManager(navigationManager)
{
	Initialize(shellBrowser, navigationEvents);

	AddEntry(
		std::make_unique<HistoryEntry>(initialPidl, HistoryEntry::InitialNavigationType::Initial));
}

ShellNavigationController::ShellNavigationController(const ShellBrowser *shellBrowser,
	BrowserWindow *browser, NavigationManager *navigationManager,
	NavigationEvents *navigationEvents,
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries, int currentEntry) :
	NavigationController(CopyPreservedHistoryEntries(preservedEntries), currentEntry),
	m_browser(browser),
	m_navigationManager(navigationManager)
{
	Initialize(shellBrowser, navigationEvents);
}

void ShellNavigationController::Initialize(const ShellBrowser *shellBrowser,
	NavigationEvents *navigationEvents)
{
	m_connections.push_back(navigationEvents->AddCommittedObserver(
		std::bind_front(&ShellNavigationController::OnNavigationCommitted, this),
		NavigationEventScope::ForShellBrowser(*shellBrowser), boost::signals2::at_front,
		SlotGroup::HighestPriority));
}

std::vector<std::unique_ptr<HistoryEntry>> ShellNavigationController::CopyPreservedHistoryEntries(
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries)
{
	std::vector<std::unique_ptr<HistoryEntry>> entries;

	for (const auto &preservedEntry : preservedEntries)
	{
		auto entry = std::make_unique<HistoryEntry>(preservedEntry->GetPidl());
		entries.push_back(std::move(entry));
	}

	return entries;
}

void ShellNavigationController::OnNavigationCommitted(const NavigationRequest *request)
{
	auto historyEntryType = request->GetNavigateParams().historyEntryType;

	auto *currentEntry = GetCurrentEntry();

	// If the current entry is the initial entry, it should be replaced, regardless of the requested
	// history entry type.
	if (currentEntry->IsInitialEntry())
	{
		// If the current entry is the initial entry, that should be the only entry.
		DCHECK_EQ(GetNumHistoryEntries(), 1);

		historyEntryType = HistoryEntryType::ReplaceCurrentEntry;
	}

	// If navigating to a history entry, the current index will be set here. It's important this is
	// done before attempting to replace the current entry.
	if (request->GetNavigateParams().historyEntryId)
	{
		auto *entry = GetEntryById(*request->GetNavigateParams().historyEntryId);

		if (entry)
		{
			auto index = GetIndexOfEntry(entry);
			SetCurrentIndex(*index);

			// The current entry has changed, so retrieve it again here.
			currentEntry = GetCurrentEntry();
		}
		else if (historyEntryType == HistoryEntryType::ReplaceCurrentEntry
			&& !currentEntry->IsInitialEntry())
		{
			// The history entry can't be replaced if it doesn't exist, so add a new entry instead.
			historyEntryType = HistoryEntryType::AddEntry;
		}
	}

	if (historyEntryType == HistoryEntryType::AddEntry
		|| historyEntryType == HistoryEntryType::ReplaceCurrentEntry)
	{
		auto entry = std::make_unique<HistoryEntry>(request->GetNavigateParams().pidl);

		if (historyEntryType == HistoryEntryType::AddEntry)
		{
			AddEntry(std::move(entry));
		}
		else
		{
			// When an entry is replaced, the set of selected items should be retained.
			entry->SetSelectedItems(currentEntry->GetSelectedItems());

			ReplaceCurrentEntry(std::move(entry));
		}
	}
}

bool ShellNavigationController::CanGoUp() const
{
	auto *currentEntry = GetCurrentEntry();
	return !IsNamespaceRoot(currentEntry->GetPidl().Raw());
}

void ShellNavigationController::GoUp()
{
	auto *currentEntry = GetCurrentEntry();

	unique_pidl_absolute pidlParent;
	HRESULT hr = GetVirtualParentPath(currentEntry->GetPidl().Raw(), wil::out_param(pidlParent));

	if (FAILED(hr))
	{
		return;
	}

	auto navigateParams = NavigateParams::Up(pidlParent.get(), currentEntry->GetPidl().Raw());
	Navigate(navigateParams);
}

void ShellNavigationController::Refresh()
{
	auto *currentEntry = GetCurrentEntry();
	auto navigateParams = NavigateParams::History(currentEntry);
	Navigate(navigateParams);
}

void ShellNavigationController::Navigate(const HistoryEntry *entry)
{
	auto navigateParams = NavigateParams::History(entry);
	Navigate(navigateParams);
}

void ShellNavigationController::Navigate(const std::wstring &path)
{
	unique_pidl_absolute pidlDirectory;
	HRESULT hr = ParseDisplayNameForNavigation(path.c_str(), pidlDirectory);

	if (FAILED(hr))
	{
		return;
	}

	auto navigateParams = NavigateParams::Normal(pidlDirectory.get());
	Navigate(navigateParams);
}

void ShellNavigationController::Navigate(NavigateParams &navigateParams)
{
	auto *currentEntry = GetCurrentEntry();
	HistoryEntry *targetEntry = nullptr;

	if (navigateParams.navigationType == NavigationType::History)
	{
		targetEntry = GetEntryById(*navigateParams.historyEntryId);
	}
	else
	{
		targetEntry = currentEntry;
	}

	// Navigations to the current directory should be treated as an implicit refresh and proceed in
	// the current tab, regardless of whether or not the tab is locked. The only exception is a
	// navigation to a history entry, except the current history entry (navigating to the current
	// history entry is an explicit refresh).
	if (targetEntry == currentEntry && currentEntry->GetPidl() == navigateParams.pidl)
	{
		navigateParams.historyEntryType = HistoryEntryType::ReplaceCurrentEntry;
		navigateParams.overrideNavigationTargetMode = true;
	}

	if (m_navigationTargetMode == NavigationTargetMode::ForceNewTab
		&& !currentEntry->IsInitialEntry() && !navigateParams.overrideNavigationTargetMode)
	{
		m_browser->OpenItem(navigateParams.pidl.Raw(), OpenFolderDisposition::ForegroundTab);
		return;
	}

	m_navigationManager->StartNavigation(navigateParams);
}

void ShellNavigationController::SetNavigationTargetMode(NavigationTargetMode navigationTargetMode)
{
	m_navigationTargetMode = navigationTargetMode;
}

NavigationTargetMode ShellNavigationController::GetNavigationTargetMode() const
{
	return m_navigationTargetMode;
}

HistoryEntry *ShellNavigationController::GetEntryById(int id)
{
	for (int i = 0; i < GetNumHistoryEntries(); i++)
	{
		auto *entry = GetEntryAtIndex(i);

		if (entry->GetId() == id)
		{
			return entry;
		}
	}

	return nullptr;
}
