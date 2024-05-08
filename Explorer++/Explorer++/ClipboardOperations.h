// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>
#include <system_error>
#include <vector>

namespace ClipboardOperations
{

struct PastedItem
{
	std::wstring path;
	std::error_code error;
};

using PastedItems = std::vector<PastedItem>;

bool CanPasteHardLinkInDirectory(PCIDLIST_ABSOLUTE pidl);

// There are two types of paste operations used within the application:
//
// 1. A paste that occurs via the shell. In that situation, the pasted items will be selected via
//    the object registered via IObjectWithSite.
// 2. A paste that is really just a file operation that's performed internally.
//
// These functions allow for the second type of paste operation to be performed.
PastedItems PasteHardLinks(const std::wstring &destination);
PastedItems PasteSymLinks(const std::wstring &destination);

}
