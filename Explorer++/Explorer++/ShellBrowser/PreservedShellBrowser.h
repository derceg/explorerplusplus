// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "FolderSettings.h"
#include <boost/core/noncopyable.hpp>
#include <memory>
#include <vector>

class PreservedHistoryEntry;
class ShellBrowser;

struct PreservedShellBrowser : private boost::noncopyable
{
public:
	PreservedShellBrowser(const ShellBrowser *shellBrowser);

	// This constructor is used in tests only.
	PreservedShellBrowser(const FolderSettings &folderSettings, const FolderColumns &folderColumns,
		std::vector<std::unique_ptr<PreservedHistoryEntry>> history, int currentEntry);

	~PreservedShellBrowser();

	const FolderSettings folderSettings;
	const FolderColumns folderColumns;

	const std::vector<std::unique_ptr<PreservedHistoryEntry>> history;
	const int currentEntry;

private:
	static std::vector<std::unique_ptr<PreservedHistoryEntry>> CopyHistoryEntries(
		const ShellBrowser *shellBrowser);
};
