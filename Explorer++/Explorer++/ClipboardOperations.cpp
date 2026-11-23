// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ClipboardOperations.h"
#include "CommandLine.h"
#include "CustomDropHelper.h"
#include "DirectoryOperationsHelper.h"
#include "MainResource.h"
#include "PasteSymLinksServer.h"
#include "ResourceLoader.h"
#include "RuntimeHelper.h"
#include "ServiceProvider.h"
#include "../Helper/Clipboard.h"
#include "../Helper/ClipboardStore.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/ShellHelper.h"

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

ClipboardOperations::PastedItems PasteLinksOfType(ClipboardStore *clipboardStore,
	const std::wstring &destination, LinkType linkType)
{
	Clipboard clipboard(clipboardStore);
	auto paths = clipboard.ReadHDropData();

	if (!paths)
	{
		return {};
	}

	ClipboardOperations::PastedItems pastedItems;

	for (const auto &path : *paths)
	{
		std::filesystem::path sourcePath(path);

		auto linkPath = destination / sourcePath.filename();

		std::error_code error;

		switch (linkType)
		{
		case LinkType::HardLink:
			std::filesystem::create_hard_link(sourcePath, linkPath, error);
			break;

		case LinkType::SymLink:
			CreateSymLink(sourcePath, linkPath, error);
			break;

		default:
			CHECK(false);
		}

		pastedItems.emplace_back(linkPath, error);
	}

	return pastedItems;
}

ClipboardOperations::PastedItems PasteSymLinksViaElevatedProcess(const std::wstring &destination)
{
	auto clientLauncher = [&destination]
	{
		std::wstring parameters =
			std::format(L"{} \"{}\"", CommandLine::PASTE_SYMLINKS_ARGUMENT, destination);
		return LaunchCurrentProcess(nullptr, parameters, LaunchProcessFlags::Elevated);
	};

	PasteSymLinksServer server;
	return server.LaunchClientAndWaitForResponse(clientLauncher, 10s);
}

std::wstring GenerateTimestampForFilename()
{
	SYSTEMTIME time;
	GetLocalTime(&time);

	return std::format(L"{:04}-{:02}-{:02} {:02}.{:02}.{:02}", time.wYear, time.wMonth, time.wDay,
		time.wHour, time.wMinute, time.wSecond);
}

}

namespace ClipboardOperations
{

bool CanPasteLinkInDirectory(const ClipboardStore *clipboardStore, PCIDLIST_ABSOLUTE pidl)
{
	return clipboardStore->IsDataAvailable(CF_HDROP) && IsFilesystemFolder(pidl);
}

void Paste(ClipboardStore *clipboardStore, const PidlAbsolute &destination,
	GetWindowCallback getWindowCallback, ServiceProvider *serviceProvider, Runtime *runtime,
	const ResourceLoader *resourceLoader)
{
	auto clipboardObject = clipboardStore->GetDataObject();

	if (!clipboardObject)
	{
		return;
	}

	if (CanShellPasteDataObject(destination.Raw(), clipboardObject.get(), PasteType::Normal))
	{
		ExecuteActionFromContextMenu(destination.Raw(), {}, getWindowCallback(), L"paste", 0,
			serviceProvider);
	}
	else
	{
		PasteCustomData(clipboardStore, destination, std::move(getWindowCallback), serviceProvider,
			runtime, resourceLoader);
	}
}

concurrencpp::null_result PasteCustomData(ClipboardStore *clipboardStore, PidlAbsolute destination,
	GetWindowCallback getWindowCallback, wil::com_ptr_nothrow<ServiceProvider> serviceProvider,
	Runtime *runtime, const ResourceLoader *resourceLoader)
{
	// The data on the clipboard can be arbitrarily large, so the extraction will be performed on a
	// background thread.
	co_await ResumeOnComStaThread(runtime);

	auto fileContents = CustomDropHelper::MaybeExtractData(clipboardStore);

	if (!fileContents)
	{
		co_return;
	}

	co_await ResumeOnUiThread(runtime);

	auto customDataObject = winrt::make_self<DataObjectImpl>();
	customDataObject->SetAsyncMode(VARIANT_TRUE);

	std::wstring basename;
	std::wstring extension;

	switch (fileContents->type)
	{
	case CustomDropHelper::DataType::Text:
		basename = resourceLoader->LoadString(IDS_CLIPBOARD_TEXT_FILENAME_PREFIX);
		extension = L"txt";
		break;

	case CustomDropHelper::DataType::PngImage:
		basename = resourceLoader->LoadString(IDS_CLIPBOARD_IMAGE_FILENAME_PREFIX);
		extension = L"png";
		break;
	}

	auto filename = std::format(L"{} ({}).{}", basename, GenerateTimestampForFilename(), extension);

	std::vector<VirtualFile> virtualFiles;
	virtualFiles.emplace_back(std::move(filename), std::move(fileContents->contents));
	HRESULT hr = WriteVirtualFilesToDataObject(customDataObject.get(), virtualFiles);

	if (FAILED(hr))
	{
		co_return;
	}

	// This is registered specifically because the shell will request the IOleWindow interface from
	// the object and then call IOleWindow::GetWindow(). That's important for any dialogs that might
	// be shown during the paste operation. Without this, a dialog that's shown during the paste
	// operation would have no parent.
	serviceProvider->RegisterService(SID_STopLevelBrowser,
		winrt::make<OleWindow>(std::move(getWindowCallback)));

	PasteViaSimulatedDrop(destination.Raw(), customDataObject.get(), serviceProvider.get(),
		DROPEFFECT_COPY);
}

PastedItems PasteHardLinks(ClipboardStore *clipboardStore, PCIDLIST_ABSOLUTE destination)
{
	wil::unique_cotaskmem_string destinationPath;
	HRESULT hr = SHGetNameFromIDList(destination, SIGDN_FILESYSPATH, &destinationPath);

	if (FAILED(hr))
	{
		return {};
	}

	return PasteLinksOfType(clipboardStore, destinationPath.get(), LinkType::HardLink);
}

PastedItems PasteSymLinks(ClipboardStore *clipboardStore, PCIDLIST_ABSOLUTE destination)
{
	wil::unique_cotaskmem_string destinationPath;
	HRESULT hr = SHGetNameFromIDList(destination, SIGDN_FILESYSPATH, &destinationPath);

	if (FAILED(hr))
	{
		return {};
	}

	auto pastedItems = PasteLinksOfType(clipboardStore, destinationPath.get(), LinkType::SymLink);

	auto itr = std::find_if(pastedItems.begin(), pastedItems.end(),
		[](const auto &pastedItem)
		{
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
	return PasteSymLinksViaElevatedProcess(destinationPath.get());
}

PastedItems PasteSymLinksToPath(ClipboardStore *clipboardStore, const std::wstring &destination)
{
	return PasteLinksOfType(clipboardStore, destination, LinkType::SymLink);
}

}
