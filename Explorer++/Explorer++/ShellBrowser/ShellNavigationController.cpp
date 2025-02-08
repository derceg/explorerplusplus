// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellNavigationController.h"
#include "NavigationManager.h"
#include "PreservedHistoryEntry.h"
#include "TabNavigationInterface.h"
#include "../Helper/ShellHelper.h"

ShellNavigationController::ShellNavigationController(NavigationManager *navigationManager,
	TabNavigationInterface *tabNavigation) :
	m_navigationManager(navigationManager),
	m_tabNavigation(tabNavigation)
{
	Initialize();
}

ShellNavigationController::ShellNavigationController(NavigationManager *navigationManager,
	TabNavigationInterface *tabNavigation,
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries, int currentEntry) :
	NavigationController(CopyPreservedHistoryEntries(preservedEntries), currentEntry),
	m_navigationManager(navigationManager),
	m_tabNavigation(tabNavigation)
{
	Initialize();
}

void ShellNavigationController::Initialize()
{
	m_connections.emplace_back(m_navigationManager->AddNavigationStartedObserver(
		std::bind_front(&ShellNavigationController::OnNavigationStarted, this),
		boost::signals2::at_front, NavigationManager::SlotGroup::HighPriority));
	m_connections.emplace_back(m_navigationManager->AddNavigationCommittedObserver(
		std::bind_front(&ShellNavigationController::OnNavigationCommitted, this),
		boost::signals2::at_front, NavigationManager::SlotGroup::HighPriority));
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

void ShellNavigationController::OnNavigationStarted(const NavigateParams &navigateParams)
{
	if (!GetCurrentEntry())
	{
		// There is no current entry, so this is the very first navigation. An initial entry should
		// be added.
		AddEntry(std::make_unique<HistoryEntry>(navigateParams.pidl.Raw(),
			HistoryEntry::InitialNavigationType::Initial));
	}
}

void ShellNavigationController::OnNavigationCommitted(const NavigateParams &navigateParams)
{
	auto historyEntryType = navigateParams.historyEntryType;

	// An initial entry should always be added when a navigation starts, so there should always be a
	// current entry at this point.
	auto *currentEntry = GetCurrentEntry();
	CHECK(currentEntry);

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
	if (navigateParams.historyEntryId)
	{
		auto *entry = GetEntryById(*navigateParams.historyEntryId);

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
		auto entry = std::make_unique<HistoryEntry>(navigateParams.pidl.Raw());

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

	if (!currentEntry)
	{
		return false;
	}

	return !IsNamespaceRoot(currentEntry->GetPidl().Raw());
}

void ShellNavigationController::GoUp()
{
	auto *currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return;
	}

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

	if (!currentEntry)
	{
		return;
	}

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
	auto currentEntry = GetCurrentEntry();
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
	if (currentEntry && targetEntry == currentEntry
		&& currentEntry->GetPidl() == navigateParams.pidl)
	{
		navigateParams.historyEntryType = HistoryEntryType::ReplaceCurrentEntry;
		navigateParams.overrideNavigationTargetMode = true;
	}

	if (m_navigationTargetMode == NavigationTargetMode::ForceNewTab && currentEntry
		&& !navigateParams.overrideNavigationTargetMode)
	{
		m_tabNavigation->CreateNewTab(navigateParams, true);
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
