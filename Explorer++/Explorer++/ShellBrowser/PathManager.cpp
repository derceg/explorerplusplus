// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PathManager.h"

PathManager::PathManager(IconFetcher *iconFetcher) :
	m_iconFetcher(iconFetcher),
	m_currentEntry(-1)
{

}

void PathManager::AddEntry(std::unique_ptr<HistoryEntry> entry)
{
	// This will implicitly remove all "forward" entries.
	m_entries.resize(m_currentEntry + 1);

	m_iconFetcher->QueueIconTask(entry->GetPidl().get(), [this, index = m_currentEntry + 1, id = entry->GetId()]
	(PCIDLIST_ABSOLUTE pidl, int iconIndex) {
		UNREFERENCED_PARAMETER(pidl);

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

int PathManager::GetNumHistoryEntries() const
{
	return static_cast<int>(m_entries.size());
}

int PathManager::GetCurrentIndex() const
{
	return m_currentEntry;
}

HistoryEntry *PathManager::GetEntry(int offset)
{
	int index = m_currentEntry + offset;

	if(index < 0 || index >= GetNumHistoryEntries())
	{
		return nullptr;
	}

	m_currentEntry = index;

	return m_entries[index].get();
}

HistoryEntry *PathManager::GetEntryWithoutUpdate(int offset) const
{
	int index = m_currentEntry + offset;

	if (index < 0 || index >= GetNumHistoryEntries())
	{
		return nullptr;
	}

	return m_entries[index].get();
}

HistoryEntry *PathManager::GetEntryAtIndex(int index) const
{
	if (index < 0 || index >= GetNumHistoryEntries())
	{
		return nullptr;
	}

	return m_entries[index].get();
}

bool PathManager::CanGoBack() const
{
	if (m_currentEntry == -1)
	{
		return false;
	}

	return m_currentEntry > 0;
}

bool PathManager::CanGoForward() const
{
	if (m_currentEntry == -1)
	{
		return false;
	}

	return (GetNumHistoryEntries() - m_currentEntry - 1) > 0;
}

std::vector<HistoryEntry *> PathManager::GetBackHistory() const
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

std::vector<HistoryEntry *> PathManager::GetForwardHistory() const
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