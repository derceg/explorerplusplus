// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

bool CanPasteHardLinkInDirectory(PCIDLIST_ABSOLUTE pidl);
void PasteHardLinks(const std::wstring &destination);
