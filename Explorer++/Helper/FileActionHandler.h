// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <shtypes.h>
#include <string>
#include <vector>

class FileActionHandler
{
public:
	struct RenamedItem
	{
		std::wstring oldFilename;
		std::wstring newFilename;
	};

	virtual ~FileActionHandler() = default;

	virtual BOOL RenameFiles(const std::vector<RenamedItem> &itemList) = 0;
	virtual HRESULT DeleteFiles(HWND hwnd, const std::vector<PCIDLIST_ABSOLUTE> &deletedItems,
		bool permanent, bool silent) = 0;

	virtual BOOL CanUndo() const = 0;
	virtual void Undo() = 0;
};
