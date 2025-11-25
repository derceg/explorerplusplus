// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Pidl.h"
#include <objidl.h>
#include <vector>

class ClipboardStore;

enum class PasteType
{
	Normal,
	Shortcut
};

enum class PathType
{
	Parsing,
	UniversalPath
};

bool CanShellPasteDataObject(PCIDLIST_ABSOLUTE destination, IDataObject *dataObject,
	PasteType pasteType);
void CopyItemPathsToClipboard(ClipboardStore *clipboardStore,
	const std::vector<PidlAbsolute> &items, PathType pathType);
