// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

namespace ClipboardOperations
{

// There are two types of paste operations used within the application:
//
// 1. A paste that occurs via the shell. In that situation, the pasted items will be selected via
//    the object registered via IObjectWithSite.
// 2. A paste that is really just a file operation that's performed internally.
//
// In the second situation, a callback with the signature here will be invoked when items are
// pasted.
using InternalPasteCallback = std::function<void(const std::vector<std::wstring> &pastedItems)>;

bool CanPasteHardLinkInDirectory(PCIDLIST_ABSOLUTE pidl);
void PasteHardLinks(const std::wstring &destination, InternalPasteCallback internalPasteCallback);

}
