// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>
#include <stack>
#include <vector>

class FileActionHandler
{
public:
	struct RenamedItem_t
	{
		std::wstring strOldFilename;
		std::wstring strNewFilename;
	};

	typedef std::list<RenamedItem_t> RenamedItems_t;
	typedef std::vector<PCIDLIST_ABSOLUTE> DeletedItems_t;

	BOOL RenameFiles(const RenamedItems_t &itemList);
	HRESULT DeleteFiles(HWND hwnd, const DeletedItems_t &deletedItems, bool permanent, bool silent);

	void Undo();
	BOOL CanUndo() const;

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

		RenamedItems_t renamedItems;
		DeletedItems_t deletedItems;
	};

	void UndoRenameOperation(const RenamedItems_t &renamedItemList);
	void UndoDeleteOperation(const DeletedItems_t &deletedItemList);

private:
	std::stack<UndoItem_t> m_stackFileActions;
};
