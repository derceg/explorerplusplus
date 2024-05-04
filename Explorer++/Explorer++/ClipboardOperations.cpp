// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ClipboardOperations.h"
#include "DirectoryOperationsHelper.h"
#include "../Helper/Clipboard.h"
#include "../Helper/FileOperations.h"

bool CanPasteHardLinkInDirectory(PCIDLIST_ABSOLUTE pidl)
{
	return IsClipboardFormatAvailable(CF_HDROP) && IsFilesystemFolder(pidl);
}

void PasteHardLinks(const std::wstring &destination)
{
	Clipboard clipboard;
	auto paths = clipboard.ReadHDropData();

	if (!paths)
	{
		return;
	}

	for (const auto &path : *paths)
	{
		FileOperations::CreateHardLinkToFile(path, destination);
	}
}
