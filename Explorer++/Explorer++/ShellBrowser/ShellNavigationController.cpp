// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellNavigationController.h"

ShellNavigationController::ShellNavigationController(NavigatorInterface *navigator, TabNavigationInterface *tabNavigation,
	IconFetcherInterface *iconFetcher) :
	NavigationController(),
	m_navigator(navigator),
	m_tabNavigation(tabNavigation),
	m_iconFetcher(iconFetcher)
{
	Initialize();
}

ShellNavigationController::ShellNavigationController(NavigatorInterface *navigator, TabNavigationInterface *tabNavigation,
	IconFetcherInterface *iconFetcher, const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries,
	int currentEntry) :
	NavigationController(CopyPreservedHistoryEntries(preservedEntries), currentEntry),
	m_navigator(navigator),
	m_tabNavigation(tabNavigation),
	m_iconFetcher(iconFetcher)
{
	Initialize();
}

void ShellNavigationController::Initialize()
{
	m_connections.push_back(m_navigator->AddNavigationCompletedObserver(
		boost::bind(&ShellNavigationController::OnNavigationCompleted, this, _1, _2),
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

void ShellNavigationController::OnNavigationCompleted(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry)
{
	if (addHistoryEntry)
	{
		TCHAR displayName[MAX_PATH];
		GetDisplayName(pidlDirectory, displayName, static_cast<UINT>(std::size(displayName)), SHGDN_INFOLDER);

		auto newEntry = std::make_unique<HistoryEntry>(pidlDirectory, displayName);
		int entryId = newEntry->GetId();
		int index = AddEntry(std::move(newEntry));

		// TODO: It would probably be better to do this somewhere else, since
		// this class is focused on navigation.
		m_iconFetcher->QueueIconTask(pidlDirectory, [this, index, entryId] (int iconIndex) {
			auto entry = GetEntryAtIndex(index);

			if (!entry || entry->GetId() != entryId)
			{
				return;
			}

			entry->SetSystemIconIndex(iconIndex);
		});
	}
}

bool ShellNavigationController::CanGoUp() const
{
	auto currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return false;
	}

	return !IsNamespaceRoot(currentEntry->GetPidl().get());
}

HRESULT ShellNavigationController::GoUp()
{
	auto currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return E_FAIL;
	}

	unique_pidl_absolute pidlParent;
	HRESULT hr = GetVirtualParentPath(currentEntry->GetPidl().get(), wil::out_param(pidlParent));

	if (FAILED(hr))
	{
		return hr;
	}

	return m_navigator->BrowseFolder(pidlParent.get());
}

HRESULT ShellNavigationController::Refresh()
{
	auto currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return E_FAIL;
	}

	return m_navigator->BrowseFolder(currentEntry->GetPidl().get(), false);
}

HRESULT ShellNavigationController::BrowseFolder(const HistoryEntry *entry, bool addHistoryEntry)
{
	return BrowseFolder(entry->GetPidl().get(), addHistoryEntry);
}

HRESULT ShellNavigationController::BrowseFolder(const std::wstring &path, bool addHistoryEntry)
{
	unique_pidl_absolute pidlDirectory;
	HRESULT hr = SHParseDisplayName(path.c_str(), nullptr, wil::out_param(pidlDirectory), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		hr = BrowseFolder(pidlDirectory.get(), addHistoryEntry);
	}

	return hr;
}

HRESULT ShellNavigationController::BrowseFolder(PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry)
{
	if (m_navigationMode == NavigationMode::ForceNewTab && GetCurrentEntry() != nullptr)
	{
		return m_tabNavigation->CreateNewTab(pidl, true);
	}

	return m_navigator->BrowseFolder(pidl, addHistoryEntry);
}

HRESULT ShellNavigationController::GetFailureValue()
{
	return E_FAIL;
}

void ShellNavigationController::SetNavigationMode(NavigationMode navigationMode)
{
	m_navigationMode = navigationMode;
}