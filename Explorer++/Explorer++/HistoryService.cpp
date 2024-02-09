// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "HistoryService.h"

void HistoryService::AddHistoryItem(const PidlAbsolute &pidl)
{
	m_historyItems.push_front(pidl);
}

const std::deque<PidlAbsolute> &HistoryService::GetHistoryItems() const
{
	return m_historyItems;
}
