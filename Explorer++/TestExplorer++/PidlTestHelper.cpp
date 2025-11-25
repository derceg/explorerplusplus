// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "PidlTestHelper.h"
#include "../Helper/Pidl.h"

// This function makes it easier to create a simple pidl in a test.
PidlAbsolute CreateSimplePidlForTest(const std::wstring &path, IShellFolder *parent,
	ShellItemType shellItemType, ShellItemExtraAttributes extraAttributes)
{
	PidlAbsolute pidl;
	HRESULT hr = CreateSimplePidl(path, pidl, parent, shellItemType, extraAttributes);
	CHECK(SUCCEEDED(hr));
	return pidl;
}
