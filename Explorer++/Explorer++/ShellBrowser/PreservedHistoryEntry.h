// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/HistoryEntry.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <optional>

struct PreservedHistoryEntry
{
	PreservedHistoryEntry(const HistoryEntry &entry) :
		id(entry.GetId()),
		pidl(ILCloneFull(entry.GetPidl().get())),
		displayName(entry.GetDisplayName()),
		systemIconIndex(entry.GetSystemIconIndex())
	{

	}

	const int id;

	unique_pidl_absolute pidl;
	std::wstring displayName;
	std::optional<int> systemIconIndex;

private:

	DISALLOW_COPY_AND_ASSIGN(PreservedHistoryEntry);
};