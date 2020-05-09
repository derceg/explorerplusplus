// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

class BookmarkTree;

namespace BookmarkRegistryStorage
{
void Load(const std::wstring &applicationKeyPath, BookmarkTree *bookmarkTree);
void Save(const std::wstring &applicationKeyPath, BookmarkTree *bookmarkTree);
}