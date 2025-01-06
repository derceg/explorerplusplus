// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PreservedHistoryEntry.h"

PreservedHistoryEntry::PreservedHistoryEntry(const PidlAbsolute &pidl) : m_pidl(pidl)
{
}

const PidlAbsolute &PreservedHistoryEntry::GetPidl() const
{
	return m_pidl;
}
