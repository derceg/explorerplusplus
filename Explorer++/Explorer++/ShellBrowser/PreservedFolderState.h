// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "FolderSettings.h"
#include "../Helper/Macros.h"

class ShellBrowserImpl;

struct PreservedFolderState
{
public:
	PreservedFolderState(const ShellBrowserImpl *shellBrowser);

	FolderSettings folderSettings;

private:
	DISALLOW_COPY_AND_ASSIGN(PreservedFolderState);
};
