// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileDialogs.h"
#include <wil/com.h>
#include <algorithm>

namespace
{

HRESULT SetOptions(IFileDialog *fileDialog, FILEOPENDIALOGOPTIONS additionalOptions)
{
	FILEOPENDIALOGOPTIONS options;
	RETURN_IF_FAILED(fileDialog->GetOptions(&options));
	RETURN_IF_FAILED(fileDialog->SetOptions(options | additionalOptions));
	return S_OK;
}

HRESULT SetDefaultValues(IFileDialog *fileDialog, const std::wstring &defaultFolder,
	const std::wstring &defaultFileName)
{
	if (wil::com_ptr_nothrow<IShellItem> defaultItem; SUCCEEDED(SHCreateItemFromParsingName(
			defaultFolder.c_str(), nullptr, IID_PPV_ARGS(&defaultItem))))
	{
		RETURN_IF_FAILED(fileDialog->SetDefaultFolder(defaultItem.get()));
	}

	RETURN_IF_FAILED(fileDialog->SetFileName(defaultFileName.c_str()));
	return S_OK;
}

HRESULT SetFilesTypes(IFileDialog *fileDialog, const std::vector<FileDialogs::FileType> &fileTypes,
	UINT fileTypeIndex)
{
	if (fileTypes.empty())
	{
		return S_OK;
	}

	std::vector<COMDLG_FILTERSPEC> filterSpec;
	std::ranges::transform(fileTypes, std::back_inserter(filterSpec), [](const auto &fileType)
		{ return COMDLG_FILTERSPEC{ fileType.description.c_str(), fileType.pattern.c_str() }; });

	RETURN_IF_FAILED(
		fileDialog->SetFileTypes(static_cast<UINT>(filterSpec.size()), filterSpec.data()));
	RETURN_IF_FAILED(fileDialog->SetFileTypeIndex(
		std::clamp(fileTypeIndex, 0u, static_cast<UINT>(filterSpec.size()))));
	return S_OK;
}

HRESULT ShowDialog(IFileDialog *fileDialog, HWND owner, std::wstring &chosenPath)
{
	RETURN_IF_FAILED(fileDialog->Show(owner));

	wil::com_ptr_nothrow<IShellItem> shellItem;
	RETURN_IF_FAILED(fileDialog->GetResult(&shellItem));

	wil::unique_cotaskmem_string parsingPath;
	RETURN_IF_FAILED(shellItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &parsingPath));

	chosenPath = parsingPath.get();

	return S_OK;
}

}

namespace FileDialogs
{

HRESULT ShowSelectFolderDialog(HWND owner, const std::wstring &defaultFolder,
	std::wstring &chosenFolderPath)
{
	auto fileOpenDialog = wil::CoCreateInstanceNoThrow<IFileOpenDialog>(CLSID_FileOpenDialog);

	if (!fileOpenDialog)
	{
		return E_FAIL;
	}

	RETURN_IF_FAILED(SetOptions(fileOpenDialog.get(), FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM));
	RETURN_IF_FAILED(SetDefaultValues(fileOpenDialog.get(), defaultFolder, L""));
	RETURN_IF_FAILED(ShowDialog(fileOpenDialog.get(), owner, chosenFolderPath));

	return S_OK;
}

HRESULT ShowSaveAsDialog(HWND owner, const std::wstring &defaultFolder,
	const std::wstring &defaultFileName, const std::vector<FileType> &fileTypes, UINT fileTypeIndex,
	std::wstring &chosenFilePath)
{
	auto fileSaveDialog = wil::CoCreateInstanceNoThrow<IFileSaveDialog>(CLSID_FileSaveDialog);

	if (!fileSaveDialog)
	{
		return E_FAIL;
	}

	RETURN_IF_FAILED(SetDefaultValues(fileSaveDialog.get(), defaultFolder, defaultFileName));
	RETURN_IF_FAILED(SetFilesTypes(fileSaveDialog.get(), fileTypes, fileTypeIndex));

	// Though this is empty, this will cause the dialog to update the extension automatically, based
	// on the selected file type.
	RETURN_IF_FAILED(fileSaveDialog->SetDefaultExtension(L""));

	RETURN_IF_FAILED(ShowDialog(fileSaveDialog.get(), owner, chosenFilePath));

	return S_OK;
}

}
