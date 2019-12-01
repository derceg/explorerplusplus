// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"

class PathManager
{
public:

	PathManager();

	int GetNumBackEntriesStored() const;
	int GetNumForwardEntriesStored() const;
	std::vector<unique_pidl_absolute> GetBackHistory() const;
	std::vector<unique_pidl_absolute> GetForwardHistory() const;

	void AddEntry(PCIDLIST_ABSOLUTE pidl);
	PIDLIST_ABSOLUTE GetEntry(int offset);
	PIDLIST_ABSOLUTE GetEntryWithoutUpdate(int offset) const;

private:

	std::vector<unique_pidl_absolute> m_entries;
	int m_currentEntry;
};