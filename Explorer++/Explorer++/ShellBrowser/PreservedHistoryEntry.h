// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include <boost/core/noncopyable.hpp>
#include <optional>

class HistoryEntry;

struct PreservedHistoryEntry : private boost::noncopyable
{
public:
	PreservedHistoryEntry(const HistoryEntry &entry);

	const int id;

	PidlAbsolute pidl;
	std::optional<int> systemIconIndex;
};
