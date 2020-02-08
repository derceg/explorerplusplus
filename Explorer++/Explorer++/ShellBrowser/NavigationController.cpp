// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NavigationController.h"

NavigationController::NavigationController(NavigatorInterface *navigator, TabNavigationInterface *tabNavigation,
	IconFetcherInterface *iconFetcher) :
	m_navigator(navigator),
	m_tabNavigation(tabNavigation),
	m_iconFetcher(iconFetcher),
	m_currentEntry(-1)
{
	Initialize();
}

NavigationController::NavigationController(NavigatorInterface *navigator, TabNavigationInterface *tabNavigation,
	IconFetcherInterface *iconFetcher, const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries,
	int currentEntry) :
	m_navigator(navigator),
	m_tabNavigation(tabNavigation),
	m_iconFetcher(iconFetcher),
	m_entries(CopyPreservedHistoryEntries(preservedEntries)),
	m_currentEntry(currentEntry)
{
	Initialize();
}

void NavigationController::Initialize()
{
	m_connections.push_back(m_navigator->AddNavigationCompletedObserver(
		boost::bind(&NavigationController::OnNavigationCompleted, this, _1, _2),
		boost::signals2::at_front));
}

std::vector<std::unique_ptr<HistoryEntry>> NavigationController::CopyPreservedHistoryEntries(
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

void NavigationController::OnNavigationCompleted(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry)
{
	if (addHistoryEntry)
	{
		TCHAR displayName[MAX_PATH];
		GetDisplayName(pidlDirectory, displayName, static_cast<UINT>(std::size(displayName)), SHGDN_INFOLDER);

		auto entry = std::make_unique<HistoryEntry>(pidlDirectory, displayName);
		AddEntry(std::move(entry));
	}
}

void NavigationController::AddEntry(std::unique_ptr<HistoryEntry> entry)
{
	// This will implicitly remove all "forward" entries.
	m_entries.resize(m_currentEntry + 1);

	// TODO: It would probably be better to do this somewhere else, since this
	// class is focused on navigation.
	m_iconFetcher->QueueIconTask(entry->GetPidl().get(), [this, index = m_currentEntry + 1, id = entry->GetId()]
	(int iconIndex) {
		if (index >= GetNumHistoryEntries())
		{
			return;
		}

		if (m_entries[index]->GetId() != id)
		{
			return;
		}

		m_entries[index]->SetSystemIconIndex(iconIndex);
	});

	m_entries.push_back(std::move(entry));
	m_currentEntry++;
}

int NavigationController::GetNumHistoryEntries() const
{
	return static_cast<int>(m_entries.size());
}

HistoryEntry *NavigationController::GetCurrentEntry() const
{
	return GetEntryAtIndex(GetCurrentIndex());
}

int NavigationController::GetCurrentIndex() const
{
	return m_currentEntry;
}

HistoryEntry *NavigationController::GetEntryAndUpdateIndex(int offset)
{
	int index = m_currentEntry + offset;

	if (index < 0 || index >= GetNumHistoryEntries())
	{
		return nullptr;
	}

	m_currentEntry = index;

	return m_entries[index].get();
}

HistoryEntry *NavigationController::GetEntry(int offset) const
{
	int index = m_currentEntry + offset;

	if (index < 0 || index >= GetNumHistoryEntries())
	{
		return nullptr;
	}

	return m_entries[index].get();
}

HistoryEntry *NavigationController::GetEntryAtIndex(int index) const
{
	if (index < 0 || index >= GetNumHistoryEntries())
	{
		return nullptr;
	}

	return m_entries[index].get();
}

bool NavigationController::CanGoBack() const
{
	if (m_currentEntry == -1)
	{
		return false;
	}

	return m_currentEntry > 0;
}

bool NavigationController::CanGoForward() const
{
	if (m_currentEntry == -1)
	{
		return false;
	}

	return (GetNumHistoryEntries() - m_currentEntry - 1) > 0;
}

bool NavigationController::CanGoUp() const
{
	auto currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return false;
	}

	return !IsNamespaceRoot(currentEntry->GetPidl().get());
}

std::vector<HistoryEntry *> NavigationController::GetBackHistory() const
{
	std::vector<HistoryEntry *> history;

	if (m_currentEntry == -1)
	{
		return history;
	}

	for (int i = m_currentEntry - 1; i >= 0; i--)
	{
		history.push_back(m_entries[i].get());
	}

	return history;
}

std::vector<HistoryEntry *> NavigationController::GetForwardHistory() const
{
	std::vector<HistoryEntry *> history;

	if (m_currentEntry == -1)
	{
		return history;
	}

	for (int i = m_currentEntry + 1; i < GetNumHistoryEntries(); i++)
	{
		history.push_back(m_entries[i].get());
	}

	return history;
}

HRESULT NavigationController::GoBack()
{
	return GoToOffset(-1);
}

HRESULT NavigationController::GoForward()
{
	return GoToOffset(1);
}

HRESULT NavigationController::GoToOffset(int offset)
{
	auto entry = GetEntryAndUpdateIndex(offset);

	if (!entry)
	{
		return E_FAIL;
	}

	return BrowseFolder(entry->GetPidl().get(), false);
}

HRESULT NavigationController::GoUp()
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

HRESULT NavigationController::Refresh()
{
	auto currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return E_FAIL;
	}

	return m_navigator->BrowseFolder(currentEntry->GetPidl().get(), false);
}

HRESULT NavigationController::BrowseFolder(const std::wstring &path, bool addHistoryEntry)
{
	unique_pidl_absolute pidlDirectory;
	HRESULT hr = SHParseDisplayName(path.c_str(), nullptr, wil::out_param(pidlDirectory), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		hr = BrowseFolder(pidlDirectory.get(), addHistoryEntry);
	}

	return hr;
}

HRESULT NavigationController::BrowseFolder(PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry)
{
	if (m_navigationMode == NavigationMode::ForceNewTab)
	{
		return m_tabNavigation->CreateNewTab(pidl, true);
	}

	return m_navigator->BrowseFolder(pidl, addHistoryEntry);
}

void NavigationController::SetNavigationMode(NavigationMode navigationMode)
{
	m_navigationMode = navigationMode;
}