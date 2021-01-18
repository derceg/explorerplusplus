// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellNavigationController.h"

ShellNavigationController::ShellNavigationController(NavigatorInterface *navigator,
	TabNavigationInterface *tabNavigation, IconFetcherInterface *iconFetcher) :
	m_navigator(navigator),
	m_tabNavigation(tabNavigation),
	m_iconFetcher(iconFetcher)
{
	Initialize();
}

ShellNavigationController::ShellNavigationController(NavigatorInterface *navigator,
	TabNavigationInterface *tabNavigation, IconFetcherInterface *iconFetcher,
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
		boost::bind(&ShellNavigationController::OnNavigationCommitted, this, _1, _2),
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

void ShellNavigationController::OnNavigationCommitted(
	PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry)
{
	if (addHistoryEntry)
	{
		std::wstring displayName;
		GetDisplayName(pidlDirectory, SHGDN_INFOLDER, displayName);

		auto newEntry = std::make_unique<HistoryEntry>(pidlDirectory, displayName);
		int entryId = newEntry->GetId();
		int index = AddEntry(std::move(newEntry));

		// TODO: It would probably be better to do this somewhere else, since
		// this class is focused on navigation.
		m_iconFetcher->QueueIconTask(pidlDirectory, [this, index, entryId](int iconIndex) {
			auto *entry = GetEntryAtIndex(index);

			if (!entry || entry->GetId() != entryId)
			{
				return;
			}

			entry->SetSystemIconIndex(iconIndex);
		});
	}
}

HRESULT ShellNavigationController::GoToOffset(int offset)
{
	auto entry = GetEntry(offset);

	if (!entry)
	{
		return E_FAIL;
	}

	auto connection = m_navigator->AddNavigationCommittedObserver(
		[this, offset](PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry) {
			UNREFERENCED_PARAMETER(pidl);
			UNREFERENCED_PARAMETER(addHistoryEntry);

			// The entry retrieval above will fail if the provided offset is invalid, so there's no
			// need to re-check the offset here.
			int index = GetCurrentIndex() + offset;
			SetCurrentIndex(index);
		},
		boost::signals2::at_front);

	auto disconnect = wil::scope_exit([&connection] {
		connection.disconnect();
	});

	return BrowseFolder(entry);
}

bool ShellNavigationController::CanGoUp() const
{
	auto *currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return false;
	}

	return !IsNamespaceRoot(currentEntry->GetPidl().get());
}

HRESULT ShellNavigationController::GoUp()
{
	auto *currentEntry = GetCurrentEntry();

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

	return BrowseFolder(pidlParent.get());
}

HRESULT ShellNavigationController::Refresh()
{
	auto *currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return E_FAIL;
	}

	return m_navigator->BrowseFolder(*currentEntry);
}

HRESULT ShellNavigationController::BrowseFolder(const HistoryEntry *entry)
{
	if (m_navigationMode == NavigationMode::ForceNewTab && GetCurrentEntry() != nullptr)
	{
		m_tabNavigation->CreateNewTab(entry->GetPidl().get(), true);
		return S_OK;
	}

	return m_navigator->BrowseFolder(*entry);
}

HRESULT ShellNavigationController::BrowseFolder(const std::wstring &path, bool addHistoryEntry)
{
	unique_pidl_absolute pidlDirectory;
	HRESULT hr =
		SHParseDisplayName(path.c_str(), nullptr, wil::out_param(pidlDirectory), 0, nullptr);

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
		m_tabNavigation->CreateNewTab(pidl, true);
		return S_OK;
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