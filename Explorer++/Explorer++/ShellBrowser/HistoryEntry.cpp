// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "HistoryEntry.h"
#include "PreservedHistoryEntry.h"

int HistoryEntry::idCounter = 0;

HistoryEntry::HistoryEntry(const PidlAbsolute &pidl, std::optional<int> systemIconIndex) :
	m_id(idCounter++),
	m_pidl(pidl),
	m_systemIconIndex(systemIconIndex)
{
}

HistoryEntry::HistoryEntry(const PreservedHistoryEntry &preservedHistoryEntry) :
	m_id(idCounter++),
	m_pidl(preservedHistoryEntry.pidl),
	m_systemIconIndex(preservedHistoryEntry.systemIconIndex)
{
}

int HistoryEntry::GetId() const
{
	return m_id;
}

PidlAbsolute HistoryEntry::GetPidl() const
{
	return m_pidl;
}

std::optional<int> HistoryEntry::GetSystemIconIndex() const
{
	return m_systemIconIndex;
}

void HistoryEntry::SetSystemIconIndex(int iconIndex)
{
	if (iconIndex == m_systemIconIndex)
	{
		return;
	}

	m_systemIconIndex = iconIndex;

	historyEntryUpdatedSignal.m_signal(*this, PropertyType::SystemIconIndex);
}

std::vector<PidlAbsolute> HistoryEntry::GetSelectedItems() const
{
	return m_selectedItems;
}

void HistoryEntry::SetSelectedItems(const std::vector<PidlAbsolute> &pidls)
{
	m_selectedItems = pidls;
}
