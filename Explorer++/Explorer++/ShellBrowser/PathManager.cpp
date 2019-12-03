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

		if (index >= m_entries.size())
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

HistoryEntry *PathManager::GetEntry(int offset)
{
	int index = m_currentEntry + offset;

	if(index < 0 || index >= m_entries.size())
	{
		return nullptr;
	}

	m_currentEntry = index;

	return m_entries[index].get();
}

HistoryEntry *PathManager::GetEntryWithoutUpdate(int offset) const
{
	int index = m_currentEntry + offset;

	if (index < 0 || index >= m_entries.size())
	{
		return nullptr;
	}

	return m_entries[index].get();
}

int PathManager::GetNumBackEntriesStored() const
{
	if (m_currentEntry == -1)
	{
		return 0;
	}

	return m_currentEntry;
}

int PathManager::GetNumForwardEntriesStored() const
{
	if (m_currentEntry == -1)
	{
		return 0;
	}

	return static_cast<int>(m_entries.size()) - m_currentEntry - 1;
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

	for (int i = m_currentEntry + 1; i < m_entries.size(); i++)
	{
		history.push_back(m_entries[i].get());
	}

	return history;
}