// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/PreservedFolderState.h"
#include "Tab.h"
#include "../Helper/Macros.h"

struct PreservedHistoryEntry;

struct PreservedTab
{
	PreservedTab(const Tab &tab, int index);
	~PreservedTab();

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

	static std::vector<std::unique_ptr<PreservedHistoryEntry>> CopyHistoryEntries(const Tab &tab);
};