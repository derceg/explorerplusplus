/******************************************************************
 *
 * Project: Helper
 * File: FileActionHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Performs file actions and saves information about them.
 * Also allows file actions to be undone.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <vector>
#include "FileActionHandler.h"
#include "../Helper/FileOperations.h"


CFileActionHandler::CFileActionHandler()
{

}

CFileActionHandler::~CFileActionHandler()
{
	while(stackFileActions.size() > 0)
	{
		UndoItem_t UndoItem = stackFileActions.top();
		stackFileActions.pop();

		delete UndoItem.pInfo;
	}
}

BOOL CFileActionHandler::RenameFiles(const std::list<RenamedItem_t> &ItemList)
{
	std::list<RenamedItem_t> *pRenamedItemList = new std::list<RenamedItem_t>;

	for each(auto Item in ItemList)
	{
		BOOL bRes = NFileOperations::RenameFile(Item.strOldFilename,Item.strNewFilename);

		if(bRes)
		{
			RenamedItem_t RenamedItem;
			RenamedItem.strOldFilename = Item.strOldFilename;
			RenamedItem.strNewFilename = Item.strNewFilename;
			pRenamedItemList->push_back(RenamedItem);
		}
	}

	/* Only store an undo operation if at least one
	file was actually renamed. */
	if(pRenamedItemList->size() > 0)
	{
		UndoItem_t UndoItem;
		UndoItem.Type = FILE_ACTION_RENAMED;
		UndoItem.pInfo = reinterpret_cast<void *>(pRenamedItemList);
		stackFileActions.push(UndoItem);

		return TRUE;
	}

	delete pRenamedItemList;

	return FALSE;
}

BOOL CFileActionHandler::DeleteFiles(HWND hwnd,const std::list<std::wstring> &FullFilenameList,
	BOOL bPermanent)
{
	BOOL bRes = NFileOperations::DeleteFiles(hwnd,FullFilenameList,bPermanent);

	if(bRes)
	{
		std::list<std::wstring> *pDeletedItemList = new std::list<std::wstring>(FullFilenameList);

		UndoItem_t UndoItem;
		UndoItem.Type = FILE_ACTION_DELETED;
		UndoItem.pInfo = reinterpret_cast<void *>(pDeletedItemList);
		stackFileActions.push(UndoItem);
	}

	return bRes;
}

void CFileActionHandler::Undo()
{
	if(!stackFileActions.empty())
	{
		UndoItem_t UndoItem = stackFileActions.top();
		stackFileActions.pop();

		assert(UndoItem.pInfo != NULL);

		switch(UndoItem.Type)
		{
		case FILE_ACTION_RENAMED:
			UndoRenameOperation(*(reinterpret_cast<std::list<RenamedItem_t> *>(UndoItem.pInfo)));
			break;

		case FILE_ACTION_COPIED:
			break;

		case FILE_ACTION_MOVED:
			break;

		case FILE_ACTION_DELETED:
			UndoDeleteOperation(*(reinterpret_cast<std::list<std::wstring> *>(UndoItem.pInfo)));
			break;
		}

		delete UndoItem.pInfo;
	}
}

void CFileActionHandler::UndoRenameOperation(const std::list<RenamedItem_t> &RenamedItemList)
{
	std::list<RenamedItem_t> UndoList;

	/* When undoing a rename operation, the new name
	becomes the old name, and vice versa. */
	for each(auto RenamedItem in RenamedItemList)
	{
		RenamedItem_t UndoItem;
		UndoItem.strOldFilename = RenamedItem.strNewFilename;
		UndoItem.strNewFilename = RenamedItem.strOldFilename;
		UndoList.push_back(UndoItem);
	}

	RenameFiles(UndoList);
}

void CFileActionHandler::UndoDeleteOperation(const std::list<std::wstring> &DeletedItemList)
{
	/* Move the file back out of the recycle bin,
	and push a delete action back onto the stack.
	Steps:
	 - Find the item in the recycle bin (probably need to read INFO2 file).
	 - Restore it (context menu command).
	 - Push delete action onto stack. */
}

BOOL CFileActionHandler::CanUndo()
{
	return !stackFileActions.empty();
}