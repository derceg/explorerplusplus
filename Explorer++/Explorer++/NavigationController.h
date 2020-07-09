// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>
#include <vector>

template <typename HistoryEntryType, typename BrowseFolderReturnType>
class NavigationController
{
public:
	NavigationController() : m_currentEntry(-1)
	{
	}

	NavigationController(
		std::vector<std::unique_ptr<HistoryEntryType>> &&entries, int currentEntry) :
		m_entries(std::move(entries)),
		m_currentEntry(currentEntry)
	{
	}

	int GetNumHistoryEntries() const
	{
		return static_cast<int>(m_entries.size());
	}

	HistoryEntryType *GetCurrentEntry() const
	{
		return GetEntryAtIndex(GetCurrentIndex());
	}

	int GetCurrentIndex() const
	{
		return m_currentEntry;
	}

	HistoryEntryType *GetEntry(int offset) const
	{
		int index = m_currentEntry + offset;

		if (index < 0 || index >= GetNumHistoryEntries())
		{
			return nullptr;
		}

		return m_entries[index].get();
	}

	HistoryEntryType *GetEntryAtIndex(int index) const
	{
		if (index < 0 || index >= GetNumHistoryEntries())
		{
			return nullptr;
		}

		return m_entries[index].get();
	}

	bool CanGoBack() const
	{
		if (m_currentEntry == -1)
		{
			return false;
		}

		return m_currentEntry > 0;
	}

	bool CanGoForward() const
	{
		if (m_currentEntry == -1)
		{
			return false;
		}

		return (GetNumHistoryEntries() - m_currentEntry - 1) > 0;
	}

	std::vector<HistoryEntryType *> GetBackHistory() const
	{
		std::vector<HistoryEntryType *> history;

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

	std::vector<HistoryEntryType *> GetForwardHistory() const
	{
		std::vector<HistoryEntryType *> history;

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

	BrowseFolderReturnType GoBack()
	{
		return GoToOffset(-1);
	}

	BrowseFolderReturnType GoForward()
	{
		return GoToOffset(1);
	}

	BrowseFolderReturnType GoToOffset(int offset)
	{
		auto entry = GetEntryAndUpdateIndex(offset);

		if (!entry)
		{
			return GetFailureValue();
		}

		return BrowseFolder(entry, false);
	}

protected:
	virtual BrowseFolderReturnType BrowseFolder(
		const HistoryEntryType *entry, bool addHistoryEntry = true) = 0;
	virtual BrowseFolderReturnType GetFailureValue() = 0;

	int AddEntry(std::unique_ptr<HistoryEntryType> entry)
	{
		// This will implicitly remove all "forward" entries.
		m_entries.resize(m_currentEntry + 1);

		m_entries.push_back(std::move(entry));
		m_currentEntry++;

		return m_currentEntry;
	}

private:
	HistoryEntryType *GetEntryAndUpdateIndex(int offset)
	{
		int index = m_currentEntry + offset;

		if (index < 0 || index >= GetNumHistoryEntries())
		{
			return nullptr;
		}

		m_currentEntry = index;

		return m_entries[index].get();
	}

	std::vector<std::unique_ptr<HistoryEntryType>> m_entries;
	int m_currentEntry;
};