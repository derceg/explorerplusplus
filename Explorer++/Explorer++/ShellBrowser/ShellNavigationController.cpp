// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellNavigationController.h"
#include "IconFetcher.h"
#include "ShellNavigator.h"
#include "TabNavigationInterface.h"
#include "../Helper/ShellHelper.h"

ShellNavigationController::ShellNavigationController(ShellNavigator *navigator,
	TabNavigationInterface *tabNavigation, IconFetcher *iconFetcher) :
	m_navigator(navigator),
	m_tabNavigation(tabNavigation),
	m_iconFetcher(iconFetcher)
{
	Initialize();
}

ShellNavigationController::ShellNavigationController(ShellNavigator *navigator,
	TabNavigationInterface *tabNavigation, IconFetcher *iconFetcher,
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries, int currentEntry) :
	NavigationController(CopyPreservedHistoryEntries(preservedEntries), currentEntry),
	m_navigator(navigator),
	m_tabNavigation(tabNavigation),
	m_iconFetcher(iconFetcher)
{
	Initialize();
}

void ShellNavigationController::Initialize()
{
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
		auto entry = std::make_unique<HistoryEntry>(*preservedEntry);
		entries.push_back(std::move(entry));
	}

	return entries;
}

void ShellNavigationController::OnNavigationCommitted(const NavigateParams &navigateParams)
{
	auto historyEntryType = navigateParams.historyEntryType;

	// If there is no current history entry (i.e. because this is the first navigation), one should
	// be added, regardless of the requested history entry type.
	if (!GetCurrentEntry())
	{
		historyEntryType = HistoryEntryType::AddEntry;
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
		else if (historyEntryType == HistoryEntryType::ReplaceCurrentEntry)
		{
			// The history entry can't be replaced if it doesn't exist, so add a new entry instead.
			historyEntryType = HistoryEntryType::AddEntry;
		}
	}

	if (historyEntryType == HistoryEntryType::AddEntry
		|| historyEntryType == HistoryEntryType::ReplaceCurrentEntry)
	{
		auto entry = BuildEntry(navigateParams);
		int entryId = entry->GetId();
		int entryIndex;

		if (historyEntryType == HistoryEntryType::AddEntry)
		{
			entryIndex = AddEntry(std::move(entry));
		}
		else
		{
			// When an entry is replaced, the set of selected items should be retained.
			auto *currentEntry = GetCurrentEntry();
			entry->SetSelectedItems(currentEntry->GetSelectedItems());

			entryIndex = ReplaceCurrentEntry(std::move(entry));
		}

		// TODO: It would probably be better to do this somewhere else, since
		// this class is focused on navigation.
		m_iconFetcher->QueueIconTask(navigateParams.pidl.Raw(),
			[this, entryIndex, entryId](int iconIndex, int overlayIndex)
			{
				UNREFERENCED_PARAMETER(overlayIndex);

				auto *entry = GetEntryAtIndex(entryIndex);

				if (!entry || entry->GetId() != entryId)
				{
					return;
				}

				entry->SetSystemIconIndex(iconIndex);
			});
	}
}

std::unique_ptr<HistoryEntry> ShellNavigationController::BuildEntry(
	const NavigateParams &navigateParams)
{
	std::wstring displayName;
	HRESULT hr = GetDisplayName(navigateParams.pidl.Raw(), SHGDN_INFOLDER, displayName);

	if (FAILED(hr))
	{
		// It's not expected that this would happen, so it would be useful to have some
		// indication if the call above ever does fail.
		DCHECK(false);

		displayName = L"(Unknown)";
	}

	auto fullPathForDisplay = GetFolderPathForDisplay(navigateParams.pidl.Raw());

	if (!fullPathForDisplay)
	{
		DCHECK(false);

		fullPathForDisplay = L"(Unknown)";
	}

	return std::make_unique<HistoryEntry>(navigateParams.pidl.Raw(), displayName,
		*fullPathForDisplay);
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
