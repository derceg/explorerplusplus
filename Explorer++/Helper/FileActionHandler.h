// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>
#include <vector>
#include <stack>

class FileActionHandler
{
public:

	struct RenamedItem_t
	{
		std::wstring	strOldFilename;
		std::wstring	strNewFilename;
	};

	typedef std::list<RenamedItem_t> RenamedItems_t;
	typedef std::vector<PCIDLIST_ABSOLUTE> DeletedItems_t;

	BOOL	RenameFiles(const RenamedItems_t &itemList);
	HRESULT	DeleteFiles(HWND hwnd, DeletedItems_t &deletedItems, bool permanent, bool silent);

	void	Undo();
	BOOL	CanUndo() const;

private:

	enum UndoType_t
	{
		FILE_ACTION_RENAMED,
		FILE_ACTION_COPIED,
		FILE_ACTION_MOVED,
		FILE_ACTION_DELETED
	};

	struct UndoItem_t
	{
		UndoType_t	Type;

		RenamedItems_t renamedItems;
		DeletedItems_t deletedItems;
	};

	void	UndoRenameOperation(const RenamedItems_t &renamedItemList);
	void	UndoDeleteOperation(const DeletedItems_t &deletedItemList);

	std::stack<UndoItem_t>	m_stackFileActions;
};