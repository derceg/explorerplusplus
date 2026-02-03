// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellWatcherManager.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"
#include "../Helper/WindowSubclass.h"

ShellWatcherManager::~ShellWatcherManager()
{
	// If a watch hasn't been stopped by this point, it means a client has a dangling pointer
	// somewhere. In other words, each client requires a pointer to this class to be able to call
	// StopWatching(). If a client hasn't called that method by the time this class is destroyed, it
	// means that the client has a dangling pointer to this class.
	CHECK(m_messageIdToWatchDetailsMap.empty());
}

std::optional<UINT> ShellWatcherManager::StartWatching(const PidlAbsolute &pidl,
	DirectoryWatcher::Filters filters, DirectoryWatcher::Callback callback,
	DirectoryWatcher::Behavior behavior)
{
	EnsureWindow();

	UINT messageId = GetNextMessageId();

	SHChangeNotifyEntry entry;
	entry.pidl = pidl.Raw();
	entry.fRecursive = (behavior == DirectoryWatcher::Behavior::Recursive);
	ULONG changeNotifyId = SHChangeNotifyRegister(m_hwnd.get(),
		SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery,
		FiltersToShellChangeEvents(filters), messageId, 1, &entry);

	if (changeNotifyId == 0)
	{
		std::wstring path;
		HRESULT hr = GetDisplayName(pidl.Raw(), SHGDN_FORPARSING, path);

		if (SUCCEEDED(hr))
		{
			LOG(WARNING) << "Couldn't monitor directory \"" << wstrToUtf8Str(path)
						 << "\" for changes.";
		}

		return std::nullopt;
	}

	auto [itr, didInsert] =
		m_messageIdToWatchDetailsMap.insert({ messageId, { changeNotifyId, callback } });
	CHECK(didInsert);

	return messageId;
}

void ShellWatcherManager::StopWatching(UINT id)
{
	auto itr = m_messageIdToWatchDetailsMap.find(id);
	CHECK(itr != m_messageIdToWatchDetailsMap.end());

	auto res = SHChangeNotifyDeregister(itr->second.changeNotifyId);
	CHECK(res);

	m_messageIdToWatchDetailsMap.erase(itr);
}

UINT ShellWatcherManager::GetNextMessageId()
{
	UINT id = m_messageIdCounter;

	m_messageIdCounter++;

	if (m_messageIdCounter > MESSAGE_ID_MAX)
	{
		m_messageIdCounter = MESSAGE_ID_MIN;
	}

	return id;
}

LONG ShellWatcherManager::FiltersToShellChangeEvents(DirectoryWatcher::Filters filters)
{
	// Multiple notifications can be combined into a single SHCNE_UPDATEDIR notification by the
	// shell, so that notification should always be included.
	LONG changeEvents = SHCNE_UPDATEDIR;

	if (WI_IsFlagSet(filters, DirectoryWatcher::Filters::FileAdded))
	{
		WI_SetFlag(changeEvents, SHCNE_CREATE);
	}

	if (WI_IsFlagSet(filters, DirectoryWatcher::Filters::FileRenamed))
	{
		WI_SetFlag(changeEvents, SHCNE_RENAMEITEM);
	}

	if (WI_IsFlagSet(filters, DirectoryWatcher::Filters::FileRemoved))
	{
		WI_SetFlag(changeEvents, SHCNE_DELETE);
	}

	if (WI_IsFlagSet(filters, DirectoryWatcher::Filters::DirectoryAdded))
	{
		WI_SetAllFlags(changeEvents, SHCNE_MKDIR | SHCNE_DRIVEADD);
	}

	if (WI_IsFlagSet(filters, DirectoryWatcher::Filters::DirectoryRenamed))
	{
		WI_SetFlag(changeEvents, SHCNE_RENAMEFOLDER);
	}

	if (WI_IsFlagSet(filters, DirectoryWatcher::Filters::DirectoryRemoved))
	{
		WI_SetAllFlags(changeEvents, SHCNE_RMDIR | SHCNE_DRIVEREMOVED);
	}

	if (WI_IsFlagSet(filters, DirectoryWatcher::Filters::Modified))
	{
		WI_SetFlag(changeEvents, SHCNE_UPDATEITEM);
	}

	if (WI_IsFlagSet(filters, DirectoryWatcher::Filters::Attributes))
	{
		WI_SetFlag(changeEvents, SHCNE_ATTRIBUTES);
	}

	return changeEvents;
}

void ShellWatcherManager::EnsureWindow()
{
	if (m_hwnd)
	{
		return;
	}

	static bool classRegistered = false;

	if (!classRegistered)
	{
		auto res = RegisterWindowClass();
		CHECK_NE(res, 0);

		classRegistered = true;
	}

	m_hwnd.reset(CreateWindow(CLASS_NAME, L"", WS_DISABLED, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(nullptr), nullptr));
	CHECK(m_hwnd);

	m_subclass = std::make_unique<WindowSubclass>(m_hwnd.get(),
		std::bind_front(&ShellWatcherManager::WndProc, this));
}

ATOM ShellWatcherManager::RegisterWindowClass()
{
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = CLASS_NAME;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.style = 0;
	return RegisterClass(&windowClass);
}

LRESULT ShellWatcherManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg >= MESSAGE_ID_MIN && msg <= MESSAGE_ID_MAX
		&& m_messageIdToWatchDetailsMap.contains(msg))
	{
		OnChangeNotify(msg, wParam, lParam);
	}

	switch (msg)
	{
	case WM_TIMER:
		if (wParam == PROCESS_CHANGES_TIMER_ID)
		{
			OnProcessChanges();
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ShellWatcherManager::OnChangeNotify(UINT msg, WPARAM wParam, LPARAM lParam)
{
	PIDLIST_ABSOLUTE *pidls;
	LONG event;
	HANDLE lock = SHChangeNotification_Lock(reinterpret_cast<HANDLE>(wParam),
		static_cast<DWORD>(lParam), &pidls, &event);

	m_changes.emplace_back(msg, event, pidls[0], pidls[1]);

	SHChangeNotification_Unlock(lock);

	SetTimer(m_hwnd.get(), PROCESS_CHANGES_TIMER_ID, PROCESS_CHANGES_TIMEOUT, nullptr);
}

void ShellWatcherManager::OnProcessChanges()
{
	KillTimer(m_hwnd.get(), PROCESS_CHANGES_TIMER_ID);

	for (const auto &change : m_changes)
	{
		auto itr = m_messageIdToWatchDetailsMap.find(change.messageId);

		if (itr == m_messageIdToWatchDetailsMap.end())
		{
			continue;
		}

		auto directoryWatcherEvent = TryConvertShellChangeEvent(change.event);

		if (!directoryWatcherEvent)
		{
			continue;
		}

		const auto &watchDetails = itr->second;
		watchDetails.callback(*directoryWatcherEvent, change.pidl1, change.pidl2);
	}

	m_changes.clear();
}

std::optional<DirectoryWatcher::Event> ShellWatcherManager::TryConvertShellChangeEvent(LONG event)
{
	switch (event)
	{
	case SHCNE_DRIVEADD:
	case SHCNE_MKDIR:
	case SHCNE_CREATE:
		return DirectoryWatcher::Event::Added;

	case SHCNE_RENAMEFOLDER:
	case SHCNE_RENAMEITEM:
		return DirectoryWatcher::Event::Renamed;

	case SHCNE_UPDATEITEM:
		return DirectoryWatcher::Event::Modified;

	case SHCNE_UPDATEDIR:
		return DirectoryWatcher::Event::DirectoryContentsChanged;

	case SHCNE_DRIVEREMOVED:
	case SHCNE_RMDIR:
	case SHCNE_DELETE:
		return DirectoryWatcher::Event::Removed;
	}

	LOG(WARNING) << "Unhandled shell change event type: " << event;

	return std::nullopt;
}
