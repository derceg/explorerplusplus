// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>
#include <vector>

namespace NFileOperations
{
	enum class OverwriteMethod
	{
		OnePass = 1,
		ThreePass = 2
	};

	HRESULT RenameFile(IShellItem *item, const std::wstring &newName);
	HRESULT DeleteFiles(
		HWND hwnd, std::vector<PCIDLIST_ABSOLUTE> &pidls, bool permanent, bool silent);
	void DeleteFileSecurely(const std::wstring &strFilename, OverwriteMethod overwriteMethod);
	HRESULT CopyFilesToFolder(HWND hOwner, const std::wstring &strTitle,
		std::vector<PCIDLIST_ABSOLUTE> &pidls, bool move);
	HRESULT CopyFiles(
		HWND hwnd, IShellItem *destinationFolder, std::vector<PCIDLIST_ABSOLUTE> &pidls, bool move);

	HRESULT CreateNewFolder(IShellItem *destinationFolder, const std::wstring &newFolderName,
		IFileOperationProgressSink *progressSink);

	TCHAR *BuildFilenameList(const std::list<std::wstring> &FilenameList);

	BOOL SaveDirectoryListing(const std::wstring &strDirectory, const std::wstring &strFilename);

	HRESULT CreateLinkToFile(const std::wstring &strTargetFilename,
		const std::wstring &strLinkFilename, const std::wstring &strLinkDescription);
	HRESULT ResolveLink(HWND hwnd, DWORD fFlags, const TCHAR *szLinkFilename, TCHAR *szResolvedPath,
		int nBufferSize);

	BOOL CreateBrowseDialog(HWND hOwner, const std::wstring &strTitle, PIDLIST_ABSOLUTE *ppidl);
};

HRESULT CopyFiles(
	const std::vector<std::wstring> &FileNameList, IDataObject **pClipboardDataObject);
HRESULT CutFiles(const std::vector<std::wstring> &FileNameList, IDataObject **pClipboardDataObject);
HRESULT CopyFilesToClipboard(
	const std::vector<std::wstring> &FileNameList, BOOL bMove, IDataObject **pClipboardDataObject);

int PasteLinksToClipboardFiles(const TCHAR *szDestination);
int PasteHardLinks(const TCHAR *szDestination);