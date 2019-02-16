// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "UniqueHandle.h"

struct MenuTraits
{
	typedef HMENU pointer;

	static HMENU invalid()
	{
		return nullptr;
	}

	static void close(HMENU value)
	{
		DestroyMenu(value);
	}
};

typedef unique_handle<MenuTraits> MenuPtr;