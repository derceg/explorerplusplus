// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ClipboardOperations.h"
#include "DirectoryOperationsHelper.h"
#include "../Helper/Clipboard.h"

namespace ClipboardOperations
{

bool CanPasteHardLinkInDirectory(PCIDLIST_ABSOLUTE pidl)
{
	return IsClipboardFormatAvailable(CF_HDROP) && IsFilesystemFolder(pidl);
}

void PasteHardLinks(const std::wstring &destination, InternalPasteCallback internalPasteCallback)
{
	Clipboard clipboard;
	auto paths = clipboard.ReadHDropData();

	if (!paths)
	{
		return;
	}

	std::vector<std::wstring> pastedItems;

	for (const auto &path : *paths)
	{
		std::filesystem::path sourceFilePath(path);

		std::filesystem::path destinationFilePath(destination);
		destinationFilePath /= sourceFilePath.filename();

		std::error_code error;
		std::filesystem::create_hard_link(sourceFilePath, destinationFilePath, error);

		if (!error)
		{
			pastedItems.push_back(destinationFilePath);
		}
	}

	if (pastedItems.empty())
	{
		return;
	}

	internalPasteCallback(pastedItems);
}

}
