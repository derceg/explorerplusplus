// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SignalWrapper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <ShlObj.h>
#include <optional>

struct PreservedHistoryEntry;

class HistoryEntry
{
public:

	enum class PropertyType
	{
		SystemIconIndex
	};

	HistoryEntry(PCIDLIST_ABSOLUTE pidl, std::wstring_view displayName, std::optional<int> systemIconIndex = std::nullopt);
	HistoryEntry(const PreservedHistoryEntry &preservedHistoryEntry);

	int GetId() const;
	unique_pidl_absolute GetPidl() const;
	std::wstring GetDisplayName() const;
	std::optional<std::wstring> GetFullPathForDisplay() const;
	void SetFullPathForDisplay(const std::wstring &fullPathForDisplay);
	std::optional<int> GetSystemIconIndex() const;
	void SetSystemIconIndex(int iconIndex);

	SignalWrapper<HistoryEntry, void(const HistoryEntry &entry, PropertyType propertyType)> historyEntryUpdatedSignal;

private:

	DISALLOW_COPY_AND_ASSIGN(HistoryEntry);

	static int idCounter;
	const int m_id;

	unique_pidl_absolute m_pidl;
	std::wstring m_displayName;
	std::optional<std::wstring> m_fullPathForDisplay;
	std::optional<int> m_systemIconIndex;
};