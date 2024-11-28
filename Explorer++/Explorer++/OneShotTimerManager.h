// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class WindowSubclass;

// Allows a one-shot timer to be created. The advantage of this class over WM_TIMER is that it
// guarantees the timer won't be run once StopTimer() has been called. When using WM_TIMER, a timer
// can still run, even after KillTimer() has been called.
// An explanation for that is given in
// https://devblogs.microsoft.com/oldnewthing/20141205-00/?p=43463. Essentially, if PeekMessage() is
// called with the PM_NOREMOVE flag, a WM_TIMER message can be generated, but not immediately
// handled. That can happen during a drag and drop operation, for example.
// When dropping an item over a window, PeekMessage() can be called with the PM_NOREMOVE flag as
// part of the COM modal loop. That can result in a WM_TIMER message being generated, but not
// immediately handled. If KillTimer() is called shortly after that, the WM_TIMER message can be
// processed after the KillTimer() call.
// This class guarantees that once StopTimer() has been called, the associated timer callback will
// never be run, even if a WM_TIMER message later comes in.
// The way that's implemented is that each timer that's scheduled is given a new ID. Then, when a
// timer is stopped, the callback for that ID is dropped. If a WM_TIMER message is later received
// for that ID, it will be ignored.
// Additionally, if a timer is reset (by placing multiple calls to StartTimer()), the old timer will
// be stopped, and a new timer, with a new ID will be generated.
// Using a new ID for each timer shouldn't be an issue, since the ID will be at least 32 bits and
// it's not practical for that to get to the point where it might wrap and reuse an ID that's
// currently in use.
class OneShotTimerManager
{
public:
	using Callback = std::function<void()>;

	OneShotTimerManager(HWND hwnd);
	~OneShotTimerManager();

	// Starts a new timer. The callback needs to remain valid until either it's invoked, or
	// StopTimer() is called.
	[[nodiscard]] std::optional<UINT_PTR> StartTimer(std::chrono::milliseconds delay,
		Callback callback);

	void StopTimer(UINT_PTR timerId);

private:
	enum class CallbackRunType
	{
		Run,
		DontRun
	};

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnTimer(UINT_PTR timerId);
	void RemoveTimerAndOptionallyRunCallback(UINT_PTR timerId, CallbackRunType runType);

	HWND m_hwnd;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;

	// Maps between timer IDs and their associated callbacks.
	std::unordered_map<UINT_PTR, Callback> m_timerCallbacks;

	UINT_PTR m_idCounter = 0;
};
