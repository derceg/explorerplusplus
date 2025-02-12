// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include <stop_token>
#include <vector>

class ShellEnumerator
{
public:
	virtual ~ShellEnumerator() = default;

	virtual HRESULT EnumerateDirectory(PCIDLIST_ABSOLUTE pidlDirectory,
		std::vector<PidlChild> &outputItems, std::stop_token stopToken) const = 0;
};
