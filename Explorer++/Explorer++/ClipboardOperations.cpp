// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ClipboardOperations.h"
#include "DirectoryOperationsHelper.h"
#include "../Helper/Clipboard.h"

namespace
{

enum class LinkType
{
	HardLink,
	SymLink
};

void CreateSymLink(const std::filesystem::path &sourceFilePath,
	const std::filesystem::path &destinationFilePath, std::error_code &error)
{
	bool isDirectory = std::filesystem::is_directory(sourceFilePath, error);

	if (error)
	{
		return;
	}

	if (isDirectory)
	{
		std::filesystem::create_directory_symlink(sourceFilePath, destinationFilePath, error);
	}
	else
	{
		std::filesystem::create_symlink(sourceFilePath, destinationFilePath, error);
	}
}

ClipboardOperations::PastedItems PasteLinksOfType(const std::wstring &destination,
	LinkType linkType)
{
	Clipboard clipboard;
	auto paths = clipboard.ReadHDropData();

	if (!paths)
	{
		return {};
	}

	ClipboardOperations::PastedItems pastedItems;

	for (const auto &path : *paths)
	{
		std::filesystem::path sourceFilePath(path);

		std::filesystem::path destinationFilePath(destination);
		destinationFilePath /= sourceFilePath.filename();

		std::error_code error;

		switch (linkType)
		{
		case LinkType::HardLink:
			std::filesystem::create_hard_link(sourceFilePath, destinationFilePath, error);
			break;

		case LinkType::SymLink:
			CreateSymLink(sourceFilePath, destinationFilePath, error);
			break;

		default:
			CHECK(false);
		}

		pastedItems.emplace_back(destinationFilePath, error);
	}

	return pastedItems;
}

}

namespace ClipboardOperations
{

bool CanPasteHardLinkInDirectory(PCIDLIST_ABSOLUTE pidl)
{
	return IsClipboardFormatAvailable(CF_HDROP) && IsFilesystemFolder(pidl);
}

PastedItems PasteHardLinks(const std::wstring &destination)
{
	return PasteLinksOfType(destination, LinkType::HardLink);
}

PastedItems PasteSymLinks(const std::wstring &destination)
{
	return PasteLinksOfType(destination, LinkType::SymLink);
}

}
