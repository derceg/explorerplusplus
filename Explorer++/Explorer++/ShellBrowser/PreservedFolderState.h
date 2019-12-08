// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "FolderSettings.h"
#include "ShellBrowser.h"
#include "../Helper/Macros.h"

struct PreservedFolderState
{
public:

	PreservedFolderState(const CShellBrowser *shellBrowser) :
		folderSettings(shellBrowser->GetFolderSettings())
	{

	}

	FolderSettings folderSettings;

private:

	DISALLOW_COPY_AND_ASSIGN(PreservedFolderState);
};