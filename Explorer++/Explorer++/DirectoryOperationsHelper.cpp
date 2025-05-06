// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DirectoryOperationsHelper.h"
#include "../Helper/DropHandler.h"
#include "../Helper/ShellHelper.h"

namespace
{

bool CanShellPasteClipboardDataInDirectory(PCIDLIST_ABSOLUTE pidl, PasteType pasteType)
{
	wil::com_ptr_nothrow<IDataObject> clipboardObject;
	HRESULT hr = OleGetClipboard(&clipboardObject);

	if (FAILED(hr))
	{
		return false;
	}

	return CanShellPasteDataObject(pidl, clipboardObject.get(), pasteType);
}

bool CanPasteCustomDataInDirectory(PCIDLIST_ABSOLUTE pidl)
{
	auto customFormats = DropHandler::GetDropFormats();
	bool dataAvailable = false;

	for (const auto &customFormat : customFormats)
	{
		if (IsClipboardFormatAvailable(customFormat))
		{
			dataAvailable = true;
			break;
		}
	}

	if (!dataAvailable)
	{
		return false;
	}

	return CanCreateInDirectory(pidl);
}

}

bool CanPasteInDirectory(PCIDLIST_ABSOLUTE pidl, PasteType pasteType)
{
	return CanShellPasteClipboardDataInDirectory(pidl, pasteType)
		|| (pasteType == PasteType::Normal && CanPasteCustomDataInDirectory(pidl));
}

bool CanCreateInDirectory(PCIDLIST_ABSOLUTE pidl)
{
	wil::com_ptr_nothrow<IShellItem> shellItem;
	HRESULT hr = SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&shellItem));

	if (FAILED(hr))
	{
		return false;
	}

	wil::com_ptr_nothrow<ITransferDestination> transferDestination;
	hr = shellItem->BindToHandler(nullptr, BHID_Transfer, IID_PPV_ARGS(&transferDestination));

	if (FAILED(hr))
	{
		return false;
	}

	// Note that SFGAO_READONLY isn't related to FILE_ATTRIBUTE_READONLY (which has no meaning for
	// directories). When the SFGAO_READONLY attribute is set on a directory, it indicates that new
	// items can't be created in that directory.
	//
	// A read-only directory can be created, for example, by mounting a virtual hard disk as
	// read-only.
	SFGAOF attributes = 0;
	hr = shellItem->GetAttributes(SFGAO_READONLY | SFGAO_STORAGE, &attributes);

	if (FAILED(hr))
	{
		return false;
	}

	if (attributes != SFGAO_STORAGE)
	{
		return false;
	}

	return true;
}

bool CanCustomizeDirectory(PCIDLIST_ABSOLUTE pidl)
{
	return IsFilesystemFolder(pidl);
}

bool IsFilesystemFolder(PCIDLIST_ABSOLUTE pidl)
{
	SFGAOF attributes = SFGAO_FILESYSTEM;
	HRESULT hr = GetItemAttributes(pidl, &attributes);

	if (FAILED(hr))
	{
		return false;
	}

	return WI_IsFlagSet(attributes, SFGAO_FILESYSTEM);
}
