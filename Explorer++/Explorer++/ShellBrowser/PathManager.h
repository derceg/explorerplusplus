// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "HistoryEntry.h"
#include "../Helper/IconFetcher.h"

class PathManager
{
public:

	PathManager(IconFetcher *iconFetcher);

	void AddEntry(std::unique_ptr<HistoryEntry> entry);
	HistoryEntry *GetEntry(int offset);
	HistoryEntry *GetEntryWithoutUpdate(int offset) const;

	int GetNumBackEntriesStored() const;
	int GetNumForwardEntriesStored() const;
	std::vector<HistoryEntry *> GetBackHistory() const;
	std::vector<HistoryEntry *> GetForwardHistory() const;

private:

	std::vector<std::unique_ptr<HistoryEntry>> m_entries;
	int m_currentEntry;

	IconFetcher *m_iconFetcher;
};