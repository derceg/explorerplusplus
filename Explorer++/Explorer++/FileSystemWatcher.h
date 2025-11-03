// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DirectoryWatcher.h"
#include "../Helper/PassKey.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <concurrencpp/concurrencpp.h>
#include <wil/filesystem.h>
#include <filesystem>
#include <memory>

// Watches a filesystem directory for changes, via ReadDirectoryChangesW().
class FileSystemWatcher : public DirectoryWatcher
{
private:
	using PassKey = PassKey<FileSystemWatcher>;

public:
	static std::unique_ptr<FileSystemWatcher> MaybeCreate(const PidlAbsolute &pidl, Filters filters,
		std::shared_ptr<concurrencpp::executor> uiThreadExecutor, Callback callback,
		Behavior behavior);

	FileSystemWatcher(const std::wstring &path, Callback callback, PassKey);

private:
	static HRESULT MaybeCreateInternal(const PidlAbsolute &pidl, Filters filters,
		std::shared_ptr<concurrencpp::executor> uiThreadExecutor, Callback callback,
		Behavior behavior, std::unique_ptr<FileSystemWatcher> &outputWatcher);
	static wil::FolderChangeEvents FiltersToWilChangeEvents(Filters filters);

	static concurrencpp::null_result OnChange(WeakPtr<FileSystemWatcher> weakSelf,
		std::shared_ptr<concurrencpp::executor> uiThreadExecutor, wil::FolderChangeEvent event,
		PCWSTR fileName);

	void ProcessChangeNotification(wil::FolderChangeEvent event, const std::wstring &fileName);

	const std::filesystem::path m_path;
	const Callback m_callback;
	wil::unique_folder_change_reader_nothrow m_changeReader;
	PidlAbsolute m_renamedItemOldPidl;

	WeakPtrFactory<FileSystemWatcher> m_weakPtrFactory{ this };
};
