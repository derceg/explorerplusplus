// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "HistoryEntry.h"

HistoryEntry::HistoryEntry(const PidlAbsolute &pidl, InitialNavigationType type) :
	m_id(idCounter++),
	m_pidl(pidl),
	m_type(type)
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

bool HistoryEntry::IsInitialEntry() const
{
	return m_type == InitialNavigationType::Initial;
}

HistoryEntry::InitialNavigationType HistoryEntry::GetInitialNavigationType() const
{
	return m_type;
}

const std::vector<PidlAbsolute> &HistoryEntry::GetSelectedItems() const
{
	return m_selectedItems;
}

void HistoryEntry::SetSelectedItems(const std::vector<PidlAbsolute> &pidls)
{
	m_selectedItems = pidls;
}
