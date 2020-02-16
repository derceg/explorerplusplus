// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "FolderSettings.h"
#include "../Helper/Macros.h"

class ShellBrowser;

struct PreservedFolderState
{
public:

	PreservedFolderState(const ShellBrowser *shellBrowser);

	FolderSettings folderSettings;

private:

	DISALLOW_COPY_AND_ASSIGN(PreservedFolderState);
};