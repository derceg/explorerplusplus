// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PidlHelper.h"
#include <list>
#include <vector>

class ClipboardStore;

enum class ClipboardAction
{
	Cut,
	Copy
};

enum class TransferAction
{
	Move,
	Copy
};

namespace FileOperations
{

enum class OverwriteMethod
{
	OnePass = 1,
	ThreePass = 2
};

HRESULT RenameFile(IShellItem *item, const std::wstring &newName);
HRESULT DeleteFiles(HWND hwnd, const std::vector<PCIDLIST_ABSOLUTE> &pidls, bool permanent,
	bool silent);
void DeleteFileSecurely(const std::wstring &strFilename, OverwriteMethod overwriteMethod);
HRESULT CopyFilesToFolder(HWND hOwner, const std::wstring &strTitle,
	std::vector<PCIDLIST_ABSOLUTE> &pidls, TransferAction action);
HRESULT CopyFiles(HWND hwnd, IShellItem *destinationFolder, std::vector<PCIDLIST_ABSOLUTE> &pidls,
	TransferAction action);

HRESULT CreateNewFolder(IShellItem *destinationFolder, const std::wstring &newFolderName,
	IFileOperationProgressSink *progressSink);

TCHAR *BuildFilenameList(const std::list<std::wstring> &FilenameList);

BOOL SaveDirectoryListing(const std::wstring &strDirectory, const std::wstring &strFilename);

HRESULT CreateLinkToFile(const std::wstring &strTargetFilename, const std::wstring &strLinkFilename,
	const std::wstring &strLinkDescription);
HRESULT ResolveLink(HWND hwnd, DWORD fFlags, const TCHAR *szLinkFilename, TCHAR *szResolvedPath,
	int nBufferSize);

BOOL CreateBrowseDialog(HWND hOwner, const std::wstring &strTitle, PIDLIST_ABSOLUTE *ppidl);

};

HRESULT CopyFiles(ClipboardStore *clipboardStore, const std::vector<PidlAbsolute> &items,
	IDataObject **dataObjectOut);
HRESULT CutFiles(ClipboardStore *clipboardStore, const std::vector<PidlAbsolute> &items,
	IDataObject **dataObjectOut);
HRESULT CopyFilesToClipboard(ClipboardStore *clipboardStore, const std::vector<PidlAbsolute> &items,
	ClipboardAction action, IDataObject **dataObjectOut);
