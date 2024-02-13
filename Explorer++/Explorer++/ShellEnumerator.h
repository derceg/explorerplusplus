// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include <shobjidl_core.h>
#include <vector>

class ShellEnumerator
{
public:
	// These flags are used when performing an enumeration.
	enum class Flags
	{
		Standard = 0,

		// Indicates that both hidden and hidden system items will be included in the enumeration.
		IncludeHidden = 1 << 0
	};

	HRESULT EnumerateDirectory(IShellFolder *shellFolder, HWND embedder, Flags flags,
		std::vector<PidlChild> &outputItems);
};

DEFINE_ENUM_FLAG_OPERATORS(ShellEnumerator::Flags);
