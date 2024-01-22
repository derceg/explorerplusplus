// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"
#include <memory>
#include <set>
#include <vector>

class WindowSubclassWrapper;

struct ShellChangeNotification
{
	LONG event;
	unique_pidl_absolute pidl1;
	unique_pidl_absolute pidl2;

	ShellChangeNotification(LONG event, PCIDLIST_ABSOLUTE pidl1, PCIDLIST_ABSOLUTE pidl2) :
		event(event),
		pidl1(pidl1 ? ILCloneFull(pidl1) : nullptr),
		pidl2(pidl2 ? ILCloneFull(pidl2) : nullptr)
	{
	}
};

class ShellChangeWatcher
{
public:
	using ProcessNotificationsCallback =
		std::function<void(const std::vector<ShellChangeNotification> &shellNotifications)>;

	ShellChangeWatcher(HWND hwnd, ProcessNotificationsCallback processNotificationsCallback);
	~ShellChangeWatcher();

	ULONG StartWatching(PCIDLIST_ABSOLUTE pidl, LONG events, bool recursive = false);
	void StopWatching(ULONG changeNotifyId);
	void StopWatchingAll();

private:
	static const UINT WM_APP_SHELL_NOTIFY = WM_APP + 200;

	static const UINT_PTR PROCESS_SHELL_CHANGES_TIMER_ID = 200;
	static const UINT PROCESS_SHELL_CHANGES_TIMEOUT = 100;

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnShellNotify(WPARAM wParam, LPARAM lParam);
	void OnProcessShellChangeNotifications();

	HWND m_hwnd;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::set<ULONG> m_changeNotifyIds;
	std::vector<ShellChangeNotification> m_shellChangeNotifications;
	ProcessNotificationsCallback m_processNotificationsCallback;
};
