// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <optional>

class HistoryEntry;

struct PreservedHistoryEntry
{
public:

	PreservedHistoryEntry(const HistoryEntry &entry);

	const int id;

	unique_pidl_absolute pidl;
	std::wstring displayName;
	std::optional<int> systemIconIndex;

private:

	DISALLOW_COPY_AND_ASSIGN(PreservedHistoryEntry);
};