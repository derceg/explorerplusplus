// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellChangeWatcher.h"
#include "../Helper/StringHelper.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <glog/logging.h>

ShellChangeWatcher::ShellChangeWatcher(HWND hwnd,
	ProcessNotificationsCallback processNotificationsCallback) :
	m_hwnd(hwnd),
	m_processNotificationsCallback(processNotificationsCallback)
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(hwnd,
		std::bind_front(&ShellChangeWatcher::WndProc, this)));
}

ShellChangeWatcher::~ShellChangeWatcher()
{
	StopWatchingAll();
}

ULONG ShellChangeWatcher::StartWatching(PCIDLIST_ABSOLUTE pidl, LONG events, bool recursive)
{
	SHChangeNotifyEntry entry;
	entry.pidl = pidl;
	entry.fRecursive = recursive;
	ULONG changeNotifyId = SHChangeNotifyRegister(m_hwnd,
		SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery, events, WM_APP_SHELL_NOTIFY,
		1, &entry);

	if (changeNotifyId == 0)
	{
		std::wstring path;
		HRESULT hr = GetDisplayName(pidl, SHGDN_FORPARSING, path);

		if (SUCCEEDED(hr))
		{
			LOG(WARNING) << "Couldn't monitor directory \"" << wstrToUtf8Str(path)
						 << "\" for changes.";
		}

		return 0;
	}

	[[maybe_unused]] auto insertionResult = m_changeNotifyIds.insert(changeNotifyId);

	// Change IDs are unique, so there should never be an attempt to insert a duplicate ID.
	assert(insertionResult.second);

	return changeNotifyId;
}

void ShellChangeWatcher::StopWatching(ULONG changeNotifyId)
{
	[[maybe_unused]] auto res = SHChangeNotifyDeregister(changeNotifyId);
	assert(res);

	[[maybe_unused]] auto numErased = m_changeNotifyIds.erase(changeNotifyId);
	assert(numErased == 1);
}

void ShellChangeWatcher::StopWatchingAll()
{
	m_shellChangeNotifications.clear();
	KillTimer(m_hwnd, PROCESS_SHELL_CHANGES_TIMER_ID);

	for (ULONG changeNotifyId : m_changeNotifyIds)
	{
		[[maybe_unused]] auto res = SHChangeNotifyDeregister(changeNotifyId);
		assert(res);
	}

	m_changeNotifyIds.clear();
}

LRESULT ShellChangeWatcher::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_APP_SHELL_NOTIFY:
		OnShellNotify(wParam, lParam);
		break;

	case WM_TIMER:
		if (wParam == PROCESS_SHELL_CHANGES_TIMER_ID)
		{
			OnProcessShellChangeNotifications();
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ShellChangeWatcher::OnShellNotify(WPARAM wParam, LPARAM lParam)
{
	PIDLIST_ABSOLUTE *pidls;
	LONG event;
	HANDLE lock = SHChangeNotification_Lock(reinterpret_cast<HANDLE>(wParam),
		static_cast<DWORD>(lParam), &pidls, &event);

	m_shellChangeNotifications.emplace_back(event, pidls[0], pidls[1]);

	SHChangeNotification_Unlock(lock);

	SetTimer(m_hwnd, PROCESS_SHELL_CHANGES_TIMER_ID, PROCESS_SHELL_CHANGES_TIMEOUT, nullptr);
}

void ShellChangeWatcher::OnProcessShellChangeNotifications()
{
	KillTimer(m_hwnd, PROCESS_SHELL_CHANGES_TIMER_ID);

	m_processNotificationsCallback(m_shellChangeNotifications);

	m_shellChangeNotifications.clear();
}
