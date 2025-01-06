// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "HistoryEntry.h"

HistoryEntry::HistoryEntry(const PidlAbsolute &pidl) : m_id(idCounter++), m_pidl(pidl)
{
}

int HistoryEntry::GetId() const
{
	return m_id;
}

const PidlAbsolute &HistoryEntry::GetPidl() const
{
	return m_pidl;
}

const std::vector<PidlAbsolute> &HistoryEntry::GetSelectedItems() const
{
	return m_selectedItems;
}

void HistoryEntry::SetSelectedItems(const std::vector<PidlAbsolute> &pidls)
{
	m_selectedItems = pidls;
}
