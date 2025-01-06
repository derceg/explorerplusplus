// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"

class PreservedHistoryEntry
{
public:
	PreservedHistoryEntry(const PidlAbsolute &pidl);

	const PidlAbsolute &GetPidl() const;

private:
	const PidlAbsolute m_pidl;
};
