// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Macros.h"
#include "../Helper/PidlHelper.h"
#include <optional>

class HistoryEntry;

struct PreservedHistoryEntry
{
public:
	PreservedHistoryEntry(const HistoryEntry &entry);

	const int id;

	PidlAbsolute pidl;
	std::wstring displayName;
	std::wstring fullPathForDisplay;
	std::optional<int> systemIconIndex;

private:
	DISALLOW_COPY_AND_ASSIGN(PreservedHistoryEntry);
};
