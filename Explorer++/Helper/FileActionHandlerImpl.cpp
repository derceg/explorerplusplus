// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Performs file actions and saves information about them.
 * Also allows file actions to be undone.
 */

#include "stdafx.h"
#include "FileActionHandlerImpl.h"
#include "FileOperations.h"
#include <Shlwapi.h>

BOOL FileActionHandlerImpl::RenameFiles(const std::vector<RenamedItem> &itemList)
{
	std::vector<RenamedItem> renamedItems;

	for (const auto &item : itemList)
	{
		/* TODO: This should actually be done by the caller. */
		IShellItem *shellItem = nullptr;
		HRESULT hr = SHCreateItemFromParsingName(item.oldFilename.c_str(), nullptr,
			IID_PPV_ARGS(&shellItem));

		if (SUCCEEDED(hr))
		{
			TCHAR newFilename[MAX_PATH];
			StringCchCopy(newFilename, std::size(newFilename), item.newFilename.c_str());
			PathStripPath(newFilename);

			/* TODO: Could rename all files in the list in a single
			operation, rather than one by one.*/
			hr = FileOperations::RenameFile(shellItem, newFilename);

			if (SUCCEEDED(hr))
			{
				renamedItems.push_back(item);
			}

			shellItem->Release();
		}
	}

	/* Only store an undo operation if at least one
	file was actually renamed. */
	if (!renamedItems.empty())
	{
		UndoItem_t undoItem;
		undoItem.type = UndoType::Renamed;
		undoItem.renamedItems = renamedItems;
		m_stackFileActions.push(undoItem);

		return TRUE;
	}

	return FALSE;
}

HRESULT FileActionHandlerImpl::DeleteFiles(HWND hwnd,
	const std::vector<PCIDLIST_ABSOLUTE> &deletedItems, bool permanent, bool silent)
{
	HRESULT hr = FileOperations::DeleteFiles(hwnd, deletedItems, permanent, silent);

	if (SUCCEEDED(hr))
	{
		UndoItem_t undoItem;
		undoItem.type = UndoType::Deleted;
		undoItem.deletedItems = deletedItems;
		m_stackFileActions.push(undoItem);
	}

	return hr;
}

BOOL FileActionHandlerImpl::CanUndo() const
{
	return !m_stackFileActions.empty();
}

void FileActionHandlerImpl::Undo()
{
	if (!m_stackFileActions.empty())
	{
		UndoItem_t &undoItem = m_stackFileActions.top();

		switch (undoItem.type)
		{
		case UndoType::Renamed:
			UndoRenameOperation(undoItem.renamedItems);
			break;

		case UndoType::Copied:
			break;

		case UndoType::Moved:
			break;

		case UndoType::Deleted:
			UndoDeleteOperation(undoItem.deletedItems);
			break;
		}

		m_stackFileActions.pop();
	}
}

void FileActionHandlerImpl::UndoRenameOperation(const std::vector<RenamedItem> &renamedItemList)
{
	std::vector<RenamedItem> undoList;

	/* When undoing a rename operation, the new name
	becomes the old name, and vice versa. */
	for (const auto &renamedItem : renamedItemList)
	{
		RenamedItem undoItem;
		undoItem.oldFilename = renamedItem.newFilename;
		undoItem.newFilename = renamedItem.oldFilename;
		undoList.push_back(undoItem);
	}

	RenameFiles(undoList);
}

void FileActionHandlerImpl::UndoDeleteOperation(
	const std::vector<PCIDLIST_ABSOLUTE> &deletedItemList)
{
	UNREFERENCED_PARAMETER(deletedItemList);

	/* Move the file back out of the recycle bin,
	and push a delete action back onto the stack.
	Steps:
	 - Find the item in the recycle bin (probably need to read INFO2 file).
	 - Restore it (context menu command).
	 - Push delete action onto stack. */
}
