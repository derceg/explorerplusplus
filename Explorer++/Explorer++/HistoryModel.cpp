// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "HistoryModel.h"
#include "../Helper/ShellHelper.h"

void HistoryModel::AddHistoryItem(const PidlAbsolute &pidl)
{
	if (!m_historyItems.empty() && (pidl == m_historyItems.front()))
	{
		// This item is the same as the most recent history item.
		return;
	}

	m_historyItems.push_front(pidl);
	m_historyChangedSignal();
}

const std::deque<PidlAbsolute> &HistoryModel::GetHistoryItems() const
{
	return m_historyItems;
}

boost::signals2::connection HistoryModel::AddHistoryChangedObserver(
	const HistoryChangedSignal::slot_type &observer)
{
	return m_historyChangedSignal.connect(observer);
}
