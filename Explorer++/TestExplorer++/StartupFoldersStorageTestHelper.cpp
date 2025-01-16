// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "StartupFoldersStorageTestHelper.h"

namespace StartupFoldersStorageTestHelper
{

std::vector<std::wstring> BuildReference()
{
	return { L"h:\\projects", L"d:\\documents\\2020", L"c:\\users\\public",
		L"c:\\program files\\common files", L"e:\\" };
}

}
