// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include <wil/resource.h>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class WindowSubclass;

// This class wraps SHChangeNotifyRegister() and allows a shell item to be watched. A single
// instance of this class can be used to watch multiple items. That's somewhat difficult to achieve,
// given the way that SHChangeNotifyRegister() works.
//
// That is, SHChangeNotifyRegister() notifies the client that a change has occurred by posting the
// provided message to the provided window. So, callers of SHChangeNotifyRegister() can use separate
// windows for their callbacks, with the message IDs typically being fixed.
//
// However, that has 2 specific downsides:
//
// 1. Each client that wants to watch an item has to provide a window handle. It's unexpected that
//    watching a shell item requires a window handle.
//
// 2. If a client wants to watch multiple items, it's potentially problematic. The client might only
//    have a single window, so figuring out which callback is for which item might be difficult.
//
// This class has the benefit that clients don't have to pass a window handle in; instead, the
// window is managed internally. Additionally, each client can pass a callback when it watches an
// item and that specific callback will be invoked when a change notification for that item is
// received.
//
// The way this works is that only a single window is used, but the message IDs are dynamically
// generated. In this case, the IDs are in the WM_USER range, from WM_USER (0x0400) to 0x7FFF.
// That's a fairly large range, with over 31,000 values available.
//
// That means that for a duplicate message ID to be generated, StartWatching() would have to be
// invoked over 31,000 times. That's not practically feasible. Because message IDs wrap around, it's
// not enough to just call StartWatching() 31,000+ times; instead, an earlier watch would have to be
// left in place, while thousands of watches are started and stopped. That's not a realistic usage
// pattern. Additionally, duplicate message IDs can be detected, so even if StartWatching() was
// called enough times, there isn't going to be a situation where the wrong callback is invoked.
//
// Rather than calling methods on this class directly, ShellChangeWatcher should be used to watch an
// item, since it will automatically stop the watch on destruction.
class ShellChangeManager
{
public:
	using Callback = std::function<void(LONG event, const PidlAbsolute &simplePidl1,
		const PidlAbsolute &simplePidl2)>;

	~ShellChangeManager();

	std::optional<UINT> StartWatching(const PidlAbsolute &pidl, LONG events, Callback callback,
		bool recursive);
	void StopWatching(UINT id);

private:
	static constexpr wchar_t CLASS_NAME[] = L"Explorer++ShellChangeWindowClass";

	static constexpr UINT MESSAGE_ID_MIN = WM_USER;
	static constexpr UINT MESSAGE_ID_MAX = 0x7FFF;

	static constexpr UINT_PTR PROCESS_CHANGES_TIMER_ID = 200;
	static constexpr UINT PROCESS_CHANGES_TIMEOUT = 100;

	struct WatchDetails
	{
		ULONG changeNotifyId;
		Callback callback;
	};

	struct ChangeDetails
	{
		UINT messageId;
		LONG event;
		PidlAbsolute pidl1;
		PidlAbsolute pidl2;
	};

	UINT GetNextMessageId();

	void EnsureWindow();
	static ATOM RegisterWindowClass();

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnChangeNotify(UINT msg, WPARAM wParam, LPARAM lParam);
	void OnProcessChanges();

	wil::unique_hwnd m_hwnd;
	std::unique_ptr<WindowSubclass> m_subclass;
	UINT m_messageIdCounter = MESSAGE_ID_MIN;
	std::unordered_map<UINT, WatchDetails> m_messageIdToWatchDetailsMap;
	std::vector<ChangeDetails> m_changes;
};
