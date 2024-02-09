// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SignalWrapper.h"
#include "../Helper/Macros.h"
#include "../Helper/PidlHelper.h"
#include <optional>
#include <vector>

struct PreservedHistoryEntry;

class HistoryEntry
{
public:
	enum class PropertyType
	{
		SystemIconIndex
	};

	HistoryEntry(const PidlAbsolute &pidl, std::wstring_view displayName,
		std::wstring_view fullPathForDisplay, std::optional<int> systemIconIndex = std::nullopt);
	HistoryEntry(const PreservedHistoryEntry &preservedHistoryEntry);

	int GetId() const;
	PidlAbsolute GetPidl() const;
	std::wstring GetDisplayName() const;
	std::wstring GetFullPathForDisplay() const;
	std::optional<int> GetSystemIconIndex() const;
	void SetSystemIconIndex(int iconIndex);
	std::vector<PidlAbsolute> GetSelectedItems() const;
	void SetSelectedItems(const std::vector<PidlAbsolute> &pidls);

	SignalWrapper<HistoryEntry, void(const HistoryEntry &entry, PropertyType propertyType)>
		historyEntryUpdatedSignal;

private:
	DISALLOW_COPY_AND_ASSIGN(HistoryEntry);

	static int idCounter;
	const int m_id;

	PidlAbsolute m_pidl;
	std::wstring m_displayName;
	std::wstring m_fullPathForDisplay;
	std::optional<int> m_systemIconIndex;
	std::vector<PidlAbsolute> m_selectedItems;
};
