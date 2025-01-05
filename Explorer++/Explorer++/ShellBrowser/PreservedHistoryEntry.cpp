// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PreservedHistoryEntry.h"
#include "HistoryEntry.h"

PreservedHistoryEntry::PreservedHistoryEntry(const HistoryEntry &entry) :
	id(entry.GetId()),
	pidl(entry.GetPidl()),
	systemIconIndex(entry.GetSystemIconIndex())
{
}
