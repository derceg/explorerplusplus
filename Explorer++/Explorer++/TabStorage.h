// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/FolderSettings.h"
#include "TabContainer.h"
#include "../Helper/PidlHelper.h"

struct TabStorageData
{
	// Currently, the pidl is used when persisting data to the registry. The directory is used when
	// persisting data to the config file.
	PidlAbsolute pidl;
	std::wstring directory;

	TabSettings tabSettings;
	FolderSettings folderSettings;
	FolderColumns columns;
};
