// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkTree.h"

namespace BookmarkRegistryStorage
{
	void Load(HKEY parentKey, BookmarkTree *bookmarkTree);
	void Save(HKEY parentKey, BookmarkTree *bookmarkTree);
}