// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"
#include <ShlObj.h>
#include <optional>

struct HistoryEntry
{
	HistoryEntry() = default;

	HistoryEntry(PCIDLIST_ABSOLUTE pidl, std::wstring_view displayName) :
		pidl(unique_pidl_absolute(ILCloneFull(pidl))),
		displayName(displayName)
	{

	}

	HistoryEntry(const HistoryEntry &other)
	{
		pidl.reset(ILCloneFull(other.pidl.get()));
		displayName = other.displayName;
	}

	unique_pidl_absolute pidl;
	std::wstring displayName;
};

class PathManager
{
public:

	PathManager();

	void AddEntry(const HistoryEntry &entry);
	std::optional<HistoryEntry> GetEntry(int offset);
	std::optional<HistoryEntry> GetEntryWithoutUpdate(int offset) const;

	int GetNumBackEntriesStored() const;
	int GetNumForwardEntriesStored() const;
	std::vector<HistoryEntry> GetBackHistory() const;
	std::vector<HistoryEntry> GetForwardHistory() const;

private:

	std::vector<HistoryEntry> m_entries;
	int m_currentEntry;
};