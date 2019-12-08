// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PreservedHistoryEntry.h"
#include "ShellBrowser/PreservedFolderState.h"
#include "Tab.h"
#include "../Helper/Macros.h"

struct PreservedTab
{
	PreservedTab(const Tab &tab, int index) :
		id(tab.GetId()),
		index(index),
		history(CopyHistoryEntries(tab)),
		currentEntry(tab.GetNavigationController()->GetCurrentIndex()),
		useCustomName(tab.GetUseCustomName()),
		customName(tab.GetUseCustomName() ? tab.GetName() : std::wstring()),
		lockState(tab.GetLockState()),
		preservedFolderState(tab.GetShellBrowser())
	{
		
	}

	int id;
	int index;

	std::vector<std::unique_ptr<PreservedHistoryEntry>> history;
	int currentEntry;

	bool useCustomName;
	std::wstring customName;
	Tab::LockState lockState;

	PreservedFolderState preservedFolderState;

private:

	DISALLOW_COPY_AND_ASSIGN(PreservedTab);

	static std::vector<std::unique_ptr<PreservedHistoryEntry>> CopyHistoryEntries(const Tab &tab)
	{
		std::vector<std::unique_ptr<PreservedHistoryEntry>> history;

		for (int i = 0; i < tab.GetNavigationController()->GetNumHistoryEntries(); i++)
		{
			auto entry = std::make_unique<PreservedHistoryEntry>(*tab.GetNavigationController()->GetEntryAtIndex(i));
			history.push_back(std::move(entry));
		}

		return history;
	}
};