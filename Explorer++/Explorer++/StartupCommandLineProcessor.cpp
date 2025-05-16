// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "StartupCommandLineProcessor.h"
#include "ClipboardOperations.h"
#include "CommandLine.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "PasteSymLinksClient.h"
#include "ResourceHelper.h"
#include "Storage.h"
#include "../Helper/SetDefaultFileManager.h"

namespace
{

struct ReplaceExplorerResults
{
	std::optional<LSTATUS> removedFileSystem;
	std::optional<LSTATUS> removedAll;
	std::optional<LSTATUS> setFileSystem;
	std::optional<LSTATUS> setAll;
};

void OnClearRegistrySettings();
void OnUpdateReplaceExplorerSetting(DefaultFileManager::ReplaceExplorerMode updatedReplaceMode);
ReplaceExplorerResults UpdateReplaceExplorerSetting(
	DefaultFileManager::ReplaceExplorerMode updatedReplaceMode);

}

namespace StartupCommandLineProcessor
{

std::optional<ExitCode> Process(const CommandLine::Settings *commandLineSettings,
	ClipboardStore *clipboardStore)
{
	if (commandLineSettings->crashedData)
	{
		HandleProcessCrashedNotification(*commandLineSettings->crashedData);
		return EXIT_CODE_NORMAL_CRASH_HANDLER;
	}

	if (commandLineSettings->pasteSymLinksDestination)
	{
		auto pastedItems = ClipboardOperations::PasteSymLinks(clipboardStore,
			*commandLineSettings->pasteSymLinksDestination);

		PasteSymLinksClient client;
		client.NotifyServerOfResult(pastedItems);

		return EXIT_CODE_NORMAL;
	}

	if (commandLineSettings->clearRegistrySettings)
	{
		OnClearRegistrySettings();
	}

	if (commandLineSettings->removeAsDefault)
	{
		OnUpdateReplaceExplorerSetting(DefaultFileManager::ReplaceExplorerMode::None);
	}
	else if (commandLineSettings->replaceExplorerMode
		!= +DefaultFileManager::ReplaceExplorerMode::None)
	{
		OnUpdateReplaceExplorerSetting(commandLineSettings->replaceExplorerMode);
	}

	if (commandLineSettings->enableLogging)
	{
		FLAGS_logtostdout = false;
		FLAGS_minloglevel = google::GLOG_INFO;
	}

	return std::nullopt;
}

}

namespace
{

void OnClearRegistrySettings()
{
	LSTATUS lStatus;

	lStatus = SHDeleteKey(HKEY_CURRENT_USER, Storage::REGISTRY_APPLICATION_KEY_PATH);

	if (lStatus == ERROR_SUCCESS)
	{
		std::wcout << L"Settings cleared successfully." << std::endl;
	}
	else
	{
		std::wcerr << L"Settings could not be cleared.\n" << std::endl;
	}
}

void OnUpdateReplaceExplorerSetting(DefaultFileManager::ReplaceExplorerMode updatedReplaceMode)
{
	auto results = UpdateReplaceExplorerSetting(updatedReplaceMode);

	switch (updatedReplaceMode)
	{
	case DefaultFileManager::ReplaceExplorerMode::None:
		if ((results.removedFileSystem && *results.removedFileSystem == ERROR_SUCCESS)
			|| (results.removedAll && *results.removedAll == ERROR_SUCCESS))
		{
			std::wcout << L"Explorer++ successfully removed as default file manager.\n"
					   << std::endl;
		}
		else
		{
			std::wcerr << L"Could not remove Explorer++ as default file manager." << std::endl;
		}
		break;

	case DefaultFileManager::ReplaceExplorerMode::FileSystem:
	case DefaultFileManager::ReplaceExplorerMode::All:
		if ((results.setFileSystem && *results.setFileSystem == ERROR_SUCCESS)
			|| (results.setAll && *results.setAll == ERROR_SUCCESS))
		{
			std::wcout << L"Explorer++ successfully set as default file manager." << std::endl;
		}
		else
		{
			std::wcerr << L"Could not set Explorer++ as default file manager." << std::endl;
		}
		break;
	}
}

ReplaceExplorerResults UpdateReplaceExplorerSetting(
	DefaultFileManager::ReplaceExplorerMode updatedReplaceMode)
{
	ReplaceExplorerResults results;

	// TODO: This text should be retrieved from the appropriate translation DLL (if necessary).
	std::wstring menuText =
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_OPEN_IN_EXPLORERPLUSPLUS);

	// Whether Explorer++ is being set as the default file manager, or being removed, the first step
	// is always to remove any existing entries.
	results.removedFileSystem = DefaultFileManager::RemoveAsDefaultFileManagerFileSystem(
		SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
	results.removedAll =
		DefaultFileManager::RemoveAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);

	switch (updatedReplaceMode)
	{
	case DefaultFileManager::ReplaceExplorerMode::None:
		// This case is effectively handled above.
		break;

	case DefaultFileManager::ReplaceExplorerMode::FileSystem:
		results.setFileSystem = DefaultFileManager::SetAsDefaultFileManagerFileSystem(
			SHELL_DEFAULT_INTERNAL_COMMAND_NAME, menuText.c_str());
		break;

	case DefaultFileManager::ReplaceExplorerMode::All:
		results.setAll = DefaultFileManager::SetAsDefaultFileManagerAll(
			SHELL_DEFAULT_INTERNAL_COMMAND_NAME, menuText.c_str());
		break;
	}

	return results;
}

}
