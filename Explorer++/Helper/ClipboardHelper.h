// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <objidl.h>

enum class PasteType
{
	Normal,
	Shortcut
};

bool CanShellPasteDataObject(PCIDLIST_ABSOLUTE destination, IDataObject *dataObject,
	PasteType pasteType);
