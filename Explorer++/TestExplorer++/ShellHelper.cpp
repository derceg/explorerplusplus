// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellHelper.h"
#include <gtest/gtest.h>

namespace
{

void CreateSimplePidlForTestInternal(const std::wstring &path, PidlAbsolute &outputPidl,
	IShellFolder *parent, ShellItemType shellItemType)
{
	HRESULT hr = CreateSimplePidl(path, outputPidl, parent, shellItemType);
	ASSERT_HRESULT_SUCCEEDED(hr);
}

}

// This function makes it easier to create a simple pidl in a test.
PidlAbsolute CreateSimplePidlForTest(const std::wstring &path, IShellFolder *parent,
	ShellItemType shellItemType)
{
	PidlAbsolute pidl;
	CreateSimplePidlForTestInternal(path, pidl, parent, shellItemType);
	return pidl;
}
