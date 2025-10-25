// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PassKey.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <concurrencpp/concurrencpp.h>
#include <wil/filesystem.h>
#include <filesystem>
#include <memory>

class FileSystemChangeWatcher
{
private:
	using PassKey = PassKey<FileSystemChangeWatcher>;

public:
	enum class Event
	{
		Added,
		Modified,
		Renamed,
		Removed,

		// Indicates that not all directory changes were recorded. An enumeration of the directory
		// is required.
		ChangesLost
	};

	enum class Behavior
	{
		NonRecursive,
		Recursive
	};

	using Callback = std::function<void(Event event, const PidlAbsolute &simplePidl1,
		const PidlAbsolute &simplePidl2)>;

	static std::unique_ptr<FileSystemChangeWatcher> MaybeCreate(const PidlAbsolute &pidl,
		wil::FolderChangeEvents filter, std::shared_ptr<concurrencpp::executor> uiThreadExecutor,
		Callback callback, Behavior behavior = Behavior::NonRecursive);

	FileSystemChangeWatcher(const std::wstring &path, Callback callback, PassKey);

private:
	static HRESULT MaybeCreateInternal(const PidlAbsolute &pidl, wil::FolderChangeEvents filter,
		std::shared_ptr<concurrencpp::executor> uiThreadExecutor, Callback callback,
		Behavior behavior, std::unique_ptr<FileSystemChangeWatcher> &outputWatcher);

	static concurrencpp::null_result OnChange(WeakPtr<FileSystemChangeWatcher> weakSelf,
		std::shared_ptr<concurrencpp::executor> uiThreadExecutor, wil::FolderChangeEvent event,
		PCWSTR fileName);

	void ProcessChangeNotification(wil::FolderChangeEvent event, const std::wstring &fileName);

	const std::filesystem::path m_path;
	const Callback m_callback;
	wil::unique_folder_change_reader_nothrow m_changeReader;
	PidlAbsolute m_renamedItemOldPidl;

	WeakPtrFactory<FileSystemChangeWatcher> m_weakPtrFactory{ this };
};
