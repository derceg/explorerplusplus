// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ClipboardOperations.h"
#include "CommandLine.h"
#include "DirectoryOperationsHelper.h"
#include "PasteSymLinksServer.h"
#include "../Helper/Clipboard.h"

using namespace std::chrono_literals;

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

ClipboardOperations::PastedItems PasteSymLinksViaElevatedProcess(const std::wstring &destination)
{
	auto clientLauncher = [&destination]
	{
		std::wstring parameters =
			std::format(L"{} \"{}\"", CommandLine::PASTE_SYMLINKS_ARGUMENT, destination);
		return LaunchCurrentProcess(nullptr, parameters, LaunchCurrentProcessFlags::Elevated);
	};

	PasteSymLinksServer server;
	return server.LaunchClientAndWaitForResponse(clientLauncher, 10s);
}

}

namespace ClipboardOperations
{

bool CanPasteLinkInDirectory(PCIDLIST_ABSOLUTE pidl)
{
	return IsClipboardFormatAvailable(CF_HDROP) && IsFilesystemFolder(pidl);
}

PastedItems PasteHardLinks(const std::wstring &destination)
{
	return PasteLinksOfType(destination, LinkType::HardLink);
}

PastedItems PasteSymLinks(const std::wstring &destination)
{
	auto pastedItems = PasteLinksOfType(destination, LinkType::SymLink);

	auto itr = std::find_if(pastedItems.begin(), pastedItems.end(),
		[](const auto &pastedItem) {
			return pastedItem.error
				== std::error_code(ERROR_PRIVILEGE_NOT_HELD, std::system_category());
		});

	if (itr == pastedItems.end())
	{
		// If none of the symlink operations failed because of insufficient privileges, it indicates
		// that either this process is elevated, or developer mode is enabled. In either case,
		// there's no need to retry the operations in an elevated process.
		// Note that this doesn't necessarily indicate that any of the symlink operations actually
		// succeeded, only that they didn't fail because symlink creation is blocked.
		return pastedItems;
	}

	// If at least one symlink operation failed due to insufficient privileges, it's assumed they
	// all failed for that reason. In which case, the operation needs to be retried in an elevated
	// process.
	return PasteSymLinksViaElevatedProcess(destination);
}

}
