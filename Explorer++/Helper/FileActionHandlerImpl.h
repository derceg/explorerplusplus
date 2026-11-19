// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "FileActionHandler.h"
#include <stack>

class FileActionHandlerImpl : public FileActionHandler
{
public:
	// FileActionHandler
	BOOL RenameFiles(const std::vector<RenamedItem> &itemList) override;
	HRESULT DeleteFiles(HWND hwnd, const std::vector<PCIDLIST_ABSOLUTE> &deletedItems,
		bool permanent, bool silent) override;
	BOOL CanUndo() const override;
	void Undo() override;

private:
	enum class UndoType
	{
		Renamed,
		Copied,
		Moved,
		Deleted
	};

	struct UndoItem_t
	{
		UndoType type;

		std::vector<RenamedItem> renamedItems;
		std::vector<PCIDLIST_ABSOLUTE> deletedItems;
	};

	void UndoRenameOperation(const std::vector<RenamedItem> &renamedItemList);
	void UndoDeleteOperation(const std::vector<PCIDLIST_ABSOLUTE> &deletedItemList);

	std::stack<UndoItem_t> m_stackFileActions;
};
