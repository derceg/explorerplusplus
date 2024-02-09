// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include <deque>

// Stores global history (i.e. the history of navigations across all tabs).
class HistoryService
{
public:
	void AddHistoryItem(const PidlAbsolute &pidl);

	// Returns the set of history items, with more recent items appearing first.
	const std::deque<PidlAbsolute> &GetHistoryItems() const;

private:
	std::deque<PidlAbsolute> m_historyItems;
};
