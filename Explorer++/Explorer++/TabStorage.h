// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/FolderSettings.h"
#include "TabContainer.h"
#include "../Helper/Pidl.h"
#include <vector>

struct TabStorageData
{
	// Currently, the pidl is used when persisting data to the registry. The directory is used when
	// persisting data to the config file.
	PidlAbsolute pidl;
	std::wstring directory;

	TabSettings tabSettings;
	FolderSettings folderSettings;
	FolderColumns columns;

	// Contains a set of items to select once navigation to the target directory has occurred.
	std::vector<PidlAbsolute> itemsToSelect;

	// This is only used in tests.
	bool operator==(const TabStorageData &) const = default;
};
