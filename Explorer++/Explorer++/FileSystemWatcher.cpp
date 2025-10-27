// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileSystemWatcher.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"

std::unique_ptr<FileSystemWatcher> FileSystemWatcher::MaybeCreate(const PidlAbsolute &pidl,
	Filters filters, std::shared_ptr<concurrencpp::executor> uiThreadExecutor, Callback callback,
	Behavior behavior)
{
	std::unique_ptr<FileSystemWatcher> watcher;
	HRESULT hr = MaybeCreateInternal(pidl, filters, uiThreadExecutor, callback, behavior, watcher);

	if (FAILED(hr))
	{
		LOG(WARNING) << std::format("Couldn't monitor directory \"{}\" for changes.",
			wstrToUtf8Str(GetDisplayNameWithFallback(pidl.Raw(), SHGDN_FORPARSING)));
		return nullptr;
	}

	return watcher;
}

HRESULT FileSystemWatcher::MaybeCreateInternal(const PidlAbsolute &pidl, Filters filters,
	std::shared_ptr<concurrencpp::executor> uiThreadExecutor, Callback callback, Behavior behavior,
	std::unique_ptr<FileSystemWatcher> &outputWatcher)
{
	std::wstring parsingName;
	RETURN_IF_FAILED(GetDisplayName(pidl.Raw(), SHGDN_FORPARSING, parsingName));

	auto watcher = std::make_unique<FileSystemWatcher>(parsingName, callback, PassKey());
	RETURN_IF_FAILED(watcher->m_changeReader.create(parsingName.c_str(),
		behavior == Behavior::Recursive, FiltersToWilChangeEvents(filters),
		std::bind_front(&FileSystemWatcher::OnChange, watcher->m_weakPtrFactory.GetWeakPtr(),
			uiThreadExecutor)));

	outputWatcher = std::move(watcher);

	return S_OK;
}

wil::FolderChangeEvents FileSystemWatcher::FiltersToWilChangeEvents(Filters filters)
{
	wil::FolderChangeEvents changeEvents = wil::FolderChangeEvents::None;

	if (WI_IsAnyFlagSet(filters, Filters::FileAdded | Filters::FileRenamed | Filters::FileRemoved))
	{
		WI_SetFlag(changeEvents, wil::FolderChangeEvents::FileName);
	}

	if (WI_IsAnyFlagSet(filters,
			Filters::DirectoryAdded | Filters::DirectoryRenamed | Filters::DirectoryRemoved))
	{
		WI_SetFlag(changeEvents, wil::FolderChangeEvents::DirectoryName);
	}

	if (WI_IsFlagSet(filters, Filters::Modified))
	{
		WI_SetAllFlags(changeEvents,
			wil::FolderChangeEvents::FileSize | wil::FolderChangeEvents::LastWriteTime
				| wil::FolderChangeEvents::Security);
	}

	if (WI_IsFlagSet(filters, Filters::Attributes))
	{
		WI_SetFlag(changeEvents, wil::FolderChangeEvents::Attributes);
	}

	return changeEvents;
}

FileSystemWatcher::FileSystemWatcher(const std::wstring &path, Callback callback, PassKey) :
	m_path(path),
	m_callback(callback)
{
}

concurrencpp::null_result FileSystemWatcher::OnChange(WeakPtr<FileSystemWatcher> weakSelf,
	std::shared_ptr<concurrencpp::executor> uiThreadExecutor, wil::FolderChangeEvent event,
	PCWSTR fileName)
{
	std::wstring fileNameCopy = fileName ? fileName : L"";

	co_await concurrencpp::resume_on(uiThreadExecutor);

	if (!weakSelf)
	{
		co_return;
	}

	weakSelf->ProcessChangeNotification(event, fileNameCopy);
}

void FileSystemWatcher::ProcessChangeNotification(wil::FolderChangeEvent event,
	const std::wstring &fileName)
{
	// This call can legitimately fail, however, it's likely that can only happen if:
	//
	// - One or more of the parameters provided to a method called within `CreateSimplePidl` are
	//   incorrect.
	// - A memory allocation fails.
	//
	// In the first case, it would be useful to have a deterministic failure, rather than have the
	// failure be silently ignored.
	//
	// The second case is unlikely, since the total amount of memory that needs to be allocated is
	// going to be small. If one of the allocations does fail, there are going to be bigger problems
	// anyway.
	//
	// Therefore, the result of this call is CHECK'd here.
	//
	// That also simplifies this method, since allowing the `CreateSimplePidl` call to fail means
	// there would need to be additional handling for `m_renamedItemOldPidl` in the failure case.
	PidlAbsolute simplePidl;

	auto fullPath = m_path;

	if (!fileName.empty())
	{
		fullPath /= fileName;
	}

	// A filename should always be provided, except in the case where changes have been lost.
	DCHECK(!fileName.empty() || event == wil::FolderChangeEvent::ChangesLost);

	HRESULT hr = CreateSimplePidl(fullPath.c_str(), simplePidl);
	CHECK(SUCCEEDED(hr));

	switch (event)
	{
	case wil::FolderChangeEvent::Added:
		m_callback(Event::Added, simplePidl, {});
		break;

	case wil::FolderChangeEvent::RenameOldName:
		if (m_renamedItemOldPidl.HasValue())
		{
			DCHECK(false);
			return;
		}

		m_renamedItemOldPidl = simplePidl;
		break;

	case wil::FolderChangeEvent::RenameNewName:
		if (!m_renamedItemOldPidl.HasValue())
		{
			DCHECK(false);
			return;
		}

		m_callback(Event::Renamed, m_renamedItemOldPidl, simplePidl);

		m_renamedItemOldPidl.Reset();
		break;

	case wil::FolderChangeEvent::Modified:
		m_callback(Event::Modified, simplePidl, {});
		break;

	case wil::FolderChangeEvent::Removed:
		m_callback(Event::Removed, simplePidl, {});
		break;

	case wil::FolderChangeEvent::ChangesLost:
		m_callback(Event::DirectoryContentsChanged, simplePidl, {});
		break;
	}
}
