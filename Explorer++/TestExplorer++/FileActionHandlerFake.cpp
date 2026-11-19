// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FileActionHandlerFake.h"

BOOL FileActionHandlerFake::RenameFiles(const std::vector<RenamedItem> &itemList)
{
	UNREFERENCED_PARAMETER(itemList);

	return false;
}

HRESULT FileActionHandlerFake::DeleteFiles(HWND hwnd,
	const std::vector<PCIDLIST_ABSOLUTE> &deletedItems, bool permanent, bool silent)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(deletedItems);
	UNREFERENCED_PARAMETER(permanent);
	UNREFERENCED_PARAMETER(silent);

	return E_FAIL;
}

BOOL FileActionHandlerFake::CanUndo() const
{
	return false;
}

void FileActionHandlerFake::Undo()
{
}
