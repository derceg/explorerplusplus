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

}

bool CanPasteInDirectory(PCIDLIST_ABSOLUTE pidl, PasteType pasteType)
{
	return CanShellPasteClipboardDataInDirectory(pidl, pasteType)
		|| (pasteType == PasteType::Normal && CanPasteCustomDataInDirectory(pidl));
}

bool CanCreateHardLinkInDirectory(PCIDLIST_ABSOLUTE pidl)
{
	return IsFilesystemFolder(pidl);
}

bool CanCreateInDirectory(PCIDLIST_ABSOLUTE pidl)
{
	if (IsFilesystemFolder(pidl))
	{
		return true;
	}

	// Library folders aren't filesystem folders, but they act like them (e.g. they allow items to
	// be created, copied and moved) and ultimately they're backed by filesystem folders. If this is
	// a library folder, file creation will be allowed.
	return IsChildOfLibrariesFolder(pidl);
}

bool CanCustomizeDirectory(PCIDLIST_ABSOLUTE pidl)
{
	return IsFilesystemFolder(pidl);
}