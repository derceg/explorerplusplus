// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "HistoryEntry.h"
#include "PreservedHistoryEntry.h"

int HistoryEntry::idCounter = 0;

HistoryEntry::HistoryEntry(
	PCIDLIST_ABSOLUTE pidl, std::wstring_view displayName, std::optional<int> systemIconIndex) :
	m_id(idCounter++),
	m_pidl(ILCloneFull(pidl)),
	m_displayName(displayName),
	m_systemIconIndex(systemIconIndex)
{
}

HistoryEntry::HistoryEntry(const PreservedHistoryEntry &preservedHistoryEntry) :
	m_id(idCounter++),
	m_pidl(ILCloneFull(preservedHistoryEntry.pidl.get())),
	m_displayName(preservedHistoryEntry.displayName),
	m_systemIconIndex(preservedHistoryEntry.systemIconIndex)
{
}

int HistoryEntry::GetId() const
{
	return m_id;
}

unique_pidl_absolute HistoryEntry::GetPidl() const
{
	return unique_pidl_absolute(ILCloneFull(m_pidl.get()));
}

std::wstring HistoryEntry::GetDisplayName() const
{
	return m_displayName;
}

std::optional<std::wstring> HistoryEntry::GetFullPathForDisplay() const
{
	return m_fullPathForDisplay;
}

void HistoryEntry::SetFullPathForDisplay(const std::wstring &fullPathForDisplay)
{
	m_fullPathForDisplay = fullPathForDisplay;
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

std::vector<unique_pidl_absolute> HistoryEntry::GetSelectedItems() const
{
	return DeepCopyPidls(m_selectedItems);
}

void HistoryEntry::SetSelectedItems(const std::vector<PCIDLIST_ABSOLUTE> &pidls)
{
	m_selectedItems = DeepCopyPidls(pidls);
}