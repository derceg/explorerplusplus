// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Remembers path history, and
 * includes the ability to browse
 * back/forward through a set
 * of paths.
 */

#include "stdafx.h"
#include "PathManager.h"

PathManager::PathManager() :
	m_currentEntry(-1)
{

}

void PathManager::AddEntry(PCIDLIST_ABSOLUTE pidl)
{
	// This will implicitly remove all "forward" entries.
	m_entries.resize(m_currentEntry + 1);

	m_entries.push_back(unique_pidl_absolute(ILCloneFull(pidl)));
	m_currentEntry++;
}

PIDLIST_ABSOLUTE PathManager::GetEntry(int offset)
{
	int index = m_currentEntry + offset;

	if(index < 0 || index >= m_entries.size())
	{
		return nullptr;
	}

	m_currentEntry = index;

	return ILCloneFull(m_entries[index].get());
}

PIDLIST_ABSOLUTE PathManager::GetEntryWithoutUpdate(int offset) const
{
	int index = m_currentEntry + offset;

	if (index < 0 || index >= m_entries.size())
	{
		return nullptr;
	}

	return ILCloneFull(m_entries[index].get());
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

	return m_entries.size() - m_currentEntry - 1;
}

std::vector<unique_pidl_absolute> PathManager::GetBackHistory() const
{
	std::vector<unique_pidl_absolute> history;

	if (m_currentEntry == -1)
	{
		return history;
	}

	for (int i = m_currentEntry - 1; i >= 0; i--)
	{
		history.push_back(unique_pidl_absolute(ILCloneFull(m_entries[i].get())));
	}

	return history;
}

std::vector<unique_pidl_absolute> PathManager::GetForwardHistory() const
{
	std::vector<unique_pidl_absolute> history;

	if (m_currentEntry == -1)
	{
		return history;
	}

	for (int i = m_currentEntry + 1; i < m_entries.size(); i++)
	{
		history.push_back(unique_pidl_absolute(ILCloneFull(m_entries[i].get())));
	}

	return history;
}