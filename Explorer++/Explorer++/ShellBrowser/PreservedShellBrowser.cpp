// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PreservedShellBrowser.h"
#include "PreservedHistoryEntry.h"
#include "ShellBrowser.h"
#include "ShellNavigationController.h"
#include "TestHelper.h"

PreservedShellBrowser::PreservedShellBrowser(const ShellBrowser *shellBrowser) :
	folderSettings(shellBrowser->GetFolderSettings()),
	folderColumns(shellBrowser->GetAllColumnSets()),
	history(CopyHistoryEntries(shellBrowser)),
	currentEntry(shellBrowser->GetNavigationController()->GetCurrentIndex())
{
}

PreservedShellBrowser::PreservedShellBrowser(const FolderSettings &folderSettings,
	const FolderColumns &folderColumns, std::vector<std::unique_ptr<PreservedHistoryEntry>> history,
	int currentEntry) :
	folderSettings(folderSettings),
	folderColumns(folderColumns),
	history(std::move(history)),
	currentEntry(currentEntry)
{
	CHECK(IsInTest());

	CHECK(!this->history.empty());
	CHECK(currentEntry >= 0 && static_cast<size_t>(currentEntry) < this->history.size());
}

PreservedShellBrowser::~PreservedShellBrowser() = default;

std::vector<std::unique_ptr<PreservedHistoryEntry>> PreservedShellBrowser::CopyHistoryEntries(
	const ShellBrowser *shellBrowser)
{
	std::vector<std::unique_ptr<PreservedHistoryEntry>> history;

	for (int i = 0; i < shellBrowser->GetNavigationController()->GetNumHistoryEntries(); i++)
	{
		auto entry = std::make_unique<PreservedHistoryEntry>(
			shellBrowser->GetNavigationController()->GetEntryAtIndex(i)->GetPidl());
		history.push_back(std::move(entry));
	}

	return history;
}
