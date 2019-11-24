// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Performs file actions and saves information about them.
 * Also allows file actions to be undone.
 */

#include "stdafx.h"
#include "FileActionHandler.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Macros.h"

BOOL CFileActionHandler::RenameFiles(const RenamedItems_t &itemList)
{
	RenamedItems_t renamedItems;

	for(const auto &item : itemList)
	{
		/* TODO: This should actually be done by the caller. */
		IShellItem *shellItem = nullptr;
		HRESULT hr = SHCreateItemFromParsingName(item.strOldFilename.c_str(), nullptr, IID_PPV_ARGS(&shellItem));

		if (SUCCEEDED(hr))
		{
			TCHAR newFilename[MAX_PATH];
			StringCchCopy(newFilename, SIZEOF_ARRAY(newFilename), item.strNewFilename.c_str());
			PathStripPath(newFilename);

			/* TODO: Could rename all files in the list in a single
			operation, rather than one by one.*/
			hr = NFileOperations::RenameFile(shellItem, newFilename);

			if (SUCCEEDED(hr))
			{
				renamedItems.push_back(item);
			}

			shellItem->Release();
		}
	}

	/* Only store an undo operation if at least one
	file was actually renamed. */
	if(renamedItems.size() > 0)
	{
		UndoItem_t UndoItem;
		UndoItem.Type = FILE_ACTION_RENAMED;
		UndoItem.renamedItems = renamedItems;	
		m_stackFileActions.push(UndoItem);

		return TRUE;
	}

	return FALSE;
}

HRESULT CFileActionHandler::DeleteFiles(HWND hwnd, DeletedItems_t &deletedItems,
	bool permanent, bool silent)
{
	HRESULT hr = NFileOperations::DeleteFiles(hwnd, deletedItems, permanent, silent);

	if(SUCCEEDED(hr))
	{
		UndoItem_t UndoItem;
		UndoItem.Type = FILE_ACTION_DELETED;
		UndoItem.deletedItems = deletedItems;
		m_stackFileActions.push(UndoItem);
	}

	return hr;
}

void CFileActionHandler::Undo()
{
	if(!m_stackFileActions.empty())
	{
		UndoItem_t &undoItem = m_stackFileActions.top();

		switch(undoItem.Type)
		{
		case FILE_ACTION_RENAMED:
			UndoRenameOperation(undoItem.renamedItems);
			break;

		case FILE_ACTION_COPIED:
			break;

		case FILE_ACTION_MOVED:
			break;

		case FILE_ACTION_DELETED:
			UndoDeleteOperation(undoItem.deletedItems);
			break;
		}

		m_stackFileActions.pop();
	}
}

void CFileActionHandler::UndoRenameOperation(const RenamedItems_t &renamedItemList)
{
	RenamedItems_t UndoList;

	/* When undoing a rename operation, the new name
	becomes the old name, and vice versa. */
	for(const auto &RenamedItem : renamedItemList)
	{
		RenamedItem_t UndoItem;
		UndoItem.strOldFilename = RenamedItem.strNewFilename;
		UndoItem.strNewFilename = RenamedItem.strOldFilename;
		UndoList.push_back(UndoItem);
	}

	RenameFiles(UndoList);
}

void CFileActionHandler::UndoDeleteOperation(const DeletedItems_t &deletedItemList)
{
	UNREFERENCED_PARAMETER(deletedItemList);

	/* Move the file back out of the recycle bin,
	and push a delete action back onto the stack.
	Steps:
	 - Find the item in the recycle bin (probably need to read INFO2 file).
	 - Restore it (context menu command).
	 - Push delete action onto stack. */
}

BOOL CFileActionHandler::CanUndo() const
{
	return !m_stackFileActions.empty();
}