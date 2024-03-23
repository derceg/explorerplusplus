// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "NavigationHelper.h"
#include <shtypes.h>
#include <string>

class Navigator
{
public:
	virtual ~Navigator() = default;

	virtual void OpenItem(const std::wstring &itemPath,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab) = 0;
	virtual void OpenItem(PCIDLIST_ABSOLUTE pidlItem,
		OpenFolderDisposition openFolderDisposition = OpenFolderDisposition::CurrentTab) = 0;
};
