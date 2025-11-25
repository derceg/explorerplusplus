// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Pidl.h"
#include <boost/signals2.hpp>
#include <deque>

// Stores global history (i.e. the history of navigations across all tabs).
class HistoryModel
{
public:
	using HistoryChangedSignal = boost::signals2::signal<void()>;

	void AddHistoryItem(const PidlAbsolute &pidl);

	// Returns the set of history items, with more recent items appearing first.
	const std::deque<PidlAbsolute> &GetHistoryItems() const;

	boost::signals2::connection AddHistoryChangedObserver(
		const HistoryChangedSignal::slot_type &observer);

private:
	std::deque<PidlAbsolute> m_historyItems;
	HistoryChangedSignal m_historyChangedSignal;
};
