// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellNavigationController.h"
#include "PreservedHistoryEntry.h"
#include "ShellNavigator.h"
#include "TabNavigationInterface.h"
#include "../Helper/ShellHelper.h"

ShellNavigationController::ShellNavigationController(ShellNavigator *navigator,
	TabNavigationInterface *tabNavigation) :
	m_navigator(navigator),
	m_tabNavigation(tabNavigation)
{
	Initialize();
}

ShellNavigationController::ShellNavigationController(ShellNavigator *navigator,
	TabNavigationInterface *tabNavigation,
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries, int currentEntry) :
	NavigationController(CopyPreservedHistoryEntries(preservedEntries), currentEntry),
	m_navigator(navigator),
	m_tabNavigation(tabNavigation)
{
	Initialize();
}

void ShellNavigationController::Initialize()
{
	m_connections.emplace_back(m_navigator->AddNavigationStartedObserver(
		std::bind_front(&ShellNavigationController::OnNavigationStarted, this),
		boost::signals2::at_front));
	m_connections.emplace_back(m_navigator->AddNavigationCommittedObserver(
		std::bind_front(&ShellNavigationController::OnNavigationCommitted, this),
		boost::signals2::at_front));
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

HRESULT ShellNavigationController::GoToOffset(int offset)
{
	auto entry = GetEntry(offset);

	if (!entry)
	{
		return E_FAIL;
	}

	return Navigate(entry);
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

HRESULT ShellNavigationController::GoUp()
{
	auto *currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return E_FAIL;
	}

	unique_pidl_absolute pidlParent;
	HRESULT hr = GetVirtualParentPath(currentEntry->GetPidl().Raw(), wil::out_param(pidlParent));

	if (FAILED(hr))
	{
		return hr;
	}

	auto navigateParams = NavigateParams::Up(pidlParent.get(), currentEntry->GetPidl().Raw());
	return Navigate(navigateParams);
}

HRESULT ShellNavigationController::Refresh()
{
	auto *currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return E_FAIL;
	}

	auto navigateParams = NavigateParams::History(currentEntry);
	return Navigate(navigateParams);
}

HRESULT ShellNavigationController::Navigate(const HistoryEntry *entry)
{
	auto navigateParams = NavigateParams::History(entry);
	return Navigate(navigateParams);
}

HRESULT ShellNavigationController::Navigate(const std::wstring &path)
{
	unique_pidl_absolute pidlDirectory;
	RETURN_IF_FAILED(ParseDisplayNameForNavigation(path.c_str(), pidlDirectory));

	auto navigateParams = NavigateParams::Normal(pidlDirectory.get());
	return Navigate(navigateParams);
}

HRESULT ShellNavigationController::Navigate(NavigateParams &navigateParams)
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
		navigateParams.overrideNavigationMode = true;
	}

	if (m_navigationMode == NavigationMode::ForceNewTab && currentEntry
		&& !navigateParams.overrideNavigationMode)
	{
		m_tabNavigation->CreateNewTab(navigateParams, true);
		return S_OK;
	}

	return m_navigator->Navigate(navigateParams);
}

HRESULT ShellNavigationController::GetFailureValue()
{
	return E_FAIL;
}

void ShellNavigationController::SetNavigationMode(NavigationMode navigationMode)
{
	m_navigationMode = navigationMode;
}

NavigationMode ShellNavigationController::GetNavigationMode() const
{
	return m_navigationMode;
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
