// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/FileActionHandler.h"

class FileActionHandlerFake : public FileActionHandler
{
public:
	// FileActionHandler
	BOOL RenameFiles(const std::vector<RenamedItem> &itemList) override;
	HRESULT DeleteFiles(HWND hwnd, const std::vector<PCIDLIST_ABSOLUTE> &deletedItems,
		bool permanent, bool silent) override;
	BOOL CanUndo() const override;
	void Undo() override;
};
