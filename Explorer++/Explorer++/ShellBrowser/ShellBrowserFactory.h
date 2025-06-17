// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include <memory>
#include <vector>

struct FolderColumns;
struct FolderSettings;
struct PreservedFolderState;
class PreservedHistoryEntry;
class ShellBrowser;

class ShellBrowserFactory
{
public:
	virtual ~ShellBrowserFactory() = default;

	virtual std::unique_ptr<ShellBrowser> Create(const PidlAbsolute &initialPidl,
		const FolderSettings &folderSettings, const FolderColumns *initialColumns) = 0;
	virtual std::unique_ptr<ShellBrowser> CreateFromPreserved(
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
		const PreservedFolderState &preservedFolderState) = 0;
};
